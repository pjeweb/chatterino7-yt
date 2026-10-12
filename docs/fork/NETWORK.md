# Network services

Every host the app talks to, why, and whether it is on by default. New hosts
in `src/` must appear here in backticks.
`scripts/fork/check-network-hosts.py` enforces this in CI for URLs added since
upstream. Hosts match exactly (a subdomain is not covered by its parent);
comment-only lines and loopback addresses are ignored.

"Upstream default" is chatterino7's; "Fork default" is ours. Rows marked
_planned_ are **not implemented**. Feature switches and evidence live in
[`FEATURES.md`](FEATURES.md); goals in [`ROADMAP.md`](ROADMAP.md).

## Required for chat

| Host                                                                             | Purpose                           | Data sent                                |
| -------------------------------------------------------------------------------- | --------------------------------- | ---------------------------------------- |
| `irc.chat.twitch.tv`, `eventsub.wss.twitch.tv`, `pubsub-edge.twitch.tv`          | Twitch chat and events            | Your Twitch token, the channels you join |
| `api.twitch.tv`, `id.twitch.tv`                                                  | Twitch Helix API, login           | Your Twitch token                        |
| `static-cdn.jtvnw.net`                                                           | Twitch emotes, badges, avatars    | IP address, requested images             |
| `www.googleapis.com`, `oauth2.googleapis.com`, `accounts.google.com` _(planned)_ | YouTube Data API v3, login        | Your Google token and OAuth client       |
| `www.youtube.com/feeds/videos.xml` _(planned)_                                   | Finding live videos for a channel | The channel ID                           |

## Emotes and badges

| Host                                                      | Purpose                                                              | Upstream default | Fork default                |
| --------------------------------------------------------- | -------------------------------------------------------------------- | ---------------- | --------------------------- |
| `7tv.io` (+ image CDNs returned by the API)               | 7TV emotes, paints, badges                                           | on               | on                          |
| `events.7tv.io`                                           | 7TV live emote and cosmetic updates                                  | on               | on                          |
| `7tv.io/v3/users/*/presences`                             | **Tells 7TV which channel you are in**, so others see your cosmetics | on               | **off** (first-run consent) |
| `api.betterttv.net`, `cdn.betterttv.net`                  | BTTV emotes and badges                                               | on               | on                          |
| `sockets.betterttv.net`                                   | BTTV live emote updates                                              | on               | on                          |
| `sockets.betterttv.net` (broadcastMe)                     | **Tells BTTV which channel you are in**                              | on               | **off** (first-run consent) |
| `api.frankerfacez.com` (+ image CDNs returned by the API) | FFZ emotes and badges                                                | on               | on                          |
| `api.chatterino.com/badges`                               | Chatterino supporter badge list (one fetch, no user data)            | on               | on                          |
| `cdn.jsdelivr.net`, `chatterino2-emoji-cdn.pajlada.se`    | Emoji images                                                         | on               | on                          |

## Optional services

| Host                                               | Purpose                                                       | Upstream default       | Fork default                                  |
| -------------------------------------------------- | ------------------------------------------------------------- | ---------------------- | --------------------------------------------- |
| `recent-messages.robotty.de`                       | Chat history when you join a channel (sends the channel name) | on                     | **off** (first-run consent)                   |
| `braize.pajlada.com`                               | Link previews (sends every link you hover over)               | off                    | off                                           |
| `api.pronouns.alejo.io`                            | Pronouns on user cards                                        | off                    | off                                           |
| `api.ivr.fi`                                       | Sub age shown on user cards (sends the user and channel)      | on (when a card opens) | **off** (first-run consent)                   |
| Image uploader (user-configured)                   | Pasting images into chat                                      | off                    | off                                           |
| `7tv.io/v3/chatterino/version`                     | Update check                                                  | on                     | **compiled out** (`CHATTERINO_UPDATER=OFF`)   |
| Translation provider (user-configured) _(planned)_ | DeepL / LibreTranslate; on-device Bergamot needs model hosts  | n/a                    | off until implemented; hosts TBD in this file |

## First-run network consent

On a **fresh** profile (`/misc/currentVersion` still empty), startup shows a
modal listing only these optional services, all unchecked: recent-messages
history, IVR sub age, 7TV activity/presence, BTTV activity/presence. Purpose
and data sent are disclosed in the dialog. **Accept** writes only the checked
boxes and sets `/misc/networkConsentCompleted`. **Cancel** / close sets the
same marker and does **not** enable any optional service. Required Twitch chat
and 7TV/BTTV/FFZ emote/live-update hosts are not consent choices. The compiled
out updater, unimplemented YouTube/translation hosts, pronouns, and link
previews are not offered here.

**Existing profiles** (non-empty `currentVersion` from a prior GUI launch) skip
the dialog: the marker is set once without changing saved service toggles.
This migration also runs in frameless embed mode so previously enabled
optional services stay available. A **fresh** frameless embed shows no dialog
and does not set the marker, so optional requests stay gated until consent is
completed another way (e.g. a normal GUI launch). Optional request sites also
require `networkConsentCompleted`, so a fresh profile cannot send those
requests before the first-run decision. Settings → “Review optional network
services…” reopens the dialog with current values; cancel there leaves
settings unchanged.

## Limits

No analytics or telemetry should be added. Transport keepalives for active
chat/WebSocket connections still run. Crashpad has no upload URL and defaults
off in CMake. Inherited Kick support still uses private web endpoints and has
**no off switch** — not removed; hide/gate is follow-up. Inventory is not a
full audit of every upstream string.
