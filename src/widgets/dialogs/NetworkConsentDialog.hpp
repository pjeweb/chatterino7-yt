// SPDX-FileCopyrightText: 2026 Contributors to chatterino7-yt
//
// SPDX-License-Identifier: MIT

#pragma once

#include "singletons/NetworkConsent.hpp"

#include <QDialog>

class QCheckBox;

namespace chatterino {

class Settings;

class NetworkConsentDialog : public QDialog
{
public:
    enum class Mode {
        FirstRun,
        Revisit,
    };

    NetworkConsentDialog(Mode mode, const Settings &settings,
                         QWidget *parent = nullptr);

    networkconsent::Choices selectedChoices() const;

    static void execFirstRun(Settings &settings, QWidget *parent);
    static void execRevisit(Settings &settings, QWidget *parent);

private:
    QCheckBox *recentMessages_{};
    QCheckBox *ivrSubAge_{};
    QCheckBox *sevenTVActivity_{};
    QCheckBox *bttvActivity_{};
};

}  // namespace chatterino
