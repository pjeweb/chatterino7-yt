# Night T12: first-run network consent

Worktree: `/home/pje/.t3/projects/c7yt-night/T12`.
Branch: `night/T12-network-consent`.
Scope: small first-run dialog for optional third-party network services, persistent
completion marker, startup hook before connect, request gates, minimal settings
revisit entry. No push/PR. Dependencies T03/T08 already on base.

## Hook choice (after inspecting startup)

`Application::initialize` already shows a modal changelog and then writes
`/misc/currentVersion`. Optional requests (recent-messages, IVR, 7TV/BTTV
presence) only fire after channel join / user-card open, which need
`Application::connect()`. Consent therefore runs at the **start of
`Application::initialize`**, before `currentVersion` is overwritten and before
Twitch/Kick initialize + connect.

Frameless embed skips the dialog (same as changelog).

## Existing vs fresh profile

| Profile                     | Signal                                              | Behavior                                                                                |
| --------------------------- | --------------------------------------------------- | --------------------------------------------------------------------------------------- |
| Fresh first-run             | `currentVersion` empty and marker false             | Show dialog; all boxes unchecked                                                        |
| Existing (prior GUI launch) | `currentVersion` non-empty                          | Set `networkConsentCompleted` once; **do not** change saved service toggles; no dialog  |
| After accept                | marker true + chosen toggles                        | Optional requests allowed when each service toggle is on                                |
| After cancel / close        | marker true; toggles untouched (fresh defaults off) | No opt-in; dialog not shown again on startup                                            |
| Settings revisit            | button in General                                   | Dialog shows **current** saved values; Accept saves; Cancel leaves everything unchanged |

Marker setting: `/misc/networkConsentCompleted` (default `false`).

## Consent choices (only these)

1. Recent-messages history — `recent-messages.robotty.de` (channel name)
2. IVR sub age — `api.ivr.fi` (user + channel)
3. 7TV activity/presence — `7tv.io` presence
4. BTTV activity/presence — `sockets.betterttv.net` `broadcast_me`

Not offered: compiled-out updater, unimplemented YouTube/translation, pronouns,
link previews, required 7TV/BTTV/FFZ emote / live-update APIs.

## Request suppression

`networkconsent::allowsOptionalRequests` (marker completed) is checked together
with each service toggle in:

- `IvrApi::getSubage`
- `TwitchChannel::loadRecentMessages` / `loadRecentMessagesReconnect`
- Twitch/Kick 7TV presence send paths
- `BttvLiveUpdates::joinChannel` and `BttvLiveUpdateClient::broadcastMe`
- Kick history fill gated by the same recent-messages setting

## Files

New:

- `src/singletons/NetworkConsent.{hpp,cpp}`
- `src/widgets/dialogs/NetworkConsentDialog.{hpp,cpp}`
- `tests/src/NetworkConsent.cpp`

Touched:

- `src/Application.cpp` (hook)
- `src/singletons/Settings.hpp` (marker)
- optional request gate sites listed above
- `src/widgets/settingspages/GeneralPage.cpp` (revisit button)
- `src/CMakeLists.txt`, `tests/CMakeLists.txt`
- `tests/src/BttvLiveUpdates.cpp` (set marker so activity-consent fixtures still exercise `sendBTTVActivity`)
- `docs/fork/NETWORK.md`, `docs/fork/ROADMAP.md`

## Validation (executed)

| Command                                                                                     | Result                                                                                      |
| ------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------- |
| `git submodule update --init --recursive` (this worktree only)                              | OK                                                                                          |
| `clang-format-19 -i` / `--dry-run --Werror` on changed C++ (Ubuntu 24.04 container, 19.1.1) | OK                                                                                          |
| `npx prettier@3.8.1 --write docs/fork/NETWORK.md docs/fork/ROADMAP.md` (+ this report)      | OK                                                                                          |
| `python3 scripts/fork/check-network-hosts.py`                                               | OK (`api.ivr.fi` already documented)                                                        |
| `python3 -m unittest discover -s scripts/fork -v`                                           | **17/17** OK                                                                                |
| `C7YT_JOBS=4 scripts/dev/docker-build.sh check`                                             | **691/691** passed (includes 9 new NetworkConsent\* tests). Log: `/tmp/night-T12-check.log` |

`check` builds `chatterino-lib` with tests (`BUILD_APP=OFF`); the new dialog and
GeneralPage button compile as part of that library. **No separate `app` UI build**
was required or run.

Did not start parallel C++ builds; jobs capped at 4.

## Deferred / gaps

- No dedicated full “Network services” settings page (roadmap still open).
- Pronouns / link previews remain separate existing toggles, not in this dialog.
- Kick private endpoints still have no off switch (pre-existing).
- Frameless embed never completes consent via UI; optional toggles stay at defaults
  and the marker gate keeps optional requests off until completed another way.
- Interactive Accept/Cancel of the live modal is covered by logic + dialog default
  construction tests, not a full GUI automation run.

## Source refs

- Startup: `Application::initialize` → `networkconsent::ensureFirstRunDecision`
- Logic: `src/singletons/NetworkConsent.cpp`
- UI: `src/widgets/dialogs/NetworkConsentDialog.cpp`
- Docs: `docs/fork/NETWORK.md` (First-run network consent section)
