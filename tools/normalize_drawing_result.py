#!/usr/bin/env python3
"""Normalize a semantic conversion result (v2 default; explicit historical v1)."""

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
OPTIONAL = {"schema", "callbacks", "header", "semanticCounts", "exactReplayBytes",
            "projectionScope", "provenance"}
COMPARISON_SCHEMA = 2


class NormalizationError(ValueError):
    pass


def _canonical_v1(value):
    if isinstance(value, float):
        if math.isnan(value):
            return "NaN"
        if math.isinf(value):
            return "Infinity" if value > 0 else "-Infinity"
        if value == 0:
            return 0.0
        return value
    if isinstance(value, list):
        return [_canonical_v1(item) for item in value]
    if isinstance(value, dict):
        return {key: _canonical_v1(item) for key, item in value.items() if key not in VOLATILE}
    return value


def _normalize_v1(value):
    if not isinstance(value, dict):
        raise NormalizationError("input must be a JSON object")
    missing = [field for field in REQUIRED if field not in value]
    if missing:
        raise NormalizationError("missing required fields: %s" % ", ".join(missing))
    result = _canonical_v1(value)
    result["schema"] = 1
    result["relationships"] = sorted(
        result["relationships"],
        key=lambda item: (str(item.get("from", "")), str(item.get("kind", "")), str(item.get("to", ""))),
    )
    result["opaquePayloadHashes"] = sorted(str(item) for item in result["opaquePayloadHashes"])
    return result


def canonical(value, path="result"):
    """Copy strict JSON values without deleting any nested semantic fields."""
    if value is None or type(value) in (str, bool, int):
        return value
    if type(value) is float:
        if not math.isfinite(value):
            raise NormalizationError(path + " contains a nonfinite semantic number")
        return value
    if type(value) is list:
        return [canonical(item, "%s[%d]" % (path, index))
                for index, item in enumerate(value)]
    if type(value) is dict:
        if any(type(key) is not str for key in value):
            raise NormalizationError(path + " has a non-string JSON key")
        return {key: canonical(item, path + "." + key) for key, item in value.items()}
    raise NormalizationError(path + " contains a non-JSON value")


def typed_equal(left, right):
    """Exact recursive comparison, including integer and Boolean type identity."""
    if type(left) is not type(right):
        return False
    if type(left) is dict:
        return left.keys() == right.keys() and all(
            typed_equal(left[key], right[key]) for key in left)
    if type(left) is list:
        return len(left) == len(right) and all(
            typed_equal(a, b) for a, b in zip(left, right))
    return left == right


def loads_json(text):
    """Reject duplicate keys and nonstandard numeric constants at every depth."""
    def pairs(items):
        value = {}
        for key, item in items:
            if key in value:
                raise NormalizationError("duplicate JSON key: " + key)
            value[key] = item
        return value

    def constant(token):
        raise NormalizationError("nonstandard JSON number: " + token)

    try:
        return canonical(json.loads(text, object_pairs_hook=pairs,
                                    parse_constant=constant))
    except (json.JSONDecodeError, RecursionError) as exc:
        raise NormalizationError("invalid JSON: " + str(exc)) from exc


