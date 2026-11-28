#!/usr/bin/env python3
"""Compare two ODA-exported DXFs from the AC1024 extrusion witness.

The DWGs and DXFs are locally generated scratch files, not repository fixtures.
"""
import math
from pathlib import Path
import sys

import ezdxf


def close(left, right):
    return math.dist(tuple(left), tuple(right)) < 1e-9


def key(entity):
    return entity.dxftype(), tuple(round(value, 9) for value in entity.dxf.extrusion)


def main(before_path, after_path, expected_count):
    def geometry(path):
        entities = ezdxf.readfile(path).modelspace()
        return sorted((e for e in entities if e.dxftype() in ("CIRCLE", "ARC")), key=key)

    before, after = geometry(before_path), geometry(after_path)
    if len(before) != expected_count or len(after) != expected_count:
        raise ValueError("unexpected CIRCLE/ARC entity counts")
    for original, converted in zip(before, after):
        if key(original) != key(converted) or not (
                close(original.dxf.center, converted.dxf.center)
                and close(original.dxf.extrusion, converted.dxf.extrusion)
                and close(original.ocs().to_wcs(original.dxf.center),
                          converted.ocs().to_wcs(converted.dxf.center))
                and abs(original.dxf.radius - converted.dxf.radius) < 1e-9
                and abs(original.dxf.thickness - converted.dxf.thickness) < 1e-9):
            raise ValueError("CIRCLE/ARC OCS fields or WCS center changed")
        if original.dxftype() == "ARC" and not (
                abs(original.dxf.start_angle - converted.dxf.start_angle) < 1e-9
                and abs(original.dxf.end_angle - converted.dxf.end_angle) < 1e-9
                and close(original.start_point, converted.start_point)
                and close(original.end_point, converted.end_point)):
            raise ValueError("ARC angles or WCS endpoints changed")
    print(f"ezdxf {ezdxf.__version__}: {expected_count} CIRCLE/ARC comparisons PASS")


if __name__ == "__main__":
    if len(sys.argv) != 4:
        raise SystemExit("usage: dwg_legacy_extrusion_oda_ezdxf.py BEFORE.dxf AFTER.dxf COUNT")
    main(Path(sys.argv[1]), Path(sys.argv[2]), int(sys.argv[3]))
