# T06: ASan/UBSan build and manual workflow

Worktree: `/home/pje/.t3/projects/c7yt-night/T06`.
Branch: `night/T06-sanitizers`. Tested base: `444e0c33a2ea96113720da0fcda33fa0d84e4cc4`.
No main-worktree/shared-board edits, pushes,
PRs, workflow enablement or automatic PR/push sanitizer triggers.

## Changes and instrumentation

- The existing `SANITIZE_ADDRESS` and `SANITIZE_UNDEFINED` option names were
  correct, but `find_package(Sanitizers)` ran before `project()` enabled compiler
  languages, so the module could not select compiler flags. Move it after
  `project()` and instrument `chatterino-lib` and `twitch-eventsub-ws` centrally;
  sanitizing only executable shells left implementation source uncovered.
- `asan` now exports compile commands and checks both flags for every command
  in the two implementation-library and two test targets. A missing target,
  missing sanitizer or later disabling flag fails before the build. Five
  stdlib tests cover combined flags, missing either flag, missing a target,
  later disabling flags and identifying the output target when the command
  also names another target's PCH.
- Sanitizer tests run once, without `until-pass` retries, with
  `ASAN_OPTIONS=halt_on_error=1:detect_leaks=1` and
  `UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`.
- The first truly instrumented Debug build failed in Qt's constexpr
  `QStringView` debug pointer assertion for QMagicEnum variable-template
  storage. A narrow same-header probe reproduced failure with both sanitizers
  and succeeded with Qt release assertions disabled. This is consistent with
  [GCC PR71962 discussion](https://gcc.gnu.org/pipermail/gcc-patches/2025-July/688787.html).
  ASan mode uses `RelWithDebInfo`, overriding C++ flags to `-O1 -g -DNDEBUG`.
  **Qt/release assertions are disabled, both sanitizers remain enabled.** No
  sanitizer exclusions or production-source workaround was added; normal
  `check` stays Debug. Shell argument quoting preserves the spaced CMake value.
- The new workflow is `workflow_dispatch` only during the heavy-CI pause. It
  builds the existing development Dockerfile (official Ubuntu 26.04 base
  pinned to `sha256:d016195d9be109539758aa4ce2a6c1813807e8d5d893586b46ef5da82444bf37`,
  hash-verified local test servers) and runs the same local script at four jobs.
  Actions use existing pinned SHAs, token permissions default empty, checkout
  credentials are not persisted, and test diagnostics are retained for 7 days.
  Automatic triggers remain a T13/user-authorization decision.
- The first full run exposed four repeated roots, addressed with small fixes:
  `Channel` never deleted its unparented completion model; pending
  `NetworkTask` objects/captured account callbacks survived worker shutdown;
  `NetworkResult.InvalidError` constructed enum values outside the permitted
  range; and the SplitInput mock supplied a null SpellChecker where production
  supplies an object. Delete the channel-owned model, tie pending tasks to
  thread shutdown and clear replies destroyed by their access manager, use
  unnamed in-range error values, and supply the missing fixture dependency.
  New regressions check model destruction and releasing a pending request's
  captured callback at shutdown. Existing input/fallback tests retain coverage.
- The next run exposed WindowManager's unparented save timer and orphaned
  ResizingTextEdit completers. Completion-test teardown crashed inside Qt's
  `QCompletionModel::filter`; focused reproduction/PIE mapping confirmed the
  function. Parent the timer to WindowManager and the current completer to
  its text widget so these objects/popups are destroyed with their owners.
  Add regressions observing both destruction paths with QPointer.

## Files changed

- Build/instrumentation: `CMakeLists.txt`, `scripts/dev/docker-build.sh`,
  `scripts/dev/check-sanitizers.py`, `scripts/dev/test_check_sanitizers.py`.
- Workflow/docs: `.github/workflows/fork-sanitizers.yml`, `docs/fork/CI.md`,
  this report.
- Ownership fixes: `src/common/Channel.cpp`,
  `src/common/network/NetworkPrivate.cpp`, `src/common/network/NetworkTask.cpp`,
  `src/singletons/WindowManager.cpp`, `src/widgets/helper/ResizingTextEdit.cpp`.
- Regressions/fixtures: `tests/src/ChannelChatters.cpp`,
  `tests/src/NetworkRequest.cpp`, `tests/src/NetworkResult.cpp`,
  `tests/src/NotebookTab.cpp`, `tests/src/SplitInput.cpp`.

## Verification

- `git submodule update --init --recursive`: completed in this worktree.
- Initial Debug ASan build: correctly detected GNU flags but failed at
  `Application.cpp`'s constexpr Qt assertion before the main suite.
  Log: `/tmp/night-T06-asan-initial.log`.
- First `C7YT_JOBS=4 scripts/dev/docker-build.sh asan` RelWithDebInfo run:
  **292/668 passed, 376 failed**, with fatal UBSan/LSan diagnostics grouped
  above. Most failures repeated the completion-model leak across IRC/EventSub
  fixtures. No retries or suppressions were used. CTest time: 185.29 seconds.
  Log: `/tmp/night-T06-asan-relwithdebinfo.log`.
- First incremental ownership/fixture ASan run: **643/670 passed, 27 failed**;
  new channel and pending-request regressions passed. Remaining diagnostics
  identified the timer leak and completion teardown above. CTest time:
  185.65 seconds. Log: `/tmp/night-T06-asan-fixed.log`.
- Final `C7YT_JOBS=4 scripts/dev/docker-build.sh asan` after all ownership
  fixes: **672/672 passed, 0 failures**, exit 0, CTest time **119.88 seconds**.
  Fatal ASan/UBSan and leak detection stayed enabled, with no retries. No
  AddressSanitizer errors, UBSan runtime errors or LeakSanitizer diagnostics
  occur in the final log: `/tmp/night-T06-asan-final.log`. Four new lifetime
  regressions account for the increase from 668 to 672 tests. Shared ccache
  used; no OOM events (observed compiler peak about 3 GB).
- Ordinary `C7YT_JOBS=4 scripts/dev/docker-build.sh check`: **672/672 passed,
  0 failures**, exit 0, CTest time **17.35 seconds**. The ordinary command
  retains its existing until-pass retry policy, but no failed or exceptional
  attempts occurred in this run. Log: `/tmp/night-T06-debug-check.log`.
- Actual selected flags:
  `ASan_GNU_FLAGS=-g -fsanitize=address -fno-omit-frame-pointer` and
  `UBSan_GNU_FLAGS=-g -fsanitize=undefined`; both CMake checks succeeded.
  `Application.cpp` compile command has both sanitizer flags, `-O1 -g`,
  `-DNDEBUG` and `QT_NO_DEBUG`.
- `python3 scripts/dev/check-sanitizers.py build-asan/compile_commands.json`:
  verified 420 chatterino-lib, 26 EventSub library, 68 main-test and 3
  EventSub-test commands. The checker uses object output paths, not PCH paths.
- Runtime proof: both test executables have ELF `NEEDED` records for
  `libasan.so.8` and `libubsan.so.1`; both `Application.cpp.o` and
  `chrono.cpp.o` reference `__asan_init` and multiple `__ubsan_handle_*`
  functions. Running the EventSub executable
  with ASan verbosity plus the fatal/leak settings logged
  `AddressSanitizer Init done` and **32/32 tests passed** with no diagnostics.
  Logs: `/tmp/night-T06-runtime-libraries.log`,
  `/tmp/night-T06-main-runtime-libraries.log` and
  `build-asan/eventsub-runtime-proof.log`.
- `python3 -m unittest discover -s scripts/dev -p test_check_sanitizers.py -v`:
  **5/5 passed**. `bash -n scripts/dev/docker-build.sh`: passed.
- `zizmor .github/workflows/fork-sanitizers.yml`: no findings.
  Whole-workflow scan exits 14 with inherited findings: 69 total, 50 suppressed,
  19 reported (7 informational, 4 low, 8 high); no new sanitizer-workflow
  finding. Full log: `/tmp/night-T06-zizmor-all.log`; upstream findings remain
  the T01 audit's scope.
- Prettier on changed Markdown/YAML and `git diff --check`: passed.
  `scripts/fork/check-network-hosts.py`: passed; no new production host.

## Limits and findings

The completed sanitizer run covers this assigned base and build configuration;
main advanced independently during the run and was not merged into this tested
checkout. Prebuilt Qt/system libraries and separately built external
dependencies are not instrumented. This build uses the fork's plugin-off
default, so conditional Lua-plugin tests are excluded. Local results do not
claim the new GitHub workflow has run. Compiler warnings in Boost/header code and test PCH rejection
(`_REENTRANT` mismatch; test sources compiled without that PCH) are separate
from runtime sanitizer findings; no broad suppressions were added.
