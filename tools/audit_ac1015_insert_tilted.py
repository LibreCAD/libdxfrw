#!/usr/bin/env python3
"""Author or inspect scratch AC1015 tilted attributed INSERT witnesses."""
import math
import sys
from pathlib import Path

import ezdxf
from audit_ac1015_attributed_insert_block_contents import author

if ezdxf.__version__ != "1.4.5":
    raise SystemExit("ezdxf 1.4.5 required")

NORMALS = {"tilted": (0., 1., 0.), "negativez": (0., 0., -1.)}
LINES = {
    "tilted": ((-1.7320508075688779, 12., 10.196152422706632),
               (-2.4282032302755105, 24., 20.99038105676658)),
    "negativez": ((-1.7320508075688779, 10.196152422706632, -12.),
                  (-2.4282032302755105, 20.99038105676658, -24.)),
}


def inspect(path, mode, generated):
    doc = ezdxf.readfile(path)
    insert = doc.modelspace().query("INSERT")[0]
    assert insert.dxf.name == "CONTENT_CHAIN"
    assert tuple(insert.dxf.insert) == (3., 4., 0.)
    assert tuple(insert.dxf.extrusion) == NORMALS[mode]
    assert (insert.dxf.xscale, insert.dxf.yscale, insert.dxf.zscale) == (2., 3., 4.)
    assert math.isclose(insert.dxf.rotation, 30., abs_tol=1e-10)
    attrs = [(a.dxf.tag, a.dxf.text, tuple(a.dxf.insert), a.dxf.rotation) for a in insert.attribs]
    assert attrs == ([('FIRST', 'ONE', (3., 4., 0.), 0)] if generated else
                     [('FIRST', 'ONE', (3., 4., 0.), 15.), ('SECOND', 'TWO', (4., 5., 0.), 25.)])
    for attr in insert.attribs:
        assert tuple(attr.dxf.get("extrusion", (0., 0., 1.))) == (0., 0., 1.)
        assert tuple(attr.ocs().to_wcs(attr.dxf.insert)) == tuple(attr.dxf.insert)
    line = [e for e in insert.virtual_entities() if e.dxftype() == "LINE"]
    assert len(line) == 1
    for actual, expected in zip((line[0].dxf.start, line[0].dxf.end), LINES[mode]):
        assert all(math.isclose(a, b, abs_tol=1e-10) for a, b in zip(actual, expected))
    print("tilted INSERT fields, attribute positions and transformed LINE PASS", mode, path)


def main():
    if len(sys.argv) != 4 or sys.argv[1] not in ("author", "source", "generated") or sys.argv[2] not in NORMALS:
        raise SystemExit("usage: audit_ac1015_insert_tilted.py author|source|generated tilted|negativez file.dxf")
    action, mode, path = sys.argv[1], sys.argv[2], Path(sys.argv[3])
    if action == "author":
        author(path)
        doc = ezdxf.readfile(path)
        insert = doc.modelspace().query("INSERT")[0]
        insert.dxf.extrusion = NORMALS[mode]
        insert.dxf.xscale, insert.dxf.yscale, insert.dxf.zscale = 2., 3., 4.
        doc.saveas(path)
    else:
        inspect(path, mode, action == "generated")


if __name__ == "__main__":
    main()
