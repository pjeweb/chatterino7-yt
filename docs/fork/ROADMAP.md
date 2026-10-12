# chatterino7-yt roadmap

Fork of [chatterino7](https://github.com/SevenTV/chatterino7) with three goals:

1. **Privacy first.** No analytics. Third-party hosts are required for chat or
   opt-in — see [`NETWORK.md`](NETWORK.md) and [`FEATURES.md`](FEATURES.md).
2. **Verifiable builds.** Public CI from tagged commits, provenance + SBOM
   _(manual draft-release workflow, not yet run; see [`RELEASES.md`](RELEASES.md))_.
3. **YouTube live chat** via official Data API v3 only _(design in
   [`youtube.md`](youtube.md); no provider code yet)_.

Non-goals: platforms beyond Twitch and YouTube; emote providers beyond
7TV/BTTV/FFZ; Twitch private GQL or borrowed client IDs.

## Principles

- **Small upstream diff.** New work under `src/providers/{youtube,translation}`,
  `docs/fork`, plus tiny hooks elsewhere.
- **Official APIs only.** Twitch Helix/EventSub; YouTube Data API v3. No scraping.
- **Document every new host** in `NETWORK.md` (enforced for fork diffs by CI).
- **Automate or it rots.** Sync, checks, and (later) releases from CI.

## Phases

### 0. Foundation

- [x] GitHub fork of SevenTV/chatterino7 with full history
- [x] Reproducible local build (`scripts/dev/docker-build.sh`)
- [x] Scheduled upstream-sync workflow (needs `SYNC_TOKEN` for workflow pushes)
- [x] Coexistence identity: app name, app ID, settings directory (icons still
      shared pixels; no full visual rebrand)

### 1. Privacy

- [ ] Complete host inventory in [`NETWORK.md`](NETWORK.md) (Kick/private
      endpoints still incomplete)
- [x] Opt-in defaults: BTTV/7TV activity, recent-messages, IVR off; updater
      compiled out (`CHATTERINO_UPDATER=OFF`) — see FEATURES.md
- [ ] "Network services" settings page plus a first-run consent screen
- [x] CI host gate (`scripts/fork/check-network-hosts.py` +
      `fork-network-hosts.yml`)

### 2. CI/CD and supply chain

Landed so far: fork workflows for zizmor, CodeQL, Scorecard, network hosts,
upstream sync. Heavy build/test matrices remain **manually paused** — see
[`CI.md`](CI.md). Still open:

- [ ] Build matrix for Windows, macOS (universal) and Linux on every PR
- [ ] Renovate for submodules, Actions (SHA-pinned), conan and vcpkg
- [ ] clang-tidy, clang-format gate, OpenSSF Scorecard (Scorecard workflow
      exists; broader lint matrix incomplete)
- [ ] Tests under ASan/UBSan on Linux
- [ ] release-please for versioning and changelogs
- [ ] Releases: provenance, SPDX SBOM, SHA256SUMS
- [ ] Signing: SignPath (Windows), Flathub (Linux); macOS ad-hoc until a
      Developer ID exists — **not claimed done**
- [ ] Automated publishing to Flathub, winget and a Homebrew tap

### 3. YouTube live chat (official API) — design only

See [`youtube.md`](youtube.md). No `src/providers/youtube` yet.

- [ ] Provider modules (`YouTubeChannel` / `YouTubeChatServer`, …)
- [ ] User-supplied Google OAuth client; tokens in system keychain
- [ ] Live discovery (Atom + `videos.list`) and chat read/send/moderate
- [ ] Quota meter (10,000 units/day per Google Cloud project)

### 4. Translation — design only

See [`translation.md`](translation.md). No provider code yet.

- [ ] On-device Bergamot; optional user-key DeepL / self-hosted LibreTranslate
- [ ] Per-channel/language rules; original text always viewable

### 5. Updates — not implemented

- [ ] Sparkle 2 / WinSparkle with EdDSA-signed appcast; Flathub on Linux
- [ ] Opt-in update check (today: compiled out, not replaced)

### 6. Twitch moderation via Helix (optional)

Official-API features upstream lacks: polls/predictions, unban requests,
blocked terms, AutoMod settings, stream title/category, channel-point
redemption queue (broadcaster, own rewards only).
