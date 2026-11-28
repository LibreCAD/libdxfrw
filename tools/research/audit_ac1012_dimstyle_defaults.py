#!/usr/bin/env python3
"""Audit three locally generated R13 DIMSTYLE default-discrimination controls.

Inputs are scratch-only DXFs, ODA-generated DWGs and ODA DXF reopens. Requires
LibreDWG dwgread and ezdxf; neither peer establishes native AutoCAD semantics.
"""
import argparse
import hashlib
import json
from pathlib import Path

import ezdxf

from audit_ac1012_pair_block_entities import observe_dimstyle
from generate_ac1012_dictionary_merge_samples import read_object_frame

FIELDS = {"DIMASZ": "dimasz", "DIMEXO": "dimexo", "DIMTXT": "dimtxt", "DIMTIH": "dimtih"}
HEADER = {"DIMASZ": 0.18, "DIMEXO": 0.0625, "DIMTXT": 0.18, "DIMTIH": 1}
DISTINCT = {"DIMASZ": 7.25, "DIMEXO": 3.5, "DIMTXT": 9.25, "DIMTIH": 0}


def require(value, message):
    if not value:
        raise ValueError(message)


def dxf_observation(path):
    doc = ezdxf.readfile(path)
    style = doc.dimstyles.get("Standard")
    require(style is not None, f"no Standard style: {path}")
    audit = doc.audit()
    require(not audit.errors and not audit.fixes, f"DXF audit changed or rejected: {path}")
    return {"sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
            "header": {name: doc.header.get("$" + name) for name in FIELDS},
            "explicit": {name: style.dxf.get(attribute) for name, attribute in FIELDS.items()
                         if style.dxf.hasattr(attribute)},
            "readerEffective": {name: getattr(style.dxf, attribute)
                                for name, attribute in FIELDS.items()}}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("root", type=Path)
    parser.add_argument("--dwgread", default="dwgread")
    args = parser.parse_args()
    result = {"reader": f"ezdxf {ezdxf.__version__}", "cases": {}}
    for name in ("omitted", "header", "distinct"):
        input_dxf = dxf_observation(args.root / "input" / (name + ".dxf"))
        output_dxf = dxf_observation(args.root / "reopen" / (name + ".dxf"))
        require(input_dxf["header"] == output_dxf["header"] == HEADER,
                "selected header fields differ")
        expected_input = {} if name == "omitted" else HEADER if name == "header" else DISTINCT
        expected_output = DISTINCT if name == "distinct" else {}
        require(input_dxf["explicit"] == expected_input, "input groups are not as specified")
        require(output_dxf["explicit"] == expected_output, "unexpected reopened groups")
        path = args.root / "dwg" / (name + ".dwg")
        offset, raw = read_object_frame(path, args.dwgread, 0x32, 69)
        override = DISTINCT if name == "distinct" else {}
        def observe(data):
            return observe_dimstyle(data, 0x32, "STANDARD", 4, 0x1e1, 0x30,
                                    expected_overrides=override)
        proof = observe(raw)
        for label in ("owner", "extension", "xref", "textStyle"):
            damaged = bytearray(raw)
            bit = proof["spans"][label]["startBit"]
            damaged[bit // 8] ^= 0x80 >> (bit % 8)
            try:
                observe(bytes(damaged))
            except ValueError:
                pass
            else:
                raise ValueError(f"wrong {label} role accepted in {name}")
        for size in range(len(raw)):
            try:
                observe(raw[:size])
            except ValueError:
                pass
            else:
                raise ValueError(f"truncated frame accepted in {name}")
        require(observe(raw) == proof, "clean retry changed")
        result["cases"][name] = {"input": input_dxf, "reopen": output_dxf,
                                 "dwgSha256": hashlib.sha256(path.read_bytes()).hexdigest(),
                                 "frameOffset": offset, "frameSha256": proof["frameSha256"],
                                 "wireValues": {field: proof["spans"][field]["value"]
                                                for field in FIELDS},
                                 "wrongRolesAndTruncationRejectedWithRetry": True}
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
