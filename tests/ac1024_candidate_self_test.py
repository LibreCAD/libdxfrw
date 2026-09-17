#!/usr/bin/env python3
"""Run the frozen AC1024 checker self-tests on every native host.

The v1 checker intentionally freezes a POSIX ``/usr/bin/env`` probe as part
of its source contract.  Windows has no equivalent path, so this harness
adapts only that self-test probe to the current absolute Python interpreter;
the frozen checker bytes and all live qualification paths remain untouched.
"""

from __future__ import annotations

import argparse
import importlib.util
import os
import sys
from pathlib import Path
from types import ModuleType


ROOT = Path(__file__).resolve().parents[1]


def load_module(path: Path, name: str) -> ModuleType:
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError("cannot load %s" % path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def portable_run_capture(module: ModuleType):
    original = module.run_capture

    def run_capture(command, timeout, *, text=False):
        if os.name == "nt" and command == ["/usr/bin/env"]:
            command = [
                sys.executable,
                "-c",
                "import os; print('\\n'.join(f'{k}={v}' "
                "for k, v in sorted(os.environ.items())))",
            ]
        return original(command, timeout, text=text)

    return run_capture


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--checker", choices=("v1", "v2"), required=True)
    parser.add_argument("--manifest", type=Path, required=True)
    args = parser.parse_args()

    if args.checker == "v1":
        checker = load_module(
            ROOT / "tools/check_ac1024_candidate_fields.py",
            "ac1024_candidate_fields_v1_selftest",
        )
        checker.run_capture = portable_run_capture(checker)
        checker.self_test(args.manifest.resolve())
    else:
        checker = load_module(
            ROOT / "tools/check_ac1024_candidate_fields_v2.py",
            "ac1024_candidate_fields_v2_selftest",
        )
        predecessor = checker.load_predecessor_checker()
        predecessor.run_capture = portable_run_capture(predecessor)
        checker.self_test(args.manifest.resolve(), predecessor)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
