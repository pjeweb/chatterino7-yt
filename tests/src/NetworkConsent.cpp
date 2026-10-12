// SPDX-FileCopyrightText: 2026 Contributors to chatterino7-yt
//
// SPDX-License-Identifier: MIT

#include "singletons/NetworkConsent.hpp"

#include "mocks/BaseApplication.hpp"
#include "providers/IvrApi.hpp"
#include "singletons/Settings.hpp"
#include "Test.hpp"
#include "widgets/dialogs/NetworkConsentDialog.hpp"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

using namespace chatterino;

namespace {

class NetworkConsentTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        this->app_ = std::make_unique<mock::BaseApplication>();
    }

    void TearDown() override
    {
        this->app_.reset();
    }

    Settings &settings()
    {
        return this->app_->settings;
    }

    std::unique_ptr<mock::BaseApplication> app_;
};

}  // namespace

TEST_F(NetworkConsentTest, FreshProfileNeedsFirstRun)
{
    EXPECT_FALSE(networkconsent::isCompleted(this->settings()));
    EXPECT_FALSE(networkconsent::isExistingProfile(this->settings()));
    EXPECT_TRUE(networkconsent::needsFirstRunPrompt(this->settings()));
    EXPECT_FALSE(networkconsent::allowsOptionalRequests(this->settings()));

    EXPECT_FALSE(this->settings().loadTwitchMessageHistoryOnConnect);
    EXPECT_FALSE(this->settings().showSubageInUserCard);
    EXPECT_FALSE(this->settings().sendSevenTVActivity);
    EXPECT_FALSE(this->settings().sendBTTVActivity);
}

TEST_F(NetworkConsentTest, ExistingProfileMigratesWithoutResettingChoices)
{
    this->settings().currentVersion.setValue(QStringLiteral("1.0.0"));
    this->settings().sendSevenTVActivity.setValue(true);
    this->settings().loadTwitchMessageHistoryOnConnect.setValue(true);

    EXPECT_TRUE(networkconsent::isExistingProfile(this->settings()));
    EXPECT_FALSE(networkconsent::needsFirstRunPrompt(this->settings()));

    networkconsent::ensureFirstRunDecision(this->settings(), nullptr);

    EXPECT_TRUE(networkconsent::isCompleted(this->settings()));
    EXPECT_TRUE(this->settings().sendSevenTVActivity);
    EXPECT_TRUE(this->settings().loadTwitchMessageHistoryOnConnect);
    EXPECT_FALSE(this->settings().showSubageInUserCard);
    EXPECT_FALSE(this->settings().sendBTTVActivity);
    EXPECT_TRUE(networkconsent::allowsOptionalRequests(this->settings()));
}

TEST_F(NetworkConsentTest, AcceptEnablesOnlyCheckedChoices)
{
    networkconsent::Choices choices{
        .recentMessages = true,
        .ivrSubAge = false,
        .sevenTVActivity = true,
        .bttvActivity = false,
    };
    networkconsent::accept(this->settings(), choices);

    EXPECT_TRUE(networkconsent::isCompleted(this->settings()));
    EXPECT_TRUE(this->settings().loadTwitchMessageHistoryOnConnect);
    EXPECT_FALSE(this->settings().showSubageInUserCard);
    EXPECT_TRUE(this->settings().sendSevenTVActivity);
    EXPECT_FALSE(this->settings().sendBTTVActivity);
    EXPECT_TRUE(networkconsent::allowsOptionalRequests(this->settings()));
}

TEST_F(NetworkConsentTest, CancelCompletesWithoutOptIn)
{
    this->settings().sendBTTVActivity.setValue(false);
    networkconsent::declineFirstRun(this->settings());

    EXPECT_TRUE(networkconsent::isCompleted(this->settings()));
    EXPECT_FALSE(networkconsent::needsFirstRunPrompt(this->settings()));
    EXPECT_FALSE(this->settings().loadTwitchMessageHistoryOnConnect);
    EXPECT_FALSE(this->settings().showSubageInUserCard);
    EXPECT_FALSE(this->settings().sendSevenTVActivity);
    EXPECT_FALSE(this->settings().sendBTTVActivity);
}

