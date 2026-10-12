// SPDX-FileCopyrightText: 2026 Contributors to chatterino7-yt
//
// SPDX-License-Identifier: MIT

#pragma once

class QWidget;

namespace chatterino {

class Settings;

/// First-run / revisit consent for optional third-party network services.
/// Required Twitch chat and 7TV/BTTV/FFZ emote APIs are not consent choices.
namespace networkconsent {

struct Choices {
    bool recentMessages = false;
    bool ivrSubAge = false;
    bool sevenTVActivity = false;
    bool bttvActivity = false;
};

/// Persistent marker: first-run dialog finished (accept or cancel) or an
/// existing profile was migrated without showing the dialog.
bool isCompleted(const Settings &settings);

/// Existing GUI profile: `currentVersion` is non-empty (written on prior
/// launches). Must be checked before Application overwrites that setting.
bool isExistingProfile(const Settings &settings);

bool needsFirstRunPrompt(const Settings &settings);

/// Optional third-party requests require a completed first-run decision.
bool allowsOptionalRequests(const Settings &settings);

Choices choicesFromSettings(const Settings &settings);

/// Apply checkbox values and mark consent completed. Only path that enables
/// optional services from the dialog.
void accept(Settings &settings, const Choices &choices);

/// Finish first-run without enabling anything (cancel / close). Does not
/// change service toggles.
void declineFirstRun(Settings &settings);

/// Migrate existing profiles (preserve saved service choices) or show the
/// first-run dialog for a fresh profile. Call once at startup before
/// `currentVersion` is updated and before Twitch connect.
void ensureFirstRunDecision(Settings &settings, QWidget *parent);

/// Re-open the dialog from settings; cancel leaves settings unchanged.
void showRevisitDialog(Settings &settings, QWidget *parent);

}  // namespace networkconsent
}  // namespace chatterino
