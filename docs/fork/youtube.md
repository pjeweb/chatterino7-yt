# YouTube live chat design

T04 proposal, checked against Google's official documentation on 2026-10-12.
No provider is implemented by this document. Use the YouTube Data API v3
(including its Live Streaming methods), user-owned credentials, and Google's
channel Atom feed. No scraping, private endpoints, credential proxy, or shared
project. Connecting a YouTube channel is an explicit network opt-in.

## Provider boundary

Mirror the responsibilities in `src/providers/kick`, not its private API or
WebSocket transports. New files belong in `src/providers/youtube/`:

| Module                                    | Responsibility                                                                        |
| ----------------------------------------- | ------------------------------------------------------------------------------------- |
| `YouTubeChannel`                          | `Channel` subclass; channel/video/chat IDs, state, send/delete entry points           |
| `YouTubeChatServer`                       | Weak channel registry, shared reader per live chat, cancellation on last split close  |
| `YouTubeApi`                              | Typed REST requests, errors, per-project quota estimates, injected endpoint for tests |
| `YouTubeLiveChatReader`                   | gRPC stream and REST polling behind one event interface                               |
| `YouTubeMessageBuilder`                   | Normalized event → existing `Message`/elements                                        |
| `YouTubeAccount`, `YouTubeAccountManager` | OAuth lifecycle, selected YouTube identity and granted scopes                         |
| `YouTubeDiscovery`                        | Handle resolution, Atom candidates, video metadata and explicit video selection       |

Keep upstream hooks localized: source registration in `src/CMakeLists.txt`;
server lifetime in `Application.{hpp,cpp}`; account integration in
`controllers/accounts/AccountController.*` and `common/ProviderId.hpp`;
channel/message types in `common/Channel.*` and `messages/Message.hpp`;
join/restore in `widgets/dialogs/SelectChannelDialog.*` and
`common/WindowDescriptors.*`. Add small dispatches in
`widgets/helper/ChannelView.cpp` and `controllers/commands/CommandController.cpp`
for provider moderation. Audit Twitch/Kick type checks in `SplitInput.cpp`
before enabling send. Defer multichannel support (`util/MultiChannel.*`) until
single-channel behavior works. Do not widen `isTwitchOrKickChannel()` blindly.

Persist stable channel/video identity; resolve a fresh chat ID on reconnect.
Use states `idle → resolving → connecting → reading`, plus `ended`,
`auth-required`, `quota-paused`, and cancellable `backoff`. Separate account,
channel owner, video and chat IDs; none is a display name.

## Accounts and setup

