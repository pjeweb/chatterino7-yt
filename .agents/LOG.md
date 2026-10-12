# Log

Newest at the bottom. One entry per supervisor check-in or decision.

## 2026-10-12

- Forked SevenTV/chatterino7 → pjeweb/chatterino7-yt. Reviewed EEVA-inc/Moltorino7
  and Fixlation/Mergerino; neither is used as a base (remote-eval login helper,
  unverifiable binaries, analytics, a localhost bridge leaking its own token).
- T00 foundation: docker dev build (`scripts/dev`), privacy defaults (BTTV/7TV
  activity, recent-messages, IVR sub age off; updater compiled out), fork
  workflows (CodeQL, zizmor, Scorecard, upstream sync), roadmap and network
  inventory. Guarded upstream's nightly release job to SevenTV.
- User decisions: BYO Google Cloud OAuth for YouTube; on-device translation
  plus optional BYO-key providers; free signing options first; Sparkle/WinSparkle
  updater only; no analytics/heartbeats; Twitch via Helix only.
- Process: supervisor + up to 3 Claude workers in worktrees; `codex exec review`
  as an independent reviewer before merges.
- First codex review (privacy-defaults commit) caught a real bug: the sub-age gate
  also disabled the Helix follow-age lookup. Fixed by gating inside
  `IvrApi::getSubage` instead, which also leaves `UserInfoPopup.cpp` identical to
  upstream. Codex review is worth running on every merge.
- T00 verified locally: 713/713 tests pass in the dev container
  (`QT_QPA_PLATFORM=minimal`, since the static Qt has no offscreen plugin).
  Pushed `chatterino7`; GitHub CI (Build, Ubuntu/Arch tests, CodeQL, zizmor,
  Scorecard, formatting, lint) triggered.
- CI on `chatterino7`: Lint (Prettier) failed on the new docs; formatted with
  `prettier@3.8.1` and added the step to the worker rules. NETWORK.md now marks
  the implemented privacy defaults as done.
- T02 → PR #1. Codex found a P2 gap (lines starting with `/*` or `*` skipped
  even when they contain code); sent back to the worker. T03 started.
- User asked to pause the expensive builds. Disabled the heavy upstream
  workflows via `gh workflow disable` (no file changes, trivially reversible)
  and cancelled running builds. The local docker check is now the merge gate;
  T13 tracks re-enabling before the first release.

- Codex continuation requested after Claude thread `a1cc37af-b92d-4d88-bfe8-6cd142813a4b`
  hit its usage limit. Read [CODEX-HANDOFF.md](CODEX-HANDOFF.md) before resuming
  T01–T04. Codex uses separate `codex/20261012-*` branches/checkouts and draft
  PRs; Claude retains the original worktrees/branches and main-branch ownership.
  Codex does not merge or push those branches and leaves heavy CI paused.
  T02 continuation is PR #2 (includes unpushed worker fix plus URL-authority/IPv6
  checks, 17 passing tests). Reconcile it with PR #1 before merging either.
- Codex pass completed: T01 → PR #3 (CI permissions/audit), T02 → PR #2
  (network gate), T03 → PR #5 (BTTV activity consent + safe-defaults audit),
  T04 → PR #4 (YouTube design only). Independent Codex reviews found no
  actionable regressions; T03 local build and 668/668 tests pass, T02 has 17
  passing Python tests. Main and original Claude task HEADs remain unchanged.
  The three shared coordination-file edits are intentional local handoff
  changes, also proposed in PR #2; retain/reconcile them rather than discard
  on resume. All implementation proposals remain drafts for reconciliation,
  with heavy builds paused and original worktrees preserved. No autonomous
  Codex timer remains running. Read CODEX-HANDOFF.md for PRs and next steps.

- User authorized the overnight Codex/Cursor/Claude loop with natural quota
  renewal and no reset credits. Cursor on-demand usage is disabled (confirmed).
  Retired the stalled old Claude process gracefully and preserved its transcript.
  Merged reviewed PRs #2/#3/#4/#5; #1 is covered by #2 and marked merged by GitHub.
  Only the designated night supervisor integrates until 08:00 Europe/Berlin.
  T06 and T10 are active; Cursor T08, Codex T09a and post-renewal Claude T07 queued.
  Heavy CI remains paused; new sanitizer workflow will be manual-only.
- Detached user service launched Cursor T08 with a real saved session. The
  independent supervisor lane passed a read-only launch smoke check, including
  board/controller access and current-thread T3 PR registration. Foreground
  parent retained integration while native T06 finishes. T10 design merged via
  PR #6 after correcting the independent review’s DeepL Authorization wording.

