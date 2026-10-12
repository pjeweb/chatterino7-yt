# Task board

Status: `todo` → `active` (worker running) → `review` → `merged` | `dropped`.
Only the supervisor edits this file. Task specs are the contract for workers.

| ID  | Title                                                   | Status | Build? | Depends         | Branch                 |
| --- | ------------------------------------------------------- | ------ | ------ | --------------- | ---------------------- |
| T00 | Foundation: dev build, privacy defaults, fork workflows | merged | yes    | –               | –                      |
| T01 | CI hygiene: zizmor + fork-safe upstream workflows       | merged | no     | T00             | PR #3                  |
| T02 | Network host gate (script + tests + workflow)           | merged | no     | T00             | PR #2 (also covers #1) |
| T03 | Risky-feature review and safe defaults                  | merged | yes    | T00             | PR #5                  |
| T04 | YouTube design doc (official API)                       | merged | no     | T00             | PR #4                  |
| T05 | Docs pass: fork README, docs index, diagrams            | merged | no     | T01–T04         |                        |
| T06 | Sanitizer test job (ASan/UBSan)                         | merged | yes    | T01             | PR #10                 |
| T07 | Release workflow: provenance, SBOM, checksums           | todo   | no     | T01             |                        |
| T08 | Coexistence: app name, settings dir, app ID             | merged | yes    | T03             | PR #7                  |
| T09 | YouTube implementation (slices from T04)                | active | yes    | T04             |                        |
| T10 | Translation design doc                                  | merged | no     | T00             | PR #6                  |
| T11 | Updater design doc (Sparkle/WinSparkle/Flathub)         | todo   | no     | T07             |                        |
| T12 | First-run network consent screen                        | todo   | yes    | T03             |                        |
| T13 | Re-enable full CI matrix and get it green               | todo   | no     | most of T01–T12 |                        |

---

## T01 CI hygiene

Make every workflow safe and quiet on the fork without diverging from upstream.

- Run `zizmor` (`pipx run zizmor .github/workflows` or a venv) over all
  workflows. Fix every finding in `fork-*.yml`. For upstream workflows, only
  fix with tiny diffs when the finding is real; list the rest in the report.
- Audit each upstream workflow for fork behaviour: anything that publishes
  (releases, tags, winget, codecov, PR comments) or needs secrets the fork
  doesn't have must be guarded with `github.repository_owner == 'SevenTV'`
  (or `'Chatterino'` where upstream already uses that) via a one-line `if:`.
  `build.yml`'s `create-release` is already guarded.
- Add `.github/zizmor.yml` only if needed to configure (not silence) checks.
- Write `docs/fork/CI.md`: a mermaid diagram of which workflows run on which
  events and what each guarantees; under 80 lines.
- Verify: zizmor output before and after in the report.

## T02 Network host gate

`scripts/fork/check-network-hosts.py` exists as a draft. Finish it:

- It must list hosts in lines added relative to the merge base with
  `upstream/chatterino7` under `src/`, and fail if a host isn't documented in
  `docs/fork/NETWORK.md` (backticked host names) or the link-only allowlist.
- Add `scripts/fork/test_check_network_hosts.py` (stdlib `unittest`, feeding
  synthetic diffs) and make the script testable without git.
- Add `.github/workflows/fork-network-hosts.yml` that runs the tests and the
  check on PRs (fetch upstream to compute the merge base; pinned actions,
  `permissions: {}`, `persist-credentials: false`).
- Verify: run both locally, including one deliberately failing case.

## T03 Risky-feature review and safe defaults

Review big upstream features for privacy, ToS and security tradeoffs against
the north star. Candidates: Lua plugins (`CHATTERINO_PLUGINS`), Kick support,
image uploader, browser extension native messaging
(`attachExtensionToAnyProcess`), streamlink, crashpad, link resolver,
spellcheck, IVR, recent-messages, 7TV/BTTV activity.

- Write `docs/fork/FEATURES.md`: a table of feature → risk → decision
  (keep / off by default / build option off) → how it's turned off. Keep it short.
- Implement only cheap, merge-friendly switches: build option defaults in the
  top-level `CMakeLists.txt`, setting defaults in `Settings.hpp`. Don't rip out
  code. If hiding Kick needs more than a few lines, document it as a
  recommendation instead.
- Verify: `scripts/dev/docker-build.sh check` passes (use `C7YT_JOBS=4`).

## T04 YouTube design doc

Write `docs/fork/youtube.md` (design only, no code) for a YouTube live chat
provider using **only the official YouTube Data API v3**:

