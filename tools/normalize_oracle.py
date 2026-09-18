#!/usr/bin/env python3
"""Normalize a JSON semantic result according to oracle schema v1.

The normalizer is intentionally format-agnostic.  Readers/converters emit a
small JSON summary; this tool removes volatile environment fields, canonicalizes
handle relationships, normalizes signed zero, and preserves the distinction
between ordered and set-like collections.  It does not compare raw drawing
bytes or manufacture a support claim.
"""

from __future__ import annotations

import argparse
import json
import math
import sys
from pathlib import Path


VOLATILE = {"elapsedMs", "host", "path", "tempPath", "timestamp"}
HANDLE_KEYS = {"handle", "owner", "parent", "reactor", "reactors", "id", "ref"}
SET_LIKE = {"warnings", "diagnostics"}


def canonical_handle(value: object, mapping: dict[str, str]) -> object:
    if not isinstance(value, str):
        return value
    upper = value.upper()
    if not upper or upper in {"0", "NONE", "NULL"}:
        return value
    if upper not in mapping:
        mapping[upper] = f"H{len(mapping) + 1:06d}"
    return mapping[upper]


def normalize(value: object, mapping: dict[str, str] | None = None,
              key: str = "") -> object:
    mapping = {} if mapping is None else mapping
    if isinstance(value, dict):
        result = {}
        for name in sorted(value):
            if name in VOLATILE:
                continue
            child = value[name]
            if name in HANDLE_KEYS:
                child = canonical_handle(child, mapping)
            result[name] = normalize(child, mapping, name)
        return result
    if isinstance(value, list):
        items = [normalize(item, mapping, key) for item in value]
        if key in SET_LIKE:
            return sorted(items, key=lambda item: json.dumps(item, sort_keys=True,
                                                              separators=(",", ":")))
        return items
    if isinstance(value, float):
        if not math.isfinite(value):
            raise ValueError(f"non-finite value in {key or 'result'}")
        return 0.0 if value == 0.0 else value
    return value


def normalize_document(document: object) -> dict:
    if not isinstance(document, dict):
        raise ValueError("oracle input must be a JSON object")
    for field in ("tool", "toolVersion", "exitStatus", "semanticCounts", "relationships"):
        if field not in document:
            raise ValueError(f"oracle input missing required field {field}")
    result = normalize(document)
    assert isinstance(result, dict)
    result["oracleSchema"] = "libdxfrw-oracle-v1"
    return result


def self_test() -> None:
    source = {
        "tool": "self-test", "toolVersion": "1", "exitStatus": 0,
        "timestamp": "volatile", "semanticCounts": {"entities": 1},
        "relationships": [{"handle": "20", "owner": "0"}],
        "warnings": ["z", "a"], "value": -0.0,
    }
    result = normalize_document(source)
    assert "timestamp" not in result
    assert result["relationships"][0]["handle"] == "H000001"
    assert result["relationships"][0]["owner"] == "0"
    assert result["warnings"] == ["a", "z"]
    assert result["value"] == 0.0
    print("normalize_oracle self-test: PASS")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", nargs="?", type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args(argv)
    if args.self_test:
        self_test()
        return 0
    try:
        raw = (args.input.read_text(encoding="utf-8") if args.input
               else sys.stdin.read())
        result = normalize_document(json.loads(raw))
        output = json.dumps(result, indent=2, sort_keys=True) + "\n"
        if args.output:
            args.output.write_text(output, encoding="utf-8")
        else:
            print(output, end="")
        return 0
    except (OSError, ValueError, json.JSONDecodeError) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
