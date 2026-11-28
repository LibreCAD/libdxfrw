#!/usr/bin/env python3
import argparse
import hashlib
import json
import subprocess
from pathlib import Path
import ezdxf


def check(condition, message):
    if not condition:
        raise SystemExit(message)


def bound(path, digest):
    check(hashlib.sha256(Path(path).read_bytes()).hexdigest() == digest, 'artifact hash mismatch')


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--dwgread', required=True)
    parser.add_argument('--dwg', required=True)
    parser.add_argument('--dwg-sha256', required=True)
    parser.add_argument('--dxf', required=True)
    parser.add_argument('--dxf-sha256', required=True)
    parser.add_argument('--entity', choices=('SOLID', 'TRACE'), required=True)
    args = parser.parse_args()
    bound(args.dwg, args.dwg_sha256)
    bound(args.dxf, args.dxf_sha256)
    result = subprocess.run([args.dwgread, '-O', 'JSON', args.dwg], capture_output=True, check=True)
    data = json.loads(result.stdout)
    check(data['FILEHEADER']['version'] == 'AC1021', 'unexpected input version')
    faces = [o for o in data['OBJECTS'] if o.get('entity') == args.entity]
    check(len(faces) == 1, 'wrong DWG entity inventory')
    obj = faces[0]
    check(obj['elevation'] == 3.0 and obj['extrusion'] == [0., 1., 0.], 'wrong DWG elevation/normal')
    for i, point in enumerate(([1.,2.], [4.,2.], [1.,4.], [1.,4.]), 1):
        check(obj['corner'+str(i)] == point, 'wrong DWG corner')
    doc = ezdxf.readfile(args.dxf)
    check(doc.dxfversion == 'AC1027', 'unexpected output version')
    audit = doc.audit()
    check(not audit.errors and not audit.fixes, 'DXF audit is not clean')
    entities = [o for o in doc.modelspace() if o.dxftype() == args.entity]
    check(len(entities) == 1, 'wrong DXF entity inventory')
    entity = entities[0]
    check(tuple(entity.dxf.extrusion) == (0.,1.,0.), 'wrong DXF normal')
    expected = ((1.,2.,3.), (4.,2.,3.), (1.,4.,3.), (1.,4.,3.))
    world = ((-1.,3.,2.),(-4.,3.,2.),(-1.,3.,4.),(-1.,3.,4.))
    for i in range(4):
        v = entity.dxf.get('vtx'+str(i))
        check(tuple(v) == expected[i] and tuple(entity.ocs().to_wcs(v)) == world[i], 'wrong DXF corner')
    print(args.entity, 'triangle source DWG and AC1027 DXF geometry passed')


if __name__ == '__main__':
    main()
