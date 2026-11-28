#!/usr/bin/env python3
"""Audit the bounded ODA-generated AC1012 merge witness pair.

Uses LibreDWG JSON plus the shared physical frame and DICTIONARY walkers.
This is source-profile evidence, not AutoCAD acceptance or merge semantics.
"""
from __future__ import annotations

import argparse
import json
import math
from pathlib import Path
import shutil
import tempfile

from generate_ac1012_dictionary_merge_samples import read_dictionary_frame, read_object_frame
from generate_ac1012_dictionary_witness import Bits, executable_identity, require, run, sha


PROFILES = {
    "destination": {"SAME_KEY": 15, "DEST_ONLY": 22},
    "source": {"SAME_KEY": 37, "SOURCE_ONLY": 51},
}


def observe_xrecord(path: Path, dwgread: str, record_id: int, owner_id: int) -> dict:
    """Walk this profile's R13 XRECORD framing, raw data and handle roles."""
    address, frame = read_object_frame(path, dwgread, record_id, 511)
    bits = Bits(frame)
    require(bits.bs() == 511 and bits.handle(0, own=True)["value"] == record_id,
            "XRECORD type or self handle mismatch")
    require(bits.bs() == 0, "unexpected XRECORD EED")
    body_end = bits.little(4)
    require(bits.bl() == 1, "XRECORD reactor count mismatch")
    count = bits.bl()
    require(count == 39, "XRECORD data size mismatch")
    data_begin = bits.position
    payload = bytes(bits.take(8) for _ in range(count))
    require(bits.position == body_end, "XRECORD body boundary mismatch")
    expected = bytes.fromhex("66000800004558544e414d455301000800005354414e44415244"
                             "02000800005374616e64617264")
    require(payload == expected, "XRECORD selected data bytes mismatch")
    owner = bits.handle(4, record_id)
    reactor = bits.handle(4, record_id)
    extension = bits.handle(3, record_id)
    require(owner["value"] == owner_id and reactor["role"] == 4
            and reactor["value"] == owner_id
            and extension["role"] == 3 and extension["value"] == 0
            and len(frame) * 8 - bits.position < 8,
            "XRECORD common handle tail mismatch")
    return {"offset": address, "sizeBytes": len(frame), "frameSha256": sha(frame),
            "crcValid": True, "bodyEndBit": body_end,
            "dataBeginBit": data_begin, "dataBytes": count, "dataHex": payload.hex(),
            "owner": owner, "reactor": reactor, "xdictionary": extension,
            "paddingBits": len(frame) * 8 - bits.position}


