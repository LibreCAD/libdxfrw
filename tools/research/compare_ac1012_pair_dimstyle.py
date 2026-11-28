#!/usr/bin/env python3
"""Compare selected generated AC1012 DIMSTYLE fields with scratch DXF outputs.

Requires ezdxf. A peer difference is a finding, not native semantic evidence.
"""
import argparse
import hashlib
import json
from pathlib import Path

import ezdxf

from audit_ac1012_pair_block_entities import observe_dimstyle
from generate_ac1012_dictionary_merge_samples import read_object_frame
from generate_ac1012_dictionary_witness import sha


FIELDS = {"DIMSCALE": "dimscale", "DIMASZ": "dimasz", "DIMTXT": "dimtxt",
          "DIMEXO": "dimexo", "DIMTIH": "dimtih"}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("observer", type=Path)
    parser.add_argument("side", choices=("source", "destination"))
    parser.add_argument("dxf", type=Path, nargs="+")
    parser.add_argument("--oda-source-dwg", type=Path,
                        help="ODA-generated AC1012 source with the documented selected profile")
    parser.add_argument("--dwgread", default="dwgread")
    args = parser.parse_args()
    source = json.loads(args.observer.read_text())[args.side]["dimstyle"]["proof"]["spans"]
    expected = {name: source[name]["value"] for name in FIELDS}
    records = []
    for path in args.dxf:
        doc = ezdxf.readfile(path)
        style = doc.dimstyles.get("Standard")
        if style is None:
            raise ValueError(f"missing Standard DIMSTYLE: {path}")
        values = {name: getattr(style.dxf, attribute) for name, attribute in FIELDS.items()}
        explicit = {name: style.dxf.get(attribute) for name, attribute in FIELDS.items()
                    if style.dxf.hasattr(attribute)}
        audit = doc.audit()
        records.append({"path": str(path), "sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
                        "handle": style.dxf.handle, "textStyle": style.dxf.dimtxsty,
                        "readerEffectiveValues": values,
                        "explicitValues": explicit,
                        "omittedFields": [name for name in FIELDS if name not in explicit],
                        "readerEffectiveDifferences": {name: {"source": expected[name], "output": value}
                                                       for name, value in values.items() if value != expected[name]},
                        "explicitDifferences": {name: {"source": expected[name], "output": value}
                                                for name, value in explicit.items() if value != expected[name]},
                        "auditErrors": len(audit.errors), "auditFixes": len(audit.fixes)})
    result = {"sourceSha256": json.loads(args.observer.read_text())[args.side]["sourceSha256"],
              "reader": f"ezdxf {ezdxf.__version__}", "sourceValues": expected, "outputs": records}
    if args.oda_source_dwg:
        offset, raw = read_object_frame(args.oda_source_dwg, args.dwgread, 0x32, 69)
        def observe(data):
            return observe_dimstyle(data, 0x32, "STANDARD", 4, 0x1e1, 0x30)
        proof = observe(raw)
        peer_values = {name: proof["spans"][name]["value"] for name in FIELDS}
        if peer_values != expected:
            raise ValueError("ODA-generated DWG DIMSTYLE differs from selected source values")
        for label in ("owner", "extension", "xref", "textStyle"):
            damaged = bytearray(raw)
            bit = proof["spans"][label]["startBit"]
            damaged[bit // 8] ^= 0x80 >> (bit % 8)
            try:
                observe(bytes(damaged))
            except ValueError:
                pass
            else:
                raise ValueError(f"wrong ODA DIMSTYLE {label} role accepted")
        for length in range(len(raw)):
            try:
                observe(raw[:length])
            except ValueError:
                pass
            else:
                raise ValueError("truncated ODA DIMSTYLE accepted")
        if observe(raw) != proof:
            raise ValueError("ODA DIMSTYLE retry differs")
        result["odaGeneratedSource"] = {"path": str(args.oda_source_dwg),
                                        "sha256": sha(args.oda_source_dwg.read_bytes()),
                                        "frameOffset": offset, "proof": proof,
                                        "wrongRoleAndTruncationRejectedWithRetry": True}
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
