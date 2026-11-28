#!/usr/bin/env python3
"""Hash-bound physical VIEW/UCS CONTROL observations, not file qualification."""
import argparse
import hashlib
import re
import subprocess
from pathlib import Path


def digest(path): return hashlib.sha256(path.read_bytes()).hexdigest()


def inspect_trace(text, mode):
    for name in ('VIEW_CONTROL','UCS_CONTROL'):
        pieces=text.split('Add object '+name+' [')
        if len(pieces)!=2: raise ValueError('control is missing or duplicated')
        frame=pieces[1].split('< Next object:',1)[0]
        expected_handle = ('7' if name=='VIEW_CONTROL' else '6') if mode=='source' else ('6' if name=='VIEW_CONTROL' else '7')
        patterns=[r'bitsize:\s*35, hdl_dat:\s*@4\.3',r'handle:\s*0\.1\.'+expected_handle,
                  r'num_entries:\s*0\s*\[(?:BS|BL)',r'ownerhandle:\s*\(4\.0\.0\)\s*abs:0',
                  r'padding:\s*\+5',r'check_CRC [^\n]*:\s*([0-9A-F]+)\s*==\s*\1']
        patterns += [r'is_xdic_missing:\s*1'] if mode=='source' else [r'is_xdic_missing:\s*0',r'xdicobjhandle:\s*\(3\.0\.0\)\s*abs:0']
        if any(not re.search(p,frame) for p in patterns):raise ValueError('unexpected '+name+' frame')
    return True


def inspect_bits(page,offset,mode):
    if offset<0 or offset+7>len(page):raise ValueError('truncated body')
    bits=''.join(f'{x:08b}' for x in page[offset:offset+7])
    expected='01000000' if mode=='source' else '0100000000110000'
    if bits[34]!='0' or bits[35:35+len(expected)]!=expected or bits[35+len(expected):40+len(expected)]!='00000':
        raise ValueError('marker, handles or terminal padding mismatch')
    return True


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--dwg',type=Path,required=True);p.add_argument('--sha256',required=True)
    p.add_argument('--dwgread',type=Path,required=True);p.add_argument('--dwgread-sha256',required=True)
    p.add_argument('--mode',choices=('source','generated'),required=True)
    p.add_argument('--object-page',type=Path,required=True);p.add_argument('--page-sha256',required=True)
    p.add_argument('--view-offset',type=int,required=True);p.add_argument('--ucs-offset',type=int,required=True)
    a=p.parse_args()
    if digest(a.dwg)!=a.sha256.lower() or digest(a.dwgread)!=a.dwgread_sha256.lower() or digest(a.object_page)!=a.page_sha256.lower():raise ValueError('input digest mismatch')
    result=subprocess.run([str(a.dwgread),'-v9',str(a.dwg)],capture_output=True,text=True,check=True)
    inspect_trace(result.stdout+'\n'+result.stderr,a.mode)
    page=a.object_page.read_bytes()
    for offset in (a.view_offset,a.ucs_offset):inspect_bits(page,offset,a.mode)
    print('VIEW/UCS exact marker, handle and padding bits and peer CRC match; page-to-DWG join is external')


if __name__=='__main__':main()
