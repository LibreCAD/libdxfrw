#!/usr/bin/env python3
"""Project a hash-bound producer NOD/material graph without changing its records."""
import argparse,hashlib
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('source',type=Path);p.add_argument('output',type=Path);p.add_argument('--sha256',required=True);a=p.parse_args()
data=a.source.read_bytes()
if hashlib.sha256(data).hexdigest()!=a.sha256:raise SystemExit('source hash mismatch')
lines=data.decode('utf-8').splitlines()
if len(lines)%2:raise SystemExit('bad pairs')
pairs=list(zip(lines[::2],lines[1::2]));starts=[i for i,(c,v) in enumerate(pairs) if c.strip()=='0']+[len(pairs)];records={}
for i,j in zip(starts,starts[1:]):
 r=pairs[i:j];handle=next((v.strip() for c,v in r if c.strip()=='5'),None)
 if handle in {'C','67','68','69','6A'} and r[0][1].strip() in {'DICTIONARY','MATERIAL'}:
  if handle in records:raise SystemExit('duplicate identity')
  records[handle]=r
if len(records)!=5:raise SystemExit('missing record')
root=records['C'];selected=[];i=0;matches=0
while i<len(root):
 c,v=root[i]
 if c.strip()=='3':
  if i+1>=len(root) or root[i+1][0].strip() not in {'350','360'}:raise SystemExit('bad root pair')
  if v.strip()=='ACAD_MATERIAL':selected.extend(root[i:i+2]);matches+=1
  i+=2
 else:selected.append(root[i]);i+=1
if matches!=1:raise SystemExit('root target count')
records['C']=selected
header=[('0','SECTION'),('2','HEADER'),('9','$ACADVER'),('1','AC1027'),('0','ENDSEC'),('0','SECTION'),('2','OBJECTS')]
output=header+sum((records[k] for k in ['C','67','69','68','6A']),[])+[('0','ENDSEC'),('0','EOF')]
a.output.write_text(''.join(c+'\n'+v+'\n' for c,v in output))
