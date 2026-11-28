#!/usr/bin/env python3
"""Audit a selected ODA VIEWPORT, a reordered projection and generated output."""
import argparse,hashlib
from pathlib import Path
import ezdxf
from ezdxf.lldxf.tagger import ascii_tags_loader,binary_tags_loader

def require(v,msg):
 if not v:raise ValueError(msg)
def load(path,sha):
 raw=path.read_bytes();require(len(raw)<64*1024*1024 and hashlib.sha256(raw).hexdigest()==sha,'digest/size: '+str(path));return raw
def records(raw):
 if raw.startswith(b'AutoCAD Binary DXF'): tags=list(binary_tags_loader(raw))
 else:
  import io
  tags=list(ascii_tags_loader(io.StringIO(raw.decode('ascii'))))
 out=[]
 for i,t in enumerate(tags):
  if t.code!=0 or str(t.value).strip()!='VIEWPORT':continue
  j=next((n for n in range(i+1,len(tags)) if tags[n].code==0),len(tags));out.append([(x.code,str(x.value).strip()) for x in tags[i:j]])
 return out
def only_color(r):return [(c,v) for c,v in r if c in (63,421,431)]
def main():
 p=argparse.ArgumentParser(description=__doc__)
 for name in ('input','source','reordered','projection','ascii','binary'):
  p.add_argument('--'+name,type=Path,required=True);p.add_argument('--'+name+'-sha256',required=True)
 a=p.parse_args();require(ezdxf.__version__=='1.4.5','ezdxf version');raw={}
 for name in ('input','source','reordered','projection','ascii','binary'):
  path=getattr(a,name).resolve(strict=True);sha=getattr(a,name+'_sha256');raw[name]=load(path,sha)
  if name!='projection':
   doc=ezdxf.readfile(path);audit=doc.audit();require(not audit.errors and not audit.fixes,name+' audit')
  require(load(path,sha)==raw[name],name+' changed')
 expected=[(63,'148'),(421,'1193046'),(431,'TEST$BLUE')]
 require(len(records(raw['source']))==1 and only_color(records(raw['source'])[0])==expected,'source color')
 require(len(records(raw['input']))==1 and only_color(records(raw['input'])[0])==[(63,'7'),(421,'1193046'),(431,'TEST$BLUE')],'input color')
 src=raw['source'].decode('ascii').splitlines();new=raw['reordered'].decode('ascii').splitlines();require(len(src)==len(new),'reordered length')
 sp=list(zip(src[::2],src[1::2]));np=list(zip(new[::2],new[1::2]))
 # Match the entire file by replacing only the selected viewport's adjacent color pairs.
 matches=[i for i in range(len(sp)-2) if [sp[i+k][0].strip() for k in range(3)]==['63','421','431'] and [sp[i+k][1].strip() for k in range(3)]==['148','1193046','TEST$BLUE']]
 require(len(matches)==1,'source color occurrence');i=matches[0];sp[i:i+3]=[sp[i+2],sp[i],sp[i+1]];require(sp==np,'reorder changes additional groups')
 reordered=records(raw['reordered']);projection=records(raw['projection']);require(len(reordered)==len(projection)==1 and reordered[0]==projection[0],'selected record projection')
 for name in ('ascii','binary'):
  views=records(raw[name]);require(len(views)==2 and only_color(views[0])==[(63,'7'),(421,'1193046'),(431,'BOOK$BLUE')],name+' output')
  require(not any(c in (421,431) for c,v in views[1]),name+' following viewport')
 print('PASS: physical selected color, controlled reorder, projection and ASCII/Binary emitted fields')
if __name__=='__main__':main()
