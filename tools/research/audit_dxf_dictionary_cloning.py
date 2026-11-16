#!/usr/bin/env python3
"""Inventory explicit dictionary cloning flags in ASCII DXF peer outputs.

This is a source-comparison tool, not a format validator or a default-value
oracle. In particular, an absent group 281 is reported as null, not zero.
"""
from __future__ import annotations

import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import sys

MAX_BYTES = 128 * 1024 * 1024


def audit_bytes(data: bytes) -> dict:
    if len(data) > MAX_BYTES or data.startswith(b"AutoCAD Binary DXF"):
        raise ValueError("expected an ASCII DXF of at most 128 MiB")
    lines = data.decode("utf-8", "surrogateescape").splitlines()
    if len(lines) % 2:
        raise ValueError("unpaired group code and value")
    tags = []
    for index in range(0, len(lines), 2):
        try:
            code = int(lines[index].strip())
        except ValueError as error:
            raise ValueError(f"invalid group code at line {index + 1}") from error
        tags.append((code, lines[index + 1].strip()))
    rows = []
    cursor = 0
    while cursor < len(tags):
        if tags[cursor] != (0, "DICTIONARY"):
            cursor += 1
            continue
        cursor += 1
        record = []
        while cursor < len(tags) and tags[cursor][0] != 0:
            record.append(tags[cursor])
            cursor += 1
        handle = None
        owner = None
        cloning = None
        entries = []
        pending = None
        app_group = False
        for code, value in record:
            if code == 102:
                if value.startswith("{"):
                    if app_group:
                        raise ValueError("nested application group")
                    app_group = True
                elif value == "}":
                    if not app_group:
                        raise ValueError("unmatched application group close")
                    app_group = False
                continue
            if app_group:
                continue
            if code == 5:
                if handle is not None:
                    raise ValueError("duplicate dictionary handle")
                handle = value.upper()
            elif code == 330:
                if owner is not None:
                    raise ValueError("duplicate dictionary owner")
                owner = value.upper()
            elif code == 281:
                if cloning is not None:
                    raise ValueError("duplicate cloning flag")
                cloning = int(value)
            elif code == 3:
                if pending is not None:
                    raise ValueError("dictionary entry lacks a target")
                pending = value
            elif code in (350, 360):
                if pending is None:
                    raise ValueError("dictionary target lacks an entry name")
                entries.append({"name": pending, "handle": value.upper(), "code": code})
                pending = None
        if app_group or pending is not None or handle is None:
            raise ValueError("incomplete dictionary record")
        rows.append({"handle": handle, "owner": owner, "cloning": cloning,
                     "entries": entries})
    by_handle = {row["handle"]: row for row in rows}
    if len(by_handle) != len(rows):
        raise ValueError("duplicate dictionary object handles")
    matches = []
    for row in rows:
        for entry in row["entries"]:
            if entry["name"].upper() == "ACAD_MLINESTYLE":
                target = by_handle.get(entry["handle"])
                if target is None:
                    raise ValueError("MLINESTYLE dictionary target missing")
                matches.append({"owner": row["handle"], "entry_code": entry["code"],
                                "target": target})
    counts = Counter("absent" if row["cloning"] is None else str(row["cloning"])
                     for row in rows)
    return {"sha256": hashlib.sha256(data).hexdigest(), "dictionaries": len(rows),
            "cloning_field_counts": dict(sorted(counts.items())),
            "mlinestyle_roots": matches}


def self_test() -> None:
    data = (b"0\nDICTIONARY\n5\nA\n330\n0\n3\nACAD_MLINESTYLE\n350\n10\n"
            b"0\nDICTIONARY\n5\n10\n102\n{ACAD_REACTORS\n330\nA\n102\n}\n"
            b"330\nA\n281\n1\n3\nStandard\n350\n22\n0\nEOF\n")
    result = audit_bytes(data)
    assert result["cloning_field_counts"] == {"1": 1, "absent": 1}
    assert result["mlinestyle_roots"][0]["target"]["owner"] == "A"
    for invalid in (data + b"0\n", data.replace(b"281\n1\n", b"281\n1\n281\n0\n")):
        try:
            audit_bytes(invalid)
        except ValueError:
            continue
        raise AssertionError("malformed input was accepted")
    print("dictionary-cloning audit self-test: PASS")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("paths", nargs="*", type=Path)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        self_test()
    if not args.paths and not args.self_test:
        parser.error("supply one or more ASCII DXF files")
    for path in args.paths:
        try:
            with path.open("rb") as stream:
                data = stream.read(MAX_BYTES + 1)
            result = audit_bytes(data)
        except (OSError, ValueError) as error:
            print(f"{path}: {error}", file=sys.stderr)
            return 1
        print(json.dumps({"path": str(path), **result}, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
