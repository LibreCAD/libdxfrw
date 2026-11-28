#!/usr/bin/env python3
import argparse,hashlib,json,math,subprocess,tempfile
import ezdxf
p=argparse.ArgumentParser();p.add_argument('--dwg',required=True);p.add_argument('--dwg-sha256',required=True);p.add_argument('--dxf');p.add_argument('--dxf-sha256');p.add_argument('--snap',type=float,required=True);p.add_argument('--twist',type=float,required=True);p.add_argument('--tilt',action='store_true');p.add_argument('--dwgread',default='/opt/homebrew/bin/dwgread');a=p.parse_args()
def need(x,msg):
 if not x:raise SystemExit(msg)
def check(p,h):
 with open(p,'rb') as f:got=hashlib.sha256(f.read()).hexdigest()
 need(got==h,'digest mismatch: '+p)
def close(x,y):return math.isclose(x,y,rel_tol=1e-12,abs_tol=1e-12)
need(bool(a.dxf)==bool(a.dxf_sha256),'DXF path/hash must be supplied together');check(a.dwg,a.dwg_sha256)
with tempfile.NamedTemporaryFile(suffix='.json') as f:
 r=subprocess.run([a.dwgread,'-O','JSON',a.dwg,'-o',f.name],capture_output=True,text=True);need(r.returncode==0,'dwgread failed');data=json.load(open(f.name))
vs=[x for x in data['OBJECTS'] if x.get('entity')=='VIEWPORT'];need(len(vs)==1,'DWG VIEWPORT count');v=vs[0];need(close(v.get('SNAPANG',float('nan')),math.radians(a.snap)) and close(v.get('VIEWTWIST',float('nan')),math.radians(a.twist)),'DWG radians mismatch')
if a.tilt:need(v.get('VIEWDIR')==[1,2,3] and v.get('view_target')==[4,5,6],'DWG tilted view mismatch')
if a.dxf:
 check(a.dxf,a.dxf_sha256);doc=ezdxf.readfile(a.dxf);audit=doc.audit();need(not audit.errors and not audit.fixes,'DXF audit');vs=[v for layout in doc.layouts for v in layout.query('VIEWPORT')];need(len(vs)==1,'DXF VIEWPORT count');v=vs[0];need(close(v.dxf.snap_angle,a.snap) and close(v.dxf.view_twist_angle,a.twist),'DXF degrees mismatch');
 if a.tilt:need(tuple(v.dxf.view_direction_vector)==(1,2,3) and tuple(v.dxf.view_target_point)==(4,5,6),'DXF tilted view mismatch')
print('PASS: pinned VIEWPORT angle and selected view fields')
