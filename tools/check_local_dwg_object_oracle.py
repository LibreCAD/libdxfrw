#!/usr/bin/env python3
"""Check locally generated DWG OBJECTS through an independent JSON oracle.

The checker deliberately keeps this lane advisory: it validates the object
carriers that the local writer owns, but it never changes the format-support
ledger or stores the generated drawings.  The writer and oracle are invoked
with argument lists (no shell) and every subprocess is timeout-bounded.
"""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
import tempfile
from pathlib import Path


VERSIONS = {
    13: "AC1015",
    14: "AC1018",
    15: "AC1021",
    16: "AC1024",
    17: "AC1027",
    18: "AC1032",
}

GROUP_HANDLE = 0xA600
DICTIONARY_HANDLE = 0xA601
XRECORD_HANDLE = 0xA602
PLOTSETTINGS_HANDLE = 0xA603
LAYOUT_HANDLE = 0xA700
MALFORMED_HANDLE = 0xA701


def parse_json_output(text: str) -> dict:
    start = text.find("{")
    if start < 0:
        raise ValueError("oracle output has no JSON object")
    payload = json.loads(text[start:])
    if not isinstance(payload, dict):
        raise ValueError("oracle JSON root is not an object")
    return payload


def record_handle(record: dict) -> int | None:
    value = record.get("handle")
    if not isinstance(value, list) or len(value) < 3:
        return None
    try:
        return int(value[2])
    except (TypeError, ValueError):
        return None


def owner_handle(record: dict) -> int | None:
    value = record.get("ownerhandle")
    if not isinstance(value, list) or len(value) < 3:
        return None
    try:
        return int(value[2])
    except (TypeError, ValueError):
        return None


def find_record(records: list[dict], object_name: str, handle: int) -> dict:
    matches = [
        record for record in records
        if isinstance(record, dict)
        and record.get("object") == object_name
        and record_handle(record) == handle
    ]
    if len(matches) != 1:
        raise ValueError(
            f"expected one {object_name} frame at handle 0x{handle:X}, "
            f"found {len(matches)}"
        )
    return matches[0]


def check_objects(payload: dict, version_name: str) -> dict:
    header = payload.get("FILEHEADER")
    if not isinstance(header, dict) or header.get("version") != version_name:
        raise ValueError(f"FILEHEADER version is not {version_name}")
    records = payload.get("OBJECTS")
    if not isinstance(records, list):
        raise ValueError("oracle JSON has no OBJECTS list")

    group = find_record(records, "GROUP", GROUP_HANDLE)
    if (group.get("name") != "LOCAL_GROUP"
            or not isinstance(group.get("groups"), list)
            or len(group["groups"]) != 1
            or owner_handle(group) != 0x0C):
        raise ValueError("GROUP name, owner, or member stream mismatch")

    dictionary = find_record(records, "DICTIONARY", DICTIONARY_HANDLE)
    if (dictionary.get("numitems") != 3
            or dictionary.get("is_hardowner") != 1
            or owner_handle(dictionary) != 0x0C):
        raise ValueError("custom DICTIONARY count, owner, or cloning mismatch")

    xrecord = find_record(records, "XRECORD", XRECORD_HANDLE)
    xdata = xrecord.get("xdata")
    if (owner_handle(xrecord) != DICTIONARY_HANDLE
            or not isinstance(xdata, list)
            or [40, 1.25] not in xdata
            or [1, "LOCAL_XRECORD"] not in xdata
            or [310, "010203"] not in xdata):
        raise ValueError("XRECORD owner or bounded payload mismatch")

    plot = find_record(records, "PLOTSETTINGS", PLOTSETTINGS_HANDLE)
    if (owner_handle(plot) != DICTIONARY_HANDLE
            or plot.get("printer_cfg_file") != "LOCAL_PAGE"
            or plot.get("paper_size") != "LOCAL_PRINTER"
            or plot.get("canonical_media_name") != "A4"):
        raise ValueError("PLOTSETTINGS owner or bounded field mismatch")

    layout = find_record(records, "LAYOUT", LAYOUT_HANDLE)
    if (owner_handle(layout) != DICTIONARY_HANDLE
            or layout.get("layout_name") != "LOCAL_LAYOUT"):
        raise ValueError("LAYOUT owner or name mismatch")

    if any(record_handle(record) == MALFORMED_HANDLE for record in records):
        raise ValueError("rolled-back malformed object was published")

    return {
        "version": version_name,
        "objectHandles": {
            "GROUP": GROUP_HANDLE,
            "DICTIONARY": DICTIONARY_HANDLE,
            "XRECORD": XRECORD_HANDLE,
            "PLOTSETTINGS": PLOTSETTINGS_HANDLE,
            "LAYOUT": LAYOUT_HANDLE,
        },
        "objectStatus": "qualified",
    }


