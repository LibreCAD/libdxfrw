#!/usr/bin/env python3
"""Hash-bound LibreDWG and ODA-DXF audit for the sampled TEXT style witness."""
import argparse, hashlib, json, tempfile
from pathlib import Path
from bounded_process import run_bounded
from audit_ac1021_geometry_peer import close, require, check_graph

def digest(p):
    require(p.is_file() and p.stat().st_size < 64000000,'invalid input')
    return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--dwgread',type=Path,required=True); p.add_argument('--dwg',type=Path,required=True); p.add_argument('--dwg-sha256',required=True)
    p.add_argument('--dxf',type=Path,required=True); p.add_argument('--dxf-sha256',required=True)
    p.add_argument('--expected-style',choices=('CUSTOM','STANDARD'),required=True); p.add_argument('--generated-graph',action='store_true')
    a=p.parse_args(); reader=a.dwgread.resolve(strict=True); dwg=a.dwg.resolve(strict=True); dxf=a.dxf.resolve(strict=True)
    v=run_bounded([str(reader),'--version'],5,4096); require(v.returncode==0 and v.stdout.strip()==b'dwgread 0.14','LibreDWG version')
    require(digest(dwg)==a.dwg_sha256 and digest(dxf)==a.dxf_sha256,'input hash')
    with tempfile.TemporaryDirectory(prefix='text-style-peer-') as t:
        out=Path(t)/'doc.json'; run=run_bounded([str(reader),'-O','JSON','-o',str(out),str(dwg)],30,1048576)
        require(run.returncode==0 and out.is_file() and out.stat().st_size<128000000,'DWG decode')
        doc=json.loads(out.read_text())
    require(doc['FILEHEADER']['version']=='AC1021','DWG version')
    objs=doc['OBJECTS']; idx={o['handle'][2]:o for o in objs if isinstance(o.get('handle'),list)}
    if a.generated_graph: check_graph(doc)
    blocks=[o for o in objs if o.get('object')=='BLOCK_HEADER' and o.get('name','').lower()=='*model_space']; require(len(blocks)==1,'model block')
    texts=[o for o in objs if o.get('entity')=='TEXT' and o.get('entmode')==2];require(len(texts)==1,'TEXT count');text=texts[0]
    entries=blocks[0]['entities']; require(len(entries)==1 and entries[0][0]==3 and entries[0][2]==text['handle'][2],'model ownership')
    ref=text['style']; require(ref[0]==5 and ref[2] in idx and idx[ref[2]].get('object')=='STYLE' and idx[ref[2]].get('name','').casefold()==a.expected_style.casefold(),'style handle/name')
    require(any(o.get('object')=='STYLE' and o.get('name','').casefold()=='custom' and o.get('font_file')=='arial.ttf' for o in objs),'CUSTOM STYLE missing')
    require(text['text_value']=='HELLO' and text['generation']==2 and text['horiz_alignment']==2 and text['vert_alignment']==2,'text flags')
    for key,target in (('ins_pt',(3.,4.)),('alignment_pt',(6.,7.)),('extrusion',(0.,1.,0.)),('elevation',2.),('height',2.),('width_factor',1.25),('rotation',0.5235987755982988),('oblique_angle',0.2617993877991494)): close(text[key],target,'DWG '+key)
    import ezdxf
    require(ezdxf.__version__=='1.4.5','ezdxf version'); d=ezdxf.readfile(dxf); audit=d.audit()
    require(d.dxfversion=='AC1027' and not audit.errors and not audit.fixes,'DXF audit')
    items=list(d.modelspace()); require(len(items)==1 and items[0].dxftype()=='TEXT','DXF inventory'); t=items[0]
    require(t.dxf.text=='HELLO' and t.dxf.style.casefold()==a.expected_style.casefold() and t.dxf.text_generation_flag==2 and t.dxf.halign==2 and t.dxf.valign==2,'DXF text flags/style')
    for value,target in ((t.dxf.insert,(3.,4.,2.)),(t.dxf.align_point,(6.,7.,2.)),(t.dxf.extrusion,(0.,1.,0.)),(t.dxf.height,2.),(t.dxf.width,1.25),(t.dxf.rotation,30.),(t.dxf.oblique,15.),(t.ocs().to_wcs(t.dxf.align_point),(-6.,2.,7.))): close(value,target,'DXF geometry')
    if a.expected_style=='CUSTOM':require(d.styles.get(t.dxf.style).dxf.font=='arial.ttf','DXF font')
    require(digest(dwg)==a.dwg_sha256 and digest(dxf)==a.dxf_sha256,'input changed')
    print('AC1021 TEXT style',a.expected_style,'graph and AC1027 DXF fields PASS')
if __name__=='__main__': main()
