// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "providers/youtube/YouTubeEvent.hpp"

#include <QJsonObject>
#include <QRegularExpression>

#include <cmath>

namespace chatterino::youtube {
namespace {
std::optional<bool> hint(const QJsonValue &value)
{
    return value.isBool() ? std::optional<bool>(value.toBool()) : std::nullopt;
}

std::optional<quint64> number(const QJsonValue &value)
{
    if (value.isString())
    {
        const auto text = value.toString();
        static const QRegularExpression decimal(QStringLiteral("^[0-9]+$"));
        bool ok = false;
        auto result = text.toULongLong(&ok);
        if (ok && decimal.match(text).hasMatch())
        {
            return result;
        }
    }
    else if (value.isDouble())
    {
        auto n = value.toDouble();
        // JSON doubles beyond this bound cannot preserve integer identity.
        if (std::isfinite(n) && n >= 0 && n <= 9007199254740991.0 &&
            std::floor(n) == n)
        {
            return static_cast<quint64>(n);
        }
    }
    return std::nullopt;
}
}  // namespace

Event parseEvent(const QJsonValue &resource)
{
    Event e;
    const auto root = resource.toObject();
    const auto s = root.value("snippet").toObject();
    const auto a = root.value("authorDetails").toObject();
    e.messageId = root.value("id").toString();
    e.chatId = s.value("liveChatId").toString();
    e.rawType = s.value("type").toString();
    e.publishedAt = QDateTime::fromString(s.value("publishedAt").toString(),
                                          Qt::ISODateWithMs);
    e.hasDisplayContent = hint(s.value("hasDisplayContent"));
    e.author.channelId = a.value("channelId").toString();
    if (e.author.channelId.isEmpty())
    {
        e.author.channelId = s.value("authorChannelId").toString();
    }
    e.author.displayName = a.value("displayName").toString();
    e.author.owner = hint(a.value("isChatOwner"));
    e.author.moderator = hint(a.value("isChatModerator"));
    e.author.member = hint(a.value("isChatSponsor"));
    e.text = s.value("displayMessage").toString();
    const auto type = e.rawType;
    QString fallback = QStringLiteral("Unsupported YouTube chat event");
    if (type == "textMessageEvent")
    {
        e.kind = EventKind::Text;
        const auto text =
            s.value("textMessageDetails").toObject().value("messageText");
        if (text.isString())
        {
            e.text = text.toString();
        }
        fallback = QStringLiteral("Unavailable YouTube message");
    }
    else if (type == "tombstone")
    {
        e.kind = EventKind::Tombstone;
        e.targetMessageId = e.messageId;
        fallback = QStringLiteral("Message deleted");
    }
    else if (type == "userBannedEvent")
    {
        e.kind = EventKind::Ban;
        const auto details = s.value("userBannedDetails").toObject();
        const auto target = details.value("bannedUserDetails").toObject();
        e.targetChannelId = target.value("channelId").toString();
        e.targetDisplayName = target.value("displayName").toString();
        e.banType = details.value("banType").toString();
        if (e.banType == "temporary")
        {
            e.banDurationSeconds = number(details.value("banDurationSeconds"));
        }
        fallback = e.banType == "temporary" ? QStringLiteral("User timed out")
                                            : QStringLiteral("User banned");
    }
    else if (type == "superChatEvent" || type == "superStickerEvent")
    {
        const bool sticker = type == "superStickerEvent";
        e.kind = sticker ? EventKind::SuperSticker : EventKind::SuperChat;
        const auto d =
            s.value(sticker ? "superStickerDetails" : "superChatDetails")
                .toObject();
        if (auto amount = number(d.value("amountMicros")))
        {
            e.amountMicros = QString::number(*amount);
        }
        e.currency = d.value("currency").toString();
        e.amountDisplay = d.value("amountDisplayString").toString();
        auto content = sticker ? d.value("superStickerMetadata")
                                     .toObject()
                                     .value("altText")
                                     .toString()
                               : d.value("userComment").toString();
        if (e.text.isEmpty())
        {
            e.text = (e.amountDisplay + " " + content).trimmed();
        }
        fallback = sticker ? QStringLiteral("Super Sticker")
                           : QStringLiteral("Super Chat");
    }
    else if (type == "newSponsorEvent" || type == "memberMilestoneChatEvent" ||
             type == "membershipGiftingEvent" ||
             type == "giftMembershipReceivedEvent")
    {
        QString key;
        if (type == "newSponsorEvent")
        {
            e.kind = EventKind::NewMember;
            key = "newSponsorDetails";
            fallback = QStringLiteral("New membership");
        }
        else if (type == "memberMilestoneChatEvent")
        {
            e.kind = EventKind::MemberMilestone;
            key = "memberMilestoneChatDetails";
            fallback = QStringLiteral("Membership milestone");
        }
        else if (type == "membershipGiftingEvent")
        {
            e.kind = EventKind::MembershipGift;
            key = "membershipGiftingDetails";
            fallback = QStringLiteral("Memberships gifted");
        }
        else
        {
            e.kind = EventKind::MembershipReceived;
            key = "giftMembershipReceivedDetails";
            fallback = QStringLiteral("Gift membership received");
        }
        const auto d = s.value(key).toObject();
        e.memberLevel = d.value(e.kind == EventKind::MembershipGift
                                    ? "giftMembershipsLevelName"
                                    : "memberLevelName")
                            .toString();
        e.memberMonths = number(d.value("memberMonth"));
        e.giftCount = number(d.value("giftMembershipsCount"));
        e.membershipUpgrade = hint(d.value("isUpgrade"));
        e.gifterChannelId = d.value("gifterChannelId").toString();
        e.giftingMessageId =
            d.value("associatedMembershipGiftingMessageId").toString();
        if (e.text.isEmpty())
        {
            e.text =
                (fallback +
                 (e.giftCount ? " " + QString::number(*e.giftCount)
                              : QString()) +
                 " " + e.memberLevel + " " + d.value("userComment").toString())
                    .trimmed();
        }
    }
    else if (type == "chatEndedEvent")
    {
        e.kind = EventKind::ChatEnded;
        fallback = QStringLiteral("Live chat ended");
    }
    if (e.text.isEmpty())
    {
        e.text = fallback;
    }
    return e;
}
}  // namespace chatterino::youtube
