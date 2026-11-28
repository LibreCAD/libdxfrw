#!/usr/bin/env python3
"""Move two count groups within a hash-bound ODA SECTIONOBJECT record."""
import argparse,hashlib
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__);p.add_argument('source',type=Path);p.add_argument('reordered',type=Path);p.add_argument('projection',type=Path);p.add_argument('--sha256',required=True);a=p.parse_args();data=a.source.read_bytes()
if len(data)>64*1024*1024 or hashlib.sha256(data).hexdigest()!=a.sha256:raise ValueError('source hash/size')
lines=data.decode('ascii').splitlines()
if len(lines)%2:raise ValueError('odd group count')
rows=list(zip(lines[::2],lines[1::2]));hits=[]
for i,(c,v) in enumerate(rows):
 if c.strip()=='0' and v.strip()=='SECTIONOBJECT':
  j=next((n for n in range(i+1,len(rows)) if rows[n][0].strip()=='0'),len(rows));hits.append((i,j))
if len(hits)!=1:raise ValueError('expected one section')
i,j=hits[0];rec=rows[i:j]
for count,component in [('92','31'),('93','32')]:
 ids=[k for k,(c,v) in enumerate(rec) if c.strip()==count]
 if len(ids)!=1 or rec[ids[0]][1].strip()!='2':raise ValueError('unexpected count')
 item=rec.pop(ids[0]);positions=[k for k,(c,v) in enumerate(rec) if c.strip()==component]
 if len(positions)!=2:raise ValueError('unexpected components')
 rec.insert(positions[-1]+1,item)
rows[i:j]=rec;a.reordered.write_text(''.join(c+'\n'+v+'\n' for c,v in rows));head=[('0','SECTION'),('2','HEADER'),('9','$ACADVER'),('1','AC1027'),('0','ENDSEC'),('0','SECTION'),('2','ENTITIES')];tail=[('0','ENDSEC'),('0','EOF')];a.projection.write_text(''.join(c.strip()+'\n'+v.strip()+'\n' for c,v in head+rec+tail))