def normalize(value, *, schema=COMPARISON_SCHEMA):
    if type(schema) is not int or schema not in (1, COMPARISON_SCHEMA):
        raise NormalizationError("unsupported comparison schema")
    if schema == 1:
        # Frozen historical semantics remain available only by explicit request.
        return _normalize_v1(value)
    if type(value) is not dict:
        raise NormalizationError("input must be a JSON object")
    missing = set(REQUIRED) - value.keys()
    unknown = value.keys() - set(REQUIRED) - OPTIONAL - VOLATILE
    if missing or unknown:
        raise NormalizationError("projection keys differ: missing=%s unknown=%s" % (
            sorted(missing), sorted(unknown)))
    if "schema" in value and (type(value["schema"]) is not int
                              or value["schema"] != COMPARISON_SCHEMA):
        raise NormalizationError("historical normalized results require explicit schema1")
    try:
        result = canonical({key: item for key, item in value.items() if key not in VOLATILE})
    except RecursionError as exc:
        raise NormalizationError("projection nesting is too deep or circular") from exc
    for key in ("format", "version", "toolVersion"):
        if type(result[key]) is not str or not result[key]:
            raise NormalizationError(key + " must be a nonempty string")
    if type(result["exitStatus"]) is not int:
        raise NormalizationError("exitStatus must be an integer, not a Boolean")
    for key in ("diagnostics", "entities", "objects", "relationships", "opaquePayloadHashes"):
        if type(result[key]) is not list:
            raise NormalizationError(key + " must be a list")
    for key in ("entities", "objects", "relationships", "callbacks"):
        if key in result and (type(result[key]) is not list
                              or any(type(item) is not dict for item in result[key])):
            raise NormalizationError(key + " must contain JSON objects")
    for key in ("header", "semanticCounts", "projectionScope", "provenance"):
        if key in result and type(result[key]) is not dict:
            raise NormalizationError(key + " must be a JSON object")
    if "exactReplayBytes" in result and type(result["exactReplayBytes"]) is not str:
        raise NormalizationError("exactReplayBytes must be a string when present")
    for edge in result["relationships"]:
        if any(type(edge.get(key)) is not str or not edge[key]
               for key in ("from", "kind", "to")):
            raise NormalizationError("relationships require nonempty from/kind/to strings")
    if any(type(item) is not str for item in result["opaquePayloadHashes"]):
        raise NormalizationError("opaquePayloadHashes must contain strings")
    # Full JSON tie-break preserves all edge attributes and duplicate edges.
    result["relationships"] = sorted(result["relationships"], key=lambda edge: (
        edge["from"], edge["kind"], edge["to"],
        json.dumps(edge, sort_keys=True, separators=(",", ":"), allow_nan=False)))
    result["opaquePayloadHashes"] = sorted(result["opaquePayloadHashes"])
    result["schema"] = COMPARISON_SCHEMA
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
    assert result["schema"] == COMPARISON_SCHEMA
    assert "timestamp" not in result
    assert result["opaquePayloadHashes"] == ["a", "b"]
    assert [item["from"] for item in result["relationships"]] == ["1", "2"]
    nested = {**source, "objects": [{"date": "semantic", "generatedAt": "application"}]}
    assert normalize(nested)["objects"] == nested["objects"]
    assert normalize(source, schema=1)["schema"] == 1
    assert normalize(nested, schema=1)["objects"] == [{}]
    assert not typed_equal(True, 1)
    assert not typed_equal(1, 1.0)
    assert not typed_equal(2**60, 2**60 + 1)
    for invalid in ('{"x":1,"x":2}', '{"x":{"y":1,"y":2}}',
                    '{"x":NaN}', '{"x":Infinity}', '{"x":1e999}'):
        try:
            loads_json(invalid)
        except NormalizationError:
            pass
        else:
            raise AssertionError("invalid JSON was admitted: " + invalid)
    for invalid in ({**source, "unknownSemantic": {}},
                    {**source, "schema": 1}, {**source, "exitStatus": True},
                    {**source, "entities": [{"value": float("nan")}]},
                    {**source, "relationships": [{"from": "1", "kind": "owner"}]},
                    {**source, "opaquePayloadHashes": [True]}):
        try:
            normalize(invalid)
        except NormalizationError:
            pass
        else:
            raise AssertionError("invalid projection was admitted")
    assert typed_equal(normalize(source), normalize(normalize(source)))
    print("normalize_drawing_result self-test: PASS")


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--input", type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--comparison-schema", type=int, choices=(1, 2), default=2,
                        help="v2 default; v1 is explicit historical replay only")
    args = parser.parse_args(argv)
    try:
        if args.self_test:
            self_test()
            return 0
        if args.input is None:
            parser.error("--input is required unless --self-test is used")
        source = args.input.read_text(encoding="utf-8")
        value = loads_json(source) if args.comparison_schema == 2 else json.loads(source)
        encoded = json.dumps(normalize(value, schema=args.comparison_schema),
                             sort_keys=True, indent=2, allow_nan=False) + "\n"
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
