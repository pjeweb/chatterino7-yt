// SPDX-FileCopyrightText: 2026 Contributors to chatterino7-yt
//
// SPDX-License-Identifier: MIT

#include "singletons/NetworkConsent.hpp"

#include "singletons/Settings.hpp"
#include "widgets/dialogs/NetworkConsentDialog.hpp"

namespace chatterino::networkconsent {

bool isCompleted(const Settings &settings)
{
    return settings.networkConsentCompleted;
}

bool isExistingProfile(const Settings &settings)
{
    return !settings.currentVersion.getValue().isEmpty();
}

bool needsFirstRunPrompt(const Settings &settings)
{
    return !isCompleted(settings) && !isExistingProfile(settings);
}

bool allowsOptionalRequests(const Settings &settings)
{
    return isCompleted(settings);
}

Choices choicesFromSettings(const Settings &settings)
{
    return {
        .recentMessages = settings.loadTwitchMessageHistoryOnConnect,
        .ivrSubAge = settings.showSubageInUserCard,
        .sevenTVActivity = settings.sendSevenTVActivity,
        .bttvActivity = settings.sendBTTVActivity,
    };
}

void accept(Settings &settings, const Choices &choices)
{
    settings.loadTwitchMessageHistoryOnConnect.setValue(choices.recentMessages);
    settings.showSubageInUserCard.setValue(choices.ivrSubAge);
    settings.sendSevenTVActivity.setValue(choices.sevenTVActivity);
    settings.sendBTTVActivity.setValue(choices.bttvActivity);
    settings.networkConsentCompleted.setValue(true);
    settings.requestSave();
}

void declineFirstRun(Settings &settings)
{
    // Leave optional toggles untouched (fresh defaults stay off).
    settings.networkConsentCompleted.setValue(true);
    settings.requestSave();
}

void ensureFirstRunDecision(Settings &settings, QWidget *parent,
                            bool allowPrompt)
{
    if (isCompleted(settings))
    {
        return;
    }

    // Upgrade path: prior GUI launches wrote currentVersion. Preserve whatever
    // the user already saved for optional services; do not show a dialog with
    // unchecked boxes that looks like a new default. Runs for GUI and embed.
    if (isExistingProfile(settings))
    {
        settings.networkConsentCompleted.setValue(true);
        settings.requestSave();
        return;
    }

    // Fresh frameless embed: no modal UI and no implicit opt-in.
    if (!allowPrompt)
    {
        return;
    }

    NetworkConsentDialog::execFirstRun(settings, parent);
}

void showRevisitDialog(Settings &settings, QWidget *parent)
{
    NetworkConsentDialog::execRevisit(settings, parent);
}

}  // namespace chatterino::networkconsent
