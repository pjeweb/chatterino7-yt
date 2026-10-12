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
    ipc::sendMessage(upstreamName.c_str(), "upstream-before");
    {
        auto [forkQueue, forkError] =
            ipc::IpcQueue::tryReplaceOrCreate(forkName.c_str(), 4, 128);
        ASSERT_TRUE(forkError.isEmpty()) << forkError.toStdString();
        ASSERT_NE(forkQueue, nullptr);
        ipc::sendMessage(forkName.c_str(), "fork");
        boost::interprocess::message_queue reader(
            boost::interprocess::open_only, forkName.c_str());
        char buffer[128];
        size_t size = 0;
        unsigned int priority = 0;
        ASSERT_TRUE(reader.try_receive(buffer, sizeof(buffer), size, priority));
        EXPECT_EQ(QByteArray(buffer, static_cast<qsizetype>(size)), "fork");
    }
    ASSERT_TRUE(ipc::IpcQueue::remove(forkName.c_str()));

    // Removing the fork queue must leave upstream reachable by its name.
    boost::interprocess::message_queue reader(boost::interprocess::open_only,
                                              upstreamName.c_str());
    char buffer[128];
    size_t size = 0;
    unsigned int priority = 0;
    ASSERT_TRUE(reader.try_receive(buffer, sizeof(buffer), size, priority));
    EXPECT_EQ(QByteArray(buffer, static_cast<qsizetype>(size)),
              "upstream-before");
    ipc::sendMessage(upstreamName.c_str(), "upstream-after");
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
