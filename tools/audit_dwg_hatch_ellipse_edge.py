#!/usr/bin/env python3
"""Hash-bound LibreDWG and ezdxf audit for three sampled HATCH edge profiles."""
import argparse, hashlib, json, math, tempfile
from pathlib import Path
from bounded_process import run_bounded
from audit_ac1021_geometry_peer import close, require, check_graph

def digest(p):
    require(p.is_file() and p.stat().st_size < 64000000, 'invalid input')
    return hashlib.sha256(p.read_bytes()).hexdigest()

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--mode',choices=('ellipse','arc','linear'),required=True)
    p.add_argument('--version',choices=('AC1015','AC1018','AC1021','AC1024','AC1027','AC1032'),required=True)
    p.add_argument('--dwgread',type=Path,required=True); p.add_argument('--dwg',type=Path,required=True); p.add_argument('--dwg-sha256',required=True)
    p.add_argument('--dxf',type=Path); p.add_argument('--dxf-sha256'); p.add_argument('--generated-graph',action='store_true')
    a=p.parse_args(); require(bool(a.dxf)==bool(a.dxf_sha256),'DXF/hash pairing')
    reader=a.dwgread.resolve(strict=True); dwg=a.dwg.resolve(strict=True)
    v=run_bounded([str(reader),'--version'],5,4096); require(v.returncode==0 and v.stdout.strip()==b'dwgread 0.14','LibreDWG version')
    require(digest(dwg)==a.dwg_sha256,'DWG hash')
    with tempfile.TemporaryDirectory(prefix='hatch-edge-') as t:
        out=Path(t)/'doc.json'; run=run_bounded([str(reader),'-O','JSON','-o',str(out),str(dwg)],30,1048576)
        require(run.returncode==0 and out.is_file() and out.stat().st_size<128000000,'DWG decode')
        doc=json.loads(out.read_text())
    require(doc['FILEHEADER']['version']==a.version,'DWG version')
    objs=doc['OBJECTS']; idx={x['handle'][2]:x for x in objs if isinstance(x.get('handle'),list)}
    if a.generated_graph: check_graph(doc)
    blocks=[o for o in objs if o.get('object')=='BLOCK_HEADER' and o.get('name','').lower()=='*model_space']
    require(len(blocks)==1,'model block'); block=blocks[0]
    ents=[o for o in objs if o.get('entity') in ('HATCH','LINE') and o.get('entmode')==2]
    require([o['entity'] for o in ents]==['HATCH','LINE'],'entity order')
    handles=[o['handle'][2] for o in ents]
    if a.version=='AC1015': require(block['first_entity'][2]==handles[0] and block['last_entity'][2]==handles[1], 'model endpoints')
    else: require([h[2] for h in block['entities']]==handles and all(h[0]==3 for h in block['entities']),'model membership')
    for o in ents: require(o['layer'][0]==5 and o['layer'][2] in idx and idx[o['layer'][2]].get('name')=='0','layer reference')
    close(ents[1]['start'],(10.,11.,0.),'line start'); close(ents[1]['end'],(12.,13.,0.),'line end')
    paths=ents[0]['paths']; require(len(paths)==1,'path count'); edges=paths[0]['segs']
    expected={'ellipse':3,'arc':2,'linear':1}[a.mode]; require(len(edges)==(4 if a.mode=='linear' else 1) and all(e['curve_type']==expected for e in edges),'edge types')
    if a.mode=='ellipse':
        e=edges[0]; close(e['center'],(3.,4.),'center'); close(e['endpoint'],(4.,0.),'axis'); close(e['minor_major_ratio'],.5,'ratio'); close(e['start_angle'],0.,'start'); close(e['end_angle'],2*math.pi,'end'); require(e['is_ccw']==1,'direction')
    elif a.mode=='arc':
        e=edges[0]; close(e['center'],(3.,4.),'center'); close(e['radius'],2.,'radius'); close(e['start_angle'],0.,'start'); close(e['end_angle'],2*math.pi,'end'); require(e['is_ccw']==1,'direction')
    else:
        points=[(0.,0.),(5.,0.),(5.,3.),(0.,3.)]
        for e,x,y in zip(edges,points,points[1:]+points[:1]): close(e['first_endpoint'],x,'line edge start'); close(e['second_endpoint'],y,'line edge end')
    if a.dxf:
        import ezdxf
        require(ezdxf.__version__=='1.4.5','ezdxf version'); dxf=a.dxf.resolve(strict=True); require(digest(dxf)==a.dxf_sha256,'DXF hash')
        d=ezdxf.readfile(dxf); audit=d.audit(); require(d.dxfversion=='AC1027' and not audit.errors and not audit.fixes,'DXF audit')
        items=list(d.modelspace()); require([e.dxftype() for e in items]==['HATCH','LINE'],'DXF inventory')
        close(items[1].dxf.start,(10.,11.,0.),'DXF line start'); close(items[1].dxf.end,(12.,13.,0.),'DXF line end')
        paths=items[0].paths.paths; require(len(paths)==1 and len(paths[0].edges)==len(edges),'DXF path')
        for actual,edge in zip(paths[0].edges,edges):
            require(type(actual).__name__=={'ellipse':'EllipseEdge','arc':'ArcEdge','linear':'LineEdge'}[a.mode],'DXF edge type')
            if a.mode=='ellipse': close(actual.center,(3.,4.),'DXF center'); close(actual.major_axis,(4.,0.),'DXF axis'); close(actual.ratio,.5,'DXF ratio'); close(actual.start_angle,0.,'DXF start'); close(actual.end_angle,360.,'DXF end'); require(actual.ccw,'DXF direction')
            elif a.mode=='arc': close(actual.center,(3.,4.),'DXF center'); close(actual.radius,2.,'DXF radius'); close(actual.start_angle,0.,'DXF start'); close(actual.end_angle,360.,'DXF end'); require(actual.ccw,'DXF direction')
            else: close(actual.start,tuple(edge['first_endpoint']),'DXF start'); close(actual.end,tuple(edge['second_endpoint']),'DXF end')
        require(digest(dxf)==a.dxf_sha256,'DXF changed')
    require(digest(dwg)==a.dwg_sha256,'DWG changed')
    print(a.version,a.mode,'HATCH graph and fields PASS')
if __name__=='__main__': main()
