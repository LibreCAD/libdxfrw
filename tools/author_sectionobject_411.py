#!/usr/bin/env python3
"""Insert one explicit raw group 411 into a hash-bound SECTIONOBJECT body."""
import argparse,hashlib
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__);p.add_argument('source',type=Path);p.add_argument('output',type=Path);p.add_argument('--sha256',required=True);a=p.parse_args();data=a.source.read_bytes()
if len(data)>64*1024*1024 or hashlib.sha256(data).hexdigest()!=a.sha256:raise ValueError('hash/size')
lines=data.decode('ascii').splitlines()
if len(lines)%2:raise ValueError('incomplete pair')
rows=list(zip(lines[::2],lines[1::2]));matches=[]
for i,(c,v) in enumerate(rows):
 if c.strip()=='0' and v.strip()=='SECTIONOBJECT':
  j=next((n for n in range(i+1,len(rows)) if rows[n][0].strip()=='0'),len(rows));rec=rows[i:j]
  if any(x.strip()=='100' and y.strip()=='AcDbSection' for x,y in rec):matches.append((i,j))
if len(matches)!=1:raise ValueError('expected one SECTIONOBJECT')
i,j=matches[0];rec=rows[i:j];start=next(n for n,(c,v) in enumerate(rec) if c.strip()=='100' and v.strip()=='AcDbSection');targets=[i+n for n,(c,v) in enumerate(rec) if n>start and c.strip()=='63']
if len(targets)!=1 or any(c.strip()=='411' for c,v in rec[start:]):raise ValueError('unexpected indicator fields')
rows.insert(targets[0]+1,('411','BOOK$BLUE'));a.output.write_text(''.join(c+'\n'+v+'\n' for c,v in rows))