## 2026-10-12 01:51 — T08 packaging review

Cursor completed T08 (671 Linux tests reported). Independent Codex review found stale Linux/macOS bundle and Windows PDB consumers after the executable rename. Draft PR #7 contains the implementation; isolated Cursor follow-up T08r corrects those consumers. T09a/T05 wait for T08r validation. Root builds BUILD_APP=ON separately; cross-platform execution remains unproven. User confirms Cursor on-demand usage is disabled. Heavy CI remains paused/manual-only.

## 2026-10-12 — T08 merged after review

Merged PR #7 at d7c6e13cd after corrected packaging paths, independent Codex review with no actionable findings, and all enabled CI green at978739a30. A clean Docker BUILD_APP=ON build passed671/671 tests with no failed attempts; the new chatterino7-yt executable ran --version under network-none and isolated HOME. Windows/macOS packaging remains source-inspected, not executed. T08/T08r worker evidence remains unchanged; downstream clean pending checkouts will advance to main before validation unlocks T09a (Codex offline parsers) and T05 (Cursor docs). Original Claude worktrees and paused heavy CI stay preserved. Native T06 is clearing real lifetime findings without suppressions.

## 2026-10-12 — T05 merged; T06 and IPC review

Merged docs PR8 after independent review, Prettier checks, exact upstream README body identity and all enabled CI green at ea95c1008. Main merge1e6ec840b. T06 published draftPR10 at178be0358:672/672 fatal/leak/no-retry sanitizer tests,672/672 Debug tests,5 guard tests,517 instrumented commands; independent review confirms actual three-dot PR diff/merge preserves T08/T10 with no actionable source findings. Parent draftPR9 isolates the residual native browser queue name and tests actual client/server lifecycle; final committed-source build is pending atd40cf8227, second review is clear. All PRs registered with T3; no heavy workflow dispatch/enablement.

## 2026-10-12 — sanitizer proof merged; offline parser review

Merged PR10 at6dd64ada2 after independent review and all enabled CI green; library/EventSub instrumentation and fatal/leak/no-retry proof are implemented, heavy sanitizer workflow is manual-only/undispatched. T09a offline model draftPR11 reports677/677 tests and six parser/projection suites; reviewer found two display fallbacks needing correction. Isolated CursorT09ar fixes target/duration notices and empty detail fallback; original evidence remains intact. Parent PR9 IPC lifecycle passed672/672 Debug tests with no failed attempts and fresh independent review; combined main+IPC sanitizer run is pending before merge. No production YouTube auth/network/UI activation, releases/signing or paused CI changes.

## 2026-10-12 — IPC and offline YouTube merged; supervisor handoff

PR9 merged2819cb56a after672 Debug tests and676 combined fatal/leak/no-retry ASan+UBSan tests,zero diagnostics,clear rereview and all enabled CI green. PR11 merged68cf2820b after677 tests,corrected ban/detail fallbacks,clear independent rereview and all enabled CI green. YouTube remains offline models/projection with no production auth/network/UI activation. Original T09a/T09ar evidence stays unchanged for controller validation. All foreground model/build reservations are released except one final full-main ASan checkpoint (/tmp/night-main-checkpoint-asan.log),running in reviews/T08ipc-main at68cf2820b. Background supervisor takes exclusive integration ownership on FOREGROUND removal; subscription-only quotas/no resets, original Claude preservation,paused heavy CI and08:00 deadline persist.

## 2026-10-12 02:49 — bounded supervisor dispatch

Clean integration handoff at `73d76167e`; GitHub confirms PRs 1–11 merged,
controller evidence T08/T08r/T05/T09a/T09ar is validated done. No ready reports
remain; no implementations repeated. Foreground marker absent; only this
controller supervisor lane active. One full-main sanitizer checkpoint at
`68cf2820b` is still running (682 tests planned), not yet claimed passed.
Queued isolated Cursor T12 consent implementation and Codex T09b secure-store/
Desktop OAuth prerequisite docs from current main, with immutable assignments
and atomic config append. Only T12 may build while the external checkpoint
occupies the other slot; jobs=4. Claude T07 retains natural 04:51 eligibility.
No releases, credentials, spending changes or paused heavy CI dispatch.
Use explicit `gh -R pjeweb/chatterino7-yt`: gh's inferred default repository
was upstream during one read-only query; corrected before any writes.

