#!/usr/bin/env python3
"""Hash-bind and compare selected SECTIONOBJECT body groups without interpreting 411."""
import argparse,hashlib,io
from pathlib import Path
import ezdxf
from ezdxf.lldxf.tagger import ascii_tags_loader,binary_tags_loader

def require(x,msg):
 if not x:raise ValueError(msg)
def load(path,sha):
 data=path.read_bytes();require(len(data)<64*1024*1024 and hashlib.sha256(data).hexdigest()==sha,'hash/size '+str(path));return data
def records(data):
 tags=list(binary_tags_loader(data)) if data.startswith(b'AutoCAD Binary DXF') else list(ascii_tags_loader(io.StringIO(data.decode('ascii'))));out=[]
 for i,t in enumerate(tags):
  if t.code!=0 or str(t.value).strip()!='SECTIONOBJECT':continue
  j=next((n for n in range(i+1,len(tags)) if tags[n].code==0),len(tags));rec=tags[i:j];idx=[n for n,x in enumerate(rec) if x.code==100 and str(x.value).strip()=='AcDbSection'];require(len(idx)==1,'subclass marker')
  body=[]
  for x in rec[idx[0]:]:
   value=str(x.value).strip()
   if x.code in (63,70,90,91,92,93):value=int(value)
   elif x.code in (10,11,12,20,21,22,30,31,32,40,41):value=float(value)
   body.append((x.code,value))
  out.append(body)
 return out
def main():
 p=argparse.ArgumentParser(description=__doc__)
 for n in ('input','loss','oda','ascii','binary'):
  p.add_argument('--'+n,type=Path,required=True);p.add_argument('--'+n+'-sha256',required=True)
 a=p.parse_args();require(ezdxf.__version__=='1.4.5','ezdxf version');data={}
 for n in ('input','loss','oda','ascii','binary'):
  path=getattr(a,n).resolve(strict=True);sha=getattr(a,n+'_sha256');data[n]=load(path,sha);doc=ezdxf.readfile(path);audit=doc.audit();require(not audit.errors and not audit.fixes,n+' audit');require(load(path,sha)==data[n],n+' changed')
 source=records(data['input']);require(len(source)==1 and [(c,v) for c,v in source[0] if c==411]==[(411,'BOOK$BLUE')],'input field')
 for n in ('ascii','binary'):require(records(data[n])==source,n+' body changed')
 expected=[x for x in source[0] if x[0]!=411];require(records(data['loss'])==[expected],'unexpected typed baseline change')
 oda=records(data['oda']);require(len(oda)==1 and not any(c==411 for c,v in oda[0]),'ODA observation')
 print('PASS: exact SECTIONOBJECT body raw replay, typed baseline loss, and ODA non-preservation observation')
if __name__=='__main__':main()