def audit(path: Path, side: str, dwgread: str, temp: Path,
          expect_extension: bool, expect_root_null: bool) -> dict:
    require(path.read_bytes().startswith(b"AC1012"), "source is not AC1012")
    decoded = temp / f"{side}.json"
    run([dwgread, "-O", "JSON", "-o", str(decoded), str(path)])
    objects = json.loads(decoded.read_text())["OBJECTS"]

    def object_at(handle: int, kind: str) -> dict:
        matches = [obj for obj in objects
                   if obj.get("object") == kind and obj.get("handle", [None])[-1] == handle]
        require(len(matches) == 1, f"{kind} handle {handle:X} is missing or duplicated")
        return matches[0]

    roots = [obj for obj in objects if obj.get("object") == "DICTIONARY"
             and "ACAD_MLINESTYLE" in obj.get("items", {})]
    require(len(roots) == 1, "ACAD_MLINESTYLE root is missing or duplicated")
    root = roots[0]
    root_id = root["handle"][-1]
    root_frame = None
    if expect_root_null:
        root_entries = {name: ref[-1] for name, ref in root["items"].items()}
        root_frame, _ = read_dictionary_frame(path, dwgread, root_id, root_entries,
                                              "ACAD_MLINESTYLE", True)
        null_entries = [entry for entry in root_frame["entries"] if entry["name"] == ""]
        require(root_frame["parent"]["role"] == 4
                and root_frame["parent"]["value"] == 0 and not root_frame["reactors"]
                and root_frame["xdictionary"]["role"] == 3
                and root_frame["xdictionary"]["value"] == 0
                and len(null_entries) == 1 and null_entries[0]["handle"]["role"] == 2
                and null_entries[0]["handle"]["value"] == 0,
                "root dictionary common links or selected null entry mismatch")
    root_entry = root["items"]["ACAD_MLINESTYLE"]
    require(root_entry[0] == 2, "style dictionary root entry is not a soft owner")
    dictionary_id = root_entry[-1]
    dictionary = object_at(dictionary_id, "DICTIONARY")
    require(dictionary.get("ownerhandle", [None])[-1] == root_id,
            "style dictionary owner mismatch")
    expected = set(PROFILES[side]) | {"STANDARD"}
    raw_entries = dictionary.get("items", {})
    require(set(raw_entries) == expected and dictionary.get("numitems") == len(expected),
            "style dictionary entries differ from the selected profile")
    require(all(item[0] == 2 and item[-1] for item in raw_entries.values()),
            "style dictionary entry role or target mismatch")
    entries = {name: item[-1] for name, item in raw_entries.items()}
    frame, _ = read_dictionary_frame(path, dwgread, dictionary_id, entries)
    require(frame["parent"]["value"] == root_id
            and [r["value"] for r in frame["reactors"]] == [root_id],
            "physical owner or reactor mismatch")
    require(frame["cloningField"] is None and len(frame["eed"]) == 1,
            "unexpected cloning field or EED count")
    eed = frame["eed"][0]
    appid = object_at(eed["appid"]["value"], "APPID")
    require(eed["appid"]["role"] == 5 and appid.get("name") == "ACAD"
            and eed["hex"] == "000d000054524541545f41535f48415244460000",
            "ACAD EED source profile mismatch")
    if root_frame is not None:
        require(root_frame["eed"] == frame["eed"], "root ACAD EED mismatch")

    extension_id = frame["xdictionary"]["value"]
    require(frame["xdictionary"]["role"] == 3
            and dictionary.get("xdicobjhandle", [0])[-1] == extension_id,
            "extension dictionary source role mismatch")
    extension_frame = xrecord_link = xrecord_frame = xrecord = None
    if expect_extension:
        require(extension_id != 0, "expected a non-null extension dictionary")
        extension = object_at(extension_id, "DICTIONARY")
        require(extension.get("ownerhandle", [None])[-1] == dictionary_id,
                "extension dictionary owner mismatch")
        xrecord_link = extension.get("items", {}).get("ACAD_XREC_ROUNDTRIP")
        require(extension.get("numitems") == 1 and xrecord_link
                and xrecord_link[0] == 2 and xrecord_link[-1] != 0,
                "extension dictionary XRECORD reference mismatch")
        extension_frame, _ = read_dictionary_frame(
            path, dwgread, extension_id, {"ACAD_XREC_ROUNDTRIP": xrecord_link[-1]},
            "ACAD_XREC_ROUNDTRIP")
        require(extension_frame["parent"]["value"] == dictionary_id
                and not extension_frame["reactors"] and not extension_frame["eed"]
                and extension_frame["xdictionary"]["role"] == 3
                and extension_frame["xdictionary"]["value"] == 0,
                "extension dictionary physical common links mismatch")
        xrecord = object_at(xrecord_link[-1], "XRECORD")
        require(xrecord.get("ownerhandle", [None])[-1] == extension_id
                and [r[-1] for r in xrecord.get("reactors", [])] == [extension_id]
                and xrecord.get("xdata") == [[102, "EXTNAMES"], [1, "STANDARD"], [2, "Standard"]],
                "extension XRECORD ownership or selected payload mismatch")
        xrecord_frame = observe_xrecord(path, dwgread, xrecord_link[-1], extension_id)
    else:
        require(extension_id == 0, "expected a null extension dictionary")

    for name, angle in PROFILES[side].items():
        style = object_at(entries[name], "MLINESTYLE")
        require(style.get("name") == name and style.get("ownerhandle", [None])[-1] == dictionary_id
                and math.isclose(style.get("start_angle", math.nan), math.radians(angle), rel_tol=0, abs_tol=1e-12)
                and math.isclose(style.get("end_angle", math.nan), math.pi / 2, rel_tol=0, abs_tol=1e-12),
                f"{name} identity, owner or angle mismatch")
        lines = style.get("lines", [])
        require(len(lines) == 2 and [line.get("offset") for line in lines] == [0.5, -0.5]
                and all(line.get("lt_index") == 32767 for line in lines),
                f"{name} two-element style mismatch")
    return {"source": str(path), "sha256": sha(path.read_bytes()),
            "rootHandle": root_id, "dictionaryHandle": dictionary_id,
            "rootExtensionDictionaryHandle": root.get("xdicobjhandle", [0])[-1],
            "rootFrame": root_frame,
            "frame": frame, "appidName": appid["name"],
            "extensionDictionaryHandle": extension_id,
            "extensionDictionaryFrame": extension_frame,
            "extensionXrecordHandle": xrecord_link[-1] if xrecord_link else None,
            "extensionXrecordFrame": xrecord_frame,
            "extensionXrecordData": xrecord["xdata"] if xrecord else None, "stylesDegrees": PROFILES[side]}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("destination", type=Path)
    parser.add_argument("source", type=Path)
    parser.add_argument("--dwgread", default="dwgread")
    parser.add_argument("--expect-style-extension", choices=("present", "absent"), default="present")
    parser.add_argument("--expect-root-null-extension", action="store_true")
    args = parser.parse_args()
    dwgread = shutil.which(args.dwgread)
    require(dwgread is not None, "dwgread executable not found")
    with tempfile.TemporaryDirectory(prefix="ac1012-oda-pair-audit-") as directory:
        temp = Path(directory)
        result = {side: audit(getattr(args, side), side, dwgread, temp,
                              args.expect_style_extension == "present",
                              args.expect_root_null_extension) for side in PROFILES}
    print(json.dumps({"dwgread": executable_identity(dwgread), "samples": result,
                      "scope": "selected physical dictionary/XRECORD frames and model graph only; no native semantics"},
                     indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
