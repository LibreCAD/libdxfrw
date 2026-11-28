#!/usr/bin/env python3
"""Project one exact ASCII LIGHTLIST record from a SHA-bound source DXF."""
import argparse
import hashlib
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('source',type=Path);p.add_argument('output',type=Path);p.add_argument('--sha256',required=True)
a=p.parse_args();raw=a.source.read_bytes()
if len(raw)>64*1024*1024 or hashlib.sha256(raw).hexdigest()!=a.sha256:
    raise ValueError('source digest mismatch or oversized')
lines=[s+'\n' for s in raw.decode('ascii').splitlines()]
if len(lines)%2:raise ValueError('odd DXF line count')
pairs=[(int(lines[i].strip()),lines[i+1].strip(),i) for i in range(0,len(lines),2)]
starts=[i for i,(c,v,_) in enumerate(pairs) if c==0 and v=='LIGHTLIST']
if len(starts)!=1:raise ValueError('expected one LIGHTLIST')
i=starts[0];j=next((j for j in range(i+1,len(pairs)) if pairs[j][0]==0),len(pairs))
record=''.join(lines[pairs[i][2]:pairs[j][2] if j<len(pairs) else len(lines)])
prefix='0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1027\n9\n$DWGCODEPAGE\n3\nANSI_1252\n0\nENDSEC\n0\nSECTION\n2\nOBJECTS\n'
a.output.write_text(prefix+record+'0\nENDSEC\n0\nEOF\n')
