#!/usr/bin/env python3
import argparse, hashlib, json, math, subprocess, tempfile
p=argparse.ArgumentParser();p.add_argument('--dwg',required=True);p.add_argument('--dwg-sha256',required=True);p.add_argument('--dxf',required=True);p.add_argument('--dxf-sha256',required=True);p.add_argument('--angle',required=True,type=float);p.add_argument('--dwgread',default='/opt/homebrew/bin/dwgread');a=p.parse_args()
def need(x,msg):
 if not x: raise SystemExit(msg)
def check(path,sha):
 with open(path,'rb') as f: got=hashlib.sha256(f.read()).hexdigest()
 need(got==sha,'digest mismatch: '+path)
check(a.dwg,a.dwg_sha256);check(a.dxf,a.dxf_sha256)
with tempfile.NamedTemporaryFile(suffix='.json') as out:
 q=subprocess.run([a.dwgread,'-O','JSON',a.dwg,'-o',out.name],capture_output=True,text=True);need(q.returncode==0,'dwgread failed');data=json.load(open(out.name))
vertices=[x for x in data['OBJECTS'] if x.get('entity')=='VERTEX_2D'];need(len(vertices)==3,'DWG vertex count');need(vertices[0].get('flag')==2,'DWG tangent flag');need(math.isclose(vertices[0].get('tangent_dir',float('nan')),a.angle*math.pi/180,rel_tol=1e-12,abs_tol=1e-12),'DWG tangent mismatch')
lines=open(a.dxf,encoding='utf8').read().splitlines(); need(len(lines)%2==0,'invalid DXF pairs');pairs=[(lines[i].strip(),lines[i+1].strip()) for i in range(0,len(lines),2)];records=[]
for code,val in pairs:
 if code=='0': records.append([val,[]])
 elif records: records[-1][1].append((code,val))
vs=[fields for kind,fields in records if kind=='VERTEX'];need(len(vs)==3,'DXF vertex count');fields=vs[0]; flag=[v for c,v in fields if c=='70']; ang=[v for c,v in fields if c=='50'];need(len(flag)==1 and int(flag[0])&2 and len(ang)==1,'DXF tangent field/flag');need(math.isclose(float(ang[0]),a.angle,abs_tol=1e-9),'DXF tangent mismatch'); print('PASS: three vertices, flagged tangent and independent DWG/DXF angle')
