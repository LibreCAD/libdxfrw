#!/usr/bin/env python3
"""Validate the measured fast-gate and validation-cadence contract.

The metadata is deliberately descriptive: it records commands, selectors, and
durations without copying any drawing payload or depending on a particular
build directory.  A fast gate may be shorter than a full run, but it never
weakens the full checkpoint policy.
"""

from __future__ import annotations

import argparse
import json
import math
import tempfile
from pathlib import Path


class SpeedError(ValueError):
    pass


def read_json(path: Path) -> dict:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as exc:
        raise SpeedError(f"cannot read {path}: {exc}") from exc
    if not isinstance(value, dict):
        raise SpeedError("speed metadata must contain an object")
    return value


def validate(value: dict) -> None:
    if value.get("schema") != 1:
        raise SpeedError("speed metadata schema must be 1")
    if value.get("kind") != "libdxfrw-implementation-speed-baseline":
        raise SpeedError("unexpected speed metadata kind")

    measurements = value.get("measurements")
    if not isinstance(measurements, list) or not measurements:
        raise SpeedError("measurements must be a non-empty array")
    by_id = {}
    for row in measurements:
        if not isinstance(row, dict):
            raise SpeedError("measurement must be an object")
        row_id = row.get("id")
        if not isinstance(row_id, str) or not row_id or row_id in by_id:
            raise SpeedError("measurement ids must be unique non-empty strings")
        if not isinstance(row.get("command"), str) or not row["command"]:
            raise SpeedError(f"measurement {row_id} has no command")
        if not isinstance(row.get("selector"), str) or not row["selector"]:
            raise SpeedError(f"measurement {row_id} has no selector")
        seconds = row.get("seconds")
        if not isinstance(seconds, (int, float)) or not math.isfinite(seconds) or seconds < 0:
            raise SpeedError(f"measurement {row_id} has invalid duration")
        by_id[row_id] = row

    required = {"incremental-build", "fast-tests", "full-build", "full-tests"}
    missing = sorted(required - set(by_id))
    if missing:
        raise SpeedError("missing required measurements: " + ", ".join(missing))
    if by_id["fast-tests"]["seconds"] >= by_id["full-tests"]["seconds"]:
        raise SpeedError("fast-tests must be materially shorter than full-tests")

    selector_map = value.get("selectorMap")
    if not isinstance(selector_map, list) or not selector_map:
        raise SpeedError("selectorMap must be a non-empty array")
    classes = set()
    for row in selector_map:
        if not isinstance(row, dict):
            raise SpeedError("selectorMap row must be an object")
        path_class = row.get("changedPathClass")
        selectors = row.get("selects")
        if not isinstance(path_class, str) or not path_class or path_class in classes:
            raise SpeedError("selectorMap path classes must be unique non-empty strings")
        if not isinstance(selectors, list) or not selectors or not all(
            isinstance(item, str) and item for item in selectors
        ):
            raise SpeedError(f"selectorMap row {path_class} has no selectors")
        classes.add(path_class)
    if "unknown" not in classes:
        raise SpeedError("selectorMap must fail closed with an unknown-path row")

    cadence = value.get("cadence")
    if not isinstance(cadence, dict):
        raise SpeedError("cadence must be an object")
    if cadence.get("fullSuite") != "checkpoint-only":
        raise SpeedError("fullSuite cadence must remain checkpoint-only")
    if cadence.get("sanitizer") != "checkpoint-or-security-triggered":
        raise SpeedError("sanitizer cadence is not explicit")
    if cadence.get("externalCorpus") != "nightly-advisory":
        raise SpeedError("external corpus cadence is not advisory/nightly")


def self_test() -> None:
    valid = {
        "schema": 1,
        "kind": "libdxfrw-implementation-speed-baseline",
        "measurements": [
            {"id": "incremental-build", "command": "cmake --build", "selector": "direct", "seconds": 1.0},
            {"id": "fast-tests", "command": "ctest -R fast", "selector": "fast", "seconds": 1.0},
            {"id": "full-build", "command": "cmake --build", "selector": "all", "seconds": 2.0},
            {"id": "full-tests", "command": "ctest", "selector": "all", "seconds": 3.0},
        ],
        "selectorMap": [
            {"changedPathClass": "private-cpp", "selects": ["direct"]},
            {"changedPathClass": "unknown", "selects": ["safe-default"]},
        ],
        "cadence": {
            "fullSuite": "checkpoint-only",
            "sanitizer": "checkpoint-or-security-triggered",
            "externalCorpus": "nightly-advisory",
        },
    }
    with tempfile.TemporaryDirectory() as directory:
        path = Path(directory) / "speed.json"
        path.write_text(json.dumps(valid), encoding="utf-8")
        validate(read_json(path))
        invalid = dict(valid)
        invalid["measurements"] = [*valid["measurements"]]
        invalid["measurements"][1] = {**invalid["measurements"][1], "seconds": 4.0}
        try:
            validate(invalid)
        except SpeedError:
            pass
        else:
            raise AssertionError("slower fast test was accepted")
    print("check_implementation_speed self-test: PASS")


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--metadata", type=Path,
        default=Path("metadata/implementation-speed-baseline-v1.json"),
    )
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args(argv)
    try:
        if args.self_test:
            self_test()
        else:
            validate(read_json(args.metadata))
            print(f"implementation speed: PASS ({args.metadata})")
        return 0
    except (OSError, UnicodeError, SpeedError, AssertionError) as exc:
        print(f"implementation speed: FAIL: {exc}")
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
