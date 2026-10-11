# Codex T04: official YouTube design

Completed 2026-10-12 on `codex/20261012-youtube-design`, in the isolated
`/home/pje/.t3/projects/c7yt-codex/T04` worktree. Ready for parent review.

## Change

Added `docs/fork/youtube.md`: provider responsibilities and actual upstream hook
points, Desktop OAuth/PKCE, discovery, gRPC streaming with capped REST fallback,
rendering limits, moderation, current quotas, two Mermaid sequences and six
mergeable T09 slices with acceptance criteria/test strategy. No implementation,
workflow, task-board or shared-report changes. This report is local to this
branch; Claude's worktrees and branches remain untouched.

Read `.agents/TASKS.md`, `README.md`, `.agents/README.md`, `.agents/REVIEW.md` and
`docs/fork/ROADMAP.md`. Inspected Kick channel/server/API/account/manager/message
builder structure and account/channel/UI/network/test integration points.

## Verification

- Browsed current official Google method, schema, quota, native OAuth,
  authorization and Atom-feed documentation; source links are adjacent to
  claims in the design. Important method/quota pages were updated 2026-10-08.
- Fetched Google's official REST discovery JSON with Python `urllib.request`:
  revision `20261006`; checked method scopes and absent streaming method.
- Checked all 22 distinct official source links with Python `urllib.request`:
  22/22 returned HTTP 200.
- `npx prettier@3.8.1 --write docs/fork/youtube.md .agents/reports/codex-T04.md`
  and matching `--check`: pass.
- `git diff --cached --check`: pass.
- `python3 scripts/fork/check-network-hosts.py`: pass. Design adds no source
  URLs; the implementation must add the planned gRPC host to `NETWORK.md`.
- `git merge-tree --write-tree HEAD upstream/chatterino7`: no conflicts.
- C++ build/tests not run: only documentation changed. Mermaid syntax reviewed
  manually; diagrams have not been rendered in a browser.

## Corrections and unresolved implementation checks

- Current [quota calculator](https://developers.google.com/youtube/v3/determine_quota_cost)
  charges message insert/delete **20**, ban insert/delete **200**, moderator
  insert/delete **200**, moderator list **50**, list/streamList **5** units per
  call. The example is 3,920 units; three hours of five-second polling would
  exceed the default daily budget. Streaming reconnects also cost quota.
- Google's [streaming guide](https://developers.google.com/youtube/v3/live/streaming-live-chat)
  documents gRPC/protobuf with a Python demo and links to C++ client generation;
  the proposed C++ adapter is our design choice. The method page now
  mentions web errors without specifying HTTP framing. The design chooses the
  documented gRPC path and requires cross-platform packaging tests, rather
  than inventing SSE/NDJSON support for `NetworkRequest`.
- The [native OAuth guide](https://developers.google.com/identity/protocols/oauth2/native-app)
  supports Desktop loopback + PKCE. It explicitly disallows incremental
  authorization for installed apps; write-mode selection requires a fresh
  flow. Google's discovery confirms read-only for REST list, but omits
  streamList entirely: read-only streaming must be smoke-tested, with REST
  retained rather than silently escalating scopes.
- Upstream has **no reusable keychain backend** in this checkout. Kick saves
  tokens/client secret through `QStringSetting`; Twitch reads settings tokens.
  The task's keychain assumption cannot be honestly fulfilled by reuse. A
  narrow system-secret-store adapter/dependency review is a T09b prerequisite;
  unavailable/locked storage offers session-only login, never plaintext.
- Official [Atom documentation](https://developers.google.com/youtube/v3/guides/push_notifications)
  uses `/xml/feeds/videos.xml`, whereas the task specifies `/feeds/videos.xml`.
  The short path is identified as a compatibility candidate, not verified live.
  Neither path is promised to discover every livestream; direct video URLs
  and multiple-candidate selection are first-class escape routes.
- API unban needs the ban **resource ID**, not user/channel ID; no ban-list
  method exists. Known IDs can be retained from successful inserts; external
  bans may require YouTube's UI. Moderator management/listing is owner-only.
- No live API/OAuth calls were made with user credentials. Feed availability,
  gRPC read-scope behavior, token interoperability, role behavior and observed
  streaming quota accounting remain explicit T09 integration checks.
- No false Claude coauthor trailer, pushing, PR actions, shared coordination
  mutations or workflow enabling. Parent owns publication and review.
