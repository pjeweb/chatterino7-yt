// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "messages/Message.hpp"
#include "providers/youtube/YouTubeEvent.hpp"

namespace chatterino::youtube {
// Offline projection only. Structured IDs/roles remain in Event for later readers.
MessagePtr buildMessage(const Event &event);
}  // namespace chatterino::youtube
