#!/usr/bin/env python3
"""Hash-bound LibreDWG/ODA-DXF audit for sampled SOLID/TRACE corners."""
import argparse,hashlib,json,tempfile
from pathlib import Path
from bounded_process import run_bounded
from audit_ac1021_geometry_peer import close,require,check_graph

def digest(p):
 require(p.is_file() and p.stat().st_size<64000000,'invalid input');return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--dwgread',type=Path,required=True);p.add_argument('--dwg',type=Path,required=True);p.add_argument('--dwg-sha256',required=True);p.add_argument('--dxf',type=Path);p.add_argument('--dxf-sha256');p.add_argument('--entity',choices=('SOLID','TRACE'),required=True);p.add_argument('--normal',choices=('default','tilt'),required=True);p.add_argument('--thickness',choices=('zero','thick'),required=True);p.add_argument('--generated-graph',action='store_true');a=p.parse_args()
 require(bool(a.dxf)==bool(a.dxf_sha256),'DXF/hash pairing');reader=a.dwgread.resolve(strict=True);dwg=a.dwg.resolve(strict=True)
 v=run_bounded([str(reader),'--version'],5,4096);require(v.returncode==0 and v.stdout.strip()==b'dwgread 0.14','LibreDWG version');require(digest(dwg)==a.dwg_sha256,'DWG hash')
 with tempfile.TemporaryDirectory(prefix='solid-trace-peer-') as tmp:
  out=Path(tmp)/'doc.json';run=run_bounded([str(reader),'-O','JSON','-o',str(out),str(dwg)],30,1048576);require(run.returncode==0 and out.is_file() and out.stat().st_size<128000000,'DWG decode');doc=json.loads(out.read_text())
 require(doc['FILEHEADER']['version']=='AC1021','DWG version');objs=doc['OBJECTS'];idx={o['handle'][2]:o for o in objs if isinstance(o.get('handle'),list)}
 if a.generated_graph:check_graph(doc)
 blocks=[o for o in objs if o.get('object')=='BLOCK_HEADER' and o.get('name','').lower()=='*model_space'];require(len(blocks)==1,'model block')
 entities=[o for o in objs if o.get('entity') in ('SOLID','TRACE','LINE') and o.get('entmode')==2];require([o['entity'] for o in entities]==[a.entity,'LINE'],'entity order')
 entries=blocks[0]['entities'];require([x[2] for x in entries]==[x['handle'][2] for x in entities] and all(x[0]==3 for x in entries),'model ownership')
 for e in entities:require(e['layer'][0]==5 and e['layer'][2] in idx and idx[e['layer'][2]].get('name')=='0','layer reference')
 e,line=entities;normal=(0.,1.,0.) if a.normal=='tilt' else (0.,0.,1.);thickness=.5 if a.thickness=='thick' else 0.
 close(e['extrusion'],normal,'DWG normal');close(e['elevation'],3.,'DWG elevation');close(e['thickness'],thickness,'DWG thickness')
 points=[(1.,2.,3.),(4.,2.,3.),(1.,4.,3.),(4.,4.,3.)]
 for i,xyz in enumerate(points,1):close(e['corner'+str(i)],xyz[:2],'DWG corner')
 close(line['start'],(10.,11.,0.),'LINE start');close(line['end'],(12.,13.,0.),'LINE end')
 if a.dxf:
  import ezdxf
  require(ezdxf.__version__=='1.4.5','ezdxf version');dxf=a.dxf.resolve(strict=True);require(digest(dxf)==a.dxf_sha256,'DXF hash');d=ezdxf.readfile(dxf);audit=d.audit();require(d.dxfversion=='AC1027' and not audit.errors and not audit.fixes,'DXF audit');items=list(d.modelspace());require([x.dxftype() for x in items]==[a.entity,'LINE'],'DXF entity order');e,line=items
  close(e.dxf.get('extrusion',(0.,0.,1.)),normal,'DXF normal');close(e.dxf.get('thickness',0.),thickness,'DXF thickness')
  for i,xyz in enumerate(points):close(e.dxf.get('vtx'+str(i)),xyz,'DXF OCS corner')
  wcs=[(-x,z,y) if a.normal=='tilt' else (x,y,z) for x,y,z in points];expected=[wcs[i] for i in (0,1,3,2)]
  actualWcs=list(e.wcs_vertices());require(len(actualWcs)==4,'DXF WCS corner count')
  for actual,target in zip(actualWcs,expected):close(actual,target,'DXF WCS corner')
  close(line.dxf.start,(10.,11.,0.),'DXF LINE start');close(line.dxf.end,(12.,13.,0.),'DXF LINE end');require(digest(dxf)==a.dxf_sha256,'DXF changed')
 require(digest(dwg)==a.dwg_sha256,'DWG changed');print('AC1021',a.entity,a.normal,a.thickness,'peer graph/geometry PASS')
if __name__=='__main__':main()
