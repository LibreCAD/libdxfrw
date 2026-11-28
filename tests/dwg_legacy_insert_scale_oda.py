#!/usr/bin/env python3
"""Inspect ODA DXFs from locally generated AC1015 scaled-INSERT controls."""
import math
from pathlib import Path
import sys

import ezdxf


def near_point(actual, expected):
    return math.dist(tuple(actual), expected) < 1e-9


def main(directory):
    scales = {"uniform": (2., 2., 2.), "xunit": (1., 2., 3.),
              "nonuniform": (2., 3., 4.), "fractional": (.5, 1.25, 2.75)}
    for mode, scale in scales.items():
        drawing = ezdxf.readfile(directory / f"scale-{mode}.dxf")
        inserts = list(drawing.modelspace().query("INSERT"))
        if len(inserts) != 2:
            raise ValueError(f"{mode}: wrong INSERT count")
        first, second = inserts
        if tuple((first.dxf.xscale, first.dxf.yscale, first.dxf.zscale)) != scale:
            raise ValueError(f"{mode}: wrong scale")
        expected = (("FIRST", "ONE", (3., 4., 0.), .5, 15.),
                    ("SECOND", "TWO", (4., 5., 0.), .75, 25.))
        if len(first.attribs) != len(expected) or len(second.attribs) != 1:
            raise ValueError(f"{mode}: wrong ATTRIB count")
        for value, (tag, text, point, height, angle) in zip(first.attribs, expected):
            if (value.dxf.tag != tag or value.dxf.text != text
                    or not near_point(value.dxf.insert, point)
                    or abs(value.dxf.height - height) > 1e-9
                    or abs(value.dxf.rotation - angle) > 1e-9):
                raise ValueError(f"{mode}: ATTRIB fields changed")
        last = second.attribs[0]
        if last.dxf.tag != "SINGLE" or last.dxf.text != "THREE" or not near_point(last.dxf.insert, (9., 10., 0.)):
            raise ValueError(f"{mode}: second INSERT ATTRIB changed")

    drawing = ezdxf.readfile(directory / "scaled-block.dxf")
    inserts = list(drawing.modelspace().query("INSERT"))
    if len(inserts) != 1:
        raise ValueError("scaled block: wrong INSERT count")
    insert = inserts[0]
    if tuple((insert.dxf.xscale, insert.dxf.yscale, insert.dxf.zscale)) != (2., 3., 4.) or abs(insert.dxf.rotation - 30.) > 1e-9:
        raise ValueError("scaled block: scale/rotation changed")
    if (len(insert.attribs) != 1 or insert.attribs[0].dxf.tag != "FIRST"
            or insert.attribs[0].dxf.text != "ONE"
            or not near_point(insert.attribs[0].dxf.insert, (3., 4., 0.))):
        raise ValueError("scaled block: ATTRIB changed")
    lines = [e for e in insert.virtual_entities() if e.dxftype() == "LINE"]
    if (len(lines) != 1
            or not near_point(lines[0].dxf.start, (1.7320508075688779, 10.196152422706632, 12.))
            or not near_point(lines[0].dxf.end, (2.4282032302755105, 20.99038105676658, 24.))):
        raise ValueError("scaled block: transformed 3D LINE changed")
    print(f"ODA DXF / ezdxf {ezdxf.__version__}: 5 scaled INSERT profiles PASS")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit("usage: dwg_legacy_insert_scale_oda.py ODA_DXF_DIRECTORY")
    main(Path(sys.argv[1]))
