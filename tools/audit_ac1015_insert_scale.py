#!/usr/bin/env python3
"""Author and independently inspect scratch AC1015 scaled INSERT sources."""
import importlib.util
import json
import math
from pathlib import Path
import subprocess
import sys
import tempfile

import ezdxf

if ezdxf.__version__ != "1.4.5":
    raise SystemExit("ezdxf 1.4.5 is required")

SCALES = {"uniform": (2., 2., 2.), "xunit": (1., 2., 3.),
          "nonuniform": (2., 3., 4.), "populated": (2., 3., 4.)}


def helper(mode):
    name = "audit_ac1015_attributed_insert_block_contents.py" if mode == "populated" else "audit_ac1015_empty_attributed_insert.py"
    spec = importlib.util.spec_from_file_location("insert_witness", Path(__file__).with_name(name))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def author(mode, path):
    target = Path(path)
    if target.exists():
        raise SystemExit("output must not already exist")
    target.parent.mkdir(parents=True, exist_ok=True)
    helper(mode).author(str(target))
    drawing = ezdxf.readfile(target)
    insert = list(drawing.modelspace().query("INSERT"))[0]
    insert.dxf.xscale, insert.dxf.yscale, insert.dxf.zscale = SCALES[mode]
    drawing.saveas(target)


def verify(mode, source, output, dwgread):
    helper(mode).verify(source, output, dwgread)
    with tempfile.TemporaryDirectory() as tmp:
        peer = Path(tmp) / "peer.json"
        subprocess.run([dwgread, "-O", "JSON", "-o", str(peer), source], check=True)
        objects = json.loads(peer.read_text())["OBJECTS"]
    expected_handle = 0x92 if mode == "populated" else 0x8E
    matches = [o for o in objects if o.get("entity") == "INSERT" and o.get("handle", [0, 0, 0])[2] == expected_handle]
    if len(matches) != 1 or tuple(matches[0]["scale"]) != SCALES[mode]:
        raise ValueError("DWG INSERT scale differs")
    drawing = ezdxf.readfile(output)
    inserts = list(drawing.modelspace().query("INSERT"))
    if not inserts or tuple((inserts[0].dxf.xscale, inserts[0].dxf.yscale, inserts[0].dxf.zscale)) != SCALES[mode]:
        raise ValueError("DXF INSERT scale differs")
    if mode == "populated":
        lines = [e for e in inserts[0].virtual_entities() if e.dxftype() == "LINE"]
        if len(lines) != 1:
            raise ValueError("missing transformed block LINE")
        expected = ((1.7320508075688779, 10.196152422706632, 12.),
                    (2.4282032302755105, 20.99038105676658, 24.))
        if any(math.dist(tuple(actual), wanted) > 1e-9
               for actual, wanted in zip((lines[0].dxf.start, lines[0].dxf.end), expected)):
            raise ValueError("transformed 3D block LINE differs")
    print("AC1015 scaled INSERT graph and fields PASS:", mode)


if __name__ == "__main__":
    if len(sys.argv) == 4 and sys.argv[1] == "author" and sys.argv[2] in SCALES:
        author(sys.argv[2], sys.argv[3])
    elif len(sys.argv) in (5, 6) and sys.argv[1] == "verify" and sys.argv[2] in SCALES:
        verify(sys.argv[2], sys.argv[3], sys.argv[4], sys.argv[5] if len(sys.argv) == 6 else "dwgread")
    else:
        raise SystemExit("usage: audit_ac1015_insert_scale.py author MODE FILE.dxf | verify MODE SOURCE.dwg OUTPUT.dxf [dwgread]")
