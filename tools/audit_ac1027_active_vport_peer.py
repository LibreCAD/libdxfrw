#!/usr/bin/env python3
"""Bounded AC1027 *ACTIVE VPORT/LINE observation; ODA lineage is a manual join."""
import argparse
import hashlib
import json
import math
import re
import tempfile
from pathlib import Path
from bounded_process import run_bounded
from audit_ac1027_material_peer import material_graph


def require(value, message):
    if not value:
        raise ValueError(message)


def digest(path):
    require(path.is_file() and path.stat().st_size <= 128 * 1024 * 1024, 'missing or oversized input')
    return hashlib.sha256(path.read_bytes()).hexdigest()


def validate_vport(doc):
    require(doc['FILEHEADER']['version'] == 'AC1027', 'wrong DWG revision')
    records = [v for v in doc['OBJECTS'] if v.get('object') == 'VPORT' and v.get('name', '').upper() == '*ACTIVE']
    require(len(records) == 1, 'expected one active VPORT')
    v = records[0]
    own = v.get('handle', []); owner = v.get('ownerhandle', [])
    require(len(own) >= 3 and own[0] == 0 and own[2] != 0, 'invalid self')
    require(len(owner) >= 3 and owner[0] == 4 and owner[2] != 0, 'invalid owner')
    controls = [x for x in doc['OBJECTS'] if x.get('object') == 'VPORT_CONTROL'
                and len(x.get('handle', [])) >= 3 and x['handle'][2] == v['ownerhandle'][2]]
    require(len(controls) == 1, 'missing VPORT control')
    edges = [e for e in controls[0].get('entries', []) if len(e) >= 3 and e[2] == v['handle'][2]]
    require(len(edges) == 1 and edges[0][0] == 2, 'invalid VPORT control edge')
    require(v.get('is_xdic_missing') == 1 and v.get('has_ds_data') == 0, 'unexpected common flags')
    for field, role in [('xref', 5), ('background', 4), ('visualstyle', 5), ('sun', 3), ('named_ucs', 5), ('base_ucs', 5)]:
        value = v.get(field, [])
        require(len(value) >= 3 and value[0] == role and value[2] == 0, 'unexpected '+field)
    return v


def validate_source_wire(page, offset):
    require(offset >= 0 and offset + 171 <= len(page), 'source page too short')
    bits = ''.join(f'{byte:08b}' for byte in page[offset:offset+171])
    expected = ''.join(f'{byte:08b}' for byte in bytes.fromhex('4108504050305050'))
    require(bits[1298] == '1' and bits[1299:1363] == expected
            and bits[1363:1368] == '00000', 'source VPORT marker/handles/padding differ')


def validate_metadata_join(native, diagnostic):
    require(len(native)==len(diagnostic) and len(native)>18
            and native[11]==0 and diagnostic[11]==6 and native[18]==diagnostic[18]==6
            and native[:11]==diagnostic[:11] and native[12:]==diagnostic[12:],
            'diagnostic metadata join differs beyond byte 0x0b')


def read_dwg(reader, path, destination):
    r = run_bounded([str(reader), '-v9', '-O', 'JSON', '-o', str(destination), str(path)], 30, 8*1024*1024)
    require(r.returncode == 0 and destination.is_file(), 'LibreDWG decode failed')
    d = json.loads(destination.read_text())
    trace = (r.stdout+r.stderr).decode('utf-8', 'replace')
    v = validate_vport(d)
    sections = re.split(r'(?m)^Add table record VPORT \[\d+\] Decode table record VPORT\s*$', trace)
    matches = [s for s in sections[1:] if re.search(r'(?mi)^name: "\*active" \[TU 2\]', s)]
    require(len(matches) == 1, 'missing physical VPORT trace')
    block = matches[0].split(' check_CRC', 1)
    require(len(block) == 2 and re.match(r' [^\n]*: ([0-9A-F]+) == \1', block[1]), 'VPORT CRC mismatch')
    return d, v


def near(a, b):
    return math.isclose(float(a), float(b), rel_tol=1e-10, abs_tol=1e-10)


