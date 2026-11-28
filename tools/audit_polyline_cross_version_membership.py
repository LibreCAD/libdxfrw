#!/usr/bin/env python3
"""Check a hash-bound source/baseline/filtered POLYLINE membership experiment."""
import argparse
import hashlib
import json
from pathlib import Path
import tempfile

from bounded_process import run_bounded


def require(condition, message):
    if not condition:
        raise ValueError(message)


def digest(path):
    require(path.is_file() and path.stat().st_size <= 64 * 1024 * 1024, 'input absent or over budget')
    return hashlib.sha256(path.read_bytes()).hexdigest()


def graph(doc, revision, kind, duplicate):
    require(doc['FILEHEADER']['version'] == revision, 'DWG revision')
    objects=doc['OBJECTS']
    index={o['handle'][2]:o for o in objects if isinstance(o.get('handle'),list)}
    blocks=[o for o in objects if o.get('object')=='BLOCK_HEADER'
            and o.get('name','').casefold()=='*model_space']
    parents=[o for o in objects if o.get('entity')=='POLYLINE_'+kind.upper() and o.get('entmode')==2]
    require(len(blocks)==len(parents)==1, 'model block/parent count')
    parent=parents[0]; parent_id=parent['handle'][2]; refs=parent['vertex']; seq=parent['seqend']
    require(len(refs)==3 and all(r[0]==3 and r[2] in index for r in refs)
            and seq[0]==3 and seq[2] in index, 'parent hard-owned references')
    ids=[r[2] for r in refs]+[seq[2]]
    require(len(set(ids+[parent_id]))==5, 'distinct parent/children')
    expected=[parent_id]+(ids if duplicate else [])
    entries=blocks[0].get('entities',[])
    require(all(r[0]==3 for r in entries) and [r[2] for r in entries]==expected, 'block entity list')
    expected_points=((0,0,0),(5,0,1 if kind=='3d' else 0),(5,3,2 if kind=='3d' else 0))
    for ref, point in zip(refs, expected_points):
        child=index[ref[2]]
        require(child.get('entity')=='VERTEX_'+kind.upper() and child.get('entmode')==0
                and child['ownerhandle'][0]==4 and child['ownerhandle'][2]==parent_id
                and child['point']==list(point), 'vertex owner/type/point')
        if kind=='3d': require(child.get('flag')==32, '3D vertex flag')
    end=index[seq[2]]
    require(end.get('entity')=='SEQEND' and end.get('entmode')==0
            and end['ownerhandle'][0]==4 and end['ownerhandle'][2]==parent_id, 'SEQEND owner/type')
    return [{k:v for k,v in obj.items() if k!='index'} for obj in objects
            if obj.get('entity') in ('POLYLINE_2D','POLYLINE_3D','VERTEX_2D','VERTEX_3D','SEQEND')]


def output_geometry(path, kind, lost):
    import ezdxf
    require(ezdxf.__version__=='1.4.5', 'requires ezdxf 1.4.5')
    doc=ezdxf.readfile(path);audit=doc.audit()
    require(doc.dxfversion=='AC1027' and not audit.errors and not audit.fixes, 'DXF revision/audit')
    model=list(doc.modelspace());require(len(model)==1 and model[0].dxftype()=='POLYLINE','DXF inventory')
    polyline=model[0]; require(polyline.is_3d_polyline==(kind=='3d'),'DXF polyline subtype')
    expected=((0,0,0),(0,0,0)) if lost else ((0,0,0),(5,0,1 if kind=='3d' else 0),(5,3,2 if kind=='3d' else 0))
    require(len(polyline.vertices)==len(expected),'DXF vertex count')
    for vertex,point in zip(polyline.vertices,expected):
        require(tuple(vertex.dxf.location)==point,'DXF vertex coordinate')
        require(vertex.dxf.get('flags',0)==(32 if kind=='3d' else 0),'DXF vertex flag')


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--version',choices=('AC1018','AC1024'),required=True)
    p.add_argument('--mode',choices=('2d','3d'),required=True)
    p.add_argument('--dwgread',type=Path,required=True)
    for group in ('source','baseline','filtered'):
        for ext in ('dwg','dxf'):
            p.add_argument('--'+group+'-'+ext,type=Path,required=True)
            p.add_argument('--'+group+'-'+ext+'-sha256',required=True)
    a=p.parse_args();reader=a.dwgread.resolve(strict=True)
    v=run_bounded([str(reader),'--version'],5,4096)
    require(v.returncode==0 and v.stdout.strip()==b'dwgread 0.14','requires LibreDWG 0.14')
    peer={}
    for group in ('source','baseline','filtered'):
        dwg=getattr(a,group+'_dwg').resolve(strict=True); dxf=getattr(a,group+'_dxf').resolve(strict=True)
        dwgsha=getattr(a,group+'_dwg_sha256'); dxfsha=getattr(a,group+'_dxf_sha256')
        require(digest(dwg)==dwgsha and digest(dxf)==dxfsha,'input hash mismatch')
        with tempfile.TemporaryDirectory(prefix='polyline-cross-peer-') as tmp:
            out=Path(tmp)/'decoded.json'; result=run_bounded([str(reader),'-O','JSON','-o',str(out),str(dwg)],30,1024*1024)
            require(result.returncode==0 and out.is_file() and out.stat().st_size<=128*1024*1024,'DWG decode')
            peer[group]=graph(json.loads(out.read_text()),a.version,a.mode,group=='baseline')
        output_geometry(dxf,a.mode,group=='baseline')
        require(digest(dwg)==dwgsha and digest(dxf)==dxfsha,'input changed during audit')
    require(peer['baseline']==peer['filtered'],'filtered entity fields differ from baseline')
    print(a.version,a.mode,'producer, duplicated baseline loss and membership-only correction verified')


if __name__=='__main__':main()
