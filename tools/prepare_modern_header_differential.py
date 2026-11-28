#!/usr/bin/env python3
"""Write hash-bound copies with selected release/header bytes replaced."""
import argparse, hashlib
from pathlib import Path
from audit_modern_writer_rejection import expected_factorial

def prepare(base,base_hash,source,source_hash,out,prefix):
    if prefix not in ('AC1027','AC1032'): raise ValueError('unsupported prefix')
    base_data=base.read_bytes();source_data=source.read_bytes()
    if hashlib.sha256(base_data).hexdigest()!=base_hash or hashlib.sha256(source_data).hexdigest()!=source_hash: raise ValueError('input hash mismatch')
    if base_data[:6]!=prefix.encode('ascii') or source_data[:6]!=prefix.encode('ascii'): raise ValueError('version mismatch')
    if out.exists() and any((out/f'{prefix}-{mask:02}.dwg').exists() for mask in range(16)): raise ValueError('refusing overwrite')
    variants=[expected_factorial(base_data,source_data,mask) for mask in range(16)]
    out.mkdir(parents=True,exist_ok=True)
    for mask,data in enumerate(variants):(out/f'{prefix}-{mask:02}.dwg').write_bytes(data)
    return len(variants)

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--base',type=Path,required=True);p.add_argument('--base-sha256',required=True);p.add_argument('--source',type=Path,required=True);p.add_argument('--source-sha256',required=True);p.add_argument('--out',type=Path,required=True);p.add_argument('--version',choices=('AC1027','AC1032'),required=True);a=p.parse_args();print('prepared',prepare(a.base,a.base_sha256,a.source,a.source_sha256,a.out,a.version),'header variants')
if __name__=='__main__':main()
