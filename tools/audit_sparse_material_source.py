#!/usr/bin/env python3
"""Hash-bound audit of ODA's sparse canonical MATERIAL producer records."""
import argparse,hashlib,json,tempfile
from pathlib import Path
from bounded_process import run_bounded

DXF='fa5d37197d271ab36ce268ecfddd942083a33c9b1a3b02411714c9fee0af3fb9'
DWG='e15bdf112893b080f6714a8e34c58f8a8100bd25527b7baf94b661c90ac673a4'
def require(ok,msg):
 if not ok:raise ValueError(msg)
def digest(p):
 require(p.is_file() and p.stat().st_size<64*1024*1024,'input missing/oversized');return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 import ezdxf
 from ezdxf.lldxf.tagger import ascii_tags_loader
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--dxf',type=Path,required=True);p.add_argument('--dwg',type=Path,required=True);p.add_argument('--dwgread',type=Path,required=True);p.add_argument('--probe',type=Path,required=True);p.add_argument('--adapterprobe',type=Path,required=True);a=p.parse_args()
 dxf=a.dxf.resolve(strict=True);dwg=a.dwg.resolve(strict=True);require(digest(dxf)==DXF and digest(dwg)==DWG,'source hash');require(ezdxf.__version__=='1.4.5','ezdxf version')
 with dxf.open() as stream:tags=list(ascii_tags_loader(stream))
 starts=[i for i,t in enumerate(tags) if t.code==0 and t.value=='MATERIAL'];require(len(starts)==3,'material count')
 names=set();expected=[5,102,330,102,330,100,1,71,91,72,273,94]
 for i in starts:
  j=next(j for j in range(i+1,len(tags)) if tags[j].code==0);g=tags[i+1:j]
  require([t.code for t in g]==expected,'sparse group sequence');v=[t.value for t in g]
  require(v[1]=='{ACAD_REACTORS' and v[3]=='}' and v[2]==v[4] and v[5]=='AcDbMaterial','common envelope')
  names.add(v[6]);require([int(v[k]) for k in (7,8,9,10,11)]==[1,-1023410177,1,1,63],'source fields')
 require(names=={'ByBlock','ByLayer','Global'},'canonical names')
 peer=a.dwgread.resolve(strict=True);version=run_bounded([str(peer),'--version'],5,4096);require(version.returncode==0 and version.stdout.strip()==b'dwgread 0.14','peer version')
 with tempfile.TemporaryDirectory(prefix='sparse-material-peer-') as tmp:
  output=Path(tmp)/'peer.json';result=run_bounded([str(peer),'-O','JSON','-o',str(output),str(dwg)],30,1024*1024)
  require(result.returncode==0 and output.is_file() and output.stat().st_size<128*1024*1024,'peer decode');doc=json.loads(output.read_text())
  mats=[o for o in doc['OBJECTS'] if o.get('object')=='MATERIAL'];require(len(mats)==3,'peer materials')
  require({o.get('name') for o in mats}==names,'peer names')
  for o in mats:require(o.get('diffuse_color.flag')==1 and o.get('diffuse_color.rgb')==0xC2FFFFFF and o.get('refractionmap.source')==1 and o.get('channel_flags')==63,'peer fields')
 result=run_bounded([str(a.probe.resolve(strict=True)),str(dxf)],30,1024*1024)
 require(result.returncode==0 and b'AC1027 sparse MATERIAL source capture PASS' in result.stdout,'native probe')
 result=run_bounded([str(a.adapterprobe.resolve(strict=True)),str(dxf)],30,1024*1024)
 require(result.returncode==0 and b'sparse MATERIAL adapter replay refused PASS' in result.stdout,'adapter refusal')
 require(digest(dxf)==DXF and digest(dwg)==DWG,'source changed');print('AC1027 sparse MATERIAL producer source audit PASS')
if __name__=='__main__':main()
