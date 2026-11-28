#!/usr/bin/env python3
"""Relocate the hard-owner flag in a hash-bound producer DICTIONARY."""
import argparse
import hashlib
from pathlib import Path

def main():
    p=argparse.ArgumentParser();p.add_argument('source',type=Path);p.add_argument('output',type=Path);p.add_argument('--sha256',required=True);a=p.parse_args()
    data=a.source.read_bytes()
    if hashlib.sha256(data).hexdigest()!=a.sha256: raise ValueError('source hash mismatch')
    lines=data.decode('utf-8').splitlines()
    if len(lines)%2: raise ValueError('odd group lines')
    tags=list(zip(lines[::2],lines[1::2]));starts=[i for i,(c,_) in enumerate(tags) if c.strip()=='0']+[len(tags)]
    for i,j in zip(starts,starts[1:]):
        row=tags[i:j]
        if row[0][1].strip()=='DICTIONARY' and any(c.strip()=='5' and v.strip()=='78' for c,v in row): break
    else: raise ValueError('dictionary 78 not found')
    flags=[k for k,(c,v) in enumerate(row) if c.strip()=='280' and v.strip()=='1']; entries=[k for k,(c,v) in enumerate(row) if c.strip()=='360' and v.strip()=='83']
    if len(flags)!=1 or len(entries)!=1: raise ValueError('unexpected flag or entry')
    flag=row.pop(flags[0]); k=next(k for k,(c,v) in enumerate(row) if c.strip()=='360' and v.strip()=='83'); row.insert(k+1,flag);tags[i:j]=row
    a.output.write_text(''.join(c+'\n'+v+'\n' for c,v in tags))
if __name__=='__main__': main()
