// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QByteArray>
#include <QDateTime>
#include <QJsonValue>
#include <QString>

#include <optional>

namespace chatterino::youtube {

enum class EventKind {
    Unknown,
    Text,
    Tombstone,
    Ban,
    SuperChat,
    SuperSticker,
    NewMember,
    MemberMilestone,
    MembershipGift,
    MembershipReceived,
    ChatEnded,
};

// Role evidence is a hint, never authorization. Missing/wrong-type is unknown.
struct Author {
    QString channelId;
    QString displayName;
    std::optional<bool> owner;
    std::optional<bool> moderator;
    std::optional<bool> member;
};

struct Event {
    EventKind kind = EventKind::Unknown;
    QString rawType;
    QString messageId;
    QString chatId;
    QDateTime publishedAt;
    std::optional<bool> hasDisplayContent;
    Author author;
    QString text;
    QString targetMessageId;
    QString targetChannelId;
    QString targetDisplayName;
    QString banType;
    std::optional<quint64> banDurationSeconds;
    QString amountMicros;  // Decimal string: never round through double.
    QString currency;
    QString amountDisplay;
    QString memberLevel;
    std::optional<quint64> memberMonths;
    std::optional<quint64> giftCount;
    std::optional<bool> membershipUpgrade;
    QString gifterChannelId;
    QString giftingMessageId;
};

// One resource, not a list envelope. Wrong shapes produce a safe unknown notice.
Event parseEvent(const QJsonValue &resource);

// Invalid JSON and non-object documents use the same unknown notice.
Event parseEventJson(const QByteArray &json);

}  // namespace chatterino::youtube
