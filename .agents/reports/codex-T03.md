# Codex T03: risky-feature review and safe defaults

Worktree: `/home/pje/.t3/projects/c7yt-codex/T03`.
Branch: `codex/20261012-risky-features`.
Scope: T03 continuation, isolated from Claude-owned worktrees. The supervisor
authorized small BTTV runtime fixes after the audit found the existing false
default was not enforced. No pushes, PRs, workflow changes or shared-board edits.

## Changes

- Default `CHATTERINO_PLUGINS` to OFF with a fork comment. Runtime plugins
  already defaulted off; the build now also needs explicit opt-in.
- Fix BTTV presence: `sendBTTVActivity` was previously referenced only in the
  settings declaration/UI, while initial joins and periodic activity sent
  `broadcast_me` regardless. Keep public emote subscriptions; skip initial
  account presence when off or anonymous. Recheck consent at the real client
  send for periodic, pending and reconnect replay paths. Presence has no
  unsubscribe message, so leaving must not resend it.
- Add an outgoing-frame regression using the existing local WebSocket echo
  server for consent changes while pending, direct OFF/ON activity, reconnect
  replay, anonymous users and leaving. The existing emote integration test
  now runs with activity OFF and ON and checks optionals before dereferencing.
- Add `docs/fork/FEATURES.md` with code links and actual controls for every
  candidate. Avoid claiming build defaults cover workflow overrides or saved
  settings. Update the end of `NETWORK.md` to distinguish optional presence
  from required transport keepalives and document audit limits.

Files: `CMakeLists.txt`, `src/providers/bttv/BttvLiveUpdates.cpp`,
`src/providers/bttv/liveupdates/BttvLiveUpdateClient.{cpp,hpp}`,
`tests/src/BttvLiveUpdates.cpp`, `docs/fork/{FEATURES,NETWORK}.md`, this report.
No `Settings.hpp` edit was needed: its candidate defaults were already safe.

## Verification

- `git submodule update --init --recursive`: completed in this worktree.
- `C7YT_JOBS=4 scripts/dev/docker-build.sh check`: first full build passed,
  then 666/667 tests passed, including `ActivityConsent`. The old `AllEvents`
  test dereferenced an unset update optional. Inspection of the official
  [v1.0.12 fixture source](https://github.com/Chatterino/twitch-pubsub-server-test/blob/v1.0.12/internal/routes/liveupdates/bttv/basic.go)
  showed it intentionally waits for `broadcast_me` before emitting
  `emote_update`. That trigger is correctly blocked by the off default.
  The test now covers public create/remove with activity OFF, and all three
  fixture events with explicit opt-in. Production broadcasts were not restored.
  First log: `/tmp/codex-T03-check.log`.
- Final `C7YT_JOBS=4 scripts/dev/docker-build.sh check`: passed, exit 0;
  **668/668 tests passed**, zero failures, CTest time 17.49 seconds. The
  `ActivityConsent` frame test and activity OFF/ON event cases all passed.
  Existing `c7yt-dev:latest` image, shared ccache, Debug test build,
  plugins/Crashpad/spellcheck/updater OFF in generated
  `build-test/CMakeCache.txt`. Conditional alpha-plugin tests are excluded by
  this build option, so counts differ from plugin-enabled builds. Final log:
  `/tmp/codex-T03-check-final.log`.
  Inherited build warnings include a Kick structured-binding copy,
  StreamerMode apostrophe and test PCH `_REENTRANT` mismatch; none was an error.
- clang-format 19.1.7 on the four changed C++ files, then
  `clang-format --dry-run --Werror`: passed.
- `npx prettier@3.8.1 --write docs/fork/FEATURES.md docs/fork/NETWORK.md`:
  completed; final check will include this report.
- `python3 scripts/fork/check-network-hosts.py`: passed. No new production
  host; test URL uses the existing local echo server.
- `git diff --check`: passed.
- `git merge-tree --write-tree HEAD upstream/chatterino7`: clean; repeated
  against the committed task tree before handoff.

## Decisions and remaining risks

- Kick is still built/exposed. It mixes public API endpoints with private
  `kick.com/api/v2` and `web.kick.com/api/v1` endpoints. Its connection enum
  has no disabled value (`Default` maps to `Any`), so a settings default cannot
  safely hide/gate it. Recommend a separate provider-off patch; no code removal.
- Paused upstream build workflow explicitly turns plugins ON for several
  Linux matrix entries, Crashpad ON in Windows builds/tests, and spellcheck ON
  in Windows/macOS builds/tests. Release/CI policy must review these before
  re-enabling heavy workflows. This task does not promise those builds are off.
- Existing user preferences and CMake caches are preserved. There is no
  migration or first-run consent UI (T12). Consent checks suppress new BTTV
  sends/replay; messages already handed to the WebSocket transport while
  consent was on cannot be retracted. Enabling presence for a channel already
  joined reports it on the next ordinary activity update, not immediately.
- BTTV still learns subscribed public channel IDs/IPs for live emotes; the
  change suppresses account-linked presence, not the public emote connection.
- Native messaging remains registered/listening; false
  `attachExtensionToAnyProcess` only restricts Windows process attachment.
  Streamlink is an explicit external-tool action; its own provider API usage
  has not been audited. Image-uploader preset import enables uploads.
- Local Crashpad dumps can contain arguments/state, but the current
  `StartHandler` call has an empty upload URL. Local crash recovery is retained.
- No Twitch GQL/GraphQL endpoint, web client ID `kimne78...`, or Kraken endpoint
  was found in `src/` by text search. This is a source audit, not a complete
  runtime egress audit. `TwitchCommon.hpp` has an inherited hardcoded default
  OAuth client-ID helper with no callers in `src/`; it is not evidence of a
  borrowed web-client request. `LoginDialog.cpp` still opens the upstream
  `chatterino.com/client_login` site and stores pasted/supplied credentials.
  Fork OAuth ownership/login should be reviewed before release.
- Legacy Twitch PubSub topic code (channel-point rewards/pinned chat) remains;
  this task does not certify current API support or migrate those paths.
