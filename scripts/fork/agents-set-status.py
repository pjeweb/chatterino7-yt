#!/usr/bin/env python3
"""Set a task's status (and optionally branch) in .agents/TASKS.md.

Usage: scripts/fork/agents-set-status.py T02 review [task/T02-network-gate]
"""

import re
import sys
from pathlib import Path

TASKS = Path(__file__).resolve().parents[2] / ".agents" / "TASKS.md"


def main(task: str, status: str, branch: str | None = None) -> None:
    lines = TASKS.read_text(encoding="utf-8").splitlines(keepends=True)
    for i, line in enumerate(lines):
        cells = line.split("|")
        if len(cells) > 6 and cells[1].strip() == task:
            cells[3] = f" {status} "
            if branch is not None:
                cells[6] = f" {branch} "
            lines[i] = "|".join(cells)
            TASKS.write_text("".join(lines), encoding="utf-8")
            return
    sys.exit(f"task {task} not found in {TASKS}")


if __name__ == "__main__":
    if len(sys.argv) not in (3, 4):
        sys.exit(__doc__)
    main(*sys.argv[1:])
