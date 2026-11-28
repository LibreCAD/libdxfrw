#!/usr/bin/env python3
"""Verify hash-bound AC1018/AC1024 paper/named POLYLINE graph and DXF."""
import argparse
import hashlib
import json
from pathlib import Path
import tempfile

from audit_ac1021_geometry_peer import close, require
from bounded_process import run_bounded


def digest(path):
    require(path.is_file() and path.stat().st_size <= 64 * 1024 * 1024, 'input missing or over budget')
    return hashlib.sha256(path.read_bytes()).hexdigest()


def verify_graph(doc, mode, version):
    require(doc.get('FILEHEADER',{}).get('version')==version,'DWG revision')
    handles=[o for o in doc['OBJECTS'] if isinstance(o.get('handle'),list) and len(o['handle'])>2]
    objects={o['handle'][2]:o for o in handles}; require(len(objects)==len(handles), 'duplicate object handle')
    named=mode.startswith('named'); is3d=mode.endswith('3d')
    blockname='owner_test' if named else '*paper_space'
    blocks=[o for o in doc['OBJECTS'] if o.get('object')=='BLOCK_HEADER'
            and o.get('name','').casefold()==blockname]
    require(len(blocks)==1,'target block missing or duplicated')
    block=blocks[0]; entries=block.get('entities',[])
    require(len(entries)==3 and all(e[0]==3 and e[2] in objects for e in entries),'block entry roles')
    members=[objects[e[2]] for e in entries]
    require([m.get('entity') for m in members]==['LINE','POLYLINE_3D' if is3d else 'POLYLINE_2D','LINE'],
            'target block member order')
    for entity,start,end in ((members[0],(-3.,-1.,0.),(-2.,-1.,0.)),(members[2],(7.,4.,0.),(8.,4.,0.))):
        close(entity['start'],start,'line start');close(entity['end'],end,'line end')
    parent=members[1]; parent_id=parent['handle'][2]
    require(parent.get('entmode')==(0 if named else 1),'parent space')
    if named:
        require(parent['ownerhandle'][0]==4 and parent['ownerhandle'][2]==block['handle'][2], 'named parent owner')
    refs=parent['vertex'];seq=parent['seqend']
    require(len(refs)==3 and all(ref[0]==3 and ref[2] in objects for ref in refs)
            and seq[0]==3 and seq[2] in objects,'owned child refs')
    require(len({parent_id,seq[2],*(r[2] for r in refs)})==5,'duplicate child')
    expected=((0.,0.,0.),(5.,0.,1. if is3d else 0.),(5.,3.,2. if is3d else 0.))
    for ref,point in zip(refs,expected):
        child=objects[ref[2]]
        require(child.get('entity')==('VERTEX_3D' if is3d else 'VERTEX_2D')
                and child.get('entmode')==0 and child['ownerhandle'][0]==4
                and child['ownerhandle'][2]==parent_id,'child owner/type')
        close(child['point'],point,'child coordinate')
        if is3d:require(child['flag']==32,'3D child flags')
    end=objects[seq[2]]
    require(end.get('entity')=='SEQEND' and end.get('entmode')==0
            and end['ownerhandle'][0]==4 and end['ownerhandle'][2]==parent_id,'SEQEND owner')


def verify_dxf(path,mode):
    import ezdxf
    require(ezdxf.__version__=='1.4.5','requires ezdxf 1.4.5')
    doc=ezdxf.readfile(path);audit=doc.audit()
    require(doc.dxfversion=='AC1027' and not audit.errors and not audit.fixes,'DXF revision/audit')
    named=mode.startswith('named');is3d=mode.endswith('3d')
    target=doc.blocks.get('OWNER_TEST') if named else doc.layout('Layout1')
    members=list(target)
    require([m.dxftype() for m in members]==['LINE','POLYLINE','LINE'],'DXF target member order')
    for e,start,end in ((members[0],(-3.,-1.,0.),(-2.,-1.,0.)),(members[2],(7.,4.,0.),(8.,4.,0.))):
        close(e.dxf.start,start,'DXF line start');close(e.dxf.end,end,'DXF line end')
    poly=members[1];require(poly.is_3d_polyline==is3d and len(poly.vertices)==3,'DXF polyline subtype/count')
    expected=((0.,0.,0.),(5.,0.,1. if is3d else 0.),(5.,3.,2. if is3d else 0.))
    for vertex,point in zip(poly.vertices,expected):
        close(vertex.dxf.location,point,'DXF vertex')
        require(vertex.dxf.get('flags',0)==(32 if is3d else 0),'DXF vertex flags')
    model=list(doc.modelspace())
    if named:
        require(len(model)==1 and model[0].dxftype()=='INSERT' and model[0].dxf.name=='OWNER_TEST','model insert')
        close(model[0].dxf.insert,(10.,20.,0.),'insert position')
    else:
        require(len(model)==1 and model[0].dxftype()=='LINE','model control line')
        close(model[0].dxf.start,(10.,20.,0.),'model line start')
        close(model[0].dxf.end,(11.,21.,0.),'model line end')


