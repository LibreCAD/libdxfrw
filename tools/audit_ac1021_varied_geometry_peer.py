#!/usr/bin/env python3
"""Hash-bound LibreDWG and ezdxf checks for the selected AC1021 varied matrix."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import tempfile

from audit_ac1021_geometry_peer import check_graph, close, require
from bounded_process import run_bounded

MODES = {'line': ['LINE'], 'ellipse': ['ELLIPSE'], 'lwpolyline': ['LWPOLYLINE'],
         'text': ['TEXT'], 'mixed': ['LINE', 'ELLIPSE', 'LWPOLYLINE', 'TEXT']}


def digest(path):
    require(path.is_file() and path.stat().st_size <= 64 * 1024 * 1024, 'input absent or over budget')
    return hashlib.sha256(path.read_bytes()).hexdigest()


def check_dwg_entity(obj, objects):
    kind = obj['entity']
    require(obj.get('entmode') == 2 and obj['layer'][0] == 5 and obj['layer'][2] in objects
            and objects[obj['layer'][2]].get('name') == '0', kind + ' model-space layer')
    if kind == 'LINE':
        close(obj['start'], (1., 2., 3.), 'line start'); close(obj['end'], (4., 5., 6.), 'line end')
    elif kind == 'ELLIPSE':
        close(obj['center'], (2., 3., 4.), 'ellipse WCS center')
        close(obj['sm_axis'], (4., 0., 0.), 'ellipse WCS major axis')
        close(obj['extrusion'], (0., 0., 1.), 'ellipse normal')
        for name, value in [('axis_ratio', .5), ('start_angle', .2), ('end_angle', 2.4)]:
            close(obj[name], value, 'ellipse ' + name)
    elif kind == 'LWPOLYLINE':
        require(len(obj['points']) == 3 and obj.get('flag', 0) & 0x200 == 0, 'polyline count/closure')
        for value, expected in zip(obj['points'], ((0., 0.), (5., 0.), (5., 3.))):
            close(value, expected, 'polyline OCS vertex')
        close(obj.get('elevation', 0.), 2., 'polyline elevation')
        require(not obj.get('bulges'), 'unexpected bulges')
    elif kind == 'TEXT':
        close(obj['ins_pt'], (3., 4.), 'text insertion')
        close(obj['height'], 2., 'text height')
        close(obj['rotation'], math.pi / 6, 'text rotation')
        close(obj.get('extrusion', (0., 0., 1.)), (0., 0., 1.), 'text normal')
        for key, expected in [('elevation', 0.), ('width_factor', 1.), ('oblique_angle', 0.)]:
            close(obj.get(key, expected), expected, 'text ' + key)
        require(obj['text_value'] == 'HELLO' and obj.get('horiz_alignment', 0) == 0
                and obj.get('vert_alignment', 0) == 0 and obj.get('generation', 0) == 0,
                'text content/alignment')
        style = obj['style']; require(style[0] == 5 and style[2] in objects
                                      and objects[style[2]].get('name', '').casefold() == 'standard', 'text style')


def check_dxf_entity(obj):
    kind = obj.dxftype(); require(obj.dxf.layer == '0', kind + ' DXF layer')
    if kind == 'LINE':
        close(obj.dxf.start, (1., 2., 3.), 'line start'); close(obj.dxf.end, (4., 5., 6.), 'line end')
    elif kind == 'ELLIPSE':
        close(obj.dxf.center, (2., 3., 4.), 'ellipse WCS center')
        close(obj.dxf.major_axis, (4., 0., 0.), 'ellipse WCS axis')
        close(obj.dxf.get('extrusion', (0., 0., 1.)), (0., 0., 1.), 'ellipse normal')
        for name, expected in [('ratio', .5), ('start_param', .2), ('end_param', 2.4)]:
            close(obj.dxf.get(name), expected, 'ellipse ' + name)
        for angle, point in zip((.2, 2.4), obj.vertices((.2, 2.4))):
            close(point, (2. + 4. * math.cos(angle), 3. + 2. * math.sin(angle), 4.), 'ellipse WCS endpoint')
    elif kind == 'LWPOLYLINE':
        require(not obj.closed, 'unexpected polyline closure')
        vertices = list(obj.get_points('xyseb')); require(len(vertices) == 3, 'polyline count')
        for actual, expected in zip(vertices, ((0.,0.,0.,0.,0.),(5.,0.,0.,0.,0.),(5.,3.,0.,0.,0.))):
            close(actual, expected, 'polyline OCS vertex/width/bulge')
        close(obj.dxf.get('elevation', 0.), 2., 'polyline elevation')
        close(obj.dxf.get('extrusion', (0.,0.,1.)), (0.,0.,1.), 'polyline normal')
        for actual, expected in zip(obj.vertices_in_wcs(), ((0.,0.,2.),(5.,0.,2.),(5.,3.,2.))):
            close(actual, expected, 'polyline WCS vertex')
    elif kind == 'TEXT':
        close(obj.dxf.insert, (3.,4.,0.), 'text insertion')
        close(obj.dxf.height, 2., 'text height'); close(obj.dxf.get('rotation', 0.), 30., 'text angle')
        require(obj.dxf.text == 'HELLO' and obj.dxf.get('style', 'STANDARD').casefold() == 'standard'
                and obj.dxf.get('halign', 0) == 0 and obj.dxf.get('valign', 0) == 0
                and obj.dxf.get('text_generation_flag', 0) == 0, 'text content/style/alignment')
        close(obj.ocs().to_wcs(obj.dxf.insert), (3.,4.,0.), 'text WCS insertion')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--mode', choices=MODES, required=True)
    parser.add_argument('--dwgread', type=Path, required=True)
    parser.add_argument('--dwg', type=Path, required=True); parser.add_argument('--dwg-sha256', required=True)
    parser.add_argument('--dxf', type=Path, required=True); parser.add_argument('--dxf-sha256', required=True)
    args = parser.parse_args()
    reader, dwg, dxf = (p.resolve(strict=True) for p in (args.dwgread, args.dwg, args.dxf))
    version = run_bounded([str(reader), '--version'], 5, 4096)
    require(version.returncode == 0 and version.stdout.strip() == b'dwgread 0.14', 'requires LibreDWG 0.14')
    require(digest(dwg) == args.dwg_sha256 and digest(dxf) == args.dxf_sha256, 'input hash mismatch')
    with tempfile.TemporaryDirectory(prefix='ac1021-varied-peer-') as tmp:
        output = Path(tmp) / 'decoded.json'
        result = run_bounded([str(reader), '-O', 'JSON', '-o', str(output), str(dwg)], 30, 1024 * 1024)
        require(result.returncode == 0 and output.is_file() and output.stat().st_size <= 128 * 1024 * 1024, 'DWG decode failed')
        doc = json.loads(output.read_text())
    require(doc.get('FILEHEADER', {}).get('version') == 'AC1021', 'DWG revision')
    objects = check_graph(doc)
    entities = [o for o in doc['OBJECTS'] if o.get('entity') and o.get('entmode') == 2
                and o['entity'] not in ('BLOCK', 'ENDBLK')]
    require([o['entity'] for o in entities] == MODES[args.mode], 'DWG entity inventory/order')
    for obj in entities: check_dwg_entity(obj, objects)
    import ezdxf
    require(ezdxf.__version__ == '1.4.5', 'requires ezdxf 1.4.5')
    doc = ezdxf.readfile(dxf); audit = doc.audit()
    require(doc.dxfversion == 'AC1027' and not audit.errors and not audit.fixes, 'DXF revision/audit')
    entities = list(doc.modelspace()); require([o.dxftype() for o in entities] == MODES[args.mode], 'DXF entity inventory/order')
    for obj in entities: check_dxf_entity(obj)
    require(digest(dwg) == args.dwg_sha256 and digest(dxf) == args.dxf_sha256, 'input changed during audit')
    print('AC1021 ' + args.mode + ' peer DWG graph/geometry and AC1027 DXF geometry PASS')


if __name__ == '__main__':
    main()
