# Night T09ar: offline T09a independent-review fixes

Worktree: `/home/pje/.t3/projects/c7yt-night/T09ar`.
Branch: `night/T09a-review-fixes`.
Base: `0326ee3c1` (completed T09a model/fixtures validation).
Scope: correct two independent-review findings in the offline YouTube event
parser and suite coverage. No push, PR, merge, other checkouts, shared boards,
coordinator files, auth/network/UI activation, billing/API keys, or workers.

## Findings corrected

| Finding                                                              | Fix                                                                                                                                                                                                                       |
| -------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Ban fallback ignored target identity and timeout duration            | `banFallback` uses `targetDisplayName`, else `targetChannelId`, never the event author; temporary notices include known `banDurationSeconds`. Nonempty `displayMessage` still wins via the existing empty-text fill path. |
| Empty `textMessageDetails.messageText` wiped usable `displayMessage` | Detail `messageText` applied only when nonempty; missing/empty detail preserves `displayMessage`.                                                                                                                         |

Preserved on purpose: official-schema normalization, tri-state role hints, exact
decimal micros, immutable plain-text offline projection, synthetic fixture
provenance in `tests/resources/youtube/README.md`, and deferred production
hooks listed in `.agents/reports/night-T09a.md`.

## Coverage added

- Temporary-ban fixture (`events.json` event-8): text
  `Example target timed out for 300 seconds` plus frozen rendered-projection
  assertions (`messageText`, `TextElement` words, `searchText`; moderator id
  not substituted as the ban target).
- Permanent-ban fixture: channel-id fallback `target-fixture banned`.
- Missing-target safety: author display/channel present, no
  `bannedUserDetails` → `User timed out for 60 seconds` without author identity.
- Supplied nonempty ban `displayMessage` unchanged.
- Empty-detail text regression beside existing missing-detail
  `displayMessage` coverage.

## Provenance

Fixtures remain hand-authored sanitized resource shapes from the official
liveChatMessages / streaming-live-chat references checked 2026-10-12 (page
updated 2026-09-14). No live Google capture, credentials, personal data, or
API calls. See `tests/resources/youtube/README.md` and
`.agents/reports/night-T09a.md`.

## Validation

Assigned checkout submodules were empty; populated from pinned local submodule
Git archives (`git archive` of existing module objects) without modifying shared
Git configuration or other worktrees.

- `C7YT_JOBS=4 scripts/dev/docker-build.sh check`: exit 0, **677/677 tests
  passed**, including all six `YouTubeEvent.*` cases (0 failed). Public httpbin
  remains off; script-managed httpbox/pubsub services are local and
  container-scoped.
- `scripts/check-format.sh` with CI-pinned clang-format **19.1.7**
  (`/tmp/codex-T03-format/bin/clang-format`): exit 0.
- Changed Markdown only: Prettier **3.8.1** on this report.
- `git diff --check`: passed on staged sources.
- No production Google calls or credentials used. No extra suites after green.

## Explicit non-claims

- Did **not** push, open/update a PR, merge, message externally, or write
  main/shared-board/other-checkout/coordinator files.
- Did **not** activate auth, network readers, channel mutation, or UI.
- Parent publishes corrections into existing PR #11 and independently
  re-reviews exact final source.

## Remaining

- Parent/supervisor: publish this branch into PR #11 and re-review.
- Production OAuth/network/dispatch/UI hooks remain deferred (T09a scope).
