# Network services

Every host the app talks to, why, and whether it is on by default. If you
add a new host to `src/`, add it here as well, in backticks.
`scripts/fork/check-network-hosts.py` enforces this in CI for URLs added
since upstream. Hosts must match exactly (a subdomain is not covered by its
parent); comment-only lines and loopback addresses are ignored.

"Upstream default" is chatterino7's default; "Fork default" is ours. Rows
marked _planned_ are not implemented yet.

## Required for chat

| Host                                                                             | Purpose                           | Data sent                                |
| -------------------------------------------------------------------------------- | --------------------------------- | ---------------------------------------- |
| `irc.chat.twitch.tv`, `eventsub.wss.twitch.tv`, `pubsub-edge.twitch.tv`          | Twitch chat and events            | Your Twitch token, the channels you join |
| `api.twitch.tv`, `id.twitch.tv`                                                  | Twitch Helix API, login           | Your Twitch token                        |
| `static-cdn.jtvnw.net`                                                           | Twitch emotes, badges, avatars    | IP address, requested images             |
| `www.googleapis.com`, `oauth2.googleapis.com`, `accounts.google.com` _(planned)_ | YouTube Data API v3, login        | Your Google token and OAuth client       |
| `www.youtube.com/feeds/videos.xml` _(planned)_                                   | Finding live videos for a channel | The channel ID                           |

## Emotes and badges

| Host                                                      | Purpose                                                              | Upstream default | Fork default |
| --------------------------------------------------------- | -------------------------------------------------------------------- | ---------------- | ------------ |
| `7tv.io` (+ image CDNs returned by the API)               | 7TV emotes, paints, badges                                           | on               | on           |
| `events.7tv.io`                                           | 7TV live emote and cosmetic updates                                  | on               | on           |
| `7tv.io/v3/users/*/presences`                             | **Tells 7TV which channel you are in**, so others see your cosmetics | on               | **off**      |
| `api.betterttv.net`, `cdn.betterttv.net`                  | BTTV emotes and badges                                               | on               | on           |
| `sockets.betterttv.net`                                   | BTTV live emote updates                                              | on               | on           |
| `sockets.betterttv.net` (broadcastMe)                     | **Tells BTTV which channel you are in**                              | on               | **off**      |
| `api.frankerfacez.com` (+ image CDNs returned by the API) | FFZ emotes and badges                                                | on               | on           |
| `api.chatterino.com/badges`                               | Chatterino supporter badge list (one fetch, no user data)            | on               | on           |
| `cdn.jsdelivr.net`, `chatterino2-emoji-cdn.pajlada.se`    | Emoji images                                                         | on               | on           |

## Optional services

| Host                                               | Purpose                                                       | Upstream default       | Fork default                                                   |
| -------------------------------------------------- | ------------------------------------------------------------- | ---------------------- | -------------------------------------------------------------- |
| `recent-messages.robotty.de`                       | Chat history when you join a channel (sends the channel name) | on                     | **off** (first-run prompt _planned_)                           |
| `braize.pajlada.com`                               | Link previews (sends every link you hover over)               | off                    | off                                                            |
| `api.pronouns.alejo.io`                            | Pronouns on user cards                                        | off                    | off                                                            |
| `api.ivr.fi`                                       | Sub age shown on user cards (sends the user and channel)      | on (when a card opens) | **off**                                                        |
| Image uploader (user-configured)                   | Pasting images into chat                                      | off                    | off                                                            |
| `7tv.io/v3/chatterino/version`                     | Update check                                                  | on                     | **compiled out** (signed Sparkle/WinSparkle updater _planned_) |
| Translation provider (user-configured) _(planned)_ | Message translation via DeepL/LibreTranslate                  | n/a                    | off (on-device by default)                                     |

## Privacy policy and current limits

No analytics or telemetry should be added. Optional BTTV/7TV presence reports
are off by default; transport keepalives for active chat/WebSocket connections
still run. Crashpad currently has no upload URL configured and defaults off
in CMake; local crash recovery/dumps remain (see `LastRunCrashDialog`). Build
workflows can override CMake defaults; see [feature review](FEATURES.md).

This inventory is incomplete for inherited Kick support, which still uses
private web endpoints and has no off switch. Hiding/gating that provider and
reviewing fork OAuth ownership remain follow-up work; the roadmap describes
the intended policy rather than a complete audit of upstream code.
