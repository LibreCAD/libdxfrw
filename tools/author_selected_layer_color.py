#!/usr/bin/env python3
"""Project the observed layer and reorder only ACI/truecolor source groups."""
import argparse
import hashlib
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('source',type=Path);p.add_argument('output',type=Path);p.add_argument('--sha256',required=True)
p.add_argument('--order',choices=('producer','truecolor-first'),required=True)
a=p.parse_args();raw=a.source.read_bytes()
if len(raw)>64*1024*1024 or hashlib.sha256(raw).hexdigest()!=a.sha256:
    raise ValueError('source digest mismatch or oversized')
lines=raw.decode('ascii').splitlines()
if len(lines)%2:raise ValueError('odd DXF line count')
pairs=list(zip(lines[::2],lines[1::2]));found=[]
for i,(c,v) in enumerate(pairs):
    if c.strip()=='0' and v.strip()=='LAYER':
        j=next((j for j in range(i+1,len(pairs)) if pairs[j][0].strip()=='0'),len(pairs))
        record=pairs[i:j]
        if any(code.strip()=='2' and value.strip()=='COLORCHECK' for code,value in record):
            found.append(list(record))
if len(found)!=1:raise ValueError('expected one source LAYER')
record=found[0]
aci=[i for i,(c,_) in enumerate(record) if c.strip()=='62']; rgb=[i for i,(c,_) in enumerate(record) if c.strip()=='420']
if len(aci)!=1 or len(rgb)!=1 or record[aci[0]][1].strip()!='-148' or record[rgb[0]][1].strip()!='1193046' or rgb[0]!=aci[0]+1:
    raise ValueError('unexpected source color fields')
if a.order=='truecolor-first':record[aci[0]],record[rgb[0]]=record[rgb[0]],record[aci[0]]
header=[('0','SECTION'),('2','HEADER'),('9','$ACADVER'),('1','AC1027'),('0','ENDSEC'),('0','SECTION'),('2','TABLES'),('0','TABLE'),('2','LAYER'),('5','1'),('330','0'),('100','AcDbSymbolTable'),('70','1')]
tail=[('0','ENDTAB'),('0','ENDSEC'),('0','EOF')]
a.output.write_text(''.join(f'{c.strip()}\n{v.strip()}\n' for c,v in header+record+tail))
