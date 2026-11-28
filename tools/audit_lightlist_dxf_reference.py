#!/usr/bin/env python3
"""Verify an ODA LIGHTLIST reference after a single LIGHT color removal."""
import argparse,hashlib
from pathlib import Path
from bounded_process import run_bounded

SOURCE='fa5d37197d271ab36ce268ecfddd942083a33c9b1a3b02411714c9fee0af3fb9'
DERIVED='3efbda60448078270e99f643ee45a8441dacbe49a9fed06e6eca98aee88649d1'
def require(ok,msg):
 if not ok:raise ValueError(msg)
def digest(p):
 require(p.is_file() and p.stat().st_size<64*1024*1024,'missing or oversized');return hashlib.sha256(p.read_bytes()).hexdigest()
def pairs(path):
 lines=path.read_text().splitlines();require(len(lines)%2==0,'odd DXF lines');return [(int(c),v) for c,v in zip(lines[::2],lines[1::2])]
def record(tags,name):
 starts=[i for i,t in enumerate(tags) if t==(0,name)];require(len(starts)==1,name+' count');i=starts[0];j=next(j for j in range(i+1,len(tags)) if tags[j][0]==0);return tags[i:j]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source',type=Path,required=True);p.add_argument('--derived',type=Path,required=True);p.add_argument('--probe',type=Path,required=True);a=p.parse_args()
 source=a.source.resolve(strict=True);derived=a.derived.resolve(strict=True);require(digest(source)==SOURCE and digest(derived)==DERIVED,'source hash')
 before=pairs(source);after=pairs(derived);lights=record(before,'LIGHT');matches=[i for i,t in enumerate(lights) if t[0]==421];require(len(matches)==1 and int(lights[matches[0]][1])==1193046,'source RGB')
 i=before.index(lights[matches[0]]);expected=before[:i]+before[i+1:];require(after==expected,'unexpected derived change')
 ll=record(before,'LIGHTLIST');require(ll==record(after,'LIGHTLIST'),'LIGHTLIST changed')
 def values(code):return [v.strip() for c,v in ll if c==code]
 require(values(5)==['F000','2F'] and values(1)==['LOCAL_LIGHT'] and values(330)==['30','30'],'LIGHTLIST graph')
 result=run_bounded([str(a.probe.resolve(strict=True)),str(derived)],30,1024*1024)
 require(result.returncode==0 and b'complete=0 selected LIGHTLIST=PASS' in result.stdout,'selected callback')
 require(digest(source)==SOURCE and digest(derived)==DERIVED,'source changed');print('LIGHTLIST source reference diagnostic PASS')
if __name__=='__main__':main()
