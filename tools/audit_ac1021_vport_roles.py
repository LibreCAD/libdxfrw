#!/usr/bin/env python3
"""Inspect SHA-bound AC1021 VPORT roles and optional ODA DXF readback."""
import hashlib
import json
import math
import subprocess
import sys
import tempfile
from pathlib import Path


def require(condition, message):
    if not condition:
        raise ValueError(message)


def main():
    if len(sys.argv) != 5:
        raise SystemExit("usage: audit_ac1021_vport_roles.py <dwgread> <dwg> <sha256> <dxf>")
    dwgread, source, digest, output = sys.argv[1:]
    raw = Path(source).read_bytes()
    require(raw[:6] == b"AC1021" and hashlib.sha256(raw).hexdigest() == digest,
            "DWG version or SHA-256 mismatch")
    with tempfile.TemporaryDirectory() as directory:
        target = Path(directory) / "decoded.json"
        subprocess.run([dwgread, "-O", "JSON", "-o", str(target), source], check=True)
        doc = json.loads(target.read_text())
    objects = doc["OBJECTS"]
    rows = [o for o in objects if o.get("object") == "VPORT"
            and o.get("name", "").upper() == "*ACTIVE"]
    require(len(rows) == 1, "expected one active VPORT")
    row = rows[0]
    expected = {"ownerhandle": [4, 1, 8, 8], "xref": [5, 0, 0, 0],
                "background": [4, 0, 0, 0], "visualstyle": [5, 0, 0, 0],
                "sun": [3, 0, 0, 0], "named_ucs": [5, 0, 0, 0],
                "base_ucs": [5, 0, 0, 0]}
    for name, value in expected.items():
        require(row.get(name) == value, f"unexpected {name}: {row.get(name)}")
    lines = [o for o in objects if o.get("entity") == "LINE"]
    require(len(lines) == 1 and lines[0]["start"] == [1., 2., 3.]
            and lines[0]["end"] == [4., 5., 6.], "DWG LINE mismatch")
    import ezdxf
    require(ezdxf.__version__ == "1.4.5", "ezdxf 1.4.5 required")
    dxf = ezdxf.readfile(output)
    lines = list(dxf.modelspace().query("LINE"))
    require(dxf.dxfversion == "AC1021" and len(lines) == 1
            and tuple(lines[0].dxf.start) == (1., 2., 3.)
            and tuple(lines[0].dxf.end) == (4., 5., 6.)
            and lines[0].dxf.layer == "0", "DXF LINE mismatch")
    viewports = [v for v in dxf.viewports if v.dxf.name.upper() == "*ACTIVE"]
    require(len(viewports) == 1, "DXF active VPORT mismatch")
    view = viewports[0].dxf
    require(math.isclose(view.height, row["VIEWSIZE"], rel_tol=1e-9, abs_tol=1e-9)
            and all(math.isclose(view.center[i], row["VIEWCTR"][i], rel_tol=1e-9, abs_tol=1e-9)
                    for i in range(2))
            and all(math.isclose(view.grid_spacing[i], row["GRIDUNIT"][i], rel_tol=1e-9, abs_tol=1e-9)
                    for i in range(2))
            and all(math.isclose(view.snap_spacing[i], row["SNAPUNIT"][i], rel_tol=1e-9, abs_tol=1e-9)
                    for i in range(2)), "DXF VPORT geometry mismatch")
    materials = dict(dxf.materials)
    require(set(("ByBlock", "ByLayer", "Global")).issubset(materials),
            "DXF material defaults missing")
    require(dxf.layers.get("0").dxf.material_handle == materials["Global"].dxf.handle,
            "DXF layer 0 Global material reference mismatch")
    print("AC1021 VPORT roles, LINE and DXF material binding PASS")


if __name__ == "__main__":
    main()
