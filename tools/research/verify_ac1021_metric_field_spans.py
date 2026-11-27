#!/usr/bin/env python3
"""Verify the frozen metric AC1021 FIELD profile against two peer reports.

The observed body byte arrays conflict with the documented TV/string-stream
rule. This checker measures those bytes and does not authorize a parser change.
"""
import argparse
from collections import Counter
import importlib.util
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]
SOURCE_SHA = "7d5507da05fb70ba55875d99e676524ca2caa0ed7a5582ad130c3c06e8d925f1"

spec = importlib.util.spec_from_file_location(
    "field_span", ROOT / "tests/ac1021_field_source_span_probe.py")
span = importlib.util.module_from_spec(spec)
spec.loader.exec_module(span)


def selected_fields(records):
    selected = {}
    for handle, record in records.items():
        evaluator = span.first(record, 1)
        if evaluator not in ("AcObjProp.16.2", "AcObjProp"):
            continue
        root_at = next((i for i, (code, _) in enumerate(record) if code == 7), None)
        if root_at is None:
            continue
        root = record[root_at + 1:]
        if span.first(root, 93) != "4" or span.first(root, 90) != "2":
            continue
        children = {}
        for index, (code, value) in enumerate(record):
            if code != 6:
                continue
            end = next((j for j in range(index + 1, len(record))
                        if record[j][0] in (6, 7)), len(record))
            span.require(value not in children, "duplicate child key")
            children[value] = record[index + 1:end]
        prop = children.get("ObjectPropertyName", [])
        ident = children.get("ObjectPropertyId", [])
        name, flags = span.first(prop, 1), span.first(prop, 93)
        objects = [value for code, value in record if code == 331]
        if not (name and flags in ("0", "2") and span.first(prop, 90) == "4"
                and span.first(prop, 94) == "0" and len(objects) == 1
                and span.first(ident, 93) == "2" and span.first(ident, 90) == "64"
                and span.first(ident, 330) == objects[0]):
            continue
        prefix = record[:next(i for i, (code, _) in enumerate(record) if code == 100)]
        owner = span.first(prefix, 330)
        span.require(owner is not None, "missing FIELD owner")
        selected[handle] = (int(owner, 16), int(objects[0], 16), name, int(flags), evaluator)
    span.require(len(selected) == 42, "expected 42 selected metric FIELD profiles")
    return selected


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", required=True, type=Path)
    parser.add_argument("--raw-probe", required=True, type=Path)
    parser.add_argument("--oda-dxf", required=True, type=Path)
    parser.add_argument("--oda-sha256", required=True)
    parser.add_argument("--acadsharp-log", required=True, type=Path)
    parser.add_argument("--acadsharp-sha256", required=True)
    args = parser.parse_args()
    span.checked_bytes(args.source, SOURCE_SHA)
    oda = span.checked_bytes(args.oda_dxf, args.oda_sha256)
    sharp = span.acadsharp_fields(span.checked_bytes(args.acadsharp_log, args.acadsharp_sha256))
    selected = selected_fields(span.records_from_dxf(oda))
    raw = subprocess.run([str(args.raw_probe), str(args.source)], capture_output=True,
                         timeout=60, check=True).stdout
    span.require(len(raw) <= span.MAX_BYTES, "oversized raw probe output")
    frames = {}
    for line in raw.decode("ascii", errors="strict").splitlines():
        if not line.startswith("RAW "):
            continue
        parts = line.split()
        span.require(len(parts) == 4, "invalid raw probe output")
        handle, count, data = int(parts[1], 16), int(parts[2]), bytes.fromhex(parts[3])
        span.require(handle not in frames and count == len(data) <= 1024 * 1024,
                     "duplicate or invalid raw FIELD frame")
        frames[handle] = data
    span.require(len(frames) == 162, "expected 162 raw metric FIELD frames")
    evaluators, body_spans = Counter(), Counter()
    for handle, (owner, target, name, flags, evaluator) in selected.items():
        span.require(handle in frames and handle in sharp, "missing selected FIELD frame or peer")
        observed = span.inspect_frame(frames[handle], handle, owner, target, name, flags)
        span.require(sharp[handle].get("name") == name
                     and sharp[handle].get("objects") == f"{target:X}",
                     f"{handle:X}: ACadSharp mismatch")
        evaluators[evaluator] += 1
        body_spans[observed] += 1
    span.require(evaluators == {"AcObjProp": 19, "AcObjProp.16.2": 23}
                 and body_spans == {(287, 309, 32, 567): 42},
                 "metric source profile differs from the frozen proof")
    print(json.dumps({"sourceSha256": SOURCE_SHA, "odaSha256": args.oda_sha256.lower(),
                      "acadsharpSha256": args.acadsharp_sha256.lower(), "rawFrames": len(frames),
                      "verifiedFields": len(selected), "evaluators": dict(sorted(evaluators.items())),
                      "bodySpan": {"childStartBit": 287, "payloadStartBit": 309,
                                   "payloadBytes": 32, "bodyEndBit": 567, "count": 42},
                      "scope": "decompressed object-local bits and peer output, not native behavior"},
                     indent=2, sort_keys=True))


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, subprocess.SubprocessError) as error:
        raise SystemExit(f"FAIL: {error}") from error
