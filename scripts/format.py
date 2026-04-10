#!/usr/bin/env python3
import argparse
import os
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path

DEFAULT_EXTENSIONS = {
    ".c",
    ".cc",
    ".cpp",
    ".cxx",
    ".h",
    ".hh",
    ".hpp",
    ".hxx",
    ".inl",
    ".ipp",
}
DEFAULT_JOBS = os.cpu_count() or 4
DEFAULT_TIMEOUT = 10

# ansi escapes
RESET = "\033[0m" if sys.stdout.isatty() else ""
GREEN = "\033[32m" if sys.stdout.isatty() else ""
YELLOW = "\033[33m" if sys.stdout.isatty() else ""
RED = "\033[31m" if sys.stdout.isatty() else ""
BOLD = "\033[1m" if sys.stdout.isatty() else ""

def format_file(path, clang_format, extra_args, timeout):
    cmd = [clang_format, "-i"]
    cmd += extra_args
    cmd.append(str(path))

    try:
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=timeout,
        )
        if result.returncode != 0:
            msg = (result.stderr or result.stdout or "non-zero exit code").strip()
            return ("error", str(path), msg)
        return ("ok", str(path), None)

    except subprocess.TimeoutExpired:
        return ("timeout", str(path), f"exceeded {timeout}s timeout")

    except FileNotFoundError:
        return ("error", str(path), f"'{clang_format}' not found — is it installed?")

    except Exception as exc:
        return ("error", str(path), str(exc))

def find_files(root, extensions, exclude_dirs):
    found = []
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames[:] = [d for d in dirnames if d not in exclude_dirs]
        for fname in filenames:
            if Path(fname).suffix.lower() in extensions:
                found.append(Path(dirpath) / fname)
    return sorted(found)

def main():
    p = argparse.ArgumentParser(
        description="Recursively runs clang-format on C/C++ files.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    p.add_argument("directory", nargs="?", default=".", type=Path)
    p.add_argument(
        "--clang-format",
        default="clang-format",
        metavar="BIN",
        help="Path to the clang-format executable. (default: clang-format)",
    )
    p.add_argument(
        "-j",
        "--jobs",
        type=int,
        default=DEFAULT_JOBS,
        metavar="N",
        help=f"Number of parallel workers. (default: {DEFAULT_JOBS})",
    )
    p.add_argument(
        "--timeout",
        type=int,
        default=DEFAULT_TIMEOUT,
        metavar="SECS",
        help=f"Per-file timeout in seconds. (default: {DEFAULT_TIMEOUT})",
    )
    p.add_argument(
        "--extensions",
        nargs="+",
        metavar="EXT",
        help="File extensions to process (including the dot)."
        f"(default: {' '.join(sorted(DEFAULT_EXTENSIONS))})",
    )
    p.add_argument(
        "--exclude",
        nargs="*",
        default=[],
        metavar="DIR",
        help="Directory names to skip during traversal (e.g. build third_party).",
    )
    p.add_argument(
        "--clang-format-args",
        nargs=argparse.REMAINDER,
        default=[],
        metavar="...",
    )
    args = p.parse_args()

    root = args.directory.resolve()
    if not root.is_dir():
        print(f"{RED}Error:{RESET} '{root}' is not a directory.", file=sys.stderr)
        return 1

    extensions = (
        {e if e.startswith(".") else f".{e}" for e in args.extensions}
        if args.extensions
        else DEFAULT_EXTENSIONS
    )

    exclude_dirs = set(args.exclude)

    files = find_files(root, extensions, exclude_dirs)
    if not files:
        print(f"{YELLOW}No source files found.{RESET}")
        return 0

    total = len(files)
    print(f"Found {BOLD}{total}{RESET} file(s).")

    counters = {"ok": 0, "timeout": 0, "error": 0}
    completed = 0

    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        futures = {
            pool.submit(
                format_file,
                f,
                args.clang_format,
                args.clang_format_args,
                args.timeout,
            ): f
            for f in files
        }

        for future in as_completed(futures):
            status, path_str, error = future.result()
            counters[status] += 1
            completed += 1
            rel = os.path.relpath(path_str, root)

            if status == "ok":
                tag = f"{GREEN}[ OK ]{RESET}"
            elif status == "timeout":
                tag = f"{YELLOW}[ TIMEOUT ]{RESET}"
            else:
                tag = f"{RED}[ ERROR ]{RESET}"

            progress = f"[{completed:>{len(str(total))}}/{total}]"
            line = f"{progress} {tag} {rel}"
            if error:
                line += f"\n         {RED}{error}{RESET}"
            print(line)

    print()
    print(f"{BOLD}Summary{RESET}")
    print(f"Total: {total}")
    print(f"{GREEN}OK: {counters['ok']}{RESET}")

    if counters["timeout"]:
        print(f"{YELLOW}Timeout: {counters['timeout']}{RESET}")
    if counters["error"]:
        print(f"{RED}Error: {counters['error']}{RESET}")

    return 1 if (counters["timeout"] or counters["error"]) else 0

if __name__ == "__main__":
    sys.exit(main())
