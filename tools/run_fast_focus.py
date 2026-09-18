#!/usr/bin/env python3
"""Run the dependency-free libdxfrw focused executable set.

This is intentionally narrower than CTest: it exercises deterministic parser,
graph, writer, and diagnostic vectors without requiring external DWG/DXF
payloads.  Fixture-backed executables remain separate and are never silently
treated as part of this fast lane.
"""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path


TARGETS = (
    "libdxfrw_wave1_tests",
    "libdxfrw_hardening_tests",
    "libdxfrw_graph_preservation_tests",
    "libdxfrw_writer_primitives_tests",
    "libdxfrw_dwg_object_vectors_tests",
    "libdxfrw_writer_version_matrix_tests",
    "libdxfrw_diagnostic_tests",
)


def executable(build_dir: Path, target: str, configuration: str | None) -> Path:
    names = (target, target + ".exe")
    roots = [build_dir]
    if configuration:
        roots.insert(0, build_dir / configuration)
    for root in roots:
        for name in names:
            candidate = root / name
            if candidate.is_file():
                return candidate
    raise FileNotFoundError(
        f"focused executable {target!r} is missing under {build_dir}"
    )


def run(build_dir: Path, configuration: str | None) -> int:
    for target in TARGETS:
        path = executable(build_dir, target, configuration)
        print(f"[{target}] {path}")
        subprocess.run([str(path)], check=True)
    print(f"fast focus: PASS ({len(TARGETS)} executables)")
    return 0


def self_test() -> None:
    assert TARGETS[0] == "libdxfrw_wave1_tests"
    assert len(TARGETS) == len(set(TARGETS))
    print("run_fast_focus self-test: PASS")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path)
    parser.add_argument("--configuration", default=None)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args(argv)
    if args.self_test:
        self_test()
        return 0
    if args.build_dir is None:
        parser.error("--build-dir is required unless --self-test is used")
    try:
        return run(args.build_dir.resolve(), args.configuration)
    except (OSError, subprocess.CalledProcessError) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
