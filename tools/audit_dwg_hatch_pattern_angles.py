#!/usr/bin/env python3
import argparse,hashlib,json,math,subprocess,tempfile
import ezdxf
p=argparse.ArgumentParser();p.add_argument('--dwg',required=True);p.add_argument('--dwg-sha256',required=True);p.add_argument('--dxf',required=True);p.add_argument('--dxf-sha256',required=True);p.add_argument('--overall',type=float,required=True);p.add_argument('--line',type=float,required=True);p.add_argument('--dwgread',default='/opt/homebrew/bin/dwgread');a=p.parse_args()
def need(x,msg):
 if not x: raise SystemExit(msg)
def close(x,y):return math.isclose(x,y,rel_tol=1e-12,abs_tol=1e-10)
def checked(p,h):
 with open(p,'rb') as f: actual=hashlib.sha256(f.read()).hexdigest()
 need(actual==h,'digest mismatch '+p)
checked(a.dwg,a.dwg_sha256);checked(a.dxf,a.dxf_sha256)
with tempfile.NamedTemporaryFile(suffix='.json') as out:
 result=subprocess.run([a.dwgread,'-O','JSON',a.dwg,'-o',out.name],capture_output=True,text=True);need(result.returncode==0,'dwgread failed');records=json.load(open(out.name))['OBJECTS']
h=[r for r in records if r.get('entity')=='HATCH'];need(len(h)==1,'DWG HATCH count');h=h[0];need(h.get('name')=='ANSI31' and h.get('is_solid_fill')==0,'DWG fill');need(close(h.get('angle',float('nan')),math.radians(a.overall)),'DWG overall angle');lines=h.get('deflines',[]);need(len(lines)==1 and close(lines[0]['angle'],math.radians(a.line)),'DWG line angle');paths=h.get('paths',[]);need(len(paths)==1 and paths[0].get('closed')==1,'DWG boundary');expected=[[0,0],[5,0],[5,3],[0,3]];need([v['point'] for v in paths[0]['polyline_paths']]==expected,'DWG vertices')
doc=ezdxf.readfile(a.dxf);audit=doc.audit();need(not audit.errors and not audit.fixes,'DXF audit');hs=list(doc.modelspace().query('HATCH'));need(len(hs)==1,'DXF HATCH count');x=hs[0];need(x.dxf.pattern_name=='ANSI31' and x.dxf.solid_fill==0,'DXF fill');need(close(x.dxf.pattern_angle,a.overall),'DXF overall angle');need(x.pattern is not None and len(x.pattern.lines)==1 and close(x.pattern.lines[0].angle,a.line),'DXF line angle');need(len(x.paths.paths)==1,'DXF boundary count');need([list(v[:2]) for v in x.paths.paths[0].vertices]==expected,'DXF vertices');print('PASS: pinned DWG/DXF HATCH pattern angles and boundary')
