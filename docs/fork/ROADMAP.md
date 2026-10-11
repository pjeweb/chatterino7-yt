# chatterino7-yt roadmap

chatterino7-yt is a fork of [chatterino7](https://github.com/SevenTV/chatterino7)
with three goals:

1. **Privacy first.** No analytics, no heartbeats. Every third-party service is
   either required for chat to work or opt-in.
2. **Verifiable builds.** Every release is built by public CI from a tagged
   commit, with build provenance attestations and an SBOM.
3. **YouTube live chat.** Uses only the official YouTube Data API v3,
   including the moderation actions it allows.

Non-goals: platforms other than Twitch and YouTube, emote providers beyond 7TV,
BTTV and FFZ, and anything that needs Twitch's private GQL API or borrowed
client IDs.

## Principles

- **Small diff against upstream.** New features live in their own directories
  (`src/providers/youtube`, `src/providers/translation`, `docs/fork`, ...). Edits
  to upstream files are kept to small hooks so upstream merges stay cheap.
- **Official APIs only.** Twitch features go through Helix and EventSub; YouTube
  features go through the Data API v3. No scraping, no private endpoints.
- **Nothing phones home by default.** A feature that contacts a new host has to
  be listed in [`NETWORK.md`](NETWORK.md) and is off until the user turns it on.
- **Automate it or it rots.** Dependency bumps, upstream syncs, releases and
  publishing all run from CI.

## Phases

### 0. Foundation

- [x] GitHub fork of SevenTV/chatterino7 with full history
- [x] Reproducible local build (`scripts/dev/docker-build.sh`)
- [ ] Scheduled upstream-sync workflow that opens a PR when chatterino7 moves
- [ ] Rebrand: app name, app ID, settings directory and icons, so the fork can
      be installed next to Chatterino/chatterino7

### 1. Privacy

- [ ] Inventory every outbound host in [`NETWORK.md`](NETWORK.md)
- [ ] Opt-in defaults for activity-revealing services: BTTV activity,
      7TV presences, recent-messages history, and the update check
- [ ] "Network services" settings page plus a first-run consent screen
- [ ] CI check that fails when a new hostname appears in `src/` without a
      `NETWORK.md` entry

### 2. CI/CD and supply chain

- [ ] Build matrix for Windows, macOS (universal) and Linux on every PR
- [ ] Renovate for submodules, GitHub Actions (pinned to SHAs), conan and vcpkg
- [ ] CodeQL (C++), clang-tidy, clang-format, zizmor (workflow linting),
      OpenSSF Scorecard
- [ ] Tests under ASan/UBSan on Linux
- [ ] release-please for versioning and changelogs (conventional commits)
- [ ] Releases: `actions/attest-build-provenance`, SBOM (SPDX), SHA256SUMS
- [ ] Signing: SignPath Foundation (Windows), Flathub (Linux); macOS ad-hoc
      until an Apple Developer ID is available
- [ ] Automated publishing to Flathub, winget and a Homebrew tap

### 3. YouTube live chat (official API)

- [ ] `YouTubeChannel` / `YouTubeChatServer` provider, modelled on upstream's
      Kick provider
- [ ] Credentials: user-supplied Google Cloud OAuth client (desktop app,
      loopback redirect, PKCE), tokens stored in the system keychain
- [ ] Find live videos via the channel's RSS feed + `videos.list`
      (`liveStreamingDetails.activeLiveChatId`)
- [ ] Read chat via `liveChatMessages.streamList`, falling back to
      `liveChatMessages.list` polling
- [ ] Render text, emoji, Super Chats, Super Stickers, memberships and
      membership gifts; member, moderator and owner badges
- [ ] Send messages (`liveChatMessages.insert`)
- [ ] Moderation: delete (`liveChatMessages.delete`), timeout and ban
      (`liveChatBans.insert`), unban (`liveChatBans.delete`), add and remove
      moderators (`liveChatModerators.*`, owner only)
- [ ] Quota meter in the UI (10,000 units per day per Google Cloud project)

### 4. Translation (incoming messages)

- [ ] On-device translation with Bergamot; language-pair models downloaded
      on demand, with their checksums verified
- [ ] Optional providers with the user's own key: DeepL, self-hosted
      LibreTranslate
- [ ] Per-channel and per-language rules; the original text is always viewable

### 5. Updates

- [ ] macOS: Sparkle 2 with an EdDSA-signed appcast
- [ ] Windows: WinSparkle with the same appcast and EdDSA signatures
- [ ] Linux: updates through Flathub (and package managers)
- [ ] Update check is opt-in and makes no request until enabled

### 6. Twitch moderation via Helix (optional)

Features that the official API supports but upstream doesn't have yet:
polls and predictions (broadcaster), unban requests, blocked terms,
AutoMod settings, stream title and category (broadcaster), and the
channel-point redemption queue (broadcaster, own rewards only).
