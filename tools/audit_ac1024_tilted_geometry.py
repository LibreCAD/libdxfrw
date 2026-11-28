#!/usr/bin/env python3
"""Scratch-only ODA/DXF and LibreDWG controls for tilted AC1024 geometry."""
import json
import math
import subprocess
import sys
import tempfile
from pathlib import Path


def author(path):
    import ezdxf
    assert ezdxf.__version__ == "1.4.5"
    doc = ezdxf.new("R2010")
    model = doc.modelspace()
    model.add_line((1, 2, 3), (4, 5, 6))
    model.add_circle((7, 8, 2), 2.5, dxfattribs={"extrusion": (0, 1, 0)})
    model.add_arc((10, 11, 3), 4, math.degrees(.25), math.degrees(1.25),
                  dxfattribs={"extrusion": (0, 1, 0)})
    doc.saveas(path)


def inspect_dwg(dwgread, path):
    assert Path(path).read_bytes()[:6] == b"AC1024"
    with tempfile.TemporaryDirectory() as tmp:
        result = Path(tmp) / "out.json"
        subprocess.run([dwgread, "-O", "JSON", "-o", str(result), path], check=True)
        objects = json.loads(result.read_text())["OBJECTS"]
    entities = [o for o in objects if o.get("entity") in ("LINE", "CIRCLE", "ARC")]
    assert [o["entity"] for o in entities] == ["LINE", "CIRCLE", "ARC"]
    line, circle, arc = entities
    assert line["start"] == [1., 2., 3.] and line["end"] == [4., 5., 6.]
    assert circle["center"] == [7., 8., 2.] and circle["radius"] == 2.5
    assert arc["center"] == [10., 11., 3.] and arc["radius"] == 4.
    assert circle["extrusion"] == arc["extrusion"] == [0., 1., 0.]
    assert math.isclose(arc["start_angle"], .25, abs_tol=1e-9)
    assert math.isclose(arc["end_angle"], 1.25, abs_tol=1e-9)


def inspect_dxf(path):
    import ezdxf
    assert ezdxf.__version__ == "1.4.5"
    entities = list(ezdxf.readfile(path).modelspace())
    assert [e.dxftype() for e in entities] == ["LINE", "CIRCLE", "ARC"]
    line, circle, arc = entities
    assert tuple(line.dxf.start) == (1., 2., 3.) and tuple(line.dxf.end) == (4., 5., 6.)
    assert tuple(circle.dxf.center) == (7., 8., 2.) and circle.dxf.radius == 2.5
    assert tuple(arc.dxf.center) == (10., 11., 3.) and arc.dxf.radius == 4.
    assert tuple(circle.dxf.extrusion) == tuple(arc.dxf.extrusion) == (0., 1., 0.)
    assert math.isclose(arc.dxf.start_angle, math.degrees(.25), abs_tol=1e-9)
    assert math.isclose(arc.dxf.end_angle, math.degrees(1.25), abs_tol=1e-9)
    assert tuple(circle.ocs().to_wcs(circle.dxf.center)) == (-7., 2., 8.)
    assert tuple(arc.ocs().to_wcs(arc.dxf.center)) == (-10., 3., 11.)
    # For extrusion (0,1,0), the OCS basis is X=(-1,0,0), Y=(0,0,1).
    for actual, angle in ((arc.start_point, .25), (arc.end_point, 1.25)):
        expected = (-10. - 4.*math.cos(angle), 3., 11. + 4.*math.sin(angle))
        assert all(math.isclose(a, b, abs_tol=1e-9) for a, b in zip(actual, expected))
    print("DXF OCS centers and independently transformed WCS centers PASS")


def main():
    if len(sys.argv) == 3 and sys.argv[1] == "author":
        author(sys.argv[2])
    elif len(sys.argv) == 4 and sys.argv[1] == "inspect-dwg":
        inspect_dwg(sys.argv[2], sys.argv[3]);print("LibreDWG geometry fields PASS")
    elif len(sys.argv) == 3 and sys.argv[1] == "inspect-dxf":
        inspect_dxf(sys.argv[2])
    else:
        raise SystemExit("usage: audit_ac1024_tilted_geometry.py author <dxf> | inspect-dwg <dwgread> <dwg> | inspect-dxf <dxf>")


if __name__ == "__main__":
    main()
