#!/usr/bin/env python3
"""Create hash-bound isolated SECTIONOBJECT count and omitted-Z controls."""
import argparse,hashlib
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__);p.add_argument('source',type=Path);p.add_argument('directory',type=Path);p.add_argument('--sha256',required=True);a=p.parse_args();data=a.source.read_bytes()
if len(data)>64*1024*1024 or hashlib.sha256(data).hexdigest()!=a.sha256:raise ValueError('source hash/size')
lines=data.decode('ascii').splitlines()
if len(lines)%2:raise ValueError('odd group count')
rows=list(zip(lines[::2],lines[1::2]));hits=[]
for i,(c,v) in enumerate(rows):
 if c.strip()=='0' and v.strip()=='SECTIONOBJECT':hits.append((i,next((n for n in range(i+1,len(rows)) if rows[n][0].strip()=='0'),len(rows))))
if len(hits)!=1:raise ValueError('expected one section')
i,j=hits[0];orig=rows[i:j];body=next(k for k,(c,v) in enumerate(orig) if c.strip()=='100' and v.strip()=='AcDbSection');a.directory.mkdir(parents=True,exist_ok=True)
for name in ('normal','noz','nocount','late'):
 rec=list(orig)
 if name=='noz':rec=rec[:body]+[x for x in rec[body:] if x[0].strip() not in ('31','32')]
 if name=='nocount':rec=rec[:body]+[x for x in rec[body:] if x[0].strip() not in ('92','93')]
 if name=='late':
  for count,component in [('92','31'),('93','32')]:
   ids=[k for k,(c,v) in enumerate(rec) if c.strip()==count]
   if len(ids)!=1:raise ValueError('count')
   value=rec.pop(ids[0]);pos=[k for k,(c,v) in enumerate(rec) if c.strip()==component]
   if len(pos)!=2:raise ValueError('coordinate count')
   rec.insert(pos[-1]+1,value)
 (a.directory/(name+'.dxf')).write_text(''.join(c+'\n'+v+'\n' for c,v in rows[:i]+rec+rows[j:]))
