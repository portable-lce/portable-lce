#!/usr/bin/env python3
"""Enumerate C/C++ source files for a meson target.

Replaces the run_command('sh', '-c', 'find ...') hack in
targets/{minecraft,app}/meson.build. Run this whenever source files are
added or removed and commit the regenerated *_sources.txt files.

Usage:
    python3 scripts/list_sources.py

Each module's sources.txt is generated relative to its meson source dir.
"""

import os
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent

# Module configuration: (output_path, scan_root, [exclude_basenames]).
# Paths in the output file are relative to the directory containing the
# meson.build that reads it.
MODULES = [
    {
        "name": "minecraft",
        "output": REPO_ROOT / "targets" / "minecraft" / "sources.txt",
        "scan_root": REPO_ROOT / "targets" / "minecraft",
        "rel_to": REPO_ROOT / "targets" / "minecraft",
        "exclude": {
            "DurangoStats.cpp",  # Durango-specific
            # Incomplete / unused
            "SkyIslandDimension.cpp",
            "MemoryChunkStorage.cpp",
            "MemoryLevelStorage.cpp",
            "MemoryLevelStorageSource.cpp",
            "NbtSlotFile.cpp",
            "ZonedChunkStorage.cpp",
            "ZoneFile.cpp",
            "ZoneIo.cpp",
            "LevelConflictException.cpp",
            "SurvivalMode.cpp",
            "CreativeMode.cpp",
            "GameMode.cpp",
            "DemoMode.cpp",
        },
    },
    {
        "name": "app_common",
        "output": REPO_ROOT / "targets" / "app" / "common_sources.txt",
        "scan_root": REPO_ROOT / "targets" / "app" / "common",
        "rel_to": REPO_ROOT / "targets" / "app",
        "exclude": {
            "UIScene_InGameSaveManagementMenu.cpp",
        },
    },
    {
        "name": "app_linux",
        "output": REPO_ROOT / "targets" / "app" / "linux_sources.txt",
        "scan_root": REPO_ROOT / "targets" / "app" / "linux",
        "rel_to": REPO_ROOT / "targets" / "app",
        "exclude": set(),
    },
]

EXTENSIONS = (".cpp", ".c")


def collect(scan_root: Path, rel_to: Path, exclude: set[str]) -> list[str]:
    out: list[str] = []
    for dirpath, _dirnames, filenames in os.walk(scan_root):
        for name in filenames:
            if not name.endswith(EXTENSIONS):
                continue
            if name in exclude:
                continue
            full = Path(dirpath) / name
            out.append(str(full.relative_to(rel_to)))
    out.sort()
    return out


def main() -> int:
    for mod in MODULES:
        sources = collect(mod["scan_root"], mod["rel_to"], mod["exclude"])
        body = "\n".join(sources) + "\n"
        out_path: Path = mod["output"]
        previous = out_path.read_text() if out_path.exists() else ""
        if previous == body:
            print(f"{mod['name']}: {len(sources)} files (unchanged)")
        else:
            out_path.write_text(body)
            print(f"{mod['name']}: {len(sources)} files (regenerated {out_path.relative_to(REPO_ROOT)})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
