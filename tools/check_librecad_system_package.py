#!/usr/bin/env python3
"""Compile LibreCAD's DXFRW filter against an installed libdxfrw package.

The checker is intentionally source-only: it rewrites one compile-database
command by removing the bundled libdxfrw include roots and adding the staged
package include roots.  It never configures or edits LibreCAD, and it never
reads or emits drawing files.
"""

from __future__ import annotations

import argparse
import json
import shlex
import subprocess
from pathlib import Path


def _is_bundled_libdxfrw(path: str) -> bool:
    normalized = path.replace("\\", "/")
    return "/libraries/libdxfrw/" in normalized or normalized.endswith(
        "/libraries/libdxfrw")


def rewrite_command(arguments: list[str], source: Path, prefix: Path) -> list[str]:
    """Return a syntax-only command using only the installed DXFRW headers."""
    source = source.resolve()
    rewritten: list[str] = []
    index = 0
    while index < len(arguments):
        argument = arguments[index]
        if argument in {"-c", "-fsyntax-only"}:
            rewritten.append("-fsyntax-only")
            index += 1
            continue
        if argument == "-o":
            index += 2
            continue
        if argument in {"-isysroot", "--sysroot"} and index + 1 < len(arguments):
            # Qt Creator can retain an SDK path after Xcode has removed that
            # SDK.  The package check is a header-consumer check; omit only a
            # missing sysroot and leave all valid toolchain flags intact.
            if not Path(arguments[index + 1]).exists():
                index += 2
                continue
            rewritten.extend((argument, arguments[index + 1]))
            index += 2
            continue
        if argument == "-isystem" and index + 1 < len(arguments):
            if not Path(arguments[index + 1]).exists():
                index += 2
                continue
        if argument == str(source) or Path(argument).resolve() == source:
            index += 1
            continue
        if argument in {"-I", "-isystem"} and index + 1 < len(arguments):
            if _is_bundled_libdxfrw(arguments[index + 1]):
                index += 2
                continue
            rewritten.extend((argument, arguments[index + 1]))
            index += 2
            continue
        if argument.startswith("-I") and len(argument) > 2:
            if _is_bundled_libdxfrw(argument[2:]):
                index += 1
                continue
        rewritten.append(argument)
        index += 1
    include = prefix.resolve() / "include" / "libdxfrw"
    rewritten.extend(("-I", str(include), "-I", str(include / "intern"), str(source)))
    if "-fsyntax-only" not in rewritten:
        rewritten.append("-fsyntax-only")
    return rewritten


def add_compiler_resource_include(arguments: list[str]) -> None:
    """Keep stale Qt Creator commands usable with the active clang install."""
    compiler = arguments[0]
    try:
        resource = subprocess.check_output(
            [compiler, "-print-resource-dir"], text=True,
            stderr=subprocess.DEVNULL).strip()
    except (OSError, subprocess.CalledProcessError):
        return
    include = str(Path(resource) / "include")
    if Path(include).is_dir() and include not in arguments:
        arguments.extend(("-isystem", include))


def add_librecad_source_includes(arguments: list[str], root: Path | None) -> None:
    """Repair incomplete IDE compile databases without touching DXFRW roots."""
    if root is None:
        return
    source_root = root.resolve() / "librecad" / "src"
    if not source_root.is_dir():
        raise ValueError("LibreCAD root has no librecad/src: %s" % root)
    existing = set(arguments)
    for directory in sorted(path for path in source_root.rglob("*")
                            if path.is_dir()):
        value = str(directory)
        if value not in existing:
            arguments.extend(("-I", value))
            existing.add(value)


def load_command(database: Path, source: Path) -> list[str]:
    entries = json.loads(database.read_text(encoding="utf-8"))
    source = source.resolve()
    for entry in entries:
        candidate = Path(entry["file"])
        if not candidate.is_absolute():
            candidate = Path(entry["directory"]) / candidate
        if candidate.resolve() != source:
            continue
        arguments = entry.get("arguments")
        if arguments is not None:
            return list(arguments)
        return shlex.split(entry["command"])
    raise ValueError("compile database has no entry for %s" % source)


def self_test() -> None:
    source = Path("/workspace/librecad/src/lib/filters/rs_filterdxfrw.cpp")
    command = [
        "clang", "-I/workspace/LibreCAD/libraries/libdxfrw/src", "-I",
        "/workspace/LibreCAD/libraries/libdxfrw/src/intern", "-Wall", "-c",
        str(source), "-o", "/tmp/filter.o",
    ]
    rewritten = rewrite_command(command, source, Path("/tmp/libdxfrw"))
    text = " ".join(rewritten)
    if "/libraries/libdxfrw/" in text or " -c " in " %s " % text:
        raise RuntimeError("bundled include or compile-only flag survived rewrite")
    expected_include = str((Path("/tmp/libdxfrw").resolve() /
                            "include" / "libdxfrw"))
    if "-I " + expected_include not in text:
        raise RuntimeError("staged include root was not added")
    if rewritten[-1] != str(source):
        raise RuntimeError("source was not placed at the end of the command")
    print("LibreCAD system-package checker self-test: PASS")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--prefix", type=Path)
    parser.add_argument("--compile-database", type=Path)
    parser.add_argument("--source", type=Path)
    parser.add_argument("--librecad-root", type=Path,
                        help="LibreCAD checkout used to fill incomplete source include paths")
    parser.add_argument("--cxx", default=None,
                        help="override the compiler (normally from compile_commands.json)")
    args = parser.parse_args()
    if args.self_test:
        self_test()
        if not (args.prefix and args.compile_database and args.source):
            return 0
    if not (args.prefix and args.compile_database and args.source):
        parser.error("--prefix, --compile-database, and --source are required")
    prefix = args.prefix.resolve()
    source = args.source.resolve()
    command = load_command(args.compile_database.resolve(), source)
    rewritten = rewrite_command(command, source, prefix)
    if any(_is_bundled_libdxfrw(item) for item in rewritten):
        raise RuntimeError("rewritten command still references bundled libdxfrw")
    if args.cxx:
        rewritten[0] = args.cxx
    add_compiler_resource_include(rewritten)
    add_librecad_source_includes(rewritten, args.librecad_root)
    result = subprocess.run(rewritten, cwd=source.parent, text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if result.returncode:
        print(result.stdout, end="")
        raise SystemExit(result.returncode)
    print("LibreCAD system-package filter compile: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
