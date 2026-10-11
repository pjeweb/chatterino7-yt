# Overnight supervisor — 2026-10-12

Run until **08:00 Europe/Berlin**, then preserve checkpoints and stop dispatch.
The user authorized Codex, Cursor and Claude workers, reviewed merges, natural
quota-window resumption, and no purchased/reset credits. Cursor on-demand usage
is disabled (user confirmed). Codex uses saved ChatGPT auth; Claude uses
claude.ai subscription auth. No provider API-key fallback, upgrades or billing
changes. Claude is queued until its known 04:50 renewal; retry after 04:51.

## Ownership

The original usage-limited Claude supervisor process was terminated gracefully;
its transcript and original task worktrees remain intact. Resuming the old
thread does not grant concurrent supervisor ownership. Read this file and
yield integration to the controller until it stops or explicit handover.

Runtime: `~/.t3/projects/c7yt-night-runtime/`. Its controller owns a process
lock, persisted sessions/cooldowns and completion validation. The foreground
Codex supervisor holds its `FOREGROUND` marker while setting up/reviewing.
After handoff, the subscription-backed supervisor lane exclusively owns
`~/.t3/projects/chatterino7-yt` on `chatterino7`; the separate
`c7yt-night/supervisor` checkout is available for review builds. Workers never
integrate or modify either supervisor area.

At most three workers plus one supervisor, and at most two C++ builds. The
detached controller uses a stricter cap of three model processes including its
supervisor; foreground/native workers reserve slots explicitly.
Each worker owns one checkout and branch under `c7yt-night/`, with no edits to
main, other worktrees, shared board, global CLI settings or billing settings.
Quota-limited providers back off and resume their exact saved session/worktree;
other providers can continue. Unknown failures are bounded, not retried forever.

## State and next work

T01–T04 are merged through PRs #2–#5. GitHub also marks #1 merged because its
commits were included in #2. Local C++ proof is 668 passing tests from T03;
all 17 network-gate tests pass after reconciliation. The old CODEX-HANDOFF.md
is the previous pass’s historical record, superseded by this ownership notice.

| Task | Provider | Status / checkout                                                      |
| ---- | -------- | ---------------------------------------------------------------------- |
| T06  | Codex    | active, `c7yt-night/T06`; local sanitizer proof + manual-only workflow |
| T10  | Codex    | merged PR #6; translation design only                                  |
| T08  | Cursor   | active under controller; c7yt-night/T08 coexistence                    |
| T09a | Codex    | queued, `c7yt-night/T09a`; offline YouTube models/parsers              |
| T07  | Claude   | queued after renewal, `c7yt-night/T07`; provenance/release workflow    |

Follow-on tasks come from TASKS.md and youtube.md’s six slices. Select small,
independent work; favor verification and privacy fixes over new features.
Update task status and LOG.md at integration checkpoints and push progress.

## Review and stop controls

Every implementation remains a draft PR until the supervisor reads the diff,
obtains an independent review, verifies required local tests and sees enabled
CI green. Register every PR using T3 link_pull_request. Merge through GitHub;
then advance the board. Do not publish/sign/tag actual releases, create keys,
add credentials, use private APIs, or enable the paused build matrices. New
heavy workflows are manual-only while the user’s pause applies.

Local stop: create `STOP` in the runtime directory; preserve incomplete work.
The controller will stop new launches and end its own worker process groups
with a checkpoint grace period. It never kills unrelated sessions or removes
worktrees. Morning summary must distinguish implemented, tested, proposed and
blocked work, including provider limits or any missing PR registration.

## Installed controller

User service `c7yt-night.service` is running with linger enabled. Its runtime
controller passed 25 recovery/limits/dispatch/shutdown tests before launch.
Cursor’s actual session started successfully from its assigned stdin prompt.
Native foreground work reserves slots until handoff; afterward the controller
schedules the independent supervisor lane and assigned workers. Logs and
statuses are in the private runtime `events.jsonl`/`state.json`, not provider
raw output. T3 PR registration uses the current thread credential; failure
must be reported and leaves the proposal unmerged.
