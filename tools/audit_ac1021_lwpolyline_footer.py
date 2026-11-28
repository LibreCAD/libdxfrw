#!/usr/bin/env python3
"""Hash-bound peer graph and geometry checks for AC1021 LWPOLYLINE controls."""
import argparse
import hashlib
import json
from pathlib import Path
import tempfile

from audit_ac1021_geometry_peer import check_graph, close, require
from bounded_process import run_bounded

MODES = ('bulge', 'width', 'closed', 'tilt', 'rich', 'constant')


def digest(path):
    require(path.is_file() and path.stat().st_size <= 64 * 1024 * 1024, 'file missing or too large')
    return hashlib.sha256(path.read_bytes()).hexdigest()


def values(mode):
    bulges = [.25, -.5, 0.] if mode in ('bulge', 'rich') else [0., 0., 0.]
    widths = [(.2, .4), (.4, .1), (0., 0.)] if mode in ('width', 'rich') else [(0., 0.)] * 3
    tilted = mode in ('tilt', 'rich')
    closed = mode in ('closed', 'rich')
    return bulges, widths, tilted, closed


def check_dwg_polyline(obj, indexed, mode):
    require(obj.get('entmode') == 2 and obj['layer'][0] == 5 and obj['layer'][2] in indexed
            and indexed[obj['layer'][2]].get('name') == '0', 'model-space layer')
    bulges, widths, tilted, closed = values(mode)
    require(bool(obj['flag'] & 0x200) == closed and bool(obj['flag'] & 0x100) == closed, 'DWG closure/plinegen')
    require(len(obj['points']) == 3, 'DWG vertex count')
    for actual, expected in zip(obj['points'], ((0., 0.), (5., 0.), (5., 3.))): close(actual, expected, 'DWG point')
    actual_bulges = list(obj.get('bulges', [])); actual_bulges.extend([0.] * (3-len(actual_bulges)))
    require(len(actual_bulges) == 3, 'DWG bulge count'); close(actual_bulges, tuple(bulges), 'DWG bulges')
    actual_widths = [(v['start'], v['end']) for v in obj.get('widths', [])]
    actual_widths.extend([(0., 0.)] * (3-len(actual_widths)))
    require(len(actual_widths) == 3, 'DWG width count')
    for actual, expected in zip(actual_widths, widths): close(actual, expected, 'DWG widths')
    close(obj.get('elevation', 0.), 2. if tilted else 0., 'DWG elevation')
    close(obj.get('thickness', 0.), .5 if tilted else 0., 'DWG thickness')
    close(obj.get('extrusion', (0., 0., 1.)), (0., 1., 0.) if tilted else (0., 0., 1.), 'DWG extrusion')
    close(obj.get('const_width', 0.), .6 if mode == 'constant' else 0., 'DWG constant width')


def check_dxf_polyline(entity, mode):
    bulges, widths, tilted, closed = values(mode)
    require(entity.dxf.layer == '0', 'DXF layer')
    require(bool(entity.dxf.get('flags', 0) & 1) == closed
            and bool(entity.dxf.get('flags', 0) & 128) == closed, 'DXF closure/plinegen')
    actual = list(entity.get_points('xyseb')); require(len(actual) == 3, 'DXF vertex count')
    for i, point in enumerate(((0., 0.), (5., 0.), (5., 3.))):
        close(actual[i], (*point, *widths[i], bulges[i]), 'DXF vertex')
    close(entity.dxf.get('elevation', 0.), 2. if tilted else 0., 'DXF elevation')
    close(entity.dxf.get('thickness', 0.), .5 if tilted else 0., 'DXF thickness')
    close(entity.dxf.get('extrusion', (0., 0., 1.)), (0., 1., 0.) if tilted else (0., 0., 1.), 'DXF extrusion')
    close(entity.dxf.get('const_width', 0.), .6 if mode == 'constant' else 0., 'DXF constant width')
    expected = ((0., 2., 0.), (-5., 2., 0.), (-5., 2., 3.)) if tilted else ((0., 0., 0.), (5., 0., 0.), (5., 3., 0.))
    for actual, point in zip(entity.vertices_in_wcs(), expected): close(actual, point, 'DXF WCS vertex')


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--mode', required=True, choices=MODES)
    p.add_argument('--dwgread', required=True, type=Path)
    p.add_argument('--dwg', required=True, type=Path)
    p.add_argument('--dwg-sha256', required=True)
    p.add_argument('--dxf', required=True, type=Path)
    p.add_argument('--dxf-sha256', required=True)
    a = p.parse_args(); dwg = a.dwg.resolve(strict=True); dxf = a.dxf.resolve(strict=True)
    reader = a.dwgread.resolve(strict=True)
    version = run_bounded([str(reader), '--version'], 5, 4096)
    require(version.returncode == 0 and version.stdout.strip() == b'dwgread 0.14', 'requires LibreDWG 0.14')
    require(digest(dwg) == a.dwg_sha256 and digest(dxf) == a.dxf_sha256, 'hash mismatch')
    with tempfile.TemporaryDirectory(prefix='lwpoly-footer-peer-') as tmp:
        out = Path(tmp) / 'decoded.json'
        result = run_bounded([str(reader), '-O', 'JSON', '-o', str(out), str(dwg)], 30, 1024 * 1024)
        require(result.returncode == 0 and out.is_file() and out.stat().st_size <= 128 * 1024 * 1024, 'decode failed')
        document = json.loads(out.read_bytes())
    require(document['FILEHEADER']['version'] == 'AC1021', 'wrong DWG version')
    indexed = check_graph(document)
    entities = [o for o in document['OBJECTS'] if o.get('entity') == 'LWPOLYLINE']
    require(len(entities) == 1, 'polyline count'); obj = entities[0]
    check_dwg_polyline(obj, indexed, a.mode)
    import ezdxf
    require(ezdxf.__version__ == '1.4.5', 'requires ezdxf 1.4.5')
    doc = ezdxf.readfile(dxf); audit = doc.audit()
    require(doc.dxfversion == 'AC1027' and not audit.errors and not audit.fixes, 'DXF version/audit')
    entities = list(doc.modelspace()); require(len(entities) == 1 and entities[0].dxftype() == 'LWPOLYLINE', 'DXF entity count')
    check_dxf_polyline(entities[0], a.mode)
    print('AC1021', a.mode, 'DWG graph and AC1027 DXF geometry PASS')


if __name__ == '__main__':
    main()
