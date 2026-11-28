#!/usr/bin/env python3
"""Optional independent readback of locally generated conversion controls.

Run the C++ test with --keep, then pass its OUTPUT directory as argv[1].
Requires ezdxf. No CAD drawing is added to the repository.
"""
from pathlib import Path
import math
import sys
import ezdxf


def close(left, right):
    return math.dist(tuple(left), tuple(right)) < 1e-9


def main(directory):
    for mode in ("ascii", "binary"):
        before = list(ezdxf.readfile(directory / f"{mode}-original.dxf").modelspace())
        after = list(ezdxf.readfile(directory / f"{mode}-converted.dxf").modelspace())
        if len(before) != 12 or len(after) != 12:
            raise ValueError("expected six CIRCLE and six ARC entities")
        for original, converted in zip(before, after):
            if original.dxftype() != converted.dxftype():
                raise ValueError("entity type changed")
            if not (close(original.dxf.center, converted.dxf.center)
                    and close(original.dxf.extrusion, converted.dxf.extrusion)
                    and close(original.ocs().to_wcs(original.dxf.center),
                              converted.ocs().to_wcs(converted.dxf.center))
                    and abs(original.dxf.radius - converted.dxf.radius) < 1e-9):
                raise ValueError("OCS fields or WCS center changed")
            if original.dxftype() == "ARC" and not (
                    close(original.start_point, converted.start_point)
                    and close(original.end_point, converted.end_point)):
                raise ValueError("ARC endpoints changed")
        print(f"{mode}: ezdxf {ezdxf.__version__} fields, centers and endpoints PASS")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit("usage: dxf_legacy_extrusion_ezdxf.py TEST_OUTPUT_DIRECTORY")
    main(Path(sys.argv[1]))
