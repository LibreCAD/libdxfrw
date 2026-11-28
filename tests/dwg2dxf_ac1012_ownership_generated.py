#!/usr/bin/env python3
"""Exercise R13 ownership-marker refusal and retry using scratch drawings."""
import subprocess
import sys
import tempfile
from pathlib import Path


def main():
    if len(sys.argv) != 5:
        raise SystemExit('usage: script <dwgadd> <dwgread> <dwgwrite> <profile-test>')
    dwgadd, dwgread, dwgwrite, profile = sys.argv[1:]
    repo = Path(__file__).resolve().parents[1]
    generator = repo / 'tools/research/generate_legacy_dictionary_eed_control.py'
    with tempfile.TemporaryDirectory(prefix='libdxfrw-r13-owner-') as scratch:
        root = Path(scratch)
        subprocess.run([sys.executable, '-B', str(generator), '--output-dir', str(root),
                        '--version', 'r13', '--nonempty-group', '--r13-ownership-markers',
                        '--dwgadd', dwgadd, '--dwgread', dwgread, '--dwgwrite', dwgwrite],
                       check=True, stdout=subprocess.DEVNULL, timeout=60)
        subprocess.run([profile, '--r13-ownership-marker', str(root / 'ownership-1.dwg'),
                        str(root / 'ownership-0.dwg'), str(root / 'output.dxf')],
                       check=True, timeout=60)


if __name__ == '__main__':
    main()
