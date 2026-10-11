#!/usr/bin/env python3
"""Fail if fork-only code introduces a hostname missing from docs/fork/NETWORK.md.

Only lines added relative to upstream are checked, so upstream's own hosts
(documentation links etc.) never trip this. Usage:

    scripts/fork/check-network-hosts.py [UPSTREAM_REF]

UPSTREAM_REF defaults to the merge base with `upstream/chatterino7`.
"""

import re
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
NETWORK_MD = REPO / "docs" / "fork" / "NETWORK.md"
URL_RE = re.compile(r"\b(?:https?|wss?)://([A-Za-z0-9.-]+\.[A-Za-z]{2,})")
# Hosts that only ever appear as links shown to the user, never contacted.
LINK_ONLY = {
    "github.com",
    "console.cloud.google.com",
    "developers.google.com",
    "support.google.com",
}


def git(*args: str) -> str:
    return subprocess.run(
        ["git", *args], cwd=REPO, check=True, capture_output=True, text=True
    ).stdout


def documented_hosts() -> set[str]:
    text = NETWORK_MD.read_text(encoding="utf-8")
    hosts = set()
    for code in re.findall(r"`([^`]+)`", text):
        host = code.split("/", 1)[0]
        if "." in host:
            hosts.add(host.lower())
    return hosts


def added_hosts(base: str) -> dict[str, list[str]]:
    diff = git("diff", "--unified=0", f"{base}...HEAD", "--", "src")
    found: dict[str, list[str]] = {}
    current = ""
    for line in diff.splitlines():
        if line.startswith("+++ b/"):
            current = line[6:]
        elif line.startswith("+") and not line.startswith("+++"):
            for host in URL_RE.findall(line):
                found.setdefault(host.lower(), []).append(current)
    return found


def main() -> int:
    if len(sys.argv) > 1:
        base = sys.argv[1]
    else:
        base = git("merge-base", "HEAD", "upstream/chatterino7").strip()

    documented = documented_hosts() | LINK_ONLY
    missing = {
        host: files
        for host, files in added_hosts(base).items()
        if host not in documented
    }
    if not missing:
        print("All hosts added by fork code are documented in NETWORK.md")
        return 0

    print("Hosts added in src/ but not documented in docs/fork/NETWORK.md:")
    for host, files in sorted(missing.items()):
        print(f"  {host}  ({', '.join(sorted(set(files)))})")
    return 1


if __name__ == "__main__":
    sys.exit(main())
