#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Bounded, hash-pinned ASCII TABLE Value shape inventory, not support proof.

Only schema metadata and group-code sequences are reported. Drawing text,
coordinates, handles and binary payload bytes are never emitted. This tool does
not use libdxfrw or infer DXF payload fields from the native DWG Value layout.
"""

import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import re
import sys


MAX_BYTES = 16 * 1024 * 1024
MAX_RECORDS = 1_000_000


def inventory(data, expected_sha256, expected_version, *,
              max_bytes=MAX_BYTES, max_records=MAX_RECORDS):
    if len(data) > max_bytes:
        raise ValueError("source byte limit exceeded")
    digest = hashlib.sha256(data).hexdigest()
    if not re.fullmatch(r"[0-9a-fA-F]{64}", expected_sha256):
        raise ValueError("expected SHA256 must contain exactly64 hex digits")
    if digest != expected_sha256.lower():
        raise ValueError("source SHA256 differs")
    if data.startswith(b"AutoCAD Binary DXF"):
        raise ValueError("ASCII inventory only; binary DXF needs a separate reader")
    # Split only DXF line separators, not arbitrary Unicode/control characters.
    lines = data.split(b"\n")
    if lines and not lines[-1]:
        lines.pop()
    lines = [line[:-1] if line.endswith(b"\r") else line for line in lines]
    if len(lines) % 2:
        raise ValueError("unpaired DXF code/value line")
    if len(lines) // 2 > max_records:
        raise ValueError("source record limit exceeded")

    section = b""
    record = b""
    subclass = b""
    variable = b""
    want_section = False
    version = None
    value = None
    forms = Counter()
    table_records = 0
    selected_records = set()
    record_index = 0
    eof = False
    application_depth = 0
    out_of_scope = 0
    metadata = {93: "flags", 90: "dataType", 94: "unitType", 92: "dataSize"}

    for offset in range(0, len(lines), 2):
        code_line, text = lines[offset:offset + 2]
        if not re.fullmatch(rb"[ \t]*[+]?[0-9]+[ \t]*", code_line):
            raise ValueError("invalid ASCII group code")
        code = int(code_line)
        if code > 1071:
            raise ValueError("group code outside bounded DXF inventory")
        if eof:
            raise ValueError("records follow EOF")
        if code == 0:
            if application_depth:
                raise ValueError("physical record boundary inside application block")
            if value is not None:
                raise ValueError("physical record boundary inside unclosed Value")
            record_index += 1
            record, subclass, variable = text, b"", b""
            want_section = text == b"SECTION"
            if text == b"ENDSEC":
                section = b""
            if text == b"EOF":
                eof = True
            if text == b"ACAD_TABLE":
                table_records += 1
        elif want_section and code == 2:
            section, want_section = text, False
        # Other object schemas can use102 as ordinary payload. Only the
        # selected TABLE grammar gives it this application-delimiter role.
        if code == 102 and record == b"ACAD_TABLE":
            if value is not None:
                raise ValueError("application block inside Value")
            if text == b"}":
                if not application_depth:
                    raise ValueError("unmatched application close")
                application_depth -= 1
            elif len(text) > 1 and text.startswith(b"{"):
                application_depth += 1
                if application_depth > 32:
                    raise ValueError("application nesting limit exceeded")
            else:
                raise ValueError("noncanonical application delimiter")
            continue
        if application_depth:
            continue
        if section == b"HEADER":
            if code == 9:
                variable = text
            elif code == 1 and variable == b"$ACADVER":
                if version is not None:
                    raise ValueError("duplicate declared revision")
                version = text.decode("ascii")
                variable = b""
        if code == 100 and value is None:
            subclass = text

        if code == 301 and text == b"CELL_VALUE":
            if value is not None:
                raise ValueError("nested Value envelope")
            if record != b"ACAD_TABLE":
                out_of_scope += 1
                continue
            if subclass != b"AcDbTable":
                raise ValueError("CELL_VALUE outside selected TABLE subclass")
            value = {"mode": None, "flags": None, "dataType": None,
                     "unitType": None, "dataSize": None, "binaryBytes": 0,
                     "codes": []}
            selected_records.add(record_index)
        elif value is not None:
            value["codes"].append(code)
            if value["mode"] is None:
                value["mode"] = "structured" if code == 93 else "legacy"
            if code in metadata:
                key = metadata[code]
                if value[key] is not None:
                    raise ValueError("duplicate scoped Value metadata")
                if not re.fullmatch(rb"[ \t]*[+-]?[0-9]+[ \t]*", text):
                    raise ValueError("noninteger scoped Value metadata")
                number = int(text)
                if not -(2 ** 31) <= number < 2 ** 31:
                    raise ValueError("scoped Value metadata outside int32")
                value[key] = number
            if code == 310:
                if len(text) > 254 or len(text) % 2 or not re.fullmatch(rb"[0-9a-fA-F]*", text):
                    raise ValueError("invalid bounded Value binary chunk")
                value["binaryBytes"] += len(text) // 2
            if code == 304:
                if text != b"ACVALUE_END":
                    raise ValueError("noncanonical Value terminator")
                forms[json.dumps(value, sort_keys=True)] += 1
                value = None
    if value is not None or application_depth or not eof:
        raise ValueError("incomplete Value or missing EOF")
    if version != expected_version:
        raise ValueError("declared revision differs")
    return {
        "schema": "libdxfrw.table-value-source-inventory.v1",
        "evidence": "STRUCTURAL_INVENTORY_ONLY",
        "sha256": digest, "bytes": len(data), "declaredVersion": version,
        "records": len(lines) // 2, "tableRecords": table_records,
        "recordsWithValues": len(selected_records),
        "outOfScopeMarkers": out_of_scope,
        "valueEnvelopes": sum(forms.values()),
        "forms": [{"shape": json.loads(key), "count": count}
                  for key, count in sorted(forms.items())],
        "payloadsEmitted": False, "supportClaimPromotion": False,
    }


def self_test():
    def document(body, newline=b"\n"):
        pairs = [(0, b"SECTION"), (2, b"HEADER"), (9, b"$ACADVER"),
                 (1, b"AC1032"), (0, b"ENDSEC"), (0, b"SECTION"),
                 (2, b"ENTITIES"), (0, b"ACAD_TABLE"),
                 (100, b"AcDbTable"), (301, b"CELL_VALUE")]
        pairs += body + [(0, b"ENDSEC"), (0, b"EOF")]
        return newline.join(part for code, text in pairs
                            for part in (str(code).encode("ascii"), text)) + newline

    body = [(93, b"6"), (90, b"32"), (11, b"1.25"), (21, b"-2.5"),
            (31, b"3.75"), (94, b"0"), (300, b""), (302, b"cache"),
            (304, b"ACVALUE_END")]
    positive = 0
    for newline in (b"\n", b"\r\n"):
        for trailing in (True, False):
            data = document(body, newline)
            if not trailing:
                data = data[:-len(newline)]
            result = inventory(data, hashlib.sha256(data).hexdigest(), "AC1032")
            shape = result["forms"][0]["shape"]
            if (result["valueEnvelopes"] != 1 or shape["dataSize"] is not None
                    or shape["flags"] != 6 or shape["dataType"] != 32
                    or result["payloadsEmitted"] or result["supportClaimPromotion"]):
                raise ValueError("inventory positive control differs")
            positive += 1
    for extra in (
        [(93, b"6"), (90, b"8"), (92, b"16"),
         (310, b"00112233445566778899AABBCCDDEEFF"), (94, b"0"),
         (300, b""), (302, b"date cache"), (304, b"ACVALUE_END")],
        [(93, b"7"), (90, b"0"), (94, b"0"), (300, b""),
         (302, b""), (304, b"ACVALUE_END")],
    ):
        data = document(extra)
        result = inventory(data, hashlib.sha256(data).hexdigest(), "AC1032")
        shape = result["forms"][0]["shape"]
        if shape["binaryBytes"] != (16 if shape["dataType"] == 8 else 0):
            raise ValueError("inventory date/null control differs")
        positive += 1
    good = document(body)
    opaque = good.replace(b"301\nCELL_VALUE\n", b"102\n{APP\n301\nCELL_VALUE\n"
                          b"93\n99\n100\nOpaqueSubclass\n102\n}\n301\nCELL_VALUE\n", 1)
    outside = good.replace(b"0\nACAD_TABLE\n", b"0\nXRECORD\n", 1)
    outside_102 = outside.replace(b"100\nAcDbTable\n", b"102\nOBJECT_DATA\n100\nAcDbTable\n", 1)
    for data, expected in ((opaque, 1), (outside, 0), (outside_102, 0)):
        result = inventory(data, hashlib.sha256(data).hexdigest(), "AC1032")
        if result["valueEnvelopes"] != expected or result["outOfScopeMarkers"] != 1 - expected:
            raise ValueError("inventory scope isolation differs")
        positive += 1
    cases = [(good, "0" * 64, "AC1032", {}),
             (good, None, "AC1027", {}),
             (good, None, "AC1032", {"max_bytes": len(good) - 1}),
             (good, None, "AC1032", {"max_records": 1}),
             (b"AutoCAD Binary DXF\r\n\x1a\0", None, "AC1032", {}),
             (good + b"0\n", None, "AC1032", {}),
             (good.replace(b"100\n", b"bad\n", 1), None, "AC1032", {}),
             (document(body[:-1]), None, "AC1032", {}),
             (document(body[:-1] + [(301, b"CELL_VALUE")]), None, "AC1032", {}),
             (document(body[:-1] + [(304, b"END_VALUE")]), None, "AC1032", {}),
             (document(body[:1] + [(93, b"6")] + body[1:]), None, "AC1032", {}),
             (good + b"0\nLINE\n", None, "AC1032", {}),
             (document(body[:-1] + [(310, b"ABC"), body[-1]]), None, "AC1032", {}),
             (document(body[:-1] + [(310, b"XY"), body[-1]]), None, "AC1032", {}),
             (good.replace(b"301\nCELL_VALUE\n", b"102\n{APP\n301\nCELL_VALUE\n", 1), None, "AC1032", {}),
             (good.replace(b"301\nCELL_VALUE\n", b"102\n}\n301\nCELL_VALUE\n", 1), None, "AC1032", {}),
             (good.replace(b"301\nCELL_VALUE\n", b"102\n{APP\n" * 33 + b"301\nCELL_VALUE\n", 1), None, "AC1032", {}),
             (document(body[:-1] + [(102, b"{APP"), (102, b"}"), body[-1]]), None, "AC1032", {})]
    for data, digest, revision, limits in cases:
        try:
            inventory(data, digest or hashlib.sha256(data).hexdigest(), revision, **limits)
        except ValueError:
            continue
        raise ValueError("inventory negative control accepted")
    print(f"TABLE Value inventory: {positive} positives; {len(cases)} refusals; no payload/support promotion")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", nargs="?", type=Path)
    parser.add_argument("--sha256")
    parser.add_argument("--version")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    try:
        if args.self_test:
            if args.source or args.sha256 or args.version:
                parser.error("self-test cannot take a source")
            self_test()
        else:
            if not args.source or not args.sha256 or not args.version:
                parser.error("source, --sha256 and --version are required")
            with args.source.open("rb") as stream:
                data = stream.read(MAX_BYTES + 1)
            print(json.dumps(inventory(data, args.sha256, args.version), indent=2))
    except (OSError, UnicodeError, ValueError) as error:
        print(f"TABLE Value inventory refused: {error}", file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    sys.exit(main())