def block_graph(doc, version, mode):
    require(doc['FILEHEADER']['version'] == version, 'revision')
    entries = [o for o in doc['OBJECTS'] if isinstance(o.get('handle'), list) and len(o['handle']) > 2]
    objects = {o['handle'][2]: o for o in entries}
    require(len(objects) == len(entries), 'duplicate handles')
    name = 'owner_test' if mode.startswith('named') else '*paper_space'
    block = [o for o in doc['OBJECTS'] if o.get('object') == 'BLOCK_HEADER' and o.get('name', '').casefold() == name]
    require(len(block) == 1, 'block identity')
    handles = block[0]['entities']
    require(all(e[0] == 3 and e[2] in objects for e in handles), 'block entity references')
    return objects, [e[2] for e in handles]


def compare_graphs(baseline, candidate, version, mode):
    old, old_ids = block_graph(baseline, version, mode)
    new, new_ids = block_graph(candidate, version, mode)
    require(len(old_ids) == 7 and len(new_ids) == 3 and [old_ids[0],old_ids[1],old_ids[-1]] == new_ids, 'membership')
    is3d = mode.endswith('3d')
    kinds = ['LINE','POLYLINE_3D' if is3d else 'POLYLINE_2D'] + ['VERTEX_3D' if is3d else 'VERTEX_2D'] * 3 + ['SEQEND','LINE']
    require([old[x].get('entity') for x in old_ids] == kinds, 'baseline members')
    for identity in old_ids:
        a, c = dict(old[identity]), dict(new[identity])
        a.pop('index', None); c.pop('index', None)
        require(a == c, 'entity frame changed')


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--version',choices=('AC1018','AC1024'),required=True)
    p.add_argument('--mode',choices=('paper2d','paper3d','named2d','named3d'),required=True)
    p.add_argument('--dwgread',type=Path,required=True)
    p.add_argument('--dwg',type=Path,required=True);p.add_argument('--dwg-sha256',required=True)
    p.add_argument('--dxf',type=Path,required=True);p.add_argument('--dxf-sha256',required=True)
    p.add_argument('--baseline-dwg',type=Path);p.add_argument('--baseline-dwg-sha256')
    p.add_argument('--baseline-dxf',type=Path);p.add_argument('--baseline-dxf-sha256')
    a=p.parse_args();reader,dwg,dxf=(x.resolve(strict=True) for x in (a.dwgread,a.dwg,a.dxf))
    version=run_bounded([str(reader),'--version'],5,4096)
    require(version.returncode==0 and version.stdout.strip()==b'dwgread 0.14','requires LibreDWG 0.14')
    require(digest(dwg)==a.dwg_sha256 and digest(dxf)==a.dxf_sha256,'input hash mismatch')
    with tempfile.TemporaryDirectory(prefix='polyline-owners-peer-') as tmp:
        out=Path(tmp)/'decoded.json'; result=run_bounded([str(reader),'-O','JSON','-o',str(out),str(dwg)],30,1024*1024)
        require(result.returncode==0 and out.is_file() and out.stat().st_size<=128*1024*1024,'DWG decode')
        candidate = json.loads(out.read_text())
        verify_graph(candidate,a.mode,a.version)
        baseline_args = (a.baseline_dwg,a.baseline_dwg_sha256,a.baseline_dxf,a.baseline_dxf_sha256)
        require(all(baseline_args) or not any(baseline_args), 'baseline arguments must be supplied together')
        if all(baseline_args):
            baseline_dwg=a.baseline_dwg.resolve(strict=True); baseline_dxf=a.baseline_dxf.resolve(strict=True)
            require(digest(baseline_dwg)==a.baseline_dwg_sha256 and digest(baseline_dxf)==a.baseline_dxf_sha256, 'baseline hash mismatch')
            baseline_out=Path(tmp)/'baseline.json'
            baseline_result=run_bounded([str(reader),'-O','JSON','-o',str(baseline_out),str(baseline_dwg)],30,1024*1024)
            require(baseline_result.returncode==0 and baseline_out.is_file() and baseline_out.stat().st_size<=128*1024*1024,'baseline decode')
            compare_graphs(json.loads(baseline_out.read_text()),candidate,a.version,a.mode)
            import ezdxf
            original=ezdxf.readfile(baseline_dxf); target=original.blocks.get('OWNER_TEST') if a.mode.startswith('named') else original.layout('Layout1')
            polys=[entity for entity in target if entity.dxftype()=='POLYLINE']
            require(len(polys)==1 and len(polys[0].vertices)==2 and all(tuple(v.dxf.location)==(0.,0.,0.) for v in polys[0].vertices),'baseline vertex loss')
            require(digest(baseline_dwg)==a.baseline_dwg_sha256 and digest(baseline_dxf)==a.baseline_dxf_sha256,'baseline changed')
    verify_dxf(dxf,a.mode)
    require(digest(dwg)==a.dwg_sha256 and digest(dxf)==a.dxf_sha256,'input changed')
    print(a.version,a.mode,'peer block/child ownership and AC1027 DXF geometry PASS')


if __name__=='__main__':main()
