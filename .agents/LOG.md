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
