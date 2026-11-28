#!/usr/bin/env python3
"""Check producer, actual emitted tags and selected ODA-returned obliques."""
import argparse, hashlib, json, math, pathlib, subprocess, tempfile
import ezdxf
from ezdxf.lldxf.tagger import ascii_tags_loader,binary_tags_loader
p=argparse.ArgumentParser()
for n in ('dwg','source','selected','returned'):
 p.add_argument('--'+n,required=True);p.add_argument('--'+n+'-sha256',required=True)
p.add_argument('--dwgread',required=True)
a=p.parse_args()
for n in ('dwg','source','selected','returned'):
 path=pathlib.Path(getattr(a,n));want=getattr(a,n+'_sha256')
 if hashlib.sha256(path.read_bytes()).hexdigest()!=want:raise SystemExit('hash mismatch '+n)
with tempfile.TemporaryDirectory() as t:
 out=str(pathlib.Path(t)/'peer.json')
 subprocess.run([a.dwgread,'-O','JSON',a.dwg,'-o',out],check=True,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
 rows=json.loads(pathlib.Path(out).read_text())['OBJECTS']
 dims=[x for x in rows if x.get('entity') in ('DIMENSION_LINEAR','DIMENSION_ALIGNED')]
 if len(dims)!=2 or not all(math.isclose(x.get('oblique_angle',math.nan),math.radians(v),abs_tol=1e-12) for x,v in zip(dims,(20.,25.))):raise SystemExit('source DWG oblique mismatch')
source=ezdxf.readfile(a.source);selected=ezdxf.readfile(a.selected);returned=ezdxf.readfile(a.returned)
for doc in (source,returned):
 dims=list(doc.modelspace().query('DIMENSION'))
 if len(dims)!=7 or [d.dxf.dimtype&7 for d in dims] != [0,1,4,3,5,2,6]:raise SystemExit('source/return subtype inventory mismatch')
 if [round(x.dxf.get('oblique_angle',0),8) for x in dims[:2]]!=[20.,25.]:raise SystemExit('source/return oblique mismatch')
 audit=doc.audit()
 if audit.errors or audit.fixes:raise SystemExit('source/return audit problems')
models=list(selected.modelspace().query('DIMENSION'))
if len(models)!=7:raise SystemExit('selected subtype inventory mismatch')
handles=[models[0].dxf.handle,models[1].dxf.handle]
raw=pathlib.Path(a.selected).read_bytes()
if raw.startswith(b'AutoCAD Binary DXF'):
 tags=list(binary_tags_loader(raw))
else:
 with open(a.selected,encoding='utf-8') as f:tags=list(ascii_tags_loader(f))
seen={}
for i,tag in enumerate(tags):
 if tag.code!=0 or tag.value!='DIMENSION':continue
 j=i+1
 while j<len(tags) and tags[j].code!=0:j+=1
 record=tags[i:j];h=next((x.value for x in record if x.code==5),None)
 if h not in handles:continue
 subclass='';values=[]
 for x in record:
  if x.code==100:subclass=x.value
  if x.code==52:values.append((subclass,float(x.value)))
 seen[h]=values
if seen.get(handles[0])!=[('AcDbAlignedDimension',20.)] or seen.get(handles[1])!=[('AcDbAlignedDimension',25.)]:raise SystemExit('selected group 52 role/value mismatch')
group=selected.groups.get('DIM_TARGET')
if group is None or [x.dxf.handle for x in group]!=[handles[0]]:raise SystemExit('group remap mismatch')
block=selected.blocks.get('ANGLE_BLOCK')
if block is None or len(list(block.query('DIMENSION')))!=1:raise SystemExit('named block missing')
audit=selected.audit()
if audit.errors or audit.fixes:raise SystemExit('selected audit problems')
print('PASS: source DWG, documented selected 52 tags, selected graph and ODA return')
