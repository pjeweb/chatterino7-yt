// SPDX-FileCopyrightText: 2026 Contributors to chatterino7-yt
//
// SPDX-License-Identifier: MIT

#include "widgets/dialogs/NetworkConsentDialog.hpp"

#include "common/ForkIdentity.hpp"
#include "singletons/Settings.hpp"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace chatterino {

namespace {

QCheckBox *makeServiceCheckBox(const QString &title, const QString &disclosure,
                               QWidget *parent)
{
    auto *box = new QCheckBox(title, parent);
    box->setChecked(false);
    box->setToolTip(disclosure);
    return box;
}

}  // namespace

NetworkConsentDialog::NetworkConsentDialog(Mode mode, const Settings &settings,
                                           QWidget *parent)
    : QDialog(parent)
{
    this->setWindowTitle(QString("%1 — network services")
                             .arg(QLatin1String(fork::DISPLAY_NAME)));
    this->setModal(true);

    auto *layout = new QVBoxLayout(this);

    auto *intro = new QLabel(
        mode == Mode::FirstRun
            ? QStringLiteral(
                  "Optional third-party services can send channel or account "
                  "data when enabled. Everything below starts off. Accept "
                  "only enables the boxes you check; cancel leaves them off "
                  "and will not ask again on startup.")
            : QStringLiteral(
                  "Review optional third-party services. Accept saves the "
                  "boxes below; cancel leaves your current choices unchanged. "
                  "Required Twitch chat and emote providers are not listed."),
        this);
    intro->setWordWrap(true);
    layout->addWidget(intro);

    this->recentMessages_ = makeServiceCheckBox(
        QStringLiteral(
            "Load chat history on join (recent-messages.robotty.de)"),
        QStringLiteral(
            "Fetches recent chat when you join a channel. Sends the channel "
            "name to recent-messages.robotty.de."),
        this);
    layout->addWidget(this->recentMessages_);

    this->ivrSubAge_ = makeServiceCheckBox(
        QStringLiteral("Show sub age on user cards (api.ivr.fi)"),
        QStringLiteral(
            "Looks up subscription age when a user card opens. Sends the "
            "target user and channel to api.ivr.fi."),
        this);
    layout->addWidget(this->ivrSubAge_);

    this->sevenTVActivity_ = makeServiceCheckBox(
        QStringLiteral("Send 7TV activity / presence"),
        QStringLiteral(
            "Tells 7TV which channel you are in (presence) and signals "
            "activity so others can see your cosmetics. Uses 7tv.io "
            "presence endpoints."),
        this);
    layout->addWidget(this->sevenTVActivity_);

    this->bttvActivity_ = makeServiceCheckBox(
        QStringLiteral("Send BetterTTV activity / presence"),
        QStringLiteral(
            "Tells BetterTTV which channel you are in (broadcast_me) so "
            "others can see your cosmetics. Uses sockets.betterttv.net."),
        this);
    layout->addWidget(this->bttvActivity_);

    if (mode == Mode::Revisit)
    {
        const auto current = networkconsent::choicesFromSettings(settings);
        this->recentMessages_->setChecked(current.recentMessages);
        this->ivrSubAge_->setChecked(current.ivrSubAge);
        this->sevenTVActivity_->setChecked(current.sevenTVActivity);
        this->bttvActivity_->setChecked(current.bttvActivity);
    }

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("Accept"));
    buttons->button(QDialogButtonBox::Cancel)
        ->setText(QStringLiteral("Cancel"));
    QObject::connect(buttons, &QDialogButtonBox::accepted, this,
                     &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, this,
                     &QDialog::reject);
    layout->addWidget(buttons);
}

networkconsent::Choices NetworkConsentDialog::selectedChoices() const
{
    return {
        .recentMessages = this->recentMessages_->isChecked(),
        .ivrSubAge = this->ivrSubAge_->isChecked(),
        .sevenTVActivity = this->sevenTVActivity_->isChecked(),
        .bttvActivity = this->bttvActivity_->isChecked(),
    };
}

void NetworkConsentDialog::execFirstRun(Settings &settings, QWidget *parent)
{
    NetworkConsentDialog dialog(Mode::FirstRun, settings, parent);
    if (dialog.exec() == QDialog::Accepted)
    {
        networkconsent::accept(settings, dialog.selectedChoices());
    }
    else
    {
        networkconsent::declineFirstRun(settings);
    }
}

void NetworkConsentDialog::execRevisit(Settings &settings, QWidget *parent)
{
    NetworkConsentDialog dialog(Mode::Revisit, settings, parent);
    if (dialog.exec() == QDialog::Accepted)
    {
        networkconsent::accept(settings, dialog.selectedChoices());
    }
    // Cancel: leave marker and service toggles unchanged.
}

}  // namespace chatterino
