#!/usr/bin/env python3
"""Hash-bind and reorder the selected physical VIEWPORT color fields."""
import argparse
import hashlib
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('source',type=Path);p.add_argument('reordered',type=Path);p.add_argument('projection',type=Path);p.add_argument('--sha256',required=True)
a=p.parse_args();data=a.source.read_bytes()
if len(data)>64*1024*1024 or hashlib.sha256(data).hexdigest()!=a.sha256:raise ValueError('source hash/size')
lines=data.decode('ascii').splitlines()
if len(lines)%2:raise ValueError('incomplete pair')
rows=list(zip(lines[::2],lines[1::2]));matches=[]
for i,(code,value) in enumerate(rows):
 if code.strip()=='0' and value.strip()=='VIEWPORT':
  j=next((n for n in range(i+1,len(rows)) if rows[n][0].strip()=='0'),len(rows));r=rows[i:j]
  if any(c.strip()=='5' and v.strip().upper()=='2F' for c,v in r):matches.append((i,j))
if len(matches)!=1:raise ValueError('expected one selected viewport')
i,j=matches[0];rec=rows[i:j];idx=[k for k,(c,_) in enumerate(rec) if c.strip() in ('63','421','431')]
if [rec[k][0].strip() for k in idx]!=['63','421','431'] or [rec[k][1].strip() for k in idx]!=['148','1193046','TEST$BLUE']:raise ValueError('unexpected color fields')
a0,b0,c0=[i+k for k in idx];rows[a0],rows[b0],rows[c0]=rows[c0],rows[a0],rows[b0]
a.reordered.write_text(''.join(c+'\n'+v+'\n' for c,v in rows))
header=[('0','SECTION'),('2','HEADER'),('9','$ACADVER'),('1','AC1027'),('0','ENDSEC'),('0','SECTION'),('2','ENTITIES')];tail=[('0','ENDSEC'),('0','EOF')]
a.projection.write_text(''.join(c.strip()+'\n'+v.strip()+'\n' for c,v in header+rows[i:j]+tail))
