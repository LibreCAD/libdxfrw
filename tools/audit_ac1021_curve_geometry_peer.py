#!/usr/bin/env python3
"""Hash-bound peer inspection of the selected AC1021 curve matrix."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import tempfile

from audit_ac1021_geometry_peer import check_graph, close, require
from bounded_process import run_bounded

MODES = {'line':['LINE'], 'spline':['SPLINE'], 'hatch':['HATCH'],
         'polyline2d':['POLYLINE_2D'], 'polyline3d':['POLYLINE_3D'],
         'mixed':['LINE','SPLINE','HATCH','POLYLINE_2D']}
POINTS2 = ((0.,0.,0.),(5.,0.,0.),(5.,3.,0.))
POINTS3 = ((0.,0.,0.),(5.,0.,1.),(5.,3.,2.))
RECT = ((0.,0.),(5.,0.),(5.,3.),(0.,3.))
KNOTS = (0.,0.,0.,1.,1.,1.)
CONTROL = ((0.,0.,0.),(2.,4.,0.),(5.,0.,0.))


def digest(path):
    require(path.is_file() and path.stat().st_size <= 64 * 1024 * 1024, 'input absent or over budget')
    return hashlib.sha256(path.read_bytes()).hexdigest()


def check_dwg_entity(obj, objects, role):
    kind = obj['entity']; handle=obj['handle'][2]
    require(obj.get('entmode') == 2 and obj['layer'][0] == 5 and obj['layer'][2] in objects
            and objects[obj['layer'][2]].get('name') == '0', kind + ' model-space layer')
    if kind == 'LINE':
        close(obj['start'],(1.,2.,3.),'line start'); close(obj['end'],(4.,5.,6.),'line end')
    elif kind == 'SPLINE':
        require(obj['scenario']==1 and obj['degree']==2 and obj['rational']==0
                and obj.get('closed_b',0)==0 and obj.get('periodic',0)==0 and obj['weighted']==0,'spline profile')
        close(obj['knots'], KNOTS, 'spline knots')
        points=obj['ctrl_pts']; require(len(points)==3,'spline control count')
        for point, expected in zip(points, CONTROL):
            close((point['x'],point['y'],point['z']),expected,'spline control')
    elif kind == 'HATCH':
        require(obj['name']=='SOLID' and obj['is_solid_fill']==1 and obj['is_associative']==0,'hatch flags')
        paths=obj['paths']; require(len(paths)==1 and paths[0]['flag'] & 2
            and paths[0]['closed']==1 and not paths[0].get('boundary_handles',[]),'hatch polyline path')
        vertices=paths[0]['polyline_paths']; require(len(vertices)==4,'hatch vertex count')
        for point, expected in zip(vertices,RECT): close(point['point'],expected,'hatch OCS boundary')
        close(obj.get('extrusion',(0.,0.,1.)),(0.,0.,1.),'hatch normal')
    else:
        require(kind in ('POLYLINE_2D','POLYLINE_3D'),'unexpected entity')
        vertices=obj['vertex']; require(len(vertices)==3 and len({v[2] for v in vertices})==3,'polyline vertices')
        expected=POINTS2 if kind=='POLYLINE_2D' else POINTS3
        for ref, point in zip(vertices,expected):
            require(ref[0]==role and ref[2] in objects,'polyline vertex handle role/target')
            child=objects[ref[2]]
            require(child['entity']==('VERTEX_2D' if kind=='POLYLINE_2D' else 'VERTEX_3D')
                and child.get('entmode')==0 and child['ownerhandle'][0]==4
                and child['ownerhandle'][2]==handle,'polyline child ownership')
            close(child['point'],point,'polyline child point')
        ref=obj['seqend']; require(ref[0]==role and ref[2] in objects,'polyline SEQEND role/target')
        end=objects[ref[2]]; require(end['entity']=='SEQEND' and end.get('entmode')==0
            and end['ownerhandle'][0]==4 and end['ownerhandle'][2]==handle,'polyline SEQEND ownership')


def check_dxf_entity(obj, loss):
    kind=obj.dxftype(); require(obj.dxf.layer=='0',kind+' DXF layer')
    if kind=='LINE':
        close(obj.dxf.start,(1.,2.,3.),'line start');close(obj.dxf.end,(4.,5.,6.),'line end')
    elif kind=='SPLINE':
        require(obj.dxf.degree==2 and not obj.dxf.get('flags',0)&7,'spline degree/flags')
        close(tuple(obj.knots),KNOTS,'spline knots'); points=list(obj.control_points)
        require(len(points)==3,'spline control count')
        for point,expected in zip(points,CONTROL):close(point,expected,'spline control')
    elif kind=='HATCH':
        require(obj.dxf.solid_fill==1 and obj.dxf.associative==0 and len(obj.paths)==1,'hatch flags')
        path=obj.paths[0]; require(path.is_closed and path.path_type_flags & 2 and len(path.vertices)==4,'hatch boundary')
        for vertex,expected in zip(path.vertices,RECT):
            close(vertex[:2],expected,'hatch OCS boundary');close(vertex[2],0.,'hatch bulge')
        close(obj.dxf.get('extrusion',(0.,0.,1.)),(0.,0.,1.),'hatch normal')
    else:
        require(kind=='POLYLINE','unexpected DXF entity')
        expected = ((0.,0.,0.),(0.,0.,0.)) if loss else (POINTS3 if obj.is_3d_polyline else POINTS2)
        require(len(obj.vertices)==len(expected),'polyline DXF vertex count')
        for vertex,point in zip(obj.vertices,expected): close(vertex.dxf.location,point,'polyline DXF vertex')


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--mode',choices=MODES,required=True)
    parser.add_argument('--dwgread',type=Path,required=True)
    parser.add_argument('--dwg',type=Path,required=True);parser.add_argument('--dwg-sha256',required=True)
    parser.add_argument('--dxf',type=Path);parser.add_argument('--dxf-sha256')
    parser.add_argument('--polyline-role',type=int,choices=(3,5),required=True)
    parser.add_argument('--expect-observed-2d-loss',action='store_true')
    args=parser.parse_args()
    require((args.dxf is None)==(args.dxf_sha256 is None),'DXF and hash must appear together')
    require(not args.expect_observed_2d_loss or (args.dxf is not None and args.mode in ('polyline2d','mixed')),'loss mode')
    reader,dwg=(p.resolve(strict=True) for p in (args.dwgread,args.dwg))
    v=run_bounded([str(reader),'--version'],5,4096)
    require(v.returncode==0 and v.stdout.strip()==b'dwgread 0.14','requires LibreDWG 0.14')
    require(digest(dwg)==args.dwg_sha256,'DWG hash mismatch')
    with tempfile.TemporaryDirectory(prefix='ac1021-curve-peer-') as temp:
        out=Path(temp)/'decoded.json'; result=run_bounded([str(reader),'-O','JSON','-o',str(out),str(dwg)],30,1024*1024)
        require(result.returncode==0 and out.is_file() and out.stat().st_size<=128*1024*1024,'DWG decode')
        doc=json.loads(out.read_text())
    require(doc.get('FILEHEADER',{}).get('version')=='AC1021','DWG revision')
    objects=check_graph(doc); entities=[o for o in doc['OBJECTS'] if o.get('entity') and o.get('entmode')==2
                                        and o['entity'] not in ('BLOCK','ENDBLK')]
    require([o['entity'] for o in entities]==MODES[args.mode],'DWG inventory/order')
    for entity in entities:check_dwg_entity(entity,objects,args.polyline_role)
    if args.dxf is not None:
        import ezdxf
        require(ezdxf.__version__=='1.4.5','requires ezdxf 1.4.5')
        dxf=args.dxf.resolve(strict=True);require(digest(dxf)==args.dxf_sha256,'DXF hash mismatch')
        doc=ezdxf.readfile(dxf);audit=doc.audit()
        require(doc.dxfversion=='AC1027' and not audit.errors and not audit.fixes,'DXF revision/audit')
        entities=list(doc.modelspace());expected=['POLYLINE' if x.startswith('POLYLINE_') else x for x in MODES[args.mode]]
        require([x.dxftype() for x in entities]==expected,'DXF inventory/order')
        for entity in entities:check_dxf_entity(entity,args.expect_observed_2d_loss)
        require(digest(dxf)==args.dxf_sha256,'DXF changed during audit')
    require(digest(dwg)==args.dwg_sha256,'DWG changed during audit')
    print('AC1021 '+args.mode+' peer checks PASS' + ('; expected 2D DXF geometry LOSS observed' if args.expect_observed_2d_loss else ''))


if __name__=='__main__':main()
