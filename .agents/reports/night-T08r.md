# Night T08r: packaging identity review fixes

Worktree: `/home/pje/.t3/projects/c7yt-night/T08r`.
Branch: `night/T08-packaging-review`.
Base: `84ccc839d` (completed T08 coexistence implementation).
Scope: correct independent-review packaging consumers that still assumed the
old `chatterino` / `chatterino.app` / `Package: chatterino` identity. No push,
PR, merge, other checkouts, or C++ builds.

## Findings corrected

| Consumer                         | Broken assumption                         | Fix                                                                                          |
| -------------------------------- | ----------------------------------------- | -------------------------------------------------------------------------------------------- |
| `.CI/CreateAppImage.sh`          | `./bin/chatterino`, AppRun, `chatterino.png` | Binary/ldd/AppRun → `chatterino7-yt`; icon → `com.chatterino7yt.chatterino7yt.png`         |
| `.CI/CreateUbuntuDeb.sh`         | `./bin/chatterino`, `Package: chatterino` | Binary check + `Package: chatterino7-yt` (coexist with chatterino/chatterino7); fork Homepage |
| `.CI/MacDeploy.sh`               | `chatterino.app`                          | `chatterino7-yt.app` (matches CMake `OUTPUT_NAME` + bundle)                                  |
| `.CI/CreateDMG.sh`               | `chatterino.app`, volume `Chatterino2`    | Bundle + DMG volume name `chatterino7-yt`                                                    |
| `.CI/dmg-settings.py`            | none (uses `-D app=`)                     | Unchanged                                                                                    |
| `.github/workflows/build.yml`    | macOS `chatterino.app`, PDB `chatterino.pdb`, AppImage `Chatterino_7TV-*` | Bundle paths, `chatterino7-yt.pdb`, AppImage path from desktop `Name=`        |
| `.github/workflows/create-installer.yml` | Artifact glob missed `chatterino7-yt-*` | Pattern `{[Cc]hatterino-[^wW]*,chatterino7-yt-*}`                                     |

Preserved on purpose: upstream Docker/container image names
(`ghcr.io/chatterino/chatterino2-build-*`), Flatpak module name `chatterino`,
Windows staging directory `Chatterino2/` (installer/portable scripts), asset
pixels (`icon.png` / `chatterino.icns`), heavy-workflow pause state.

## Identity consumers verified (static)

Aligned with `src/common/ForkIdentity.hpp`:

- CMake `OUTPUT_NAME` / `MACOSX_BUNDLE_*` / desktop+icon install rename
- `resources/com.chatterino7yt.chatterino7yt.desktop` (`Exec` / `Icon`)
- Flatpak manifest `command` + app-id (already correct from T08)
- Inno Setup exe/AppData (already correct from T08)
- Packaging scripts and `build.yml` paths listed above

## Validation (no C++ build, no Win/mac execution)

- `sh -n` on `CreateAppImage.sh`, `CreateUbuntuDeb.sh`, `CreateDMG.sh`
- `bash -n` on `MacDeploy.sh` / `CreateDMG.sh` (MacDeploy uses bash arrays)
- `python3 -m py_compile .CI/dmg-settings.py`
- Fixture: missing `./bin/chatterino7-yt` → AppImage/deb scripts error with new name
- Fixture: present binary + stub `ldd`/`make` → AppImage passes binary/ldd gate then fails at make (expected)
- Fixture: deb control template emits `Package: chatterino7-yt`
- Static assertions: no leftover `./bin/chatterino` gates, `chatterino.app`
  packaging paths, `bin/chatterino.pdb`, or `Chatterino_7TV-x86_64.AppImage`
- Confirmed `build.yml` still declares `workflow_dispatch`; heavy workflows were
  not enabled

## Explicit non-claims

- Did **not** run AppImage/deb packaging end-to-end, macOS deploy/DMG, Windows
  PDB/zip, or any `BUILD_APP=ON` / docker C++ build.
- Did **not** push, open a PR, merge, or modify other worktrees/boards.

## Remaining

- Parent/supervisor publishes and reviews this branch (T08 + T08r).
- Optional: rename Windows staging folder off `Chatterino2/`; refresh
  `BUILDING_ON_MAC.md` / `BUILDING_ON_WINDOWS.md` example binary paths (docs
  only; not CI packaging consumers).
- Parent already building `BUILD_APP=ON` separately for path-suite coverage.
