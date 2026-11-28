#!/usr/bin/env python3
"""Audit ODA SUN color, controlled tag reordering and scalar-only outputs."""
import argparse,hashlib,io
from pathlib import Path
import ezdxf
from ezdxf.lldxf.tagger import ascii_tags_loader,binary_tags_loader

def require(value,msg):
 if not value:raise ValueError(msg)
def load(path,sha):
 data=path.read_bytes();require(len(data)<64*1024*1024 and hashlib.sha256(data).hexdigest()==sha,'hash/size '+str(path));return data
def records(data):
 tags=list(binary_tags_loader(data)) if data.startswith(b'AutoCAD Binary DXF') else list(ascii_tags_loader(io.StringIO(data.decode('ascii'))))
 result=[]
 for i,t in enumerate(tags):
  if t.code!=0 or str(t.value).strip()!='SUN':continue
  j=next((n for n in range(i+1,len(tags)) if tags[n].code==0),len(tags));result.append([(v.code,str(v.value).strip()) for v in tags[i:j]])
 return result
def color(r):return [(c,v) for c,v in r if c in (63,421)]
def main():
 p=argparse.ArgumentParser(description=__doc__)
 for n in ('input','source','reordered','projection','ascii','binary'):
  p.add_argument('--'+n,type=Path,required=True);p.add_argument('--'+n+'-sha256',required=True)
 a=p.parse_args();require(ezdxf.__version__=='1.4.5','ezdxf version');raw={}
 for n in ('input','source','reordered','projection','ascii','binary'):
  path=getattr(a,n).resolve(strict=True);sha=getattr(a,n+'_sha256');raw[n]=load(path,sha)
  if n!='projection':
   doc=ezdxf.readfile(path);audit=doc.audit();require(not audit.errors and not audit.fixes,n+' audit')
   suns=list(doc.objects.query('SUN'));require(len(suns)==1,n+' typed SUN')
   if n in ('input','source','reordered'):require(suns[0].dxf.hasattr('true_color') and suns[0].dxf.true_color==1193046,n+' typed true color')
   else:require(suns[0].dxf.color==7 and not suns[0].dxf.hasattr('true_color'),n+' typed scalar')
  require(load(path,sha)==raw[n],n+' changed')
 for n,aci in [('input','7'),('source','148')]:
  r=records(raw[n]);require(len(r)==1 and color(r[0])==[(63,aci),(421,'1193046')],n+' fields')
 first=raw['source'].decode('ascii').splitlines();second=raw['reordered'].decode('ascii').splitlines();require(len(first)==len(second),'length');first=list(zip(first[::2],first[1::2]));second=list(zip(second[::2],second[1::2]))
 ix=[i for i in range(len(first)-1) if [first[i+k][0].strip() for k in (0,1)]==['63','421'] and [first[i+k][1].strip() for k in (0,1)]==['148','1193046']];require(len(ix)==1,'source pair');i=ix[0];first[i],first[i+1]=first[i+1],first[i];require(first==second,'additional mutation')
 x=records(raw['reordered']);y=records(raw['projection']);require(len(x)==len(y)==1 and x[0]==y[0] and color(y[0])==[(421,'1193046'),(63,'148')],'projection')
 for n in ('ascii','binary'):
  r=records(raw[n]);require(len(r)==1 and color(r[0])==[(63,'7')],n+' scalar output')
 print('PASS: SUN source color, reordered projection and scalar-only outputs')
if __name__=='__main__':main()
