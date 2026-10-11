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