def output_check(doc, v, path):
    import ezdxf
    d = ezdxf.readfile(path)
    report = d.audit()
    require(d.dxfversion == 'AC1027' and not report.errors and not report.fixes, 'DXF audit/revision mismatch')
    native_lines = [x for x in doc['OBJECTS'] if x.get('entity') == 'LINE' and x.get('entmode') == 2]
    layers = [x for x in doc['OBJECTS'] if x.get('object') == 'LAYER' and x.get('name') == '0']
    require(len(native_lines) == len(layers) == 1, 'unexpected native model inventory')
    native_line = native_lines[0]
    require(native_line.get('start') == [1.,2.,3.] and native_line.get('end') == [4.,5.,6.]
            and native_line.get('layer', [])[:1] == [5]
            and native_line['layer'][2] == layers[0]['handle'][2], 'native LINE differs')
    model = list(d.modelspace())
    require(len(model) == 1 and model[0].dxftype() == 'LINE'
            and int(model[0].dxf.handle,16) == native_line['handle'][2], 'unexpected model inventory')
    require(tuple(model[0].dxf.start) == (1.,2.,3.) and tuple(model[0].dxf.end) == (4.,5.,6.)
            and model[0].dxf.layer == '0', 'LINE differs')
    candidates = [x for x in d.viewports if x.dxf.name.upper() == '*ACTIVE']
    require(len(candidates) == 1, 'output active VPORT missing')
    out = candidates[0]
    require(int(out.dxf.handle,16) == v['handle'][2] and int(out.dxf.owner,16) == v['ownerhandle'][2], 'VPORT identity mismatch')
    scalar = [('height','VIEWSIZE'),('aspect_ratio','aspect_ratio'),('focal_length','LENSLENGTH'),
              ('front_clipping','FRONTZ'),('back_clipping','BACKZ'),('view_twist','VIEWTWIST'),
              ('snap_rotation','SNAPANG'),('elevation','ucs_elevation')]
    for dxf,dwg in scalar:
        require(near(out.dxf.get(dxf), v[dwg]), 'VPORT '+dxf+' differs')
    vectors = [('center','VIEWCTR'),('target','view_target'),('direction','VIEWDIR'),
               ('lower_left','lower_left'),('upper_right','upper_right'),('snap_base','SNAPBASE'),
               ('snap_spacing','SNAPUNIT'),('grid_spacing','GRIDUNIT'),('ucs_origin','ucsorg'),
               ('ucs_xaxis','ucsxdir'),('ucs_yaxis','ucsydir')]
    for dxf,dwg in vectors:
        a = out.dxf.get(dxf); b = v[dwg]
        require(all(near(a[i], b[i]) for i in range(len(b))), 'VPORT '+dxf+' differs')
    graph = material_graph(doc)
    dictionary = d.rootdict.get('ACAD_MATERIAL')
    require(dictionary is not None, 'output material dictionary missing')
    items = dict(dictionary.items())
    require(set(items) == {'ByBlock','ByLayer','Global'}, 'output material inventory differs')
    for name, material in graph.items():
        require(int(items[name].dxf.handle,16) == material['handle'][2], 'material identity differs')
    require(int(d.layers.get('0').dxf.material_handle,16) == graph['Global']['handle'][2], 'layer material differs')
    require(int(d.header['$CMATERIAL'],16) == graph['ByLayer']['handle'][2], 'current material differs')
    # ODA creates a visual-style target and changes the ambient ACI fallback in
    # this sample; neither is asserted identical to the DWG source slots.
    return len(model)


def main():
    p=argparse.ArgumentParser(description=__doc__)
    for label in ('source1','source2','native','generated','dxf','dwgread'):
        p.add_argument('--'+label, type=Path, required=True)
        p.add_argument('--'+label+'-sha256', required=True)
    p.add_argument('--source-page',type=Path,required=True)
    p.add_argument('--source-page-sha256',required=True)
    p.add_argument('--source-offset',type=int,required=True)
    args=p.parse_args()
    labels=('source1','source2','native','generated','dxf','dwgread','source_page')
    for name in labels:
        require(digest(getattr(args,name)) == getattr(args,name+'_sha256').lower(), name+' digest mismatch')
    validate_source_wire(args.source_page.read_bytes(),args.source_offset)
    native=args.native.read_bytes(); diagnostic=args.generated.read_bytes()
    validate_metadata_join(native, diagnostic)
    version=run_bounded([str(args.dwgread),'--version'],5,4096)
    require(version.returncode==0 and version.stdout.strip()==b'dwgread 0.14', 'wrong LibreDWG version')
    with tempfile.TemporaryDirectory(prefix='ac1027-vport-peer-') as tmp:
        data=[]
        for name in ('source1','source2','native','generated'):
            data.append(read_dwg(args.dwgread,getattr(args,name),Path(tmp)/(name+'.json')))
        for d,_ in data: material_graph(d)
        require(data[2][1] == data[3][1], 'native and diagnostic VPORT differ')
        output_check(data[3][0],data[3][1],args.dxf)
    for name in labels:
        require(digest(getattr(args,name)) == getattr(args,name+'_sha256').lower(), name+' changed')
    print('AC1027 source/generated VPORT roles and CRC, selected geometry and material graph: PASS; page-to-DWG and ODA lineage are external')


if __name__ == '__main__': main()
