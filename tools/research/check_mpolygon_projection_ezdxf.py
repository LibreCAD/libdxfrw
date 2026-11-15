#!/usr/bin/env python3
"""Independently check scratch MPOLYGON projections with optional ezdxf.

Usage: python3 tools/research/check_mpolygon_projection_ezdxf.py GENERATOR
GENERATOR is the built libdxfrw_mpolygon_edge_projection_tests executable.
All DXF files are generated in and removed with a temporary directory.
"""
import math
from pathlib import Path
import subprocess
import sys
import tempfile

try:
    import ezdxf
    from ezdxf.math import bulge_to_arc
except ImportError as exc:
    raise SystemExit("Install ezdxf in the selected Python environment") from exc


PROFILES = (
    (0.0, math.pi),
    (0.0, math.pi / 2.0),
    (0.0, 3.0 * math.pi / 2.0),
    (3.0 * math.pi / 2.0, math.pi / 2.0),
)


def close(actual, expected):
    return math.isclose(actual, expected, rel_tol=1e-11, abs_tol=1e-11)


def check(directory, binary, ccw, start, end):
    name = (f"{'binary' if binary else 'ascii'}-"
            f"{'ccw' if ccw else 'cw'}-{int(start * 1000)}-"
            f"{int(end * 1000)}.dxf")
    doc = ezdxf.readfile(directory / name)
    entities = list(doc.modelspace().query("MPOLYGON"))
    if len(entities) != 1:
        raise AssertionError((name, "entity count", len(entities)))
    path = entities[0].paths[0]
    if not path.is_closed or len(path.vertices) != 2:
        raise AssertionError((name, "path", path.vertices))
    line_start, arc_start = path.vertices
    signed_start = start if ccw else -start
    signed_end = end if ccw else -end
    expected_arc = (5.0 + 5.0 * math.cos(signed_start),
                    5.0 * math.sin(signed_start))
    expected_line = (5.0 + 5.0 * math.cos(signed_end),
                     5.0 * math.sin(signed_end))
    if not all(close(a, b) for a, b in zip(arc_start[:2], expected_arc)):
        raise AssertionError((name, "arc start", arc_start, expected_arc))
    if not all(close(a, b) for a, b in zip(line_start[:2], expected_line)):
        raise AssertionError((name, "arc end", line_start, expected_line))
    center, first_angle, last_angle, radius = bulge_to_arc(
        arc_start[:2], line_start[:2], arc_start[2])
    expected_sweep = (end - start) % math.tau
    actual_sweep = (last_angle - first_angle) % math.tau
    if not (close(center.x, 5.0) and close(center.y, 0.0)
            and close(radius, 5.0) and close(actual_sweep, expected_sweep)
            and (arc_start[2] > 0.0) == ccw):
        raise AssertionError((name, "reconstructed arc", center, radius,
                              actual_sweep, expected_sweep, arc_start[2]))


def main():
    if len(sys.argv) != 2:
        raise SystemExit(f"usage: {sys.argv[0]} GENERATOR")
    executable = Path(sys.argv[1]).resolve()
    with tempfile.TemporaryDirectory(prefix="libdxfrw-mpolygon-peer-") as temp:
        directory = Path(temp)
        subprocess.run([str(executable), "--peer-output", str(directory)], check=True)
        for binary in (False, True):
            for ccw in (False, True):
                for start, end in PROFILES:
                    check(directory, binary, ccw, start, end)
        if len(list(directory.glob("*.dxf"))) != 16:
            raise AssertionError("Expected exactly 16 generated peer files")
    print(f"ezdxf {ezdxf.__version__}: 16 MPOLYGON arc reconstructions PASS")


if __name__ == "__main__":
    main()
