#!/usr/bin/env python3
"""Create a from-scratch GEODATA mesh DXF for an external producer probe.

The result is a scratch input, not a repository fixture. Use the pinned
ezdxf version so its entity scaffold matches the recorded witness.
"""
import argparse
import hashlib
import json
from pathlib import Path

import ezdxf
from ezdxf import units

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("output", type=Path)
args = parser.parse_args()
if ezdxf.__version__ != "1.4.5":
    parser.error("ezdxf 1.4.5 is required")
if args.output.exists():
    parser.error("output must not already exist")
args.output.parent.mkdir(parents=True, exist_ok=True)
document = ezdxf.new("R2013", units=units.M)
model = document.modelspace()
geo = model.new_geodata()
geo.setup_local_grid(design_point=(100, 200), reference_point=(1718030, 5921664))
geo.dxf.block_record_handle = model.block_record.dxf.handle
geo.source_vertices.extend([(1, 2), (4, 2), (1, 5)])
geo.target_vertices.extend([(1001, 2002), (1004, 2002), (1001, 2005)])
geo.faces.append((0, 1, 2))
model.add_line((0, 0), (100, 0))
if (geo.dxf.handle, geo.dxf.owner, geo.dxf.block_record_handle) != ("30", "2F", "17"):
    parser.error("unexpected source identities")
document.saveas(args.output)
print(json.dumps({"ezdxf": ezdxf.__version__, "path": str(args.output),
                  "sha256": hashlib.sha256(args.output.read_bytes()).hexdigest()}))
