# T01: CI hygiene continuation

Worktree: `/home/pje/.t3/projects/c7yt-codex/T01`.
Branch: `codex/20261012-ci-hygiene`.
Audited on 2026-10-12 from snapshot `2b3f97f34`.

## Changes

- Added explicit `contents: read` defaults to ten upstream check/build
  workflows; publishing jobs retain their existing per-job overrides. WinGet
  uses its explicit PAT and now has `permissions: {}`. These are the only
  behavioral YAML changes: no triggers, jobs or guards changed. The fork sync
  comment now accurately notes that GITHUB_TOKEN PR CI requires approval.
- Finished the previously untracked `docs/fork/CI.md` (74 lines), with an
  event diagram, actual outputs, paused-workflow state and upstream owner
  guards. It distinguishes configured triggers from enabled checks and
  SARIF reporting from merge protection. Automatic PR CI uses SYNC_TOKEN;
  the GITHUB_TOKEN fallback can queue PR CI awaiting approval.
- Existing snapshot commits `45399e957` and `2b3f97f34` already restricted
  installer releases and clang-tidy PR comments to SevenTV and hardened the
  fork workflows. The nightly release/tag, Codecov and WinGet also already
  have upstream owner guards. No additional publishing guard was needed.
- No suppression configuration, action upgrades or broad upstream refactors.

Files changed: `build-freebsd.yml`, `build.yml`, `check-formatting.yml`,
`clang-tidy.yml`, `eventsub-test.yml`, `lint.yml`, `test-arch-linux.yml`,
`test-macos.yml`, `test-windows.yml`, `test.yml`, `winget.yml` and the comment
in `fork-upstream-sync.yml` under
`.github/workflows`, plus `docs/fork/CI.md` and this report.

## Verification

Installed tool already present: `/tmp/zizmor-venv/bin/zizmor`, version 1.30.1.
A bare command defaults to offline mode without a token. Authenticated online
runs supplied the existing `gh` token through `GH_TOKEN`, never printed it,
and enabled the same online audits used by zizmor-action.

Commands and results:

```text
zizmor --offline --format json .github/workflows          # before: 34 findings, exit 14
GH_TOKEN=<existing gh auth> zizmor --format json .github/workflows
                                                        # before: 35 findings, exit 14
                                                        # after: 20 findings, exit 14
GH_TOKEN=<existing gh auth> zizmor --format json .github/workflows/fork-*.yml
                                                        # after: [], exit 0
```

Before/after output summary (online):

| Audit                 | Before |  After |
| --------------------- | -----: | -----: |
| excessive-permissions |     15 |      0 |
| template-injection    |      7 |      7 |
| unpinned-images       |      5 |      5 |
| superfluous-actions   |      3 |      3 |
| dangerous-triggers    |      2 |      2 |
| cache-poisoning       |      1 |      1 |
| adhoc-packages        |      1 |      1 |
| ref-version-mismatch  |      1 |      1 |
| **Total**             | **35** | **20** |
| **Fork workflows**    |  **0** |  **0** |

The online-only additional finding is WinGet's moving `# main` annotation.
Raw output remains in `/tmp/codex-T01-zizmor-{before,online-before,after,fork}.json`
for this session; the tables record all remaining findings below.

- PyYAML `BaseLoader` structural comparison against `HEAD` proved all eleven
  YAML diffs change only top-level permissions; event filters, steps,
  publishing guards, dependency skips and job permissions are unchanged.
- `npx --yes prettier@3.8.1 --check` on changed YAML and both Markdown files:
  passed. `git diff --check`: passed.
- `wc -l docs/fork/CI.md`: 74 (under the 80-line limit).
- Read-only `gh api repos/pjeweb/chatterino7-yt/actions/workflows`: confirmed
  the ten paused workflows listed below. Repository token defaults are
  already `read`; the YAML now makes that restriction portable.
- No C++ changed, so no build was necessary. No workflow was enabled,
  dispatched, pushed or otherwise changed on GitHub.

## Remaining upstream findings

