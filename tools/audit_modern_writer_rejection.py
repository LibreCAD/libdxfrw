#!/usr/bin/env python3
"""Verify hash-bound local AC1027/AC1032 ODA comparison; no causal assertion."""
import argparse,hashlib,json,subprocess,tempfile
from pathlib import Path

def require(ok,msg):
 if not ok: raise ValueError(msg)

def checked_path(root, name):
    rel=Path(name)
    require(not rel.is_absolute() and '..' not in rel.parts, 'unsafe manifest path')
    resolved=(root/rel).resolve(strict=True)
    require(root == resolved or root in resolved.parents, 'manifest path escapes root')
    return resolved


def required_paths():
    required={'source-in/line.dxf'}
    for version in ('AC1027','AC1032'):
        required.update({f'source-dwg/{version}/line.dwg', f'source-out/{version}/line.dxf',
                         f'generated/{version}-line.dwg', f'material-in/{version}-line.dwg',
                         f'material-out/{version}-line.dxf.err'})
        for mode in ('line','polyline2d','polyline3d'):
            required.update({f'generated/{version}-{mode}.dwg', f'oda-out/{version}-{mode}.dxf.err'})
        for mask in range(16):
            required.update({f'factorial-in/{version}-{mask:02}.dwg', f'factorial-out/{version}-{mask:02}.dxf.err'})
    return required


def checked_manifest(root, expected):
    path=root/'manifest.json'
    require(path.is_file() and path.stat().st_size < 1024*1024, 'manifest missing or oversized')
    data=path.read_bytes()
    require(hashlib.sha256(data).hexdigest()==expected,'manifest hash mismatch')
    manifest=json.loads(data)
    require(isinstance(manifest,dict) and bool(manifest), 'empty manifest')
    require(required_paths().issubset(manifest),'manifest omits required evidence')
    for name, digest in manifest.items():
        require(isinstance(name,str) and isinstance(digest,str) and len(digest)==64,'invalid manifest entry')
        checked_path(root,name)
    return manifest


def expected_factorial(base, source, mask):
    require(0 <= mask < 16 and len(base)>18 and len(source)>18,'invalid header mutation inputs')
    value=bytearray(base)
    for i,off in enumerate((11,12,17,18)):
        if mask & (1<<i): value[off]=source[off]
    return bytes(value)


def expected_error(version, mask):
    require(version in ('AC1027','AC1032') and 0 <= mask < 16,'invalid outcome key')
    return 'CRC does not match' if version=='AC1032' or mask&1 else 'Out of range'


def main():
 import ezdxf
 p=argparse.ArgumentParser();p.add_argument('directory',type=Path);p.add_argument('--manifest-sha256',required=True);p.add_argument('--dwgread',type=Path,required=True);p.add_argument('--dwgread-sha256',required=True);args=p.parse_args();b=args.directory.resolve(strict=True)
 require(ezdxf.__version__=='1.4.5','ezdxf version');manifest=checked_manifest(b,args.manifest_sha256)
 require(hashlib.sha256(args.dwgread.read_bytes()).hexdigest()==args.dwgread_sha256,'dwgread hash')
 def digest(path):
  require(path.is_file() and path.stat().st_size<64*1024*1024,'missing or oversized file')
  return hashlib.sha256(path.read_bytes()).hexdigest()
 require(all(digest(checked_path(b,k))==v for k,v in manifest.items()),'manifest hash mismatch')
 version=subprocess.run([str(args.dwgread),'--version'],capture_output=True,text=True,check=True,timeout=5);require(version.stdout.strip()=='dwgread 0.14','LibreDWG version')
 for v in ('AC1027','AC1032'):
  dxf=b/'source-out'/v/'line.dxf';doc=ezdxf.readfile(dxf);audit=doc.audit();items=list(doc.modelspace())
  require(doc.dxfversion=='AC1027' and not audit.errors and not audit.fixes and len(items)==1 and items[0].dxftype()=='LINE','source DXF')
  require(tuple(items[0].dxf.start)==(1.,2.,3.) and tuple(items[0].dxf.end)==(4.,5.,6.),'source geometry')
  require(not (b/'source-out'/v/'line.dxf.err').exists(),'source ODA error')
  source=(b/'source-dwg'/v/'line.dwg').read_bytes();base=(b/'generated'/(v+'-line.dwg')).read_bytes()
  with tempfile.TemporaryDirectory() as temp:
   docs=[]
   for label,path in [('source',b/'source-dwg'/v/'line.dwg'),('generated',b/'generated'/(v+'-line.dwg')),('material',b/'material-in'/(v+'-line.dwg'))]:
    out=Path(temp)/(label+'.json');subprocess.run([str(args.dwgread),'-O','JSON','-o',str(out),str(path)],check=True,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL,timeout=30);require(out.is_file() and out.stat().st_size<128*1024*1024,'decoded output size');doc=json.loads(out.read_text());require(doc['FILEHEADER']['version']==v,'decoded revision');lines=[o for o in doc['OBJECTS'] if o.get('entity')=='LINE'];require(len(lines)==1 and lines[0]['start']==[1.,2.,3.] and lines[0]['end']==[4.,5.,6.],'decoded LINE');docs.append(doc)
   for name in ('ByBlock','ByLayer','Global'):
    arr=[[o for o in d['OBJECTS'] if o.get('object')=='MATERIAL' and o.get('name')==name] for d in (docs[0],docs[2])];require(all(len(x)==1 for x in arr),'material identity'); a,c=arr[0][0],arr[1][0];ignored={'bitsize','handle','ownerhandle','reactors','index','size','address','type'};require(all(a.get(k)==c.get(k) for k in (set(a)|set(c))-ignored),'modeled material fields')
  for mask in range(16):
   p=b/'factorial-in'/f'{v}-{mask:02}.dwg';require(p.read_bytes()==expected_factorial(base,source,mask),'header mutation')
   err=(b/'factorial-out'/f'{v}-{mask:02}.dxf.err').read_text();require(expected_error(v,mask) in err and not (b/'factorial-out'/f'{v}-{mask:02}.dxf').exists(),'factorial outcome')
  for kind in ('line','polyline2d','polyline3d'):
   err=(b/'oda-out'/(v+'-'+kind+'.dxf.err')).read_text();require(('Out of range' if v=='AC1027' else 'CRC does not match') in err and not (b/'oda-out'/(v+'-'+kind+'.dxf')).exists(),'baseline outcome')
  err=(b/'material-out'/(v+'-line.dxf.err')).read_text();require(('Out of range' if v=='AC1027' else 'CRC does not match') in err and not (b/'material-out'/(v+'-line.dxf')).exists(),'material outcome')
  print(v,'source geometry, modeled material comparison and ODA failures verified')
 require(all(digest(checked_path(b,k))==v for k,v in manifest.items()),'input changed')
 require(hashlib.sha256((b/'manifest.json').read_bytes()).hexdigest()==args.manifest_sha256,'manifest changed')
if __name__=='__main__': main()
