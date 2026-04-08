#!/usr/bin/env python3
"""Conservative sweep: remove dead app/linux/LinuxGame.h includes from minecraft/.

LinuxGame.h is included by 155 files in targets/minecraft/ but most of
those are leftover from before the IGameServices refactor. The only real
reasons to include it are:
  - to use the global `LinuxGame app;` extern
  - to use the LinuxGame class type by name
  - to use C4JStringTable (forward-declared in the header)

For each minecraft/ file that includes LinuxGame.h, this script checks
whether the file references any of: LinuxGame, C4JStringTable, or
contains `app.`/`app::`. If none of those appear, the include is dead
and removed.

Usage:
    python3 scripts/sweep_linuxgame_includes.py [--apply]
"""

from __future__ import annotations

import argparse
import os
import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
MINECRAFT_ROOT = REPO_ROOT / "targets" / "minecraft"

INCLUDE_RE = re.compile(
    r'^[ \t]*#[ \t]*include[ \t]*"app/linux/LinuxGame\.h"[ \t]*\n',
    re.MULTILINE,
)

# Patterns that mean the include is genuinely needed.
# LinuxGame.h transitively pulls in Game.h (LinuxGame : public Game), so any
# reference to the Game class also keeps the include alive.
USAGE_PATTERNS = [
    re.compile(r'\bLinuxGame\b'),
    re.compile(r'\bC4JStringTable\b'),
    re.compile(r'\bapp\s*\.'),
    re.compile(r'\bapp\s*::'),
    re.compile(r'\bGame\s*::'),
    re.compile(r'\bGame\s*\*'),
    re.compile(r'\bGame\s*&'),
    re.compile(r'class\s+\w+\s*:\s*(?:public|private|protected)?\s*Game\b'),
]


def strip_comments(text: str) -> str:
    text = re.sub(r'/\*.*?\*/', '', text, flags=re.DOTALL)
    text = re.sub(r'//[^\n]*', '', text)
    return text


def file_needs_linuxgame(text: str) -> bool:
    # Strip the LinuxGame.h include line itself before checking, to avoid
    # the include path matching `LinuxGame`. Also strip comments so
    # references in commented-out code don't keep the include alive.
    scan = INCLUDE_RE.sub("", text)
    scan = strip_comments(scan)
    for pat in USAGE_PATTERNS:
        if pat.search(scan):
            return True
    return False


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--apply", action="store_true",
                        help="Actually remove the dead includes")
    args = parser.parse_args()

    candidates: list[Path] = []
    for dirpath, _dirnames, filenames in os.walk(MINECRAFT_ROOT):
        for name in filenames:
            if not name.endswith((".cpp", ".c", ".h", ".hpp")):
                continue
            path = Path(dirpath) / name
            try:
                text = path.read_text(encoding="utf-8", errors="surrogateescape")
            except OSError:
                continue
            if not INCLUDE_RE.search(text):
                continue
            if file_needs_linuxgame(text):
                continue
            candidates.append(path)

    candidates.sort()
    for path in candidates:
        print(path.relative_to(REPO_ROOT))

    print(f"\n{len(candidates)} files have a dead LinuxGame.h include")

    if args.apply:
        for path in candidates:
            text = path.read_text(encoding="utf-8", errors="surrogateescape")
            new_text = INCLUDE_RE.sub("", text)
            path.write_text(new_text, encoding="utf-8", errors="surrogateescape")
        print(f"Removed from {len(candidates)} files")

    return 0


if __name__ == "__main__":
    sys.exit(main())
