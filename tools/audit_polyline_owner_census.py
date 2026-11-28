#!/usr/bin/env python3
"""Verify a hash-bound producer POLYLINE ownership census via LibreDWG."""
import argparse
import hashlib
import json
from pathlib import Path
import tempfile

from bounded_process import run_bounded


def require(value, message):
    if not value:
        raise ValueError(message)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--dwgread', required=True, type=Path)
    p.add_argument('--dwg', required=True, type=Path)
    p.add_argument('--sha256', required=True)
    p.add_argument('--version', required=True, choices=('AC1018', 'AC1021', 'AC1024', 'AC1027', 'AC1032'))
    p.add_argument('--mode', required=True, choices=('2d', '3d'))
    a = p.parse_args()
    dwg = a.dwg.resolve(strict=True); reader = a.dwgread.resolve(strict=True)
    require(dwg.stat().st_size <= 64 * 1024 * 1024, 'source exceeds budget')
    require(hashlib.sha256(dwg.read_bytes()).hexdigest() == a.sha256, 'source digest mismatch')
    v = run_bounded([str(reader), '--version'], 5, 4096)
    require(v.returncode == 0 and v.stdout.strip() == b'dwgread 0.14', 'requires LibreDWG 0.14')
    with tempfile.TemporaryDirectory(prefix='polyline-owner-census-') as tmp:
        out = Path(tmp) / 'peer.json'
        result = run_bounded([str(reader), '-O', 'JSON', '-o', str(out), str(dwg)], 30, 1024 * 1024)
        require(result.returncode == 0 and out.exists() and out.stat().st_size <= 128 * 1024 * 1024, 'DWG decode failed')
        doc = json.loads(out.read_bytes())
    require(doc['FILEHEADER']['version'] == a.version, 'revision mismatch')
    objects = doc['OBJECTS']
    parents = [o for o in objects if o.get('entity') == 'POLYLINE_' + a.mode.upper()]
    require(len(parents) == 1, 'unexpected parent count')
    parent = parents[0]; refs = parent['vertex']; seq = parent['seqend']
    require(len(refs) == 3 and all(ref[0] == 3 and ref[2] != 0 for ref in refs)
            and seq[0] == 3 and seq[2] != 0, 'expected hard-owner tokens')
    handles = [r[2] for r in refs] + [seq[2]]
    require(len(set(handles)) == 4, 'duplicate child handles')
    byhandle = {o['handle'][2]: o for o in objects if isinstance(o.get('handle'), list) and len(o['handle']) >= 3}
    for index, handle in enumerate(handles):
        child = byhandle[handle]
        expected_type = 'SEQEND' if index == 3 else 'VERTEX_' + a.mode.upper()
        require(child.get('entity') == expected_type and child.get('ownerhandle', [None])[0] == 4
                and child['ownerhandle'][2] == parent['handle'][2], 'child type or owner mismatch')
    points = [(0, 0, 0), (5, 0, 1 if a.mode == '3d' else 0), (5, 3, 2 if a.mode == '3d' else 0)]
    for ref, point in zip(refs, points):
        require(byhandle[ref[2]]['point'] == list(point), 'vertex order or coordinates mismatch')
    print(a.version, a.mode, 'hard-owner child and SEQEND source roles PASS')


if __name__ == '__main__':
    main()
