#!/usr/bin/env python3
"""Report handle-shaped inbound candidates in LibreDWG JSON.

The output is a search index, not a proof of absence: opaque bits, binary
payloads, unmodeled fields and numeric data may hide or mimic references.
"""
import argparse
import hashlib
import json
from collections import Counter
from pathlib import Path


def candidates(value, path, targets):
    if isinstance(value, dict):
        for key, child in value.items():
            yield from candidates(child, path + [str(key)], targets)
    elif isinstance(value, list):
        # LibreDWG serializes many handles as [role, size, value, absolute]
        # and some EED handles as [role, size, value]. Numeric arrays can also
        # fit this shape, so every result is intentionally a candidate only.
        if (len(value) in (3, 4)
                and all(type(item) is int for item in value)
                and 0 <= value[0] <= 15 and 0 <= value[1] <= 8
                and value[-1] in targets):
            yield {"path": path, "encoded": value, "target": value[-1]}
        else:
            for index, child in enumerate(value):
                yield from candidates(child, path + [str(index)], targets)


def inspect(document, targets, digest):
    if not isinstance(document, dict) or not isinstance(document.get("OBJECTS"), list):
        raise ValueError("expected LibreDWG JSON object with an OBJECTS array")
    objects = document["OBJECTS"]
    found = []
    opaque = Counter()
    types = Counter()
    for index, record in enumerate(objects):
        if not isinstance(record, dict):
            raise ValueError("OBJECTS entry is not an object")
        kind = record.get("object") or record.get("entity") or "UNCLASSIFIED"
        types[kind] += 1
        handle = record.get("handle")
        handle = handle[-1] if isinstance(handle, list) and handle and type(handle[-1]) is int else None
        if "unknown_bits" in record:
            opaque["objectsWithUnknownBits"] += 1
            bits = record.get("num_unknown_bits")
            if type(bits) is int and bits >= 0:
                opaque["reportedUnknownBits"] += bits
        if record.get("eed"):
            opaque["objectsWithEed"] += 1
        if record.get("xdata"):
            opaque["objectsWithXdata"] += 1
            if any(isinstance(item, list) and item and item[0] == 310
                   for item in record["xdata"]):
                opaque["objectsWithBinaryXdata"] += 1
        for match in candidates(record, [], targets):
            if match["path"] == ["handle"]:
                continue
            found.append({"objectIndex": index, "sourceHandle": handle,
                          "sourceKind": kind, **match})
    for key, value in document.items():
        if key != "OBJECTS":
            for match in candidates(value, [key], targets):
                found.append({"objectIndex": None, "sourceHandle": None,
                              "sourceKind": "NON_OBJECT_SECTION", **match})
    return {"schema": 1, "kind": "libredwg-inbound-candidates",
            "inputSha256": digest, "targetHandles": sorted(targets),
            "objectCount": len(objects), "objectTypes": dict(sorted(types.items())),
            "opaqueIndicators": dict(sorted(opaque.items())), "candidates": found,
            "limits": "Candidate shapes can be false positives; absence from JSON does not exclude opaque or unmodeled references. This output cannot authorize data removal or support claims."}


def self_test():
    sample = {"HEADER": {"example": [4, 1, 9, 9]}, "OBJECTS": [
        {"object": "DICTIONARY", "handle": [0, 1, 2],
         "items": {"k": [2, 1, 9, 9]}, "unknown_bits": "FF", "num_unknown_bits": 8},
        {"entity": "LINE", "handle": [0, 1, 3], "eed": [{"code": 5, "value": [5, 8, 9]}],
         "xdata": [[310, "FF"]]},
    ]}
    report = inspect(sample, {9}, "test")
    assert len(report["candidates"]) == 3
    assert report["opaqueIndicators"]["reportedUnknownBits"] == 8
    assert report["opaqueIndicators"]["objectsWithBinaryXdata"] == 1
    assert all(row["path"] != ["handle"] for row in report["candidates"])
    try:
        inspect({"OBJECTS": [None]}, {9}, "test")
    except ValueError:
        pass
    else:
        raise AssertionError("invalid OBJECTS entry was accepted")
    print("LibreDWG inbound-candidate self-test: PASS")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("json_file", nargs="?", type=Path)
    parser.add_argument("--target", action="append", default=[], type=lambda value: int(value, 0),
                        help="target handle as decimal or 0x-prefixed hex; repeatable")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        self_test()
        return
    if args.json_file is None or not args.target or any(not 0 < value <= 0xFFFFFFFF for value in args.target):
        parser.error("provide a JSON file and at least one nonzero 32-bit --target")
    raw = args.json_file.read_bytes()
    print(json.dumps(inspect(json.loads(raw), set(args.target), hashlib.sha256(raw).hexdigest()),
                     indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
