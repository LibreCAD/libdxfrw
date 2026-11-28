#!/usr/bin/env python3
"""Hash-bound LibreDWG audit of an empty BLOCK_CONTROL frame; no acceptance claim."""
import argparse
import hashlib
import re
import subprocess
from pathlib import Path


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def inspect(text, mode):
    sections = text.split('Add object BLOCK_CONTROL [')
    if len(sections) != 2:
        raise ValueError('expected exactly one BLOCK_CONTROL')
    frame = sections[1].split('< Next object:', 1)[0]
    common = [r'bitsize:\s*35, hdl_dat:\s*@4\.3', r'num_entries:\s*0\s*\[BL',
              r'ownerhandle:\s*\(4\.0\.0\)\s*abs:0', r'model_space:\s*\(3\.1\.17\)\s*abs:17',
              r'padding:\s*\+5', r'check_CRC [^\n]*:\s*([0-9A-F]+)\s*==\s*\1']
    checks = common + ([r'handle:\s*0\.1\.9', r'is_xdic_missing:\s*1',
                        r'paper_space:\s*\(3\.1\.1B\)\s*abs:1B'] if mode == 'source' else
                       [r'handle:\s*0\.1\.1', r'is_xdic_missing:\s*0',
                        r'xdicobjhandle:\s*\(3\.0\.0\)\s*abs:0',
                        r'paper_space:\s*\(3\.1\.18\)\s*abs:18'])
    if any(not re.search(pattern, frame) for pattern in checks):
        raise ValueError('BLOCK_CONTROL frame does not match the selected physical profile')
    return True


def inspect_bits(page, offset, mode):
    if offset < 0 or offset + 11 > len(page):
        raise ValueError('body outside supplied page')
    raw = page[offset:offset + 11]
    bits = ''.join(f'{value:08b}' for value in raw)
    tail = bytes.fromhex('403117311b' if mode == 'source' else '403031173118')
    encoded = ''.join(f'{value:08b}' for value in tail)
    end = 35 + len(encoded)
    if bits[34] != '0' or bits[35:end] != encoded or bits[end:end+5] != '00000':
        raise ValueError('bad physical marker, handle tokens or terminal padding')
    return True


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--dwg',type=Path,required=True);p.add_argument('--sha256',required=True)
    p.add_argument('--dwgread',type=Path,required=True);p.add_argument('--dwgread-sha256',required=True)
    p.add_argument('--mode',choices=('source','generated'),required=True)
    p.add_argument('--object-page',type=Path,required=True);p.add_argument('--page-sha256',required=True)
    p.add_argument('--body-offset',type=int,required=True)
    a=p.parse_args()
    if (digest(a.dwg)!=a.sha256.lower() or digest(a.dwgread)!=a.dwgread_sha256.lower()
            or digest(a.object_page)!=a.page_sha256.lower()):
        raise ValueError('input or peer digest mismatch')
    result=subprocess.run([str(a.dwgread),'-v9',str(a.dwg)],capture_output=True,text=True,check=True)
    inspect(result.stdout+'\n'+result.stderr,a.mode)
    inspect_bits(a.object_page.read_bytes(), a.body_offset, a.mode)
    print('BLOCK_CONTROL exact bit, handle roles, physical marker/handle/padding bits and peer CRC matched; page-to-DWG binding is external')


if __name__=='__main__':
    main()
