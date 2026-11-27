#!/usr/bin/env python3
"""Verify the frozen metric AC1021 FIELD profile against two peer reports.

The observed body byte arrays conflict with the documented TV/string-stream
rule. This checker measures those bytes and does not authorize a parser change.
"""
import argparse
from collections import Counter
import importlib.util
import json
import re
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


def sharp_values(data):
    fields, current = {}, None
    for line in data.decode("utf-8", errors="strict").splitlines():
        match = re.match(r"^FIELD handle=0x([0-9A-F]+) owner=0x([0-9A-F]+).*? rootflags=(\d+) roottype=(\w+).*? rootvalue=(.*?) rootformat=", line)
        if match:
            current = int(match.group(1), 16)
            span.require(current not in fields, "duplicate ACadSharp FIELD")
            fields[current] = {"owner": int(match.group(2), 16),
                               "flags": int(match.group(3)), "type": match.group(4),
                               "root": match.group(5)}
        elif current is not None:
            match = re.match(r"^CHILD key=(.*?) flags=.*? type=String unit=.*? value=(.*?) format=", line)
            if match:
                span.require(match.group(1) not in fields[current], "duplicate ACadSharp child")
                fields[current][match.group(1)] = match.group(2)
    return fields


def remaining_spans(records, covered, frames, peer):
    profiles, examples = Counter(), {}
    total_values = 0
    for handle, record in records.items():
        evaluator = span.first(record, 1)
        if evaluator not in ("AcObjProp", "AcObjProp.16.2", "AcSm") or handle in covered:
            continue
        root_at = next(i for i, (code, _) in enumerate(record) if code == 7)
        root = record[root_at + 1:]
        flags, data_type = int(span.first(root, 93)), int(span.first(root, 90))
        span.require(handle in peer and peer[handle]["flags"] == flags and
                     peer[handle]["type"] == {0: "Unknown", 4: "String"}.get(data_type),
                     f"{handle:X}: peer root mismatch")
        prefix = record[:next(i for i, (code, _) in enumerate(record) if code == 100)]
        owner = int(span.first(prefix, 330), 16)
        span.require(owner == handle - 1 and peer[handle]["owner"] == owner and
                     not any(code == 331 for code, _ in record), f"{handle:X}: owner/edge mismatch")
        values = []
        if data_type == 4 and not (flags & 1):
            values.append(("root", span.first(root, 1), flags))
        for index, (code, key) in enumerate(record):
            if code != 6:
                continue
            end = next((j for j in range(index + 1, len(record))
                        if record[j][0] in (6, 7)), len(record))
            child = record[index + 1:end]
            child_type, child_flags = int(span.first(child, 90)), int(span.first(child, 93))
            if child_type == 4 and not (child_flags & 1):
                values.append((key, span.first(child, 1), child_flags))
        span.require(handle in frames and values, f"{handle:X}: missing frame/value")
        data = frames[handle]
        bits = "".join(f"{value:08b}" for value in data)
        span.require(bits[:18] == "001111110000000001", "unexpected FIELD type framing")
        object_end = int.from_bytes(span.bit_bytes(bits, 18, 4), "little")
        span.require(17 <= object_end <= len(bits) and bits[object_end - 1] == "1",
                     "invalid FIELD string-stream marker")
        string_size = int.from_bytes(span.bit_bytes(bits, object_end - 17, 2), "little")
        span.require((string_size & 0x8000) == 0, "extended string stream outside profile")
        body_end = object_end - 17 - string_size
        span.require(0 <= body_end < object_end - 17, "invalid body/string bounds")
        candidate_sets = []
        for key, text, value_flags in values:
            span.require(text is not None and peer[handle].get(key) == text,
                         f"{handle:X}: {key} peer value mismatch")
            payload = (text + "\0").encode("utf-16le")
            pattern = "".join(f"{value:08b}" for value in payload)
            candidates, at = [], bits.find(pattern, 0, body_end)
            while at >= 0:
                for start in range(max(0, at - 78), at):
                    try:
                        first, cursor = span.bit_long(bits, start)
                        kind, cursor = span.bit_long(bits, cursor)
                        count, cursor = span.bit_long(bits, cursor)
                        if (first, kind, count, cursor) != (value_flags, 4, len(payload), at):
                            continue
                        unit, end = span.bit_long(bits, at + len(payload) * 8)
                        if unit == 0 and end <= body_end:
                            candidates.append((key, start, at, len(payload), end))
                    except ValueError:
                        continue
                at = bits.find(pattern, at + 1, body_end)
            span.require(0 < len(candidates) <= 8, f"{handle:X}: invalid candidate count")
            candidate_sets.append(candidates)
        paths = [[]]
        for candidates in candidate_sets:
            paths = [path + [item] for path in paths for item in candidates
                     if not path or item[1] >= path[-1][-1]]
            span.require(len(paths) <= 128, f"{handle:X}: ambiguous candidate budget")
        paths = [path for path in paths if path and path[-1][-1] == body_end]
        span.require(len(paths) == 1, f"{handle:X}: ambiguous ordered spans")
        token, after = span.handle(bits, object_end)
        span.require(token == (8, 0) and len(bits) - after < 8,
                     f"{handle:X}: owner role or handle tail mismatch")
        profile = (evaluator, flags, data_type, len(values))
        profiles[profile] += 1
        total_values += len(values)
        examples.setdefault(profile, {"handle": f"0x{handle:X}",
                                      "bodyEndBit": body_end, "spans": paths[0]})
    expected = {("AcSm", 0, 4, 3): 9, ("AcObjProp.16.2", 0, 4, 3): 1,
                ("AcObjProp.16.2", 1, 0, 2): 14}
    span.require(profiles == expected and total_values == 58,
                 "remaining metric FIELD profiles differ from the frozen proof")
    return {"fields": sum(profiles.values()), "valueSpans": total_values,
            "profiles": [{"evaluator": key[0], "rootFlags": key[1],
                          "rootType": key[2], "stringsPerField": key[3],
                          "count": count, "example": examples[key]}
                         for key, count in sorted(profiles.items())]}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", required=True, type=Path)
    parser.add_argument("--raw-probe", required=True, type=Path)
    parser.add_argument("--oda-dxf", required=True, type=Path)
    parser.add_argument("--oda-sha256", required=True)
    parser.add_argument("--acadsharp-log", required=True, type=Path)
    parser.add_argument("--acadsharp-sha256", required=True)
    parser.add_argument("--include-remaining", action="store_true")
    args = parser.parse_args()
    span.checked_bytes(args.source, SOURCE_SHA)
    oda = span.checked_bytes(args.oda_dxf, args.oda_sha256)
    sharp_data = span.checked_bytes(args.acadsharp_log, args.acadsharp_sha256)
    sharp = span.acadsharp_fields(sharp_data)
    records = span.records_from_dxf(oda)
    selected = selected_fields(records)
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
    remaining = remaining_spans(records, selected, frames, sharp_values(sharp_data)) if args.include_remaining else None
    print(json.dumps({"sourceSha256": SOURCE_SHA, "odaSha256": args.oda_sha256.lower(),
                      "acadsharpSha256": args.acadsharp_sha256.lower(), "rawFrames": len(frames),
                      "verifiedFields": len(selected), "remaining": remaining,
                      "evaluators": dict(sorted(evaluators.items())),
                      "bodySpan": {"childStartBit": 287, "payloadStartBit": 309,
                                   "payloadBytes": 32, "bodyEndBit": 567, "count": 42},
                      "scope": "decompressed object-local bits and peer output, not native behavior"},
                     indent=2, sort_keys=True))


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, subprocess.SubprocessError) as error:
        raise SystemExit(f"FAIL: {error}") from error
