# Merge checklist

A branch is merged into `chatterino7` only if every box applies.

**Fits the goals**
- [ ] Serves the north star (Twitch + YouTube chatters/mods, important emotes,
      mod tools) or improves quality (tests, tooling, docs, CI, security)
- [ ] Doesn't add a feature outside [`ROADMAP.md`](../docs/fork/ROADMAP.md)
- [ ] Official APIs only: no Twitch GQL, borrowed client IDs, or scraping
- [ ] No new outbound host unless it's in [`NETWORK.md`](../docs/fork/NETWORK.md)
      and opt-in (or required for chat)
- [ ] No analytics, telemetry or heartbeats in any form

**Merges well**
- [ ] Upstream files are touched minimally; new code lives in new files
- [ ] No reformatting or drive-by refactors of upstream code
- [ ] `git merge-tree` against `upstream/chatterino7` stays clean

**Quality**
- [ ] Builds, and the test suite passes (`scripts/dev/docker-build.sh check`)
      when C++ changed
- [ ] New logic has tests (parsers, URL builders, quota math, state machines)
- [ ] Workflows: actions pinned to SHAs, `permissions: {}` at top level,
      `persist-credentials: false`, zizmor clean
- [ ] Secrets never logged; tokens stored in the keychain where upstream supports it
- [ ] Docs updated where behaviour changed: shorter and clearer beats longer

**Review**
- [ ] Supervisor read the full diff
- [ ] `codex exec review` findings addressed or consciously dismissed (noted in LOG.md)
