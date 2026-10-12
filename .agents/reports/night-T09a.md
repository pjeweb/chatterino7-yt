# night/T09a — offline YouTube model

Branch: `night/T09a-youtube-model`; assigned checkout only.

## Implemented

- Provider-local `youtube::Event`, `Author`, event kinds and JSON-resource parser.
  Separates chat/message/author/moderation target IDs; preserves tri-state
  owner/moderator/member and display-content hints, timestamps, money as exact
  decimal micros, membership/gift attribution and temporary ban duration.
- Offline `buildMessage`: existing immutable Message layout, plain TextElement,
  Unicode, local textual role labels, readable notices, frozen const result.
  No avatar/custom emote/sticker asset requests or APIs.
- Minimal explicit source/test CMake registrations and QRC fixture registration.
  No Application/account/channel/UI/dispatch/network changes.

## Provenance and primary verification

Checked official pages via documentation browsing on 2026-10-12:
https://developers.google.com/youtube/v3/live/docs/liveChatMessages
(last updated 2026-09-14) and
https://developers.google.com/youtube/v3/live/streaming-live-chat
(including the embedded official `stream_list.proto`). Read inherited
Message/MessageBuilder/MessageElement, KickMessageBuilder, suite/CMake/build scripts.

`tests/resources/youtube/events.json` is a hand-authored, sanitized recording of
resource **shapes**, not a live capture. Fixture README documents order and fake
IDs/names/text. No personal data or credentials; no Google API calls. The uint64
maximum is a boundary case. Official current schema includes polls/Jewel gifts:
these retain supplied display text as unknown structured types. Neither current
resource representation nor proto documents messageDeletedEvent; documented
tombstones normalize deletion using the resource ID, without an invented
messageDeletedDetails API. Tombstones are historical placeholders, not proof of
a real-time deletion notification.

## Deferred hooks

All OAuth/account/secret storage, network/discovery/readers/protobuf parsing,
channel/cache mutation, deduplication, production rendering/platform dispatch,
moderation/send, and UI activation belong to subsequent slices. Existing
MessagePlatform/layout and Twitch/Kick dispatch helpers remain untouched;
this projection is offline and must not be inserted into production channels
until an explicit provider/platform path exists. Raw unknown types stay available;
missing target IDs/durations never substitute the event author or event ID.
Role hints never grant permissions. No billing/reset/fallbacks/workers.

## Validation checkpoint

Six suite tests added: `YouTubeEvent.TextIdentityRolesAndRenderer`,
`PaidAndMembership`, `ModerationAndEnded`, `UnknownMissingAndMalformed`,
`InvalidNumbersAndDisplayFallback`, and `JsonDecodingBoundary`. They assert normalized values and rendered
TextElement words, IDs, frozen output, role unknown/false/true, malformed numbers,
unknown/incomplete/wrong-shape records, syntactically invalid JSON and non-object
documents, notices, and Unicode.

Initial requested build stopped because assigned worktree submodules were empty.
Populated this checkout from pinned local submodule Git archives, reading the
existing object cache without modifying shared Git configuration or worktrees.
`C7YT_JOBS=4 scripts/dev/docker-build.sh check` is running; final results follow.
Formatting uses CI-pinned clang-format 19. Host gate runs after local commit.