Full-main checkpoint completed before cycle exit: `/tmp/night-main-checkpoint-asan.log`
reports 682/682 tests passed, zero failed, 120.40 seconds at `68cf2820b`; no
ASan/LeakSanitizer/runtime-error diagnostics found. The associated Docker
container and build process exited. Source changes since that commit are only
supervisor metadata. T12 and T09b have started under controller ownership.

## 2026-10-12 02:55 — T09b reviewed draft; registration blocker

Published T09b docs-only `75d16917a` as draft PR #12. Full three-dot diff has
only its report, secure-storage/OAuth plan and a link; no gitlinks. Clean
merge-tree against main; independent Codex review in new isolated checkout
completed without actionable findings. Supervisor source/local-link checks,
Prettier/whitespace and primary Google/QtKeychain claims verified; no build,
store, platform or live-login proof claimed. Enabled CI still running.
Immediate T3 link failed HTTP 401 Unauthorized, so draft remains unmerged and
controller validation is withheld. No authentication/global configuration
changes or alternate credentials attempted. T12 is running without new local
commits; T07 remains queued for natural 04:51 renewal. No new dispatch needed.

## 2026-10-12 03:06 — T12 reviewed; isolated embed correction

Published committed T12 `75fcc1386` as draft PR #13; scope has no gitlinks or
unrelated pins. Clean merge-tree, format/whitespace, network gate and 17 Python
tests pass; worker log verifies 691 tests and exit 0. Supervisor clean build at
exact HEAD is in progress in new reviews/T12 checkout. Independent Codex review
found P2: frameless embed guard skips existing-profile migration, disabling
saved optional services via the new marker gates. Queued narrow Cursor T12r
in new checkout/branch from that HEAD with regression tests; original evidence
untouched, immutable assignment appended atomically. Review posted to PR #13.
T09b PR #12 exact reviewed HEAD unchanged, all enabled CI green. T3 registration
retry for #12 and immediate link for #13 both failed HTTP 401; both drafts
remain unmerged, no controller validation and no credential changes. Claude
T07 still waits for natural 04:51 renewal; no broader dispatch or heavy CI.

## 2026-10-12 03:14 — T12 independent build checkpoint

Clean exact-head T12 review build exited 0 with 691/691 tests, no failed
attempts, 20.06 seconds test time; UI library compiled. All enabled CI is
green at unchanged PR #13 HEAD `75fcc1386`. Independent-review embed migration
P2 still requires T12r correction and rereview. T12r running, no committed
progress yet; original checkout/evidence preserved. T3 list also failed 401,
so PRs #12/#13 remain draft and unvalidated. Supervisor build/reviewer exited;
only T12r worker build may remain. No further dispatch this bounded cycle.

## 2026-10-12 03:19 — T12 correction reviewed

T12r `fa3006df5` fixes existing-profile embed migration and adds two regression
tests. Ordinary fast-forward published it to existing draft PR #13; original
local T12 `75fcc1386` and T12r evidence unchanged. Full merge-base scope/no
gitlinks, clean merge-tree, docs/format/whitespace, host gate and 17 Python
tests pass. Worker build log verifies compiled changed sources, both new
tests and 693/693 passing with no failed attempts, exit 0. Fresh independent
Codex review in new isolated reviews/T12r context exits 0 with no actionable
regressions. Prior independent original-head 691-test proof remains separate;
no redundant supervisor rebuild. Corrected-head enabled CI still running.
T3 retries for #12/#13 remain 401; no merge, validation or auth changes.
No new dispatch while these drafts await registration; T07 natural 04:51
renewal remains queued. No platform/live-modal proof invented.

## 2026-10-12 03:36 — reviewed drafts await registration

PR #12 `75d16917a` and corrected PR #13 `fa3006df5` remain unchanged, independently
reviewed, and now have all enabled CI green. Both read-only merge-tree checks
against current main are clean; worker evidence remains unchanged. T3 link
attempts and list still return HTTP 401 Unauthorized. Preserve drafts and
withhold merge/controller validation; no authentication or account changes.
No new commits, workers, reviewers or builds to process. Existing queue stays
bounded; Claude T07 remains pending natural 04:51 renewal.

## 2026-10-12 05:09 — Claude renewal worker active

Controller started assigned Claude T07 after natural 04:51 eligibility. It is
running with no new local commits at this checkpoint; preserve its session
and checkout. Claude supervisor lane is waiting, not active; this controller
Codex lane is the sole running supervisor. PRs #12/#13 reviewed HEADs remain
unchanged with all enabled CI green; registration links/list still return401.
Drafts retained, no merge/validation or auth changes. No new dispatch/review/
build; T11 waits for reviewed T07 integration.
