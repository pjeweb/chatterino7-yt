#!/usr/bin/env python3
"""Fail if the ASan/UBSan build leaves application or test targets uninstrumented."""

import json
from pathlib import Path
import re
import shlex
import sys


TARGETS = (
    "chatterino-lib",
    "twitch-eventsub-ws",
    "chatterino-test",
    "twitch-eventsub-ws-test",
)


def check(database: Path) -> bool:
    counts = dict.fromkeys(TARGETS, 0)
    errors = []
    for entry in json.loads(database.read_text()):
        arguments = entry.get("arguments") or shlex.split(entry["command"])
        output = entry.get("output")
        if not output and "-o" in arguments:
            output = arguments[arguments.index("-o") + 1]
        target_match = re.search(r"CMakeFiles/([^/]+)\.dir/", output or "")
        if not target_match or target_match[1] not in counts:
            continue
        target = target_match[1]
        counts[target] += 1
        sanitizers = set()
        for argument in arguments:
            if argument.startswith("-fsanitize="):
                sanitizers.update(argument.split("=", 1)[1].split(","))
            elif argument.startswith("-fno-sanitize="):
                disabled = set(argument.split("=", 1)[1].split(","))
                if "all" in disabled:
                    sanitizers.clear()
                else:
                    sanitizers.difference_update(disabled)
        if not {"address", "undefined"}.issubset(sanitizers):
            errors.append(f"{target}: missing ASan/UBSan flags for {entry['file']}")

    errors.extend(
        f"{target}: no compile commands found"
        for target, count in counts.items()
        if not count
    )
    if errors:
        print("\n".join(errors), file=sys.stderr)
        return False
    for target, count in counts.items():
        print(f"ASan+UBSan compile flags verified: {target} ({count} commands)")
    return True


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit("usage: check-sanitizers.py <compile_commands.json>")
    sys.exit(0 if check(Path(sys.argv[1])) else 1)
