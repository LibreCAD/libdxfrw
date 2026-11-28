#!/usr/bin/env python3
"""Hash-bound selected ODA/LibreDWG/ezdxf DIMENSION angle audit."""
import argparse, hashlib, json, math, pathlib, subprocess, tempfile
import ezdxf
p=argparse.ArgumentParser()
for n in ('dwg','source','selected'):
 p.add_argument('--'+n,required=True);p.add_argument('--'+n+'-sha256',required=True)
p.add_argument('--count',type=int,choices=(1,7),required=True)
p.add_argument('--dwgread',required=True);p.add_argument('--dwgread-sha256',required=True)
a=p.parse_args()
for name in ('dwg','source','selected'):
 f=pathlib.Path(getattr(a,name)); expected=getattr(a,name+'_sha256')
 if hashlib.sha256(f.read_bytes()).hexdigest()!=expected:raise SystemExit('hash mismatch '+name)
if hashlib.sha256(pathlib.Path(a.dwgread).read_bytes()).hexdigest()!=a.dwgread_sha256:raise SystemExit('dwgread digest mismatch')
if pathlib.Path(a.dwg).read_bytes()[:6]!=b'AC1024':raise SystemExit('unexpected DWG revision')
with tempfile.TemporaryDirectory() as tmp:
 out=str(pathlib.Path(tmp)/'peer.json')
 subprocess.run([a.dwgread,'-O','JSON',a.dwg,'-o',out],check=True,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
 objects=json.loads(pathlib.Path(out).read_text())['OBJECTS']
 records=[o for o in objects if str(o.get('entity','')).startswith('DIMENSION_')]
 if len(records)!=a.count:raise SystemExit('wrong DWG DIMENSION inventory')
 for r in records:
  if not math.isclose(r.get('text_rotation',math.nan),math.pi/4,abs_tol=1e-12) or not math.isclose(r.get('horiz_dir',math.nan),math.pi/12,abs_tol=1e-12):raise SystemExit('DWG radians mismatch')
src=ezdxf.readfile(a.source); dst=ezdxf.readfile(a.selected)
if src.dxfversion!='AC1027' or dst.dxfversion!='AC1024':raise SystemExit('unexpected DXF revision')
source=list(src.modelspace().query('DIMENSION')); selected=list(dst.modelspace().query('DIMENSION'))
if len(source)!=a.count or len(selected)!=a.count:raise SystemExit('wrong selected DIMENSION inventory')
for x,y in zip(source,selected):
 if x.dxf.dimtype!=y.dxf.dimtype:raise SystemExit('dimension subtype mismatch')
 for prop in ('text_rotation','horizontal_direction','angle'):
  v=x.dxf.get(prop,0.);w=y.dxf.get(prop,0.)
  if not math.isclose(v,w,rel_tol=1e-12,abs_tol=1e-10):raise SystemExit('angle mismatch '+prop)
 for prop in ('defpoint','defpoint2','defpoint3','defpoint4','text_midpoint','extrusion'):
  fallback=(0.,0.,1.) if prop=='extrusion' else None
  v=x.dxf.get(prop,fallback);w=y.dxf.get(prop,fallback)
  if v is None or w is None:
   if v is not None or w is not None:raise SystemExit('point presence mismatch '+prop)
  elif any(not math.isclose(m,n,rel_tol=1e-12,abs_tol=1e-10) for m,n in zip(v,w)):raise SystemExit('point mismatch '+prop)
 if y.dxf.geometry not in dst.blocks:raise SystemExit('dimension picture block missing')
block=dst.blocks.get('ANGLE_BLOCK')
if block is None or len(list(block.query('DIMENSION')))!=1:raise SystemExit('named-block copied dimension missing')
named=list(block.query('DIMENSION'))[0]
if not math.isclose(named.dxf.text_rotation,45.,abs_tol=1e-10) or not math.isclose(named.dxf.horizontal_direction,15.,abs_tol=1e-10):raise SystemExit('named-block angles mismatch')
group=dst.groups.get('DIM_TARGET')
if group is None or [e.dxf.handle for e in group]!=[selected[0].dxf.handle]:raise SystemExit('GROUP reference mismatch')
audit=dst.audit()
if audit.errors or audit.fixes:raise SystemExit('ezdxf audit reported errors or fixes')
print('PASS: selected DIMENSION 50/51/53, represented points, picture blocks, named block, GROUP and audit; group 52 is not qualified')
