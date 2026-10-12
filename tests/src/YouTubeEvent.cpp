// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "providers/youtube/YouTubeEvent.hpp"

#include "messages/MessageElement.hpp"
#include "providers/youtube/YouTubeMessageBuilder.hpp"
#include "Test.hpp"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

using namespace chatterino;
using namespace chatterino::youtube;

namespace {
QJsonArray fixtures()
{
    QFile file(":/youtube/events.json");
    EXPECT_TRUE(file.open(QIODevice::ReadOnly));
    QJsonParseError error;
    auto doc = QJsonDocument::fromJson(file.readAll(), &error);
    EXPECT_EQ(error.error, QJsonParseError::NoError);
    return doc.array();
}
}  // namespace

TEST(YouTubeEvent, TextIdentityRolesAndRenderer)
{
    auto e = parseEvent(fixtures()[0]);
    EXPECT_EQ(e.kind, EventKind::Text);
    EXPECT_EQ(e.text, QString::fromUtf8("Hello 🌍 😀"));
    EXPECT_EQ(e.messageId, "event-0");
    EXPECT_EQ(e.chatId, "chat-fixture");
    EXPECT_EQ(e.author.channelId, "author-fixture");
    EXPECT_EQ(e.author.displayName, "Example Viewer");
    EXPECT_EQ(e.author.owner, true);
    EXPECT_EQ(e.author.moderator, false);
    EXPECT_EQ(e.author.member, true);
    EXPECT_TRUE(e.publishedAt.isValid());
    auto message = buildMessage(e);
    EXPECT_TRUE(message->frozen);
    EXPECT_EQ(message->id, e.messageId);
    EXPECT_EQ(message->userID, e.author.channelId);
    EXPECT_EQ(message->messageText, e.text);
    EXPECT_EQ(message->channelName, e.chatId);
    EXPECT_EQ(message->serverReceivedTime, e.publishedAt);
    ASSERT_EQ(message->elements.size(), 1);
    EXPECT_EQ(static_cast<TextElement *>(message->elements[0].get())
                  ->words()
                  .join(" "),
              message->searchText);
    EXPECT_EQ(
        message->searchText,
        QString::fromUtf8("[owner] [member] Example Viewer: Hello 🌍 😀"));
    EXPECT_FALSE(message->searchText.contains("[mod]"));
}

TEST(YouTubeEvent, PaidAndMembership)
{
    auto f = fixtures();
    auto paid = parseEvent(f[1]);
    EXPECT_EQ(paid.kind, EventKind::SuperChat);
    EXPECT_EQ(paid.amountMicros, "18446744073709551615");
    EXPECT_EQ(paid.currency, "USD");
    EXPECT_EQ(paid.text, "$5.00 Thanks!");
    auto sticker = parseEvent(f[2]);
    EXPECT_EQ(sticker.kind, EventKind::SuperSticker);
    EXPECT_EQ(sticker.text, "$2.00 Waving hello");
    auto member = parseEvent(f[3]);
    EXPECT_EQ(member.kind, EventKind::NewMember);
    EXPECT_EQ(member.membershipUpgrade, true);
    EXPECT_EQ(member.memberLevel, "Example level");
    auto milestone = parseEvent(f[4]);
    EXPECT_EQ(milestone.memberMonths, 12);
    EXPECT_EQ(milestone.text, "Membership milestone Example level One year!");
    auto gift = parseEvent(f[5]);
    EXPECT_EQ(gift.kind, EventKind::MembershipGift);
    EXPECT_EQ(gift.giftCount, 5);
    EXPECT_EQ(gift.text, "Memberships gifted 5 Example level");
    auto received = parseEvent(f[6]);
    EXPECT_EQ(received.kind, EventKind::MembershipReceived);
    EXPECT_EQ(received.gifterChannelId, "gifter-fixture");
    EXPECT_EQ(received.giftingMessageId, "event-5");
}

