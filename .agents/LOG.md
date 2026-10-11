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