Google documents Desktop authorization with system-browser login, loopback
redirect and PKCE. Bind `127.0.0.1` on an ephemeral port before opening the
browser; generate a fresh verifier and `S256` challenge plus random `state`.
Request offline access for refresh; check granted scopes before enabling features.
Reject mismatched state, unexpected paths and duplicate callbacks; close the
listener on success, cancellation or timeout. Exchange the code at
`https://oauth2.googleapis.com/token` with the same redirect URI and verifier.
Import the user's Desktop client JSON; retain its optional client secret if
supplied. It is not proof of desktop-app confidentiality. Google does not
support incremental authorization for installed apps: switching from read to
write means a new authorization flow for the selected mode.
[Desktop OAuth guide](https://developers.google.com/identity/protocols/oauth2/native-app).

| Mode          | Scope (prefix `https://www.googleapis.com/auth/`) |
| ------------- | ------------------------------------------------- |
| Read          | `youtube.readonly`                                |
| Send/moderate | `youtube.force-ssl` only; also permits reads      |

Google's [REST discovery document](https://www.googleapis.com/discovery/v1/apis/youtube/v3/rest)
(revision `20261006`) confirms these scopes for `list` and mutations, but does
not include `streamList`. Verify read-only OAuth with streaming in T09d;
retain REST reading if unsupported. Public streaming also supports a user API
key, according to the demo; OAuth is the initial account path. There is no
chat-only write scope: `youtube.force-ssl` also grants substantial access to
videos, comments and captions. Explain that before consent; request no email,
profile, upload or partner scopes.
[Scope catalog](https://developers.google.com/identity/protocols/oauth2/scopes#youtube).

**Storage prerequisite:** this checkout has no reusable keychain backend.
`KickAccountData::save()` in `KickAccount.cpp` writes tokens and the client
secret with `QStringSetting`; Twitch's manager reads `/accounts/*/oauthToken`
from settings. Reuse account-selection signals, not that persistence. Introduce
a narrow secret-store adapter backed by system credential storage (for example
QtKeychain after dependency review), keyed by fork app ID, client ID and YouTube
identity. Persist refresh tokens and any imported secret there; keep access
tokens in memory and non-secret IDs/preferences in settings. If unavailable or
locked, offer session-only login; never silently fall back to plaintext.

Serialize refresh per account, honor token expiry, preserve an existing refresh
token when a response omits it, and stop on `invalid_grant` for reauthorization.
Send bearer tokens in headers; redact headers, OAuth bodies and callback URLs
from logs. Sign-out cancels readers and deletes local secrets; offer a separate
Google grant-revocation action. Google documents refresh-token expiry, including
seven days for External apps in Testing using YouTube scopes.
[OAuth token lifecycle](https://developers.google.com/identity/protocols/oauth2).

Setup guide outline:

1. Create a personal Google Cloud project and enable YouTube Data API v3.
2. Configure the consent screen/audience; add your account as a test user when
   using External/Testing. Explain token expiry and production verification
   requirements rather than promising a permanent unverified login.
3. Create an OAuth client of type **Desktop app** and import its JSON locally.
4. Choose read or send/moderate, sign in in the system browser, and confirm the
   selected YouTube channel using `channels.list(mine=true)`.
5. Join by video URL, channel ID or handle; show estimated quota and secret-store
   availability. User-owned credentials do not waive Google's policies.

The API does not support service-account login for this use case.
[YouTube authorization](https://developers.google.com/youtube/v3/guides/authentication).

## Discovery and reading

Resolve `@handle` with `channels.list(part=id,forHandle=...)`; cache its returned
channel ID. Use `channels.list(part=id,snippet,mine=true)` after login for account
identity. Both filters are documented; do not send them together.
[Channels list](https://developers.google.com/youtube/v3/docs/channels/list).

Read channel Atom candidates, then batch candidate video IDs with
`videos.list(part=snippet,liveStreamingDetails,id=...)`.
[Videos list](https://developers.google.com/youtube/v3/docs/videos/list).
Connect only when `liveStreamingDetails.activeLiveChatId` is present; it exists
for currently live videos with chat and disappears on completion. A channel
can have multiple candidates: offer selection and direct video-URL entry.
[Video live details](https://developers.google.com/youtube/v3/docs/videos#liveStreamingDetails.activeLiveChatId).

The task's `/feeds/videos.xml?channel_id=` is a compatibility candidate, not a
discovery guarantee. Google's official Atom guide documents
`https://www.youtube.com/xml/feeds/videos.xml?channel_id=...`. Prefer that
documented path; validate either path before implementation. The guide describes
upload/title/description notifications, not complete live-start discovery.
Use the feed only as a bounded candidate list; it may miss an older scheduled
or unlisted broadcast. On misses, ask for the video URL. Poll candidates at a
modest, configurable interval only while waiting for a channel; stop when closed.
[Official feed guide](https://developers.google.com/youtube/v3/guides/push_notifications).

**Transport choice:** propose a C++ gRPC client for `streamList`, generated from
Google's official `stream_list.proto`. The guide's working demo is Python;
it links to C++ client generation, but supplies no C++ implementation. Use
TLS to `youtube.googleapis.com:443`, with a cancellable worker delivering queued
Qt events. gRPC runs over HTTP/2; it is not a plain JSON `QNetworkReply` stream.
The current method page mentions web traffic but does not specify its HTTP
framing; do not assume SSE or newline-delimited JSON. Existing
`common/network/NetworkRequest.hpp` exposes completed-response callbacks, not
a streaming consumer. This adds protobuf/gRPC dependencies: T09d must prove
packaging on all target OSes before streaming is enabled.
[Streaming guide and proto](https://developers.google.com/youtube/v3/live/streaming-live-chat).

Request `id,snippet,authorDetails`, with `maxResults=500`. Save each fully
processed response's `nextPageToken` and resume with `pageToken`; deduplicate
message IDs across responses and reconnects. Initial history is limited, not
an archive. Treat chat-disabled/ended as terminal; the documented gRPC
`FAILED_PRECONDITION` cannot currently distinguish them. Recheck video state
once to improve the explanation. A stream call costs 5 units, including each
reconnection; no fixed connection lifetime is promised.
[Stream list](https://developers.google.com/youtube/v3/live/docs/liveChatMessages/streamList).

Fallback is REST `liveChatMessages.list` through `NetworkRequest`, with the
returned token and a single-shot Qt timer waiting **at least**
`pollingIntervalMillis` after every response. Never loop immediately through
pages. Use bounded jittered backoff for transient failures; distinguish
permission, rate-limit and project quota errors. Pause at quota exhaustion;
never retry it indefinitely or automatically switch to another project.
On an invalid continuation, reconnect without it and show a possible history
gap. Changing transport must not run two readers or promise token
interoperability until tested.
[List contract](https://developers.google.com/youtube/v3/live/docs/liveChatMessages/list).

```mermaid
sequenceDiagram
    actor User
    participant Channel as YouTubeChannel
    participant Discovery
    participant API as Google API/feed
    participant Reader
    User->>Channel: Join handle or video URL (opt-in)
    Channel->>Discovery: Resolve candidates
    Discovery->>API: channels.list if handle; Atom feed if channel
    Discovery->>API: videos.list(candidate IDs)
    API-->>Discovery: Active chat ID(s)
    Discovery-->>Channel: Selected video/chat or explicit missing state
    Channel->>Reader: Start one cancellable reader
    Reader->>API: streamList(pageToken if resuming)
    loop While connected
        API-->>Reader: Protobuf response + nextPageToken
        Reader-->>Channel: Queued normalized events; deduplicate IDs
    end
    opt Streaming unavailable; quota and permissions allow fallback
        Reader->>API: list(pageToken)
        API-->>Reader: JSON + nextPageToken + pollingIntervalMillis
        Note over Reader: Wait at least pollingIntervalMillis before next list
    end
    User->>Channel: Close last split
    Channel->>Reader: Cancel RPC/request/timer
```

## Rendering and actions

Normalize JSON and protobuf into one event model. Render text and Unicode
emoji, paid-message amount/display text, membership and gifting notices; use
`authorDetails` owner/moderator/member flags for local badge glyphs. Super Sticker
image URLs are explicitly unavailable: show supplied alt text and amount.
The resource does not expose a custom-emote/member-badge asset catalog; do not
promise YouTube's full visual fidelity. Apply deletion/ban events to cached
messages and use readable fallback text for unknown events (including polls).
[Message schema](https://developers.google.com/youtube/v3/live/docs/liveChatMessages).

| Action            | Request and permission boundary                                                                                                      |
| ----------------- | ------------------------------------------------------------------------------------------------------------------------------------ |
| Send              | `messages.insert(part=snippet)`: chat ID, `type=textMessageEvent`, `textMessageDetails.messageText`; authenticated permitted chatter |
| Delete            | `messages.delete(id=messageId)`; owner or moderator, still subject to protected-message rejection                                    |
| Timeout/ban       | `bans.insert(part=snippet)`: chat ID, target channel ID, `type=temporary` plus duration in seconds, or `permanent`; owner/moderator  |
| Unban             | `bans.delete(id=banResourceId)`; owner/moderator; never substitute user/channel/message ID                                           |
| Manage moderators | `moderators.list/insert/delete`; chat owner only; deletion requires moderator resource ID                                            |

These operations use the write scope above. Official references:
[send](https://developers.google.com/youtube/v3/live/docs/liveChatMessages/insert),
[delete](https://developers.google.com/youtube/v3/live/docs/liveChatMessages/delete),
[ban](https://developers.google.com/youtube/v3/live/docs/liveChatBans/insert),
[unban](https://developers.google.com/youtube/v3/live/docs/liveChatBans/delete),
[list moderators](https://developers.google.com/youtube/v3/live/docs/liveChatModerators/list),
[add moderator](https://developers.google.com/youtube/v3/live/docs/liveChatModerators/insert),
[remove moderator](https://developers.google.com/youtube/v3/live/docs/liveChatModerators/delete).

Observe the current account's own chat author flags when available; owner
identity can also be compared with the video's channel ID. Treat role evidence
as a UI hint, with explicit unknown state; Google decides permissions for every
mutation. Moderators cannot enumerate or change other moderators via these
methods, and attempts to ban an owner/moderator or delete protected messages
can fail. Do not infer moderator status from possession of a write scope.

Store the returned ban ID with chat and target ID after a successful ban.
There is no `liveChatBans.list`: unban is available only for known ban IDs;
bans created elsewhere may require YouTube's UI.
[Ban methods](https://developers.google.com/youtube/v3/live/docs/liveChatBans).
No API acknowledgement means no success toast. Never automatically replay a
send or ban after an ambiguous network failure; it may already have succeeded.

```mermaid
sequenceDiagram
    actor Mod
    participant Channel as YouTubeChannel
    participant API as YouTubeApi
    participant Google
    Mod->>Channel: Timeout target channel ID for N seconds
    Channel->>API: Check write scope, role hint, quota budget
    API->>Google: liveChatBans.insert(chatId, temporary, targetId, N)
    alt API success
        Google-->>API: Ban resource with id
        API-->>Channel: Save ban ID; confirm timeout; enable known-ID unban
    else Permission/protected target/quota error
        Google-->>API: Error
        API-->>Channel: Explain failure; no optimistic success
    else Connection lost before acknowledgement
        API-->>Channel: Outcome unknown; no automatic replay
    end
```

## Quota and network budget

Current official costs (not historical values):

| Method                                                   | Units per call |
| -------------------------------------------------------- | -------------: |
| `channels.list`, `videos.list`                           |              1 |
| `liveChatMessages.streamList`, `liveChatMessages.list`   |              5 |
| `liveChatMessages.insert`, `liveChatMessages.delete`     |             20 |
| `liveChatBans.insert`, `liveChatBans.delete`             |            200 |
| `liveChatModerators.list`                                |             50 |
| `liveChatModerators.insert`, `liveChatModerators.delete` |            200 |

The combined default budget for these endpoints is 10,000 units per project
per day, resetting at midnight Pacific Time (DST-aware). Every extra page and
reconnection is another call; invalid calls consume at least one unit.
[Quota calculator](https://developers.google.com/youtube/v3/determine_quota_cost).

Example: one chat with one streaming connection and nine reconnects (50), 20
discovery requests (20), 50 sends (1,000), 10 deletions (200), 10 bans/timeouts
(2,000), two unbans (400), one moderator-list page (50) and one moderator change
(200) totals **3,920 units**. This assumes the stream remains available; account
setup and failed calls are extra. At a hypothetical allowed five-second polling
interval, reading alone costs `720 × 5 = 3,600 units/hour`; three hours exceeds
the default budget. Do not ship fallback as unlimited always-on chat.

Show a local **estimate**, shared across all readers for a client/project,
with a configurable daily cap and moderation reserve. Other applications or
devices consume the same project's quota; the estimate is not Google's balance.
Cancel fallback before its cap, explain the state, and link the user's Cloud
quota console. No automatic credential/project rotation.

T09 must update `NETWORK.md` before adding URLs: REST `www.googleapis.com`,
gRPC `youtube.googleapis.com`, OAuth `accounts.google.com`/
`oauth2.googleapis.com`, and feed `www.youtube.com`. Separate browser login from
application requests. Leave avatar fetches off initially; enabling them needs
a review of API-returned image hosts, redirect policy and network inventory.
YouTube reader gRPC settings must not add periodic keepalive pings by default.

## Mergeable implementation slices

Each slice owns tests and documentation; no production network calls until
its opt-in path is complete.

| Slice | Scope                                                           | Acceptance                                                                                                                                                          |
| ----- | --------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| T09a  | Provider types, normalized events, parsers, minimal hooks       | Builds with no network activity; recorded/sanitized JSON fixtures cover unknown/missing fields and renderer output                                                  |
| T09b  | Desktop OAuth, account manager, secret-store dependency/backend | Loopback/PKCE/state tests; wrong-state/replay/cancel rejected; refresh serialized; no secrets in settings/logs; unavailable store gives session-only login          |
| T09c  | Discovery + capped REST reader                                  | Handle/video joins, official Atom path validation, multiple/no-chat states; tokens and server polling delay honored; quota cap stops reads; last split cancels work |
| T09d  | gRPC `streamList`, default reader after validation              | Dependency packaging on Linux/macOS/Windows; protobuf/JSON parity; read-only OAuth smoke check; bounded reconnect/dedup; clean shutdown; explicit capped fallback   |
| T09e  | Sending and paid/membership display                             | Write-mode reauthorization; successful insert reconciles with incoming ID; ambiguous failures do not replay; sticker alt text; member/mod/owner glyphs              |
| T09f  | Delete, timeout/ban, known-ID unban, owner moderator management | Owner/mod/viewer matrix; protected targets and quota errors; returned resource IDs retained; unknown ban IDs explained; no unauthorized success UI                  |

Parser fixtures should include text, emoji, paid messages, memberships/gifts,
deleted/banned events, unknown types and incomplete resources; retain schema
provenance and remove personal data. Test protobuf normalization separately.
Use injected clocks/transports for polling, tokens, deduplication, quotas,
auth refresh, cancellation and late callbacks. Exercise HTTP headers, JSON
bodies, failures and timeouts against local httpbox where supported
(`CHATTERINO_TEST_HTTPBOX_ADDR=127.0.0.1:9051`, public-httpbox mode disabled);
use a small local scripted fixture server for sequences httpbox cannot model.
httpbox is not a gRPC server: test streaming with a local fake service generated
from the same proto. Live Google tests are opt-in with the user's project and a
controlled broadcast, never unattended CI. Validate read scope, role behavior,
stream resumptions, transport switching, feed availability and quota accounting
there before calling those contracts proven.