All 20 remaining findings are upstream; none are in `fork-*.yml`.
Locations below are lines in this continuation's final YAML.

| Audit                    | Location(s)                                        | Reason left in place                                                                                                                                                                   |
| ------------------------ | -------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| template-injection (6)   | `build.yml`: 120, 319, 344, 362, 411, 540          | Build-type output is a fixed On/Off value; architecture/matrix values and serialized job results are workflow-defined. Moving every expression to env would enlarge the upstream diff. |
| template-injection (1)   | `create-installer.yml`: 83                         | Workflow-defined architecture/Qt version in the extraction command; upstream-only guarded job.                                                                                         |
| unpinned-images (4)      | `build.yml`: 47 / matrix                           | Floating Ubuntu build images (22.04, twice 24.04, 26.04). Digest maintenance requires a separate upstream-compatible dependency policy.                                                |
| unpinned-images (1)      | `test.yml`: 31                                     | Same floating Ubuntu 26.04 build image.                                                                                                                                                |
| cache-poisoning (1)      | `build.yml`: 4 / Conan cache step                  | Shared Conan cache feeds upstream publishing. Owner guard disables that publishing on this fork; altering cache save/restore needs separate validation.                                |
| dangerous-triggers (1)   | `create-installer.yml`: 3                          | Privileged `workflow_run` consumes build artifacts; SevenTV-only entry job, with dependent release jobs skipped on the fork.                                                           |
| dangerous-triggers (1)   | `post-clang-tidy-review.yml`: 4                    | Privileged `workflow_run` posts reviews; SevenTV-only job.                                                                                                                             |
| superfluous-actions (3)  | `build.yml`: 516; `create-installer.yml`: 120, 250 | Pinned release-action versus `gh release` is an informational replacement suggestion; all publishing is owner-guarded.                                                                 |
| adhoc-packages (1)       | `lint.yml`: 38                                     | Upstream `npm i ajv-cli` is unpinned. Correctly replacing it needs a dependency manifest/lockfile and schema-tool validation.                                                          |
| ref-version-mismatch (1) | `winget.yml`: 13                                   | Pinned old revision has moving `# main` annotation; no SHA upgrade or annotation-only suppression attempted. Chatterino-only publishing.                                               |

zizmor also warns that it cannot infer the shell for three installer steps
from the dynamic runner matrix and assumes bash. Those diagnostics are not
additional findings. The workflows already run those steps on Windows.

## GitHub state and caveats

Manually disabled at audit time: Build, Build FreeBSD, clang-tidy,
Create installer, Post clang-tidy review comments, Test Ubuntu,
Test Arch Linux, Test MacOS, Test Windows, Publish to WinGet.
Lightweight formatting/lint/EventSub/REUSE and the four fork workflows are
active. The Actions inventory also includes a Network host gate registered
from a sibling task; it is not present in this snapshot or changed here.
Administrative workflow state can change separately from YAML.

No remote execution was needed to validate these permission-only edits.
Heavy workflow execution remains deliberately paused. Existing upstream
findings stay visible in code scanning; fork cleanliness does not assert
that every upstream workflow or pinned dependency is risk-free.

Primary references consulted:

- [GitHub workflow permissions](https://docs.github.com/en/actions/reference/workflows-and-actions/workflow-syntax#permissions): setting specific permissions disables unspecified permissions; jobs can override defaults.
- [Pinned zizmor-action README](https://github.com/zizmorcore/zizmor-action/blob/cc914d7f3750a2d13d75c7f184a1060aa0e9d482/README.md): advanced-security mode does not fail on findings, and online audits default to enabled. Read through GitHub's contents API at the pinned revision.
- [GitHub workflow trigger restrictions](https://docs.github.com/en/actions/how-tos/writing-workflows/choosing-when-your-workflow-runs/triggering-a-workflow): GITHUB_TOKEN-created PR workflows require approval; a PAT or app token allows automatic PR CI.

No shared Claude worktree, branch, task board or report was edited. All work
is committed only on the isolated Codex branch, without Claude coauthor
trailers. No push, PR creation, PR comment or main-branch merge was performed.
