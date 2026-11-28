#!/usr/bin/env python3
"""Author a GEODATA local-grid DXF for an independent converter probe.

This creates files from scratch and does not invoke a converter. The DXF is
intended to be converted externally to an AC1027 DWG, then passed to the
optional --geodata-physical-v3 wave1 selector; it is not a checked-in fixture.
"""
import argparse
import hashlib
import json
from pathlib import Path

import ezdxf
from ezdxf import units

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('output', type=Path, help='new DXF file to create')
args = parser.parse_args()
if args.output.exists():
    parser.error('output must not already exist')
args.output.parent.mkdir(parents=True, exist_ok=True)
doc = ezdxf.new('R2013', units=units.M)
model = doc.modelspace()
geo = model.new_geodata()
geo.setup_local_grid(design_point=(100, 200), reference_point=(1718030, 5921664))
model.add_line((0, 0), (100, 0))
# ezdxf creates the dictionary link, but its new object defaults the separate
# host soft pointer to null. Set that host to the actual model block record.
geo.dxf.block_record_handle = model.block_record.dxf.handle
if (geo.dxf.handle, geo.dxf.owner, geo.dxf.block_record_handle) != ('30', '2F', '17'):
    parser.error('unexpected scaffold handles; update the optional selector')
doc.saveas(args.output)
print(json.dumps({'ezdxfVersion': ezdxf.__version__, 'file': str(args.output),
                  'sha256': hashlib.sha256(args.output.read_bytes()).hexdigest(),
                  'geodata': geo.dxf.handle, 'ownerDictionary': geo.dxf.owner,
                  'hostBlockRecord': geo.dxf.block_record_handle}, indent=2))
