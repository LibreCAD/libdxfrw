#!/usr/bin/env python3
"""Audit a generated acadrust AC1012 pair with independent bounded wire checks.

The manifest must come from generate_ac1012_acadrust_pair.py. LibreDWG locates
and independently decodes the objects; the shared specification-directed
walkers check selected dictionary and MLINESTYLE frames, field spans and handle
roles. This is not proof of native AutoCAD open or cloning semantics.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import struct
import subprocess
import tempfile

from generate_ac1012_dictionary_merge_samples import (
    observe_populated_style_frame, read_dictionary_frame, read_object_frame,
)

PINNED = "d7c71f5032fc2d8844142ad7df7a6f0ee5ac535f"
EXPECTED = {"destination": {"SAME_KEY": 15, "DEST_ONLY": 22},
            "source": {"SAME_KEY": 37, "SOURCE_ONLY": 51}}


def require(condition, message):
    if not condition:
        raise ValueError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def audit(path, side, resaved, dwgread, scratch):
    output = scratch / (path.stem + ".json")
    run = subprocess.run([str(dwgread), "-O", "JSON", "-o", str(output), str(path)],
                         capture_output=True, timeout=120)
    require(run.returncode == 0 and output.is_file(), "independent DWG read failed")
    document = json.loads(output.read_text())
    objects = document["OBJECTS"]
    roots = [obj for obj in objects if obj.get("object") == "DICTIONARY"
             and "ACAD_MLINESTYLE" in obj.get("items", {})]
    require(len(roots) == 1, "expected one root style dictionary")
    root = roots[0]
    root_id = root["handle"][-1]
    root_items = {name: value[-1] for name, value in root["items"].items()}
    expected_root = {"ACAD_GROUP", "ACAD_MLINESTYLE", "AcDbVariableDictionary"}
    if resaved:
        expected_root.add("ACAD_LAYOUT")
    require(set(root_items) == expected_root, "unexpected root dictionary entries")
    style_id = root_items["ACAD_MLINESTYLE"]
    matches = [obj for obj in objects if obj.get("object") == "DICTIONARY"
               and obj.get("handle", [])[-1:] == [style_id]]
    require(len(matches) == 1, "style dictionary is missing or duplicated")
    dictionary = matches[0]
    items = {name: value[-1] for name, value in dictionary["items"].items()}
    require(set(items) == {"Standard", *EXPECTED[side]}, "unexpected style dictionary entries")
    require(dictionary.get("ownerhandle", [])[-1:] == [root_id], "style dictionary owner mismatch")
    root_wire, _ = read_dictionary_frame(path, str(dwgread), root_id, root_items, "ACAD_MLINESTYLE")
    style_wire, _ = read_dictionary_frame(path, str(dwgread), style_id, items, "SAME_KEY")
    for wire, owner in ((root_wire, 0), (style_wire, root_id)):
        require(wire["parent"]["role"] == 4 and wire["parent"]["value"] == owner
                and not wire["reactors"] and not wire["eed"]
                and wire["xdictionary"]["role"] == 3 and wire["xdictionary"]["value"] == 0
                and all(item["handle"]["role"] == 2 and item["handle"]["value"] != 0
                        for item in wire["entries"]), "dictionary physical roles differ")
    styles = {}
    for name, degrees in EXPECTED[side].items():
        handle = items[name]
        matches = [obj for obj in objects if obj.get("object") == "MLINESTYLE"
                   and obj.get("handle", [])[-1:] == [handle]]
        require(len(matches) == 1, "style target missing or duplicated")
        obj = matches[0]
        require(obj.get("name") == name and obj.get("ownerhandle", [])[-1:] == [style_id]
                and math.isclose(obj.get("start_angle", math.nan), math.radians(degrees), abs_tol=1e-10),
                "style name, owner or angle mismatch")
        offset, frame = read_object_frame(path, str(dwgread), handle, 73)
        proof = observe_populated_style_frame(frame, handle, name, style_id, math.radians(degrees), 0)
        bit = proof["spans"]["extension"]["startBit"] + 3
        damaged = bytearray(frame)
        damaged[bit // 8] ^= 1 << (7 - bit % 8)
        for candidate in (bytes(damaged), frame[:0], frame[:len(frame) // 2], frame[:-1]):
            try:
                observe_populated_style_frame(candidate, handle, name, style_id, math.radians(degrees), 0)
            except (ValueError, IndexError, struct.error):
                pass
            else:
                raise ValueError("damaged or truncated style frame was accepted")
        require(observe_populated_style_frame(frame, handle, name, style_id,
                                              math.radians(degrees), 0) == proof,
                "style frame retry changed")
        styles[name] = {"offset": offset, "handle": handle, "wire": proof,
                        "wrongRoleAndTruncationsRejectedAndRetryPassed": True}
    lines = [obj for obj in objects if obj.get("entity") == "LINE"]
    require(len(lines) == 1 and lines[0].get("start") == [1.0, 2.0, 0.0]
            and lines[0].get("end") == [4.0, 5.0, 0.0], "selected LINE geometry differs")
    return {"sha256": sha(path), "root": root_wire, "styleDictionary": style_wire,
            "styles": styles, "line": {"start": lines[0]["start"], "end": lines[0]["end"]}}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--dwgread", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists():
        raise ValueError("audit output already exists")
    manifest_path = args.manifest.resolve(strict=True)
    manifest = json.loads(manifest_path.read_text())
    require(manifest.get("schema") == "libdxfrw.ac1012-acadrust-pair.v1"
            and manifest.get("ok") is True and manifest.get("producerCommit") == PINNED,
            "manifest is not the pinned witness profile")
    dwgread = args.dwgread.resolve(strict=True)
    with tempfile.TemporaryDirectory(prefix="ac1012-acadrust-audit-") as directory:
        rows = {}
        for side in EXPECTED:
            for resaved in (False, True):
                name = side + ("-resaved" if resaved else "") + ".dwg"
                path = manifest_path.parent / name
                require(path.is_file() and path.read_bytes()[:6] == b"AC1012"
                        and sha(path) == manifest["samples"][name]["sha256"], "sample hash/version mismatch")
                rows[name] = audit(path, side, resaved, dwgread, Path(directory))
        for side in EXPECTED:
            original, resaved = rows[side + ".dwg"], rows[side + "-resaved.dwg"]
            require(original["styleDictionary"]["frameSha256"] == resaved["styleDictionary"]["frameSha256"]
                    and all(original["styles"][name]["wire"]["frameSha256"] ==
                            resaved["styles"][name]["wire"]["frameSha256"] for name in EXPECTED[side]),
                    "selected source/resave frames changed")
    report = {"schema": "libdxfrw.ac1012-acadrust-audit.v1", "ok": True,
              "manifestSha256": sha(manifest_path), "dwgreadSha256": sha(dwgread),
              "producerCommit": PINNED, "samples": rows, "supportClaimEffect": "none"}
    with args.output.open("x", encoding="utf-8") as stream:
        stream.write(json.dumps(report, indent=2) + "\n")
    print(json.dumps({"ok": True, "output": str(args.output)}))


if __name__ == "__main__":
    main()
