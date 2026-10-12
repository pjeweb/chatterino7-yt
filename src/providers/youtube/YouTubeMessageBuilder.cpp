// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "providers/youtube/YouTubeMessageBuilder.hpp"

#include "messages/MessageBuilder.hpp"

namespace chatterino::youtube {
MessagePtr buildMessage(const Event &event)
{
    MessageBuilder builder;
    builder->id = event.messageId;
    builder->userID = event.author.channelId;
    builder->displayName = event.author.displayName;
    builder->channelName = event.chatId;
    builder->serverReceivedTime = event.publishedAt;
    builder->messageText = event.text;
    QString rendered;
    if (event.author.owner.value_or(false))
    {
        rendered += QStringLiteral("[owner] ");
    }
    if (event.author.moderator.value_or(false))
    {
        rendered += QStringLiteral("[mod] ");
    }
    if (event.author.member.value_or(false))
    {
        rendered += QStringLiteral("[member] ");
    }
    if (!event.author.displayName.isEmpty())
    {
        rendered += event.author.displayName + ": ";
    }
    rendered += event.text;
    builder->searchText = rendered;
    builder.emplace<TextElement>(rendered, MessageElementFlag::Text,
                                 MessageColor::Text);
    if (event.kind != EventKind::Text)
    {
        builder->flags.set(MessageFlag::System);
    }
    auto message = builder.release();
    message->freeze();
    return message;
}
}  // namespace chatterino::youtube
