#!/usr/bin/env python3
"""Heuristic dead-include detector for app/ includes in minecraft/.

For each minecraft/ source file that includes a header from app/, check
whether the file references any of the top-level identifiers that header
defines. If zero references, the include is a candidate for removal.

Usage:
    python3 scripts/find_dead_app_includes.py [--apply] [DIR ...]

Without --apply, prints candidates only. With --apply, removes them.
DIR is one or more subdirectories of targets/minecraft/ to scope the
sweep (e.g. world/entity, server). Defaults to all of targets/minecraft/.

Caveats:
- The "identifiers a header defines" heuristic catches type names,
  function names, struct/class/enum names, and macros. It can miss
  constants used through unusual paths and is fooled by includes that
  are needed only for transitive type completion. Always build clean
  after applying.
- Comments and strings are not stripped from the consumer scan, so a
  file that mentions an app symbol only in a comment will look "live"
  and the include is conservatively kept.
"""

from __future__ import annotations

import argparse
import os
import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
MINECRAFT_ROOT = REPO_ROOT / "targets" / "minecraft"
APP_ROOT = REPO_ROOT / "targets" / "app"

INCLUDE_RE = re.compile(r'^\s*#\s*include\s*"(app/[^"]+)"\s*$', re.MULTILINE)

# Identifier-extracting regexes for header analysis. Best-effort.
IDENT_RES = [
    # class/struct/union/enum tag definitions
    re.compile(r'\b(?:class|struct|union|enum(?:\s+class)?)\s+([A-Za-z_]\w*)'),
    # typedef NAME or typedef ... NAME;
    re.compile(r'\btypedef\b[^;]*?\b([A-Za-z_]\w*)\s*(?:\[|;)'),
    # using NAME = ...
    re.compile(r'\busing\s+([A-Za-z_]\w*)\s*='),
    # function declarations: WORD WORD ( where second WORD is identifier
    # this is too loose; skip in favour of usage by name
    # #define MACRO
    re.compile(r'^\s*#\s*define\s+([A-Za-z_]\w*)', re.MULTILINE),
    # extern variable declarations
    re.compile(r'\bextern\b[^;]*?\b([A-Za-z_]\w*)\s*[;\[(]'),
]

CXX_KEYWORDS = {
    "if", "else", "while", "for", "do", "switch", "case", "default",
    "break", "continue", "return", "void", "int", "char", "short", "long",
    "float", "double", "bool", "true", "false", "nullptr", "class", "struct",
    "union", "enum", "namespace", "using", "typedef", "template", "typename",
    "const", "constexpr", "static", "extern", "inline", "virtual", "override",
    "final", "public", "private", "protected", "friend", "this", "new",
    "delete", "sizeof", "auto", "decltype", "operator", "throw", "try",
    "catch", "noexcept", "mutable", "volatile", "register", "explicit",
    "signed", "unsigned", "wchar_t", "char8_t", "char16_t", "char32_t",
    "size_t", "ptrdiff_t", "nullptr_t", "ifndef", "ifdef", "endif", "define",
    "include", "pragma", "elif", "error", "warning", "line", "undef",
    "alignas", "alignof", "concept", "requires", "co_await", "co_yield",
    "co_return", "consteval", "constinit", "static_cast", "dynamic_cast",
    "reinterpret_cast", "const_cast",
}


def extract_header_identifiers(header_path: Path) -> set[str]:
    """Best-effort extraction of identifiers a header defines."""
    if not header_path.exists():
        return set()
    try:
        text = header_path.read_text(encoding="utf-8", errors="surrogateescape")
    except OSError:
        return set()
    idents: set[str] = set()
    for regex in IDENT_RES:
        for match in regex.finditer(text):
            name = match.group(1)
            if name and name not in CXX_KEYWORDS and not name.startswith("_"):
                idents.add(name)
    return idents


def file_references_any(file_text: str, idents: set[str]) -> bool:
    """Check if any identifier appears as a whole-word match in the file."""
    if not idents:
        return False
    # Build one big alternation
    pattern = r'\b(?:' + '|'.join(re.escape(i) for i in idents) + r')\b'
    return re.search(pattern, file_text) is not None


def collect_minecraft_files(roots: list[Path]) -> list[Path]:
    files: list[Path] = []
    for root in roots:
        for dirpath, _dirnames, filenames in os.walk(root):
            for name in filenames:
                if name.endswith((".cpp", ".c", ".h", ".hpp")):
                    files.append(Path(dirpath) / name)
    files.sort()
    return files


def analyse(roots: list[Path], apply: bool) -> int:
    files = collect_minecraft_files(roots)
    header_cache: dict[str, set[str]] = {}
    candidate_count = 0

    for path in files:
        try:
            text = path.read_text(encoding="utf-8", errors="surrogateescape")
        except OSError:
            continue
        includes = INCLUDE_RE.findall(text)
        if not includes:
            continue
        # Strip the include lines from the text we scan for symbols, so we
        # don't false-positive on the include path itself mentioning the
        # symbol name (e.g. ColourTable.h).
        scan_text = INCLUDE_RE.sub("", text)
        dead_includes: list[str] = []
        for include_path in includes:
            cache_key = include_path
            if cache_key not in header_cache:
                header_path = REPO_ROOT / "targets" / include_path
                header_cache[cache_key] = extract_header_identifiers(header_path)
            idents = header_cache[cache_key]
            if not idents:
                # Header has no extractable identifiers (or doesn't exist).
                # Conservatively skip - don't claim it's dead.
                continue
            if not file_references_any(scan_text, idents):
                dead_includes.append(include_path)
        if dead_includes:
            candidate_count += len(dead_includes)
            rel = path.relative_to(REPO_ROOT)
            for inc in dead_includes:
                print(f"{rel}: {inc}")
            if apply:
                new_text = text
                for inc in dead_includes:
                    pattern = re.compile(
                        r'^\s*#\s*include\s*"' + re.escape(inc) + r'"\s*\n',
                        re.MULTILINE,
                    )
                    new_text = pattern.sub("", new_text)
                path.write_text(new_text, encoding="utf-8", errors="surrogateescape")

    print(f"\n{candidate_count} candidate dead include lines"
          f" {'removed' if apply else 'identified'}")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--apply", action="store_true",
                        help="Actually remove the candidate includes")
    parser.add_argument("dirs", nargs="*",
                        help="Subdirectories of targets/minecraft/ to scan")
    args = parser.parse_args()

    if args.dirs:
        roots = [MINECRAFT_ROOT / d for d in args.dirs]
        for r in roots:
            if not r.exists():
                print(f"error: {r} does not exist", file=sys.stderr)
                return 1
    else:
        roots = [MINECRAFT_ROOT]

    return analyse(roots, args.apply)


if __name__ == "__main__":
    sys.exit(main())
