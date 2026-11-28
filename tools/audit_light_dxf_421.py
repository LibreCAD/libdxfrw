#!/usr/bin/env python3
"""Audit hash-bound ODA LIGHT color controls and generated DXF tag values."""
import argparse,hashlib,json
from pathlib import Path

def require(ok,msg):
 if not ok:raise ValueError(msg)
def digest(p):
 require(p.is_file() and p.stat().st_size<64*1024*1024,'missing or oversized');return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 import ezdxf
 from ezdxf.lldxf.tagger import ascii_tags_loader,binary_tags_loader
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('directory',type=Path);p.add_argument('--manifest-sha256',required=True);a=p.parse_args();root=a.directory.resolve(strict=True)
 m=root/'manifest.json';require(digest(m)==a.manifest_sha256,'manifest hash');mapping=json.loads(m.read_text());require(ezdxf.__version__=='1.4.5','ezdxf version')
 names=('black','rgb','white');values=(0,1193046,16777215)
 required={f'{d}/{n}.{ext}' for n in names for d,ext in (('in','dxf'),('dwg','dwg'),('out','dxf'))}|{f'{n}-{mode}.dxf' for n in names for mode in ('ascii','binary')}
 require(set(mapping)==required,'manifest inventory')
 for n,h in mapping.items():
  path=(root/n).resolve(strict=True);require(root in path.parents and digest(path)==h,'artifact hash')
 for name,rgb in zip(names,values):
  require(not (root/'out'/(name+'.dxf.err')).exists(),'ODA error')
  for path,mode in [(root/'in'/(name+'.dxf'),'ascii'),(root/'out'/(name+'.dxf'),'ascii'),(root/(name+'-ascii.dxf'),'ascii'),(root/(name+'-binary.dxf'),'binary')]:
   if mode=='binary': tags=list(binary_tags_loader(path.read_bytes()))
   else:
    with path.open() as stream:tags=list(ascii_tags_loader(stream))
   starts=[i for i,t in enumerate(tags) if t.code==0 and t.value=='LIGHT'];require(len(starts)==1,'LIGHT count')
   i=starts[0];j=next(j for j in range(i+1,len(tags)) if tags[j].code==0)
   record=tags[i:j];colors=[int(t.value) for t in record if t.code==421];aci=[int(t.value) for t in record if t.code==63]
   require(colors==[rgb] and len(aci)==1,'LIGHT color fields')
   if path.parent!=root/'out':require(aci==[7],'input/generated ACI')
   doc=ezdxf.readfile(path);lights=[e for e in doc.modelspace() if e.dxftype()=='LIGHT']
   require(len(lights)==1 and lights[0].dxf.name=='LOCAL_LIGHT' and tuple(lights[0].dxf.location)==(1.,2.,3.),'LIGHT geometry')
 for n,h in mapping.items():require(digest(root/n)==h,'artifact changed')
 require(digest(m)==a.manifest_sha256,'manifest changed');print('LIGHT 421 source and ASCII/Binary generated tag audit PASS')
if __name__=='__main__':main()
