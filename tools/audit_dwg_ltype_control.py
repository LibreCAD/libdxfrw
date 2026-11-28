#!/usr/bin/env python3
"""Author scratch DXF and independently check a DWG LTYPE_CONTROL graph."""
import json
import subprocess
import sys
import tempfile
from pathlib import Path


def author(path):
    import ezdxf
    assert ezdxf.__version__ == "1.4.5"
    drawing = ezdxf.new("R2010")
    drawing.linetypes.new("USER_DASH", dxfattribs={"description": "User dash", "pattern": [1., .5, -.5]})
    drawing.layers.new("DASH_LAYER", dxfattribs={"linetype": "USER_DASH"})
    drawing.modelspace().add_line((1, 2, 3), (4, 5, 6))
    drawing.modelspace().add_line((10, 20), (30, 40), dxfattribs={"layer": "DASH_LAYER", "linetype": "USER_DASH"})
    drawing.saveas(path)


def verify(dwgread, path):
    with tempfile.TemporaryDirectory() as tmp:
        output = Path(tmp) / "read.json"
        subprocess.run([dwgread, "-O", "JSON", "-o", str(output), path], check=True)
        objects = json.loads(output.read_text())["OBJECTS"]
    records = {o["handle"][2]: o for o in objects if o.get("object") == "LTYPE"}
    controls = [o for o in objects if o.get("object") == "LTYPE_CONTROL"]
    assert len(controls) == 1
    control = controls[0]
    def named(reference, role, name):
        assert reference[0] == role and reference[3] in records, reference
        assert records[reference[3]]["name"].upper() == name, reference
    assert len(control["entries"]) == 2
    named(control["entries"][0], 2, "CONTINUOUS")
    named(control["entries"][1], 2, "USER_DASH")
    named(control["byblock"], 3, "BYBLOCK")
    named(control["bylayer"], 3, "BYLAYER")
    assert len({x[3] for x in control["entries"] + [control["byblock"], control["bylayer"]]}) == 4
    dash = records[control["entries"][1][3]]
    assert dash["numdashes"] == 2 and dash["pattern_len"] == 1.
    layers = [o for o in objects if o.get("object") == "LAYER" and o.get("name", "").upper() == "DASH_LAYER"]
    if layers:
        assert len(layers) == 1
        named(layers[0]["ltype"], 5, "USER_DASH")
    entities = [o for o in objects if o.get("entity") == "LINE"]
    assert any(o.get("ltype", [0, 0, 0, 0])[0] == 5
               and len(o["ltype"]) > 3 and o["ltype"][3] == control["entries"][1][3] for o in entities)
    print("LibreDWG LTYPE_CONTROL roles, order, names and custom payload PASS")


def verify_dxf(path):
    import ezdxf
    assert ezdxf.__version__ == "1.4.5"
    drawing = ezdxf.readfile(path)
    assert drawing.layers.get("0").dxf.linetype.upper() == "CONTINUOUS"
    entities = list(drawing.modelspace())
    assert len(entities) == 2 and all(e.dxftype() == "LINE" for e in entities)
    assert entities[0].dxf.get("linetype", "BYLAYER").upper() == "BYLAYER"
    assert entities[1].dxf.get("linetype", "BYLAYER").upper() == "BYBLOCK"
    assert all(e.dxf.layer == "0" for e in entities)
    print("ODA DXF resolves CONTINUOUS layer and BYLAYER/BYBLOCK entities PASS")


def main():
    if len(sys.argv) == 3 and sys.argv[1] == "author":
        author(sys.argv[2])
    elif len(sys.argv) == 4 and sys.argv[1] == "verify":
        verify(sys.argv[2], sys.argv[3])
    elif len(sys.argv) == 3 and sys.argv[1] == "verify-dxf":
        verify_dxf(sys.argv[2])
    else:
        raise SystemExit("usage: audit_dwg_ltype_control.py author <scratch.dxf> | verify <dwgread> <scratch.dwg> | verify-dxf <scratch.dxf>")


if __name__ == "__main__":
    main()
