#!/usr/bin/env python3
"""Create a scratch 3D CIRCLE/ARC source for the AC1024 extrusion audit."""
import argparse
import hashlib
from pathlib import Path

import ezdxf

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("output", type=Path)
args = parser.parse_args()
if ezdxf.__version__ != "1.4.5":
    parser.error("ezdxf 1.4.5 is required")
if args.output.exists():
    parser.error("output must not already exist")
args.output.parent.mkdir(parents=True, exist_ok=True)
document = ezdxf.new("R2010")
model = document.modelspace()
for normal in ((0, 1, 0), (0, 0, -1)):
    model.add_circle((7, 8, 2), radius=2.5,
                     dxfattribs={"extrusion": normal, "thickness": 1.25})
    model.add_arc((10, 11, 3), radius=4, start_angle=20, end_angle=115,
                  dxfattribs={"extrusion": normal, "thickness": 0.5})
document.saveas(args.output)
print("sha256=" + hashlib.sha256(args.output.read_bytes()).hexdigest())
