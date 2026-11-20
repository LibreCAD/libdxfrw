#!/usr/bin/env python3
"""Report VIEWPORT shade-plot wire fields in ASCII/Binary DXF.

This is a bounded structural observer for P5-H, not a DXF validator or a
semantic/support verdict. It intentionally does not map API enum values to
DXF group 170. Compare its per-file handles and values with native reports.
The supported input profile is UTF-8 AC1021 or newer; this is not a complete
DXF grammar or graph validator. Resolve ambiguity using native evidence.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
from pathlib import Path
from typing import Any

from audit_binary_dxf_layout_nod import SENTINEL, read_pairs, read_records
from audit_p5d_dxf_versions import header_version


def ascii_pairs(data: bytes) -> tuple[tuple[int, Any], ...]:
    lines = data.splitlines()
    if len(lines) % 2:
        raise ValueError("truncated ASCII group/value pair")
    pairs = []
    for i in range(0, len(lines), 2):
        try:
            code = int(lines[i].strip())
            value = lines[i + 1].decode("utf-8").strip()
            if code in (91, 170):
                value = int(value)
        except (ValueError, UnicodeDecodeError) as exc:
            raise ValueError("invalid ASCII group or UTF-8 value") from exc
        pairs.append((code, value))
    return tuple(pairs)


def handle(value: Any) -> str:
    if not isinstance(value, str):
        raise ValueError("invalid handle")
    result = value.strip().upper()
    if not result or any(c not in "0123456789ABCDEF" for c in result):
        raise ValueError("invalid handle")
    return result.lstrip("0") or "0"


def outside_appdata(values: tuple[tuple[int, Any], ...]) -> list[tuple[int, Any]]:
    depth = 0
    result = []
    for code, value in values:
        if code == 102:
            text = str(value)
            if text.startswith("{"):
                depth += 1
            elif text == "}":
                depth -= 1
                if depth < 0:
                    raise ValueError("unbalanced application data")
            continue
        if depth == 0:
            result.append((code, value))
    if depth:
        raise ValueError("unbalanced application data")
    return result


def audit_bytes(data: bytes, name: str) -> dict[str, Any]:
    binary = data.startswith(SENTINEL)
    pairs = read_pairs(data) if binary else ascii_pairs(data)
    if not pairs or pairs[-1] != (0, "EOF"):
        raise ValueError("DXF stream must end at EOF")
    version = header_version(iter(pairs))
    if (not isinstance(version, str) or len(version) != 6
            or not version.startswith("AC") or not version[2:].isdigit()
            or int(version[2:]) < 1021):
        raise ValueError("expected UTF-8 DXF version AC1021 or newer")
    records = read_records(pairs)
    targets: dict[str, list[Any]] = {}
    for record in records:
        if record.section is None:
            continue
        identities = [handle(v) for c, v in outside_appdata(record.values) if c == 5]
        if len(identities) > 1:
            raise ValueError("record has ambiguous group-5 identity")
        if identities and identities[0] != "0":
            targets.setdefault(identities[0], []).append(record)

    viewports = []
    for record in records:
        if record.type != "VIEWPORT" or record.section not in ("ENTITIES", "BLOCKS"):
            continue
        values = outside_appdata(record.values)
        markers = [i for i, v in enumerate(values) if v == (100, "AcDbViewport")]
        if len(markers) != 1:
            raise ValueError("VIEWPORT must contain exactly one AcDbViewport marker")
        start = markers[0] + 1
        end = next((i for i in range(start, len(values)) if values[i][0] == 100), len(values))
        body = values[start:end]
        modes = [int(v) for c, v in body if c == 170]
        shade_ids = [handle(v) for c, v in body if c == 333]
        visual_ids = [handle(v) for c, v in body if c == 348]
        owner_ids = [handle(v) for c, v in values[:markers[0]] if c == 330]
        identities = [handle(v) for c, v in values if c == 5]
        if len(identities) != 1 or identities[0] == "0":
            raise ValueError("VIEWPORT lacks a unique nonzero handle")
        resolutions = []
        for ref in shade_ids:
            found = targets.get(ref, []) if ref != "0" else []
            resolutions.append({
                "handle": ref,
                "state": "null" if ref == "0" else "missing" if not found
                    else "unique" if len(found) == 1 else "ambiguous",
                "records": [{"section": x.section, "type": x.type,
                             "subclasses": [str(v) for c, v in x.values if c == 100]}
                            for x in found],
            })
        viewports.append({
            "section": record.section,
            "handle": identities[0],
            "owner330": owner_ids,
            "layerOverride91": [int(v) for c, v in body if c == 91],
            "shadePlot170": modes,
            "shadePlot333": shade_ids,
            "visualStyle348": visual_ids,
            "shadeTargets": resolutions,
        })
    if not viewports:
        raise ValueError("no VIEWPORT entity found")
    return {"file": name, "sha256": hashlib.sha256(data).hexdigest(),
            "encoding": "binary" if binary else "ascii", "acadver": version,
            "viewports": viewports}


def self_test() -> None:
    # Locally authored in-memory data only: no drawing fixture is created.
    common = [(0,"SECTION"),(2,"HEADER"),(9,"$ACADVER"),(1,"AC1024"),(0,"ENDSEC"),
              (0,"SECTION"),(2,"ENTITIES"),(0,"VIEWPORT"),(5,"A"),(330,"1F"),
              (100,"AcDbEntity"),(100,"AcDbViewport"),(91,27),(170,4),(333,"B"),(348,"C"),
              (0,"ENDSEC"),(0,"SECTION"),(2,"OBJECTS"),(0,"VISUALSTYLE"),(5,"B"),
              (100,"AcDbVisualStyle"),(0,"ENDSEC"),(0,"EOF")]
    def ascii_data(pairs):
        return "".join(f"{c}\n{v}\n" for c,v in pairs).encode()
    def binary_data(pairs):
        data = bytearray(SENTINEL)
        for code,value in pairs:
            data.extend(struct.pack("<H",code))
            if code == 91:
                data.extend(struct.pack("<i",value))
            elif code == 170:
                data.extend(struct.pack("<h",value))
            else:
                data.extend(str(value).encode()+b"\0")
        return bytes(data)
    for encode in (ascii_data,binary_data):
        r=audit_bytes(encode(common),"synthetic")
        v=r["viewports"][0]
        assert r["acadver"]=="AC1024" and v["shadePlot170"]==[4]
        assert v["layerOverride91"] == [27] and v["visualStyle348"] == ["C"]
        assert v["shadePlot333"]==["B"] and v["shadeTargets"][0]["state"]=="unique"
        assert v["shadeTargets"][0]["records"][0]["type"]=="VISUALSTYLE"
        double=common.copy();double.insert(15,(333,"C"))
        assert len(audit_bytes(encode(double),"synthetic")["viewports"][0]["shadePlot333"])==2
        duplicate=common.copy();duplicate[-2:-2]=[(0,"VISUALSTYLE"),(5,"B"),(100,"AcDbVisualStyle")]
        assert audit_bytes(encode(duplicate),"synthetic")["viewports"][0]["shadeTargets"][0]["state"]=="ambiguous"
        missing=[(c,"D" if c==333 else v) for c,v in common]
        assert audit_bytes(encode(missing),"synthetic")["viewports"][0]["shadeTargets"][0]["state"]=="missing"
        null=[(c,"0" if c==333 else v) for c,v in common]
        assert audit_bytes(encode(null),"synthetic")["viewports"][0]["shadeTargets"][0]["state"]=="null"
        doubleMode=common.copy();doubleMode.insert(14,(170,5))
        assert audit_bytes(encode(doubleMode),"synthetic")["viewports"][0]["shadePlot170"]==[4,5]
        appdata = common.copy()
        appdata[14:14] = [(102, "{APP"), (333, "FFFF"), (102, "}")]
        assert audit_bytes(encode(appdata), "synthetic")["viewports"][0]["shadePlot333"] == ["B"]
        invalid = [common[:-1],
                   [(c, "" if c == 333 else v) for c, v in common],
                   [(c, "AC1018" if v == "AC1024" else v) for c, v in common]]
        duplicate_identity = common.copy()
        duplicate_identity.insert(10, (5, "A"))
        invalid.append(duplicate_identity)
        unbalanced = common.copy()
        unbalanced.insert(10, (102, "{BAD"))
        invalid.append(unbalanced)
        for candidate in invalid:
            try:
                audit_bytes(encode(candidate), "synthetic")
            except ValueError:
                pass
            else:
                raise AssertionError("malformed input accepted")
    print("P5-H wire observer self-test: PASS")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("files", nargs="*", type=Path)
    args = parser.parse_args()
    if args.self_test:
        if args.files:
            parser.error("--self-test cannot be combined with files")
        self_test()
        return
    if not args.files:
        parser.error("provide one or more DXF files")
    try:
        results = [audit_bytes(path.read_bytes(), path.name) for path in args.files]
    except (OSError, ValueError, StopIteration, struct.error) as exc:
        parser.exit(1, f"P5-H wire observation failed: {exc}\n")
    print(json.dumps({"result":"OBSERVED_WIRE_NOT_QUALIFIED", "files":results}, indent=2))


if __name__ == "__main__":
    main()
