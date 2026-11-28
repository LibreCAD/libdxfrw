#!/usr/bin/env python3
"""Audit a producer-derived compact MATERIAL graph and strict CLI outputs."""
import argparse,hashlib
from pathlib import Path
import ezdxf
from ezdxf.lldxf.tagger import ascii_tags_loader,binary_tags_loader
from io import StringIO

def need(ok,message):
 if not ok:raise ValueError(message)
def records(path):
 data=path.read_bytes();seq=binary_tags_loader(data) if data.startswith(b'AutoCAD Binary DXF') else ascii_tags_loader(StringIO(data.decode('utf-8').replace('\r\n','\n')))
 out=[];row=[]
 for t in seq:
  if t.code==0:
   if row:out.append(row)
   row=[]
  row.append((t.code,t.value))
 if row:out.append(row)
 return out

def by_handle(rows,h):
 found=[r for r in rows if r[0][1] in ('MATERIAL','DICTIONARY') and (5,h) in r]
 need(len(found)==1,'object identity count '+h);return found[0]

def graph(path):
 d=ezdxf.readfile(path);audit=d.audit();need(not audit.errors and not audit.fixes,'ezdxf audit')
 root=d.rootdict;md=root['ACAD_MATERIAL'];need(md.dxftype()=='DICTIONARY' and md.dxf.owner==root.dxf.handle,'root dictionary owner')
 expect={'ByBlock','ByLayer','Global'};need(set(k for k,v in md.items())==expect,'material names')
 for name,m in md.items():
  need(m.dxftype()=='MATERIAL' and m.dxf.owner==md.dxf.handle and m.dxf.name==name,'material owner/name')
  row=by_handle(records(path),m.dxf.handle);need([x[0] for x in row]==[0,5,102,330,102,330,100,1,72,94],'raw material shape')
  need(row[2][1]=='{ACAD_REACTORS' and row[3][1]==md.dxf.handle and row[4][1]=='}' and row[5][1]==md.dxf.handle and row[6][1]=='AcDbMaterial' and row[7][1]==name and int(row[8][1])==1 and int(row[9][1])==127,'raw common/body values')
 return root.dxf.handle,md.dxf.handle
p=argparse.ArgumentParser()
for k in ('source','selected','ascii','binary','repeat'):
 p.add_argument('--'+k,type=Path,required=True);p.add_argument('--'+k+'-sha256',required=True)
a=p.parse_args();need(ezdxf.__version__=='1.4.5','ezdxf version')
for k in ('source','selected','ascii','binary','repeat'):
 path=getattr(a,k);need(hashlib.sha256(path.read_bytes()).hexdigest()==getattr(a,k+'_sha256'),k+' hash')
s=records(a.source);q=records(a.selected)
root=by_handle(s,'C');filtered=[];i=0;matches=0
while i<len(root):
 if root[i][0]==3:
  need(i+1<len(root) and root[i+1][0] in (350,360),'source root pair')
  if root[i][1]=='ACAD_MATERIAL':filtered.extend(root[i:i+2]);matches+=1
  i+=2
 else:filtered.append(root[i]);i+=1
need(matches==1 and filtered==by_handle(q,'C'),'projection root differs')
for h in ('67','68','69','6A'):need(by_handle(s,h)==by_handle(q,h),'projection record differs: '+h)
for pth in (a.source,a.selected,a.ascii,a.binary,a.repeat):graph(pth)
print('PASS: physical source material records, selected graph and ASCII/Binary/repeat output')
