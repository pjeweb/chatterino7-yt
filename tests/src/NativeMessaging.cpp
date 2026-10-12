// SPDX-FileCopyrightText: 2025 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "singletons/NativeMessaging.hpp"

#include "common/ForkIdentity.hpp"
#include "Test.hpp"
#include "util/CombinePath.hpp"
#include "util/IpcQueue.hpp"

#include <QDir>
#include <QJsonDocument>
#include <QScopeGuard>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QUuid>

#ifdef Q_OS_LINUX
#    define BOOST_INTERPROCESS_SHARED_DIR_FUNC
#    include <boost/interprocess/ipc/message_queue.hpp>

#    include <atomic>
#    include <chrono>
#endif

using namespace chatterino;

using namespace chatterino::nm::detail;

class NativeMessagingFixture : public ::testing::Test
{
    QTemporaryDir tempDir;

protected:
    NativeMessagingFixture()
        : dir(combinePath(this->tempDir.path(), QString("native-messaging")))
    {
    }

    QDir dir;
};

#ifdef Q_OS_LINUX
namespace {
std::atomic<bool> receivedForkAction{false};
std::atomic<QtMessageHandler> previousMessageHandler{nullptr};

void observeForkAction(QtMsgType type, const QMessageLogContext &context,
                       const QString &message)
{
    if (message.contains(QStringLiteral("NM unknown action")) &&
        message.contains(QStringLiteral("ipc-isolation")))
    {
        receivedForkAction.store(true);
    }
    if (const auto previous = previousMessageHandler.load())
    {
        previous(type, context, message);
    }
}
}  // namespace

TEST(NativeMessaging, ForkQueuePreservesUpstreamQueue)
{
    // Unique names exercise real POSIX shared memory without touching live apps.
    const auto prefix =
        QUuid::createUuid().toString(QUuid::Id128).toStdString();
    const auto upstreamName = prefix + "_chatterino_gui";
    const auto forkName = prefix + "_" + fork::NATIVE_MESSAGE_QUEUE_NAME;
    ASSERT_NE(upstreamName, forkName);
    const auto cleanup = qScopeGuard([&] {
        ipc::IpcQueue::remove(upstreamName.c_str());
        ipc::IpcQueue::remove(forkName.c_str());
    });

    auto [upstreamQueue, upstreamError] =
        ipc::IpcQueue::tryReplaceOrCreate(upstreamName.c_str(), 4, 128);
    ASSERT_TRUE(upstreamError.isEmpty()) << upstreamError.toStdString();
    ASSERT_NE(upstreamQueue, nullptr);
    nm::client::sendMessage("upstream-before", upstreamName.c_str());

    receivedForkAction.store(false);
    previousMessageHandler.store(qInstallMessageHandler(observeForkAction));
    const auto restoreHandler = qScopeGuard([&] {
        qInstallMessageHandler(previousMessageHandler.load());
        previousMessageHandler.store(nullptr);
    });
    {
        NativeMessagingServer server(forkName);
        server.start();
        bool created = false;
        const auto deadline =
            std::chrono::steady_clock::now() + std::chrono::seconds(2);
        do
        {
            try
            {
                boost::interprocess::message_queue probe(
                    boost::interprocess::open_only, forkName.c_str());
                created = true;
            }
            catch (const boost::interprocess::interprocess_exception &)
            {
                QThread::msleep(5);
            }
        } while (!created && std::chrono::steady_clock::now() < deadline);
        ASSERT_TRUE(created);

        nm::client::sendMessage(R"({"action":"ipc-isolation"})",
                                forkName.c_str());
        const auto receiveDeadline =
            std::chrono::steady_clock::now() + std::chrono::seconds(2);
        while (!receivedForkAction.load() &&
               std::chrono::steady_clock::now() < receiveDeadline)
        {
            QThread::msleep(5);
        }
        ASSERT_TRUE(receivedForkAction.load());
    }
    EXPECT_THROW(boost::interprocess::message_queue(
                     boost::interprocess::open_only, forkName.c_str()),
                 boost::interprocess::interprocess_exception);

    // Fork server destruction must leave upstream reachable by its name.
    boost::interprocess::message_queue reader(boost::interprocess::open_only,
                                              upstreamName.c_str());
    char buffer[128];
    size_t size = 0;
    unsigned int priority = 0;
    ASSERT_TRUE(reader.try_receive(buffer, sizeof(buffer), size, priority));
    EXPECT_EQ(QByteArray(buffer, static_cast<qsizetype>(size)),
              "upstream-before");
    nm::client::sendMessage("upstream-after", upstreamName.c_str());
    ASSERT_TRUE(reader.try_receive(buffer, sizeof(buffer), size, priority));
    EXPECT_EQ(QByteArray(buffer, static_cast<qsizetype>(size)),
              "upstream-after");
}
#endif

TEST_F(NativeMessagingFixture, writeManifestToSubDirCreated)
{
    // writeManifestTo should succeed if the subdir directory is created
    ASSERT_TRUE(this->dir.mkpath("."));

    ASSERT_TRUE(writeManifestTo(this->dir.path(), "native-messaging-hosts",
                                "test.json", QJsonDocument())
                    .has_value());

    QDir nmDir = combinePath(this->dir.path(), "native-messaging-hosts");

    ASSERT_TRUE(nmDir.exists());

    ASSERT_TRUE(QFile(combinePath(nmDir.path(), "test.json")).exists());
}

TEST_F(NativeMessagingFixture, writeManifestToSubDirNotCreated)
{
    // writeManifestTo should fail if the subdir is not created
    ASSERT_EQ(writeManifestTo(this->dir.path(), "native-messaging-hosts",
                              "test.json", QJsonDocument())
                  .error(),
              QString("Failed to create native-messaging-hosts in %1")
                  .arg(this->dir.path()));
}

TEST_F(NativeMessagingFixture, writeManifestToWindowsCurrentDir)
{
    // writeManifestTo should succeed if the subdir is created and the path to create is "."
    ASSERT_TRUE(this->dir.mkpath("."));

    ASSERT_TRUE(
        writeManifestTo(this->dir.path(), ".", "test.json", QJsonDocument())
            .has_value());

    ASSERT_TRUE(QFile(combinePath(this->dir.path(), "test.json")).exists());
}

#ifndef Q_OS_WIN
TEST(NativeMessaging, parseCustomPath)
{
    ASSERT_EQ(parseCustomPath("/my/custom/path/to/manifest.json"),
              std::optional{"/my/custom/path/to/manifest.json"});

    ASSERT_EQ(parseCustomPath(""), std::nullopt);

    ASSERT_EQ(parseCustomPath("~/path/to/manifest.json"),
              std::optional{QDir::homePath() % "/path/to/manifest.json"});

    ASSERT_EQ(parseCustomPath("relative/path/to/manifest.json"), std::nullopt);

#    ifdef Q_OS_LINUX
    ASSERT_EQ(parseCustomPath("$XDG_CONFIG_HOME/path/to/manifest.json"),
              std::optional{QStandardPaths::standardLocations(
                                QStandardPaths::GenericConfigLocation)
                                .constFirst() %
                            "/path/to/manifest.json"});

    ASSERT_EQ(parseCustomPath("$XDG_DATA_HOME/path/to/manifest.json"),
              std::optional{QStandardPaths::standardLocations(
                                QStandardPaths::GenericDataLocation)
                                .constFirst() %
                            "/path/to/manifest.json"});
#    endif
}
#endif
