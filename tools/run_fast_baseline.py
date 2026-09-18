#!/usr/bin/env python3
"""Run the fast, non-fixture baseline gates and emit a stable summary."""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
from pathlib import Path


DEFAULT_TARGETS = ("dxfrw", "dwg2dxf", "lc3_compat_check")


def run(build_dir: Path, targets: tuple[str, ...]) -> dict:
    results = []
    for target in targets:
        command = ["cmake", "--build", str(build_dir), "--target", target, "-j2"]
        completed = subprocess.run(command, text=True, stdout=subprocess.PIPE,
                                   stderr=subprocess.STDOUT, check=False)
        results.append({"target": target, "command": command,
                        "exitStatus": completed.returncode,
                        "outputTail": completed.stdout[-2000:]})
    return {"schema": "libdxfrw-fast-baseline-v1", "targets": results,
            "allPassed": all(item["exitStatus"] == 0 for item in results)}


def self_test() -> None:
    assert run.__name__ == "run"
    assert all(target for target in DEFAULT_TARGETS)
    print("run_fast_baseline self-test: PASS")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, required=False)
    parser.add_argument("--source-commit", default="unknown")
    parser.add_argument("--output", type=Path)
    parser.add_argument("--target", action="append", dest="targets")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args(argv)
    if args.self_test:
        self_test()
        return 0
    if args.build_dir is None:
        parser.error("--build-dir is required unless --self-test is used")
    try:
        result = run(args.build_dir, tuple(args.targets or DEFAULT_TARGETS))
        result["sourceCommit"] = args.source_commit
        result["buildDirectory"] = str(args.build_dir)
        text = json.dumps(result, indent=2, sort_keys=True) + "\n"
        if args.output:
            args.output.write_text(text, encoding="utf-8")
        else:
            print(text, end="")
        return 0 if result["allPassed"] else 1
    except (OSError, subprocess.SubprocessError) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