TEST(YouTubeEvent, ModerationAndEnded)
{
    auto f = fixtures();
    auto deleted = parseEvent(f[7]);
    EXPECT_EQ(deleted.kind, EventKind::Tombstone);
    EXPECT_EQ(deleted.targetMessageId, "event-7");
    EXPECT_EQ(deleted.text, "Message deleted");
    auto ban = parseEvent(f[8]);
    EXPECT_EQ(ban.kind, EventKind::Ban);
    EXPECT_EQ(ban.author.channelId, "moderator-fixture");
    EXPECT_EQ(ban.targetChannelId, "target-fixture");
    EXPECT_EQ(ban.targetDisplayName, "Example target");
    EXPECT_EQ(ban.banDurationSeconds, 300);
    EXPECT_EQ(ban.text, "Example target timed out for 300 seconds");
    auto rendered = buildMessage(ban);
    EXPECT_TRUE(rendered->frozen);
    EXPECT_EQ(rendered->messageText, ban.text);
    ASSERT_EQ(rendered->elements.size(), 1);
    EXPECT_EQ(static_cast<TextElement *>(rendered->elements[0].get())
                  ->words()
                  .join(" "),
              rendered->searchText);
    EXPECT_EQ(rendered->searchText, "Example target timed out for 300 seconds");
    EXPECT_FALSE(rendered->searchText.contains("moderator-fixture"));
    auto permanent = parseEvent(f[9]);
    EXPECT_EQ(permanent.banType, "permanent");
    EXPECT_FALSE(permanent.banDurationSeconds);
    EXPECT_TRUE(permanent.targetDisplayName.isEmpty());
    EXPECT_EQ(permanent.text, "target-fixture banned");
    EXPECT_EQ(parseEvent(f[13]).kind, EventKind::ChatEnded);
    auto missingTarget = parseEvent(QJsonObject{
        {"snippet", QJsonObject{{"type", "userBannedEvent"},
                                {"authorChannelId", "moderator-fixture"},
                                {"userBannedDetails",
                                 QJsonObject{{"banType", "temporary"},
                                             {"banDurationSeconds", "60"}}}}},
        {"authorDetails", QJsonObject{{"channelId", "moderator-fixture"},
                                      {"displayName", "Example Moderator"}}}});
    EXPECT_EQ(missingTarget.kind, EventKind::Ban);
    EXPECT_TRUE(missingTarget.targetDisplayName.isEmpty());
    EXPECT_TRUE(missingTarget.targetChannelId.isEmpty());
    EXPECT_EQ(missingTarget.author.displayName, "Example Moderator");
    EXPECT_EQ(missingTarget.text, "User timed out for 60 seconds");
    EXPECT_FALSE(missingTarget.text.contains("Example Moderator"));
    EXPECT_FALSE(missingTarget.text.contains("moderator-fixture"));
    auto suppliedDisplay = parseEvent(QJsonObject{
        {"snippet",
         QJsonObject{
             {"type", "userBannedEvent"},
             {"displayMessage", "Supplied ban notice"},
             {"userBannedDetails",
              QJsonObject{{"bannedUserDetails",
                           QJsonObject{{"channelId", "target-fixture"},
                                       {"displayName", "Example target"}}},
                          {"banType", "temporary"},
                          {"banDurationSeconds", "300"}}}}}});
    EXPECT_EQ(suppliedDisplay.text, "Supplied ban notice");
}

TEST(YouTubeEvent, UnknownMissingAndMalformed)
{
    auto f = fixtures();
    EXPECT_EQ(parseEvent(f[10]).text, "Example poll");
    EXPECT_EQ(parseEvent(f[11]).text, "Example Jewel gift");
    EXPECT_EQ(parseEvent(f[12]).rawType, "futureEvent");
    EXPECT_EQ(parseEvent(f[12]).kind, EventKind::Unknown);
    EXPECT_EQ(parseEvent(f[14]).text, "Unavailable YouTube message");
    for (int i = 15; i <= 18; ++i)
    {
        auto e = parseEvent(f[i]);
        EXPECT_EQ(e.kind, EventKind::Unknown);
        EXPECT_EQ(e.text, "Unsupported YouTube chat event");
        EXPECT_FALSE(e.author.owner);
        EXPECT_TRUE(e.messageId.isEmpty());
        EXPECT_FALSE(buildMessage(e)->elements.empty());
    }
    auto malformed = parseEvent(f[19]);
    EXPECT_FALSE(malformed.banDurationSeconds);
    EXPECT_FALSE(malformed.publishedAt.isValid());
    EXPECT_TRUE(malformed.targetChannelId.isEmpty());
}

TEST(YouTubeEvent, InvalidNumbersAndDisplayFallback)
{
    for (const auto &value :
         {QJsonValue(-1), QJsonValue(1.5), QJsonValue("18446744073709551616"),
          QJsonValue("oops"), QJsonValue(true), QJsonValue(9007199254740992.0)})
    {
        QJsonObject details{{"amountMicros", value}};
        auto e = parseEvent(QJsonObject{
            {"snippet", QJsonObject{{"type", "superChatEvent"},
                                    {"superChatDetails", details}}}});
        EXPECT_TRUE(e.amountMicros.isEmpty());
        EXPECT_EQ(e.text, "Super Chat");
    }
    auto missingDetail = parseEvent(QJsonObject{
        {"snippet", QJsonObject{{"type", "textMessageEvent"},
                                {"displayMessage", "Fallback text"}}}});
    EXPECT_EQ(missingDetail.text, "Fallback text");
    EXPECT_FALSE(missingDetail.author.member);
    auto emptyDetail = parseEvent(QJsonObject{
        {"snippet", QJsonObject{{"type", "textMessageEvent"},
                                {"displayMessage", "Fallback text"},
                                {"textMessageDetails",
                                 QJsonObject{{"messageText", ""}}}}}});
    EXPECT_EQ(emptyDetail.text, "Fallback text");
}

TEST(YouTubeEvent, JsonDecodingBoundary)
{
    for (const auto &json :
         {QByteArray("{broken"), QByteArray("[]"), QByteArray("null"),
          QByteArray("42"), QByteArray()})
    {
        auto event = parseEventJson(json);
        EXPECT_EQ(event.kind, EventKind::Unknown);
        EXPECT_EQ(event.text, "Unsupported YouTube chat event");
        EXPECT_TRUE(event.targetMessageId.isEmpty());
        EXPECT_TRUE(event.targetChannelId.isEmpty());
    }
    auto event = parseEventJson(
        R"({"id":"decoded-id","snippet":{"type":"textMessageEvent","hasDisplayContent":false,"textMessageDetails":{"messageText":"decoded text"}}})");
    EXPECT_EQ(event.messageId, "decoded-id");
    EXPECT_EQ(event.text, "decoded text");
    EXPECT_EQ(event.hasDisplayContent, false);
    EXPECT_FALSE(event.author.owner);
}