def run(writer: str, oracle: str, timeout: float) -> dict:
    records = []
    with tempfile.TemporaryDirectory(prefix="libdxfrw-object-oracle-") as directory:
        output_dir = Path(directory)
        try:
            generated = subprocess.run(
                [writer, "--keep-dir", str(output_dir)],
                capture_output=True,
                text=True,
                timeout=timeout,
                check=False,
            )
        except subprocess.TimeoutExpired:
            return {"status": "timeout", "stage": "writer"}
        if generated.returncode != 0:
            return {
                "status": "writer-failed",
                "returncode": generated.returncode,
            }

        for code, version_name in VERSIONS.items():
            path = output_dir / f"libdxfrw-local-roundtrip-{code}.dwg"
            if not path.exists():
                records.append({"version": version_name, "status": "missing-output"})
                continue
            try:
                result = subprocess.run(
                    [oracle, "-O", "JSON", str(path)],
                    capture_output=True,
                    text=True,
                    timeout=timeout,
                    check=False,
                )
            except subprocess.TimeoutExpired:
                records.append({"version": version_name, "status": "timeout"})
                continue
            if result.returncode != 0:
                records.append({
                    "version": version_name,
                    "status": "oracle-failed",
                    "returncode": result.returncode,
                })
                continue
            try:
                summary = check_objects(parse_json_output(result.stdout), version_name)
                records.append(summary | {"status": "qualified"})
            except (ValueError, json.JSONDecodeError) as exc:
                records.append({
                    "version": version_name,
                    "status": "mismatch",
                    "reason": str(exc),
                })

    status = "complete" if records and all(
        record.get("status") == "qualified" for record in records
    ) else "incomplete"
    return {"status": status, "records": records, "writer": writer, "oracle": oracle}


def self_test() -> None:
    payload = {
        "FILEHEADER": {"version": "AC1024"},
        "OBJECTS": [
            {"object": "GROUP", "handle": [0, 1, GROUP_HANDLE],
             "ownerhandle": [4, 1, 0x0C, 0x0C], "name": "LOCAL_GROUP",
             "groups": [[5, 1, 0x1234]]},
            {"object": "DICTIONARY", "handle": [0, 1, DICTIONARY_HANDLE],
             "ownerhandle": [4, 1, 0x0C, 0x0C], "numitems": 3,
             "is_hardowner": 1},
            {"object": "XRECORD", "handle": [0, 1, XRECORD_HANDLE],
             "ownerhandle": [4, 1, DICTIONARY_HANDLE, DICTIONARY_HANDLE],
             "xdata": [[40, 1.25], [1, "LOCAL_XRECORD"], [310, "010203"]]},
            {"object": "PLOTSETTINGS", "handle": [0, 1, PLOTSETTINGS_HANDLE],
             "ownerhandle": [4, 1, DICTIONARY_HANDLE, DICTIONARY_HANDLE],
             "printer_cfg_file": "LOCAL_PAGE", "paper_size": "LOCAL_PRINTER",
             "canonical_media_name": "A4"},
            {"object": "LAYOUT", "handle": [0, 1, LAYOUT_HANDLE],
             "ownerhandle": [4, 1, DICTIONARY_HANDLE, DICTIONARY_HANDLE],
             "layout_name": "LOCAL_LAYOUT"},
        ],
    }
    check_objects(payload, "AC1024")
    try:
        bad = json.loads(json.dumps(payload))
        bad["OBJECTS"].append({"object": "XRECORD", "handle": [0, 1, MALFORMED_HANDLE]})
        check_objects(bad, "AC1024")
    except ValueError:
        pass
    else:
        raise AssertionError("malformed object was not rejected")
    print("local DWG object oracle: PASS")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--writer")
    parser.add_argument("--oracle")
    parser.add_argument("--timeout", type=float, default=10.0)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args(argv)
    if args.self_test:
        self_test()
        return 0
    if not args.writer or not args.oracle:
        parser.error("--writer and --oracle are required unless --self-test is used")
    print(json.dumps(run(args.writer, args.oracle, args.timeout), indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    sys.exit(main())
