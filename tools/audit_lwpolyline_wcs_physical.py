#!/usr/bin/env python3
import argparse
import hashlib
import subprocess
from pathlib import Path
import ezdxf


def check(value, message):
    if not value:
        raise SystemExit(message)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--dxf', required=True)
    parser.add_argument('--sha256', required=True)
    parser.add_argument('--probe', required=True)
    args = parser.parse_args()
    check(hashlib.sha256(Path(args.dxf).read_bytes()).hexdigest() == args.sha256, 'source hash mismatch')
    doc = ezdxf.readfile(args.dxf)
    check(doc.dxfversion == 'AC1027', 'wrong DXF version')
    entities = list(doc.modelspace().query('LWPOLYLINE'))
    check(len(entities) == 1, 'unexpected LWPOLYLINE inventory')
    entity = entities[0]
    check(entity.dxf.elevation == 2. and tuple(entity.dxf.extrusion) == (0., 1., 0.), 'wrong OCS frame')
    check([tuple(p) for p in entity.get_points('xy')] == [(0.,0.),(5.,0.),(5.,3.)], 'wrong OCS vertices')
    check([tuple(p) for p in entity.vertices_in_wcs()] == [(0.,2.,0.),(-5.,2.,0.),(-5.,2.,3.)], 'wrong WCS vertices')
    audit = doc.audit()
    check(not audit.errors and not audit.fixes, 'DXF audit error')
    probe = subprocess.run([args.probe, args.dxf], capture_output=True, text=True, check=True)
    check(probe.stdout.strip() == 'physical callback=1 complete-read=0', 'unexpected in-tree source observation')
    check(hashlib.sha256(Path(args.dxf).read_bytes()).hexdigest() == args.sha256, 'source changed during test')
    print('physical callback and ezdxf vertex checks passed; complete source read refused')


if __name__ == '__main__':
    main()
