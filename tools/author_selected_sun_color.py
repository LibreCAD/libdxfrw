#!/usr/bin/env python3
"""Hash-bind and reorder a selected ODA-produced SUN color pair."""
import argparse,hashlib
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__);p.add_argument('source',type=Path);p.add_argument('reordered',type=Path);p.add_argument('projection',type=Path);p.add_argument('--sha256',required=True);a=p.parse_args()
raw=a.source.read_bytes()
if len(raw)>64*1024*1024 or hashlib.sha256(raw).hexdigest()!=a.sha256:raise ValueError('source hash or size')
lines=raw.decode('ascii').splitlines()
if len(lines)%2:raise ValueError('incomplete pair')
rows=list(zip(lines[::2],lines[1::2]));hits=[]
for i,(code,value) in enumerate(rows):
 if code.strip()=='0' and value.strip()=='SUN':
  j=next((n for n in range(i+1,len(rows)) if rows[n][0].strip()=='0'),len(rows));r=rows[i:j]
  if any(c.strip()=='5' and v.strip().upper()=='30' for c,v in r):hits.append((i,j))
if len(hits)!=1:raise ValueError('SUN count')
i,j=hits[0];rec=rows[i:j];idx=[k for k,(c,_) in enumerate(rec) if c.strip() in ('63','421')]
if [rec[k][0].strip() for k in idx]!=['63','421'] or [rec[k][1].strip() for k in idx]!=['148','1193046'] or idx[1]!=idx[0]+1:raise ValueError('color fields')
x,y=[i+k for k in idx];rows[x],rows[y]=rows[y],rows[x]
a.reordered.write_text(''.join(c+'\n'+v+'\n' for c,v in rows))
header=[('0','SECTION'),('2','HEADER'),('9','$ACADVER'),('1','AC1027'),('0','ENDSEC'),('0','SECTION'),('2','OBJECTS')];tail=[('0','ENDSEC'),('0','EOF')]
a.projection.write_text(''.join(c.strip()+'\n'+v.strip()+'\n' for c,v in header+rows[i:j]+tail))
