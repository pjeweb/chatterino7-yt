// SPDX-FileCopyrightText: 2026 Contributors to chatterino7-yt
//
// SPDX-License-Identifier: MIT

#include "singletons/Paths.hpp"

#include "common/Args.hpp"
#include "common/ForkIdentity.hpp"
#include "common/Modes.hpp"
#include "Test.hpp"
#include "util/CombinePath.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTemporaryDir>

using namespace chatterino;

namespace {

class PathsIdentity : public ::testing::Test
{
protected:
    void SetUp() override
    {
        this->previousAppName_ = QCoreApplication::applicationName();
        QStandardPaths::setTestModeEnabled(true);
        QCoreApplication::setApplicationName(
            QLatin1String(fork::APPLICATION_NAME));
    }

    void TearDown() override
    {
        QCoreApplication::setApplicationName(this->previousAppName_);
        QStandardPaths::setTestModeEnabled(false);
    }

    QString previousAppName_;
};

}  // namespace

TEST_F(PathsIdentity, DefaultRootUsesForkApplicationName)
{
    Args args;
    Modes modes(args);
    Paths paths(args, modes);

    const auto root = QDir::cleanPath(paths.rootAppDataDirectory);
    EXPECT_TRUE(root.contains(QLatin1String(fork::APPLICATION_NAME)))
        << root.toStdString();
    // Must not collapse onto the upstream "chatterino" / Chatterino2 directory.
    EXPECT_FALSE(root.endsWith(QStringLiteral("/chatterino")));
    EXPECT_FALSE(root.endsWith(QStringLiteral("\\chatterino")));
    EXPECT_FALSE(root.endsWith(QStringLiteral("/Chatterino2")));
    EXPECT_FALSE(root.endsWith(QStringLiteral("\\Chatterino2")));

    EXPECT_EQ(QFileInfo(paths.settingsDirectory).dir().absolutePath(), root);
}

TEST_F(PathsIdentity, PortableDirectoryOverrideWins)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());

    Args args;
    args.portableEnable = true;
    args.portableDirectory = dir.path();
    Modes modes(args);
    Paths paths(args, modes);

    EXPECT_EQ(QDir::cleanPath(paths.rootAppDataDirectory),
              QDir::cleanPath(dir.path()));
    EXPECT_EQ(
        QDir::cleanPath(paths.settingsDirectory),
        QDir::cleanPath(combinePath(dir.path(), QStringLiteral("Settings"))));
}

TEST_F(PathsIdentity, PortableWithoutOverrideUsesApplicationDir)
{
    Args args;
    args.portableEnable = true;
    Modes modes(args);
    Paths paths(args, modes);

    EXPECT_EQ(QDir::cleanPath(paths.rootAppDataDirectory),
              QDir::cleanPath(QCoreApplication::applicationDirPath()));
}