- Module layout modelled on upstream's Kick provider (`src/providers/kick`):
  read it and mirror its shape (`YouTubeChannel`, `YouTubeChatServer`, API
  client, message builder). Note the hook points in upstream files and keep
  them few.
- Auth: user-supplied Google Cloud OAuth client (desktop app type), loopback
  redirect plus PKCE, scopes (least privilege), token storage via upstream's
  credential/keychain mechanism. Include a short user setup guide outline.
- Discovery: channel RSS feed (`/feeds/videos.xml?channel_id=`) +
  `videos.list` → `liveStreamingDetails.activeLiveChatId`. Handles: `@handle`
  → channel ID via `channels.list?forHandle=`.
- Reading: `liveChatMessages.streamList`; fallback `liveChatMessages.list`
  honouring `pollingIntervalMillis`. Check whether streamList is usable from
  Qt (HTTP streaming vs gRPC) and say which you'd use.
- Writing and moderation: insert, delete, bans (temporary/permanent), unban,
  moderators (owner only). Quota cost table and a daily budget example.
- Mermaid sequence diagrams for connect/read and for a ban.
- Test strategy: parsers tested against recorded JSON fixtures; network
  tests via the local httpbox where possible.
- Slice the implementation into 5–7 mergeable steps with acceptance criteria
  (this becomes T09a…).
- Cite the official docs pages you used. Verify claims against the docs
  rather than memory.

## T05 Docs pass

After T01–T04 land. Goal: better, not more.

- Replace the top of `README.md` with a short fork section (what, why, how
  it differs from chatterino7, links), keeping upstream's text below a
  separator so upstream merges stay easy.
- `docs/fork/README.md`: index of fork docs with one-line descriptions plus
  one architecture/data-flow mermaid diagram (what talks to which host, which
  parts are opt-in).
- Tighten `ROADMAP.md` and `NETWORK.md` wording; remove duplication between docs.

## T06 Sanitizer test job

- Get `scripts/dev/docker-build.sh asan` passing locally (fix the script if
  sanitizer flags differ; check `cmake/sanitizers-cmake` usage in upstream).
- Add `.github/workflows/fork-sanitizers.yml` running the test suite under
  ASan+UBSan on Linux in the official build container, on PRs and pushes to
  `chatterino7`. Same hardening rules as other fork workflows.
- If upstream code has real sanitizer findings, list them in the report;
  don't patch upstream code unless it's a clear, tiny fix.

## T07 Release workflow

`.github/workflows/fork-release.yml`, triggered by `v*` tags:

- Reuse upstream's build matrix artifacts (call or wait for `build.yml`'s
  run for the tag, or restructure so release jobs depend on the builds)
  without copying the whole build workflow.
- Produce `SHA256SUMS`, an SPDX SBOM (`anchore/sbom-action`), and build
  provenance attestations (`actions/attest-build-provenance`) for every
  artifact. Create a **draft** GitHub release.
- `docs/fork/RELEASES.md`: how to verify a download (`gh attestation verify`,
  `sha256sum -c`), short.

## T08 Coexistence

Find how chatterino7 differs from chatterino2 in app name, settings
directory, desktop file/app ID and macOS bundle ID. Propose and implement the
smallest change that lets chatterino7-yt be installed and run alongside
Chatterino and chatterino7 without sharing settings. Document it in the report.

## T09 YouTube implementation

Sliced by T04's design. Each slice is its own task (T09a, T09b, …) and branch.

## T10 Translation design doc

`docs/fork/translation.md`: on-device first (Bergamot/Firefox Translations
models: binary size, build complexity, model download/verification, CPU
cost per message), optional user-key DeepL and self-hosted LibreTranslate.
Compare honestly, including "BYO-key only" as the low-code option. Recommend
one, with a slicing plan. Cite sources.

## T11 Updater design doc

`docs/fork/updates.md`: Sparkle 2 (macOS) + WinSparkle (Windows) with an
EdDSA-signed appcast published by the release workflow; Flathub/package
managers on Linux. Key management (where the private key lives, rotation),
opt-in update check UX, how it integrates with Qt and upstream's `Updates`
singleton. Cite sources.

## T12 First-run network consent

A small dialog shown once on first run listing the opt-in services from
`NETWORK.md` (recent messages, sub age, 7TV/BTTV activity, …) with
checkboxes, all off by default. Minimal hooks into upstream.

## T13 Re-enable full CI

Heavy upstream workflows were paused with `gh workflow disable` at the
user's request (see `.agents/README.md`). Before the first release, ask the
user, then re-enable them (`gh workflow enable <file>`), run them on
`chatterino7`, and fix anything fork-specific that fails on macOS/Windows.