TEST_F(NetworkConsentTest, FirstRunDialogDefaultsAllOff)
{
    NetworkConsentDialog dialog(NetworkConsentDialog::Mode::FirstRun,
                                this->settings());
    const auto choices = dialog.selectedChoices();
    EXPECT_FALSE(choices.recentMessages);
    EXPECT_FALSE(choices.ivrSubAge);
    EXPECT_FALSE(choices.sevenTVActivity);
    EXPECT_FALSE(choices.bttvActivity);
}

TEST_F(NetworkConsentTest, RevisitDialogReflectsSavedChoices)
{
    this->settings().loadTwitchMessageHistoryOnConnect.setValue(true);
    this->settings().sendBTTVActivity.setValue(true);
    this->settings().networkConsentCompleted.setValue(true);

    NetworkConsentDialog dialog(NetworkConsentDialog::Mode::Revisit,
                                this->settings());
    const auto choices = dialog.selectedChoices();
    EXPECT_TRUE(choices.recentMessages);
    EXPECT_FALSE(choices.ivrSubAge);
    EXPECT_FALSE(choices.sevenTVActivity);
    EXPECT_TRUE(choices.bttvActivity);
}

TEST_F(NetworkConsentTest, OptionalRequestsSuppressedUntilConsent)
{
    // Even if a toggle is somehow on, incomplete consent must suppress.
    this->settings().showSubageInUserCard.setValue(true);
    ASSERT_FALSE(networkconsent::allowsOptionalRequests(this->settings()));

    IvrApi ivr;
    bool failed = false;
    bool succeeded = false;
    ivr.getSubage(
        QStringLiteral("user"), QStringLiteral("channel"),
        [&](const IvrSubage &) {
            succeeded = true;
        },
        [&] {
            failed = true;
        });

    EXPECT_TRUE(failed);
    EXPECT_FALSE(succeeded);
}

TEST_F(NetworkConsentTest, AcceptPersistsToSettingsFile)
{
    networkconsent::accept(this->settings(), {
                                                 .recentMessages = true,
                                                 .ivrSubAge = true,
                                                 .sevenTVActivity = false,
                                                 .bttvActivity = true,
                                             });

    QFile file(
        this->app_->settingsDir.filePath(QStringLiteral("settings.json")));
    ASSERT_TRUE(file.open(QIODevice::ReadOnly | QIODevice::Text));
    const auto doc = QJsonDocument::fromJson(file.readAll());
    ASSERT_TRUE(doc.isObject());
    const auto misc = doc.object().value(QStringLiteral("misc")).toObject();
    EXPECT_TRUE(misc.value(QStringLiteral("networkConsentCompleted")).toBool());
    EXPECT_TRUE(misc.value(QStringLiteral("twitch"))
                    .toObject()
                    .value(QStringLiteral("loadMessageHistoryOnConnect"))
                    .toBool());
    EXPECT_TRUE(misc.value(QStringLiteral("showSubageInUserCard")).toBool());

    const auto emotes = doc.object().value(QStringLiteral("emotes")).toObject();
    EXPECT_FALSE(emotes.value(QStringLiteral("seventv"))
                     .toObject()
                     .value(QStringLiteral("sendActivity"))
                     .toBool());
    EXPECT_TRUE(emotes.value(QStringLiteral("bttv"))
                    .toObject()
                    .value(QStringLiteral("sendActivity"))
                    .toBool());
}

TEST(NetworkConsentPersistence, LoadsSavedChoicesFromJson)
{
    mock::BaseApplication app(QStringLiteral(R"({
        "misc": {
            "networkConsentCompleted": true,
            "showSubageInUserCard": true,
            "twitch": {
                "loadMessageHistoryOnConnect": true
            }
        },
        "emotes": {
            "bttv": { "sendActivity": true },
            "seventv": { "sendActivity": false }
        }
    })"));

    EXPECT_TRUE(app.settings.networkConsentCompleted);
    EXPECT_TRUE(app.settings.loadTwitchMessageHistoryOnConnect);
    EXPECT_TRUE(app.settings.showSubageInUserCard);
    EXPECT_FALSE(app.settings.sendSevenTVActivity);
    EXPECT_TRUE(app.settings.sendBTTVActivity);
    EXPECT_TRUE(networkconsent::allowsOptionalRequests(app.settings));
    EXPECT_FALSE(networkconsent::needsFirstRunPrompt(app.settings));
}
