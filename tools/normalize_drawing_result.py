#!/usr/bin/env python3
"""Normalize a semantic conversion result under oracle-normalization schema v1."""

from __future__ import annotations

import argparse
import json
import math
import sys
from pathlib import Path


REQUIRED = (
    "format",
    "version",
    "toolVersion",
    "exitStatus",
    "diagnostics",
    "entities",
    "objects",
    "relationships",
    "opaquePayloadHashes",
)
VOLATILE = {"timestamp", "date", "generatedAt", "toolInvocation"}


class NormalizationError(ValueError):
    pass


def canonical(value):
    if isinstance(value, float):
        if math.isnan(value):
            return "NaN"
        if math.isinf(value):
            return "Infinity" if value > 0 else "-Infinity"
        if value == 0:
            return 0.0
        return value
    if isinstance(value, list):
        return [canonical(item) for item in value]
    if isinstance(value, dict):
        return {key: canonical(item) for key, item in value.items() if key not in VOLATILE}
    return value


def normalize(value):
    if not isinstance(value, dict):
        raise NormalizationError("input must be a JSON object")
    missing = [field for field in REQUIRED if field not in value]
    if missing:
        raise NormalizationError("missing required fields: %s" % ", ".join(missing))
    result = canonical(value)
    result["schema"] = 1
    result["relationships"] = sorted(
        result["relationships"],
        key=lambda item: (str(item.get("from", "")), str(item.get("kind", "")), str(item.get("to", ""))),
    )
    result["opaquePayloadHashes"] = sorted(str(item) for item in result["opaquePayloadHashes"])
    return result


def self_test():
    source = {
        "format": "DXF",
        "version": "AC1027",
        "toolVersion": "test",
        "exitStatus": 0,
        "diagnostics": [],
        "entities": [{"handle": "1"}],
        "objects": [],
        "relationships": [{"from": "2", "to": "1", "kind": "owner"}, {"from": "1", "to": "0", "kind": "owner"}],
        "opaquePayloadHashes": ["b", "a"],
        "timestamp": "volatile",
    }
    result = normalize(source)
    assert result["schema"] == 1
    assert "timestamp" not in result
    assert result["opaquePayloadHashes"] == ["a", "b"]
    assert [item["from"] for item in result["relationships"]] == ["1", "2"]
    print("normalize_drawing_result self-test: PASS")


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--input", type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args(argv)
    try:
        if args.self_test:
            self_test()
            return 0
        if args.input is None:
            parser.error("--input is required unless --self-test is used")
        value = json.loads(args.input.read_text(encoding="utf-8"))
        encoded = json.dumps(normalize(value), sort_keys=True, indent=2) + "\n"
        if args.output:
            args.output.write_text(encoded, encoding="utf-8")
        else:
            sys.stdout.write(encoded)
        return 0
    except (OSError, UnicodeError, json.JSONDecodeError, NormalizationError) as exc:
        print("normalization: FAIL: %s" % exc, file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
