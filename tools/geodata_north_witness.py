#!/usr/bin/env python3
"""Author a scratch GEODATA north/following-field DWG producer control."""
import argparse
import hashlib
import json
from pathlib import Path
import ezdxf
from ezdxf import units

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('output', type=Path)
args = parser.parse_args()
if args.output.exists():
    parser.error('output already exists')
args.output.parent.mkdir(parents=True, exist_ok=True)
doc = ezdxf.new('R2013', units=units.M)
model = doc.modelspace()
geo = model.new_geodata()
geo.setup_local_grid(design_point=(42, 71), reference_point=(1200000, 2300000))
geo.dxf.block_record_handle = model.block_record.dxf.handle
geo.dxf.north_direction = (0.6, 0.8, 0.0)
geo.dxf.scale_estimation_method = 2
geo.dxf.user_scale_factor = 1.23456789
geo.dxf.sea_level_correction = 1
geo.dxf.sea_level_elevation = 42.75
geo.dxf.coordinate_projection_radius = 6371000.0
model.add_line((0, 0), (100, 0))
doc.saveas(args.output)
print(json.dumps({'ezdxfVersion': ezdxf.__version__, 'output': str(args.output),
                  'sha256': hashlib.sha256(args.output.read_bytes()).hexdigest()}, indent=2))
