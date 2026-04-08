#!/usr/bin/env python3
"""Conservative sweep: remove dead app/linux/Stubs/winapi_stubs.h includes.

winapi_stubs.h provides Windows API typedefs and constants for Linux
(LARGE_INTEGER, FILETIME, ERROR_*, etc.). Many minecraft/ files include
it as transitive leftovers and never actually reference any of those
symbols.

Usage:
    python3 scripts/sweep_winapi_stubs_includes.py [--apply]
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
    r'^[ \t]*#[ \t]*include[ \t]*"app/linux/Stubs/winapi_stubs\.h"[ \t]*\n',
    re.MULTILINE,
)

# Symbols defined by winapi_stubs.h. If any appear in the consumer, the
# include is needed.
USAGE_PATTERNS = [
    re.compile(r'\bLARGE_INTEGER\b'),
    re.compile(r'\bULARGE_INTEGER\b'),
    re.compile(r'\bFILETIME\b'),
    re.compile(r'\bSYSTEMTIME\b'),
    re.compile(r'\bERROR_SUCCESS\b'),
    re.compile(r'\bERROR_IO_PENDING\b'),
    re.compile(r'\bERROR_CANCELLED\b'),
    re.compile(r'\bMEM_COMMIT\b'),
    re.compile(r'\bMEM_RESERVE\b'),
    re.compile(r'\bMEM_DECOMMIT\b'),
    re.compile(r'\bPAGE_READWRITE\b'),
    re.compile(r'\bDWORD\b'),
    re.compile(r'\bBYTE\b'),
    re.compile(r'\bWORD\b'),
    re.compile(r'\bHANDLE\b'),
    re.compile(r'\bHRESULT\b'),
    re.compile(r'\bGetLastError\b'),
    re.compile(r'\bGetFileSize\b'),
    re.compile(r'\bGetSystemTime\b'),
    re.compile(r'\bSystemTimeToFileTime\b'),
    re.compile(r'\bMakeAbsoluteSD\b'),
    re.compile(r'\bSetFilePointer\b'),
    re.compile(r'\bReadFile\b'),
    re.compile(r'\bWriteFile\b'),
    re.compile(r'\bCloseHandle\b'),
    re.compile(r'\bCreateFile\b'),
    re.compile(r'\bVirtualAlloc\b'),
    re.compile(r'\bVirtualFree\b'),
    re.compile(r'\bSleep\s*\('),
    re.compile(r'\bWaitForSingleObject\b'),
    re.compile(r'\bSetEvent\b'),
    re.compile(r'\bResetEvent\b'),
    re.compile(r'\bInterlocked'),
    re.compile(r'\bOutputDebugString\b'),
    re.compile(r'\bMessageBox\b'),
]


def needs_winapi(text: str) -> bool:
    scan = INCLUDE_RE.sub("", text)
    return any(p.search(scan) for p in USAGE_PATTERNS)


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
            if needs_winapi(text):
                continue
            candidates.append(path)

    candidates.sort()
    for path in candidates:
        print(path.relative_to(REPO_ROOT))

    print(f"\n{len(candidates)} files have a dead winapi_stubs.h include")

    if args.apply:
        for path in candidates:
            text = path.read_text(encoding="utf-8", errors="surrogateescape")
            new_text = INCLUDE_RE.sub("", text)
            path.write_text(new_text, encoding="utf-8", errors="surrogateescape")
        print(f"Removed from {len(candidates)} files")

    return 0


if __name__ == "__main__":
    sys.exit(main())
