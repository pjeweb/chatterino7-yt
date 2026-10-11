# T02 Codex continuation

Claude's stopped worker had already committed the comment-only-line fix on
`task/T02-network-gate`; this separate branch preserves those commits and
merges the latest foundation before continuing. Claude's branch is untouched.

Review found that a URL such as `https://api.twitch.tv@tracker.example.com/`
was incorrectly attributed to the allowed Twitch host. The checker now parses
URL authority with Python's `urlsplit`, and also handles remote IPv6 addresses
and bracketed IPv6 entries in the network inventory. Loopback remains exempt.

Verification:

- `python3 -m unittest discover -s scripts/fork -p 'test_*.py' -v`: 17 tests pass, including mixed block comments/code, userinfo destinations, IPv6 and IPv6 documentation.
- `python3 scripts/fork/check-network-hosts.py`: passes against `upstream/chatterino7`.
- `/tmp/zizmor-venv/bin/zizmor --offline .github/workflows/fork-network-hosts.yml`: no findings.
- `npx --yes prettier@3.8.1 --check docs/fork/NETWORK.md .github/workflows/fork-network-hosts.yml .agents/reports/codex-T02.md`: checked before publication.

This remains a diff-based tripwire, not a network sandbox: computed hosts,
URLs split across literals, scheme-less/IDN hosts, and malformed URLs are not
fully covered. Link-only exemptions rely on review to ensure they remain links.
No C++ changed; no application build needed. No heavy CI was enabled.

The continuation PR supersedes the implementation proposed in PR #1; choose
one when reconciling. Nothing was merged to the main branch this pass.
