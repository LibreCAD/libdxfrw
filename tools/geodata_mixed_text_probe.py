#!/usr/bin/env python3
"""Author a scratch GEODATA DXF with literal-caret and newline controls.

The XML fragment is a text transport witness, not a geospatial CRS claim.
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
geo.coordinate_system_definition = "<x>literal^ J|newline^Jend</x>"
geo.dxf.block_record_handle = model.block_record.dxf.handle
if (geo.dxf.handle, geo.dxf.owner, geo.dxf.block_record_handle) != ("30", "2F", "17"):
    parser.error("unexpected source identities")
document.saveas(args.output)
print(json.dumps({"ezdxf": ezdxf.__version__, "path": str(args.output),
                  "sha256": hashlib.sha256(args.output.read_bytes()).hexdigest()}))
