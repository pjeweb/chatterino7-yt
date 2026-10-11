#!/usr/bin/env python3
"""Fail if fork-only code introduces a hostname missing from docs/fork/NETWORK.md.

Only lines added under src/ relative to the merge base with upstream are
checked, so upstream's own hosts never trip this. Usage:

    scripts/fork/check-network-hosts.py [UPSTREAM_REF]

UPSTREAM_REF defaults to `upstream/chatterino7`. Committed changes are checked
(HEAD), not the working tree.

Rules:
- A host is any `http(s)://` or `ws(s)://` URL host in an added line.
- It passes if NETWORK.md mentions it in backticks (`host` or `host/path`), or
  it is in LINK_ONLY. Matching is exact: subdomains are not covered.
- Comment-only lines, localhost and loopback addresses are ignored.

This is a tripwire, not a proof: hosts assembled at runtime are not seen.
"""

import ipaddress
import re
import subprocess
import sys
from pathlib import Path
from typing import NamedTuple

REPO = Path(__file__).resolve().parents[2]
NETWORK_MD = REPO / "docs" / "fork" / "NETWORK.md"
DEFAULT_REF = "upstream/chatterino7"
UPSTREAM_URL = "https://github.com/SevenTV/chatterino7"

# Hosts that only ever appear as links shown to the user, never contacted.
# github.com is deliberately absent: an update appcast may be fetched from it.
LINK_ONLY = {
    "console.cloud.google.com",
    "developers.google.com",
    "support.google.com",
}

URL_RE = re.compile(r"\b(?:https?|wss?)://([a-z0-9.-]+)", re.IGNORECASE)
# A whole backticked span that looks like `host`, `host/path` or `scheme://host`.
DOC_HOST_RE = re.compile(
    r"(?:[a-z][a-z0-9+.-]*://)?([a-z0-9.-]+)(?::\d+)?(?:/\S*)?", re.IGNORECASE
)
COMMENT_RE = re.compile(r"\s*(?://|/\*|\*(?:\s|/|$))")
HUNK_RE = re.compile(r"@@ -\d+(?:,\d+)? \+(\d+)(?:,\d+)? @@")


class Hit(NamedTuple):
    host: str
    path: str
    line: int


def normalize_host(raw: str) -> str | None:
    """Lower-case host, or None for things that aren't remote hosts."""
    host = raw.strip(".").lower()
    if "." not in host:
        return None  # localhost, single-label names, empty
    try:
        return None if ipaddress.ip_address(host).is_loopback else host
    except ValueError:
        pass
    return host if re.fullmatch(r"[a-z]{2,}", host.rsplit(".", 1)[1]) else None


def hosts_in(text: str) -> list[str]:
    if COMMENT_RE.match(text.lstrip("﻿")):  # some files start with a BOM
        return []
    return [h for h in map(normalize_host, URL_RE.findall(text)) if h]


def documented_hosts(markdown: str) -> set[str]:
    hosts = set()
    for span in re.findall(r"`([^`\n]+)`", markdown):
        match = DOC_HOST_RE.fullmatch(span.strip())
        if match and (host := normalize_host(match.group(1))):
            hosts.add(host)
    return hosts


def _diff_path(header: str) -> str:
    path = header.strip().strip('"')
    return path[2:] if path.startswith("b/") else path


def added_hosts(diff: str) -> list[Hit]:
    """Hosts in added lines of a `git diff` (any context size)."""
    hits = []
    path, line_no, in_hunk = "", 0, False
    for line in diff.splitlines():
        if line.startswith("diff --git "):
            path, in_hunk = "", False
        elif line.startswith("@@"):
            match = HUNK_RE.match(line)
            line_no, in_hunk = (int(match.group(1)) if match else 0), True
        elif not in_hunk:
            if line.startswith("+++ "):
                path = _diff_path(line[4:])
        elif line.startswith("+"):
            hits += [Hit(host, path, line_no) for host in hosts_in(line[1:])]
            line_no += 1
        elif line.startswith(" "):
            line_no += 1
    return hits


def undocumented(hits: list[Hit], allowed: set[str]) -> list[Hit]:
    return [hit for hit in hits if hit.host not in allowed]


def format_report(missing: list[Hit]) -> str:
    by_host: dict[str, list[Hit]] = {}
    for hit in missing:
        by_host.setdefault(hit.host, []).append(hit)
    lines = ["Hosts added under src/ but not documented in docs/fork/NETWORK.md:", ""]
    for host in sorted(by_host):
        lines.append(f"  {host}")
        lines += [f"    {h.path}:{h.line}" for h in by_host[host]]
    lines += [
        "",
        "Add each host in backticks to docs/fork/NETWORK.md (purpose, data sent,",
        "default), or remove the URL. Links that are only shown to the user and",
        "never fetched belong in LINK_ONLY in scripts/fork/check-network-hosts.py.",
    ]
    return "\n".join(lines)


def git(*args: str) -> str:
    return subprocess.run(
        ["git", *args], cwd=REPO, check=True, capture_output=True, text=True
    ).stdout


def main(argv: list[str]) -> int:
    ref = argv[1] if len(argv) > 1 else DEFAULT_REF
    try:
        base = git("merge-base", "HEAD", ref).strip()
        diff = git(
            "-c", "core.quotePath=false",
            "diff", "--no-color", "--no-ext-diff", "--unified=0",
            "--find-renames", "--src-prefix=a/", "--dst-prefix=b/",
            base, "HEAD", "--", "src",
        )
    except subprocess.CalledProcessError as err:
        cmd = " ".join(err.cmd)
        print(f"error: {cmd} failed: {err.stderr.strip()}", file=sys.stderr)
        print(
            f"hint: git fetch {UPSTREAM_URL} chatterino7:refs/remotes/{DEFAULT_REF}"
            " (needs full history, not a shallow clone)",
            file=sys.stderr,
        )
        return 2

    hits = added_hosts(diff)
    allowed = documented_hosts(NETWORK_MD.read_text(encoding="utf-8")) | LINK_ONLY
    missing = undocumented(hits, allowed)
    print(f"Checked lines added under src/ since {base[:10]} (merge base with {ref}).")
    if missing:
        print(format_report(missing))
        return 1
    seen = ", ".join(sorted({hit.host for hit in hits})) or "none"
    print(f"OK: every host is documented or link-only. Hosts seen: {seen}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
