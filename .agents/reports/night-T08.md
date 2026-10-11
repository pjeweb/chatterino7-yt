# Night T08: coexistence identity

Worktree: `/home/pje/.t3/projects/c7yt-night/T08`.
Branch: `night/T08-coexistence`.
Scope: smallest upstream-merge-friendly fork identity so chatterino7-yt can
install/run beside Chatterino and chatterino7 without silently sharing
settings. No push, PR, or shared-board edits.

## Audit (before)

chatterino7 already diverges from chatterino2 in **display** and some OS IDs,
but still **shares the settings directory** with Chatterino:

| Surface                | chatterino2 / chatterino7 (before) | Notes                                       |
| ---------------------- | ---------------------------------- | ------------------------------------------- |
| Qt `applicationName`   | `chatterino`                       | Drives `QStandardPaths::AppDataLocation`    |
| Linux data dir         | `~/.local/share/chatterino`        | Shared                                      |
| Windows data dir       | `%APPDATA%\Chatterino2`            | Paths.cpp appends `2` for name `chatterino` |
| Desktop / AppStream ID | `com.chatterino.chatterino`        | Shared executable name `chatterino`         |
| macOS bundle ID        | `app.7tv.chatterino7` (c7 only)    | Distinct from c2; not from this fork yet    |
| Windows AUMID          | `SevenTV.Chatterino7TV` (c7)       | Distinct taskbar ID; still same AppData     |
| Native messaging host  | `com.chatterino.chatterino`        | Would overwrite the other install’s host    |

Existing fork switches (T03): CMake `CHATTERINO_PLUGINS` / updater defaults,
privacy settings comments — unrelated to packaging identity. No prior
coexistence path logic.

## After (implemented)

Single source of truth: `src/common/ForkIdentity.hpp`.

| Surface                  | chatterino7-yt value                                                                                                |
| ------------------------ | ------------------------------------------------------------------------------------------------------------------- |
| Qt application name      | `chatterino7-yt`                                                                                                    |
| Display / window title   | `chatterino7-yt`                                                                                                    |
| Executable `OUTPUT_NAME` | `chatterino7-yt`                                                                                                    |
| Default data directory   | Qt `AppDataLocation` for that name (e.g. Linux `~/.local/share/chatterino7-yt`; Windows `%APPDATA%\chatterino7-yt`) |
| Desktop / icon / NM ID   | `com.chatterino7yt.chatterino7yt`                                                                                   |
| macOS bundle ID          | `app.chatterino7yt.chatterino7yt`                                                                                   |
| Windows AUMID            | `Chatterino7YT.Chatterino7YT`                                                                                       |
| Inno Setup AppId         | new GUID (distinct from chatterino7)                                                                                |

**Overrides preserved:** `--portable` and `--portable-dir` still select the
root before AppData logic; cache path setting unchanged.

**Windows Paths.cpp:** the historical `Chatterino2` rewrite runs only when
`applicationName` is still `chatterino` (upstream behaviour). Distinct fork
names keep Qt’s path.

**Not done (intentional):** no logo/asset replacement (same `icon.png` /
`chatterino.icns` pixels, new install names only); no About-page / Kick
copyright string sweep; Flathub nightly `.flatpakref` still points at
upstream’s Flathub app (comment only).

## Exact files

- `src/common/ForkIdentity.hpp` (new)
- `src/main.cpp`, `src/singletons/Paths.cpp`, `src/common/Version.cpp`,
  `src/common/Args.cpp`
- `src/singletons/Toasts.cpp`, `src/singletons/NativeMessaging.cpp`
- `src/CMakeLists.txt`, `tests/CMakeLists.txt`, `tests/src/Paths.cpp` (new)
- `resources/com.chatterino7yt.chatterino7yt.{desktop,appdata.xml}` (renamed + edited)
- `.CI/com.chatterino7yt.chatterino7yt.yml` (renamed + edited),
  `.CI/chatterino-installer.iss`, `.CI/CreateAppImage.sh`,
  `.CI/chatterino-nightly.flatpakref` (comment)
- `.github/workflows/build.yml` (flatpak manifest path + Windows exe name)
- `docs/fork/ROADMAP.md`, `docs/make-{release,beta}.md`
- this report

## Validation

- `git submodule update --init --recursive` in this worktree.
- clang-format 19.1.0 (`--dry-run --Werror`) on changed C++ files.
- Prettier 3.8.1 on changed markdown (`ROADMAP`, make-release/beta, this report).
- `C7YT_JOBS=4 scripts/dev/docker-build.sh check`: **671/671 passed**, including
  `PathsIdentity.*` (default fork AppData leaf, `--portable-dir` override,
  portable without override → application dir). Log: `/tmp/night-T08-check.log`.
- `git merge-tree --write-tree HEAD upstream/chatterino7`: clean.
- `git diff --check`: clean.

## Platform limitations

- **Linux:** exercised via docker check (path tests + full suite). Desktop file
  / notifications not launched in a real session here.
- **macOS / Windows:** identity strings and installer/CMake bundle properties
  inspected and edited only; **not executed** on those OSes. Windows
  `Chatterino2` gate is compile-time `#ifdef Q_OS_WIN` and covered by review,
  not by the Linux test binary.
- Paused `build.yml` still stages into a `Chatterino2/` folder name and PDB
  artifact names may still follow the CMake target `chatterino`; binary copy
  uses `chatterino7-yt.exe`. Revisit with T13 when heavy CI is re-enabled.
- Browser extension still expects the upstream native-messaging host name until
  a fork-aware extension exists; the fork no longer overwrites upstream’s host.

## Remaining

- Supervisor: push branch, open draft PR, REVIEW.md + codex review.
- Optional later: dedicated icons; migrate helper if someone wants to copy
  settings from Chatterino2; finish Windows CI packaging folder/PDB names.
