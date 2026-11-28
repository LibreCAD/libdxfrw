#!/usr/bin/env python3
"""Compare selected generated AC1012 DIMSTYLE fields with scratch DXF outputs.

Requires ezdxf. A peer difference is a finding, not native semantic evidence.
"""
import argparse
import hashlib
import json
from pathlib import Path

import ezdxf


FIELDS = {"DIMSCALE": "dimscale", "DIMASZ": "dimasz", "DIMTXT": "dimtxt",
          "DIMEXO": "dimexo", "DIMTIH": "dimtih"}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("observer", type=Path)
    parser.add_argument("side", choices=("source", "destination"))
    parser.add_argument("dxf", type=Path, nargs="+")
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
        audit = doc.audit()
        records.append({"path": str(path), "sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
                        "handle": style.dxf.handle, "textStyle": style.dxf.dimtxsty,
                        "values": values,
                        "differences": {name: {"source": expected[name], "output": value}
                                        for name, value in values.items() if value != expected[name]},
                        "auditErrors": len(audit.errors), "auditFixes": len(audit.fixes)})
    print(json.dumps({"sourceSha256": json.loads(args.observer.read_text())[args.side]["sourceSha256"],
                      "reader": f"ezdxf {ezdxf.__version__}", "sourceValues": expected,
                      "outputs": records}, indent=2))


if __name__ == "__main__":
    main()
