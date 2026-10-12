# T05 Docs pass — night worker

Worktree: `/home/pje/.t3/projects/c7yt-night/T05`
Branch: `night/T05-docs`
Scope: docs only. No runtime code, dependencies, workflows, push/PR, shared
boards, billing/reset, agents, or persistent loops.

## Source verification (implemented vs proposed)

| Claim                        | Evidence                                                             | Doc status                 |
| ---------------------------- | -------------------------------------------------------------------- | -------------------------- |
| Twitch + 7TV/BTTV/FFZ emotes | Inherited upstream; presence gates in Settings.hpp                   | Documented as today        |
| BTTV/7TV activity off        | `sendBTTVActivity` / `sendSevenTVActivity` = false; send paths gated | NETWORK + FEATURES         |
| Recent-messages / IVR off    | `loadTwitchMessageHistoryOnConnect` / `showSubageInUserCard` = false | NETWORK + FEATURES         |
| Updater compiled out         | `CHATTERINO_UPDATER=OFF` → `CHATTERINO_DISABLE_UPDATER`              | Not a signed updater       |
| Coexistence identity         | `ForkIdentity.hpp`; T08 merged                                       | ROADMAP phase 0 checked    |
| Network host CI gate         | `scripts/fork/check-network-hosts.py` + `fork-network-hosts.yml`     | ROADMAP phase 1 checked    |
| Upstream-sync workflow       | `fork-upstream-sync.yml` (needs `SYNC_TOKEN` for workflow pushes)    | ROADMAP phase 0 checked    |
| YouTube provider             | No `src/providers/youtube`; only `docs/fork/youtube.md`              | Design only                |
| Translation                  | No provider code; `docs/fork/translation.md` (T10 merged)            | Design only                |
| Signed auto-updater          | Not present; Sparkle/WinSparkle still roadmap                        | Not claimed                |
| Signing / cross-platform CI  | Heavy build/test workflows paused per CI.md                          | Not claimed                |
| Kick removed/gated           | Kick still in tree; FEATURES says recommend-off, no switch           | Explicitly not implemented |

## Changes

- `README.md`: short fork intro + separator; upstream body byte-identical to
  `HEAD:README.md` (verified before/after Prettier on the intro only).
- `docs/fork/README.md`: one-line index + one architecture/network Mermaid
  diagram (solid = in-tree; dashed = opt-in activity or proposed providers).
- `docs/fork/ROADMAP.md`: fewer lines; checkboxes match code/workflow evidence;
  phases 3–5 labelled design/not implemented; CI pause called out via CI.md.
- `docs/fork/NETWORK.md`: inventory kept; duplication with ROADMAP/FEATURES
  trimmed; Kick ungated and planned hosts still marked _planned_.

Preserved: `LICENSE`, LICENSES/, REUSE, upstream README attribution below the
separator. FEATURES.md / CI.md / youtube.md / translation.md untouched.

## Format verification

- `npx --yes prettier@3.8.1 --write` then `--check` on changed Markdown:
  `README.md`, `docs/fork/README.md`, `docs/fork/ROADMAP.md`,
  `docs/fork/NETWORK.md`, `.agents/reports/night-T05.md`.
- `git diff --check` clean.
- Upstream README body identity assert passed after Prettier.
- `python3 -m unittest discover -s scripts/fork -p 'test_*.py'`: 17 OK.
- `python3 scripts/fork/check-network-hosts.py`: OK vs `upstream/chatterino7`.

## Limits / remaining

- No push, PR, merge, or shared-board/`TASKS.md` status flip (coordinator).
- Did not enable paused CI, claim release signing, or expand features.
- NETWORK inventory still incomplete for Kick/private endpoints.
- First-run consent / Network services settings page still open (ROADMAP).
- YouTube/translation/updater remain design or not started (T09/T10/T11).
