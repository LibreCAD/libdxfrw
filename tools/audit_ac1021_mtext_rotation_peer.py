#!/usr/bin/env python3
"""Hash-bound LibreDWG and ODA-DXF audit for sampled MTEXT WCS directions."""
import argparse, hashlib, json, math, tempfile
from pathlib import Path
from bounded_process import run_bounded
from audit_ac1021_geometry_peer import close, require, check_graph

def digest(p):
    require(p.is_file() and p.stat().st_size < 64000000,'invalid input')
    return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--dwgread',type=Path,required=True);p.add_argument('--dwg',type=Path,required=True);p.add_argument('--dwg-sha256',required=True)
    p.add_argument('--dxf',type=Path,required=True);p.add_argument('--dxf-sha256',required=True)
    p.add_argument('--mode',choices=('default','tilt','negative','direction','baseline'),required=True);p.add_argument('--generated-graph',action='store_true')
    a=p.parse_args();reader=a.dwgread.resolve(strict=True);dwg=a.dwg.resolve(strict=True);dxf=a.dxf.resolve(strict=True)
    ver=run_bounded([str(reader),'--version'],5,4096);require(ver.returncode==0 and ver.stdout.strip()==b'dwgread 0.14','LibreDWG version')
    require(digest(dwg)==a.dwg_sha256 and digest(dxf)==a.dxf_sha256,'input hash')
    with tempfile.TemporaryDirectory(prefix='mtext-rotation-peer-') as tmp:
        out=Path(tmp)/'doc.json';run=run_bounded([str(reader),'-O','JSON','-o',str(out),str(dwg)],30,1048576)
        require(run.returncode==0 and out.is_file() and out.stat().st_size<128000000,'DWG decode');doc=json.loads(out.read_text())
    require(doc['FILEHEADER']['version']=='AC1021','DWG version')
    objs=doc['OBJECTS'];idx={o['handle'][2]:o for o in objs if isinstance(o.get('handle'),list)}
    if a.generated_graph:check_graph(doc)
    blocks=[o for o in objs if o.get('object')=='BLOCK_HEADER' and o.get('name','').lower()=='*model_space'];require(len(blocks)==1,'model block')
    texts=[o for o in objs if o.get('entity')=='MTEXT' and o.get('entmode')==2];require(len(texts)==1,'MTEXT count');text=texts[0]
    entries=blocks[0]['entities'];require(len(entries)==1 and entries[0][0]==3 and entries[0][2]==text['handle'][2],'model ownership')
    ref=text['style'];require(ref[0]==5 and ref[2] in idx and idx[ref[2]].get('object')=='STYLE' and idx[ref[2]].get('name','').casefold()=='standard','style handle')
    c=math.cos(math.pi/6); modes={'default':((0.,0.,1.),(c,.5,0.)),'tilt':((0.,1.,0.),(-c,0.,.5)),'negative':((0.,0.,-1.),(-c,.5,0.)),'direction':((0.,1.,0.),(0.,0.,1.)),'baseline':((0.,0.,1.),(0.,0.,0.))};normal,axis=modes[a.mode]
    close(text['ins_pt'],(3.,4.,2.),'DWG insertion');close(text['extrusion'],normal,'DWG extrusion');close(text['x_axis_dir'],axis,'DWG direction')
    close(text['rect_width'],10.,'DWG width');close(text['text_height'],2.,'DWG height');require(text['text']=='HELLO' and text['attachment']==1 and text['flow_dir']==1,'DWG text fields')
    import ezdxf
    require(ezdxf.__version__=='1.4.5','ezdxf version');doc=ezdxf.readfile(dxf);audit=doc.audit();require(doc.dxfversion=='AC1027' and not audit.errors and not audit.fixes,'DXF audit')
    items=list(doc.modelspace());require(len(items)==1 and items[0].dxftype()=='MTEXT','DXF inventory');e=items[0]
    close(e.dxf.insert,(3.,4.,2.),'DXF insertion');close(e.dxf.get('extrusion',(0.,0.,1.)),normal,'DXF extrusion')
    close(e.dxf.width,10.,'DXF width');close(e.dxf.char_height,2.,'DXF height');require(e.text=='HELLO' and e.dxf.attachment_point==1 and e.dxf.flow_direction==1,'DXF text fields')
    if a.mode=='baseline':require(not e.dxf.hasattr('text_direction') and abs(e.get_rotation())<1e-10,'baseline direction')
    else:close(e.dxf.text_direction,axis,'DXF WCS direction')
    require(digest(dwg)==a.dwg_sha256 and digest(dxf)==a.dxf_sha256,'input changed')
    print('AC1021 MTEXT',a.mode,'graph and AC1027 DXF direction PASS')
if __name__=='__main__':main()
