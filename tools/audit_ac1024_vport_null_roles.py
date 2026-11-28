#!/usr/bin/env python3
"""Scratch author and LibreDWG inspector for AC1024 synthetic VPORT roles.

Run author <input.dxf> [style] with ezdxf 1.4.5. Convert the DXF to ACAD2010
DWG using ODA File Converter. Run verify <dwgread> <source.dwg> [style]; the
style variant requires the referenced VISUALSTYLE to be physically present.
"""
import json
import subprocess
import sys
import tempfile
from pathlib import Path


def author(path, style):
    import ezdxf
    assert ezdxf.__version__ == "1.4.5"
    doc = ezdxf.new("R2010", setup=style)
    doc.modelspace().add_line((1, 2, 3), (4, 5, 6))
    doc.saveas(path)


def verify(dwgread, source, style):
    assert Path(source).read_bytes()[:6] == b"AC1024"
    with tempfile.TemporaryDirectory() as tmp:
        result = Path(tmp) / "source.json"
        subprocess.run([dwgread, "-O", "JSON", "-o", str(result), source], check=True)
        objects = json.loads(result.read_text())["OBJECTS"]
    vports = [o for o in objects if o.get("object") == "VPORT" and o.get("name", "").upper() == "*ACTIVE"]
    assert len(vports) == 1
    vport = vports[0]
    assert vport["is_xdic_missing"] == 1
    assert vport["ownerhandle"] == [4, 1, 8, 8]
    assert vport["xref"] == [5, 0, 0, 0]
    assert vport["background"] == [4, 0, 0, 0]
    assert vport["sun"] == [3, 0, 0, 0]
    assert vport["named_ucs"] == [5, 0, 0, 0]
    assert vport["base_ucs"] == [5, 0, 0, 0]
    visual = vport["visualstyle"]
    assert visual[0] == 5
    if style:
        assert visual[3] != 0
        matches = [o for o in objects if o.get("handle", [0, 0, 0])[2] == visual[3]]
        assert len(matches) == 1 and matches[0].get("object") == "VISUALSTYLE"
        assert matches[0].get("description") == "2dWireframe"
    else:
        assert visual == [5, 0, 0, 0]
    assert vport["upper_right"] == [1., 1.] and vport["grid_major"] == 5
    lines = [o for o in objects if o.get("entity") == "LINE"]
    assert len(lines) == 1 and lines[0]["start"] == [1., 2., 3.] and lines[0]["end"] == [4., 5., 6.]
    print("ODA-authored AC1024 VPORT role witness PASS")


def main():
    if len(sys.argv) in (3, 4) and sys.argv[1] == "author" and (len(sys.argv) == 3 or sys.argv[3] == "style"):
        author(sys.argv[2], len(sys.argv) == 4)
    elif len(sys.argv) in (4, 5) and sys.argv[1] == "verify" and (len(sys.argv) == 4 or sys.argv[4] == "style"):
        verify(sys.argv[2], sys.argv[3], len(sys.argv) == 5)
    else:
        raise SystemExit("usage: audit_ac1024_vport_null_roles.py author <dxf> [style] | verify <dwgread> <dwg> [style]")


if __name__ == "__main__":
    main()
