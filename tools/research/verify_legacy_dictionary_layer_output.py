#!/usr/bin/env python3
"""Verify scratch R13/R14 dictionary layer-EED exports with ezdxf.

The six source directories are created locally with
`generate_legacy_dictionary_eed_control.py`: one group, --eed-on-nod and
--eed-on-mline directory for each of r13 and r14. No fixture is committed.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--candidate', required=True)
    parser.add_argument('--output-dir', type=Path, required=True)
    for version in ('r13', 'r14'):
        for kind in ('group', 'nod', 'mline'):
            parser.add_argument(f'--{version}-{kind}', type=Path, required=True)
    args = parser.parse_args()
    import ezdxf

    args.output_dir.mkdir(parents=True, exist_ok=True)
    root = Path(tempfile.mkdtemp(prefix='legacy-dictionary-layer-', dir=args.output_dir))
    report = {'ezdxfVersion': ezdxf.__version__, 'candidateSha256': sha(Path(args.candidate)), 'profiles': {}}
    geometry = [((1., 2., 3.), (4., 6., 9.)), ((2., 3., 4.), (5., 7., 10.)),
                ((3., 4., 5.), (6., 8., 11.))]
    for version in ('r13', 'r14'):
        for kind in ('group', 'nod', 'mline'):
            source_dir = getattr(args, f'{version}_{kind}')
            for order, filename in (('little', 'unqualified.dwg'), ('big', 'big-layer.dwg')):
                source = source_dir / filename
                if source.read_bytes()[:6] != (b"AC1012" if version == "r13" else b"AC1014"):
                    raise ValueError(f"wrong source revision: {source}")
                key = f'{version}-{kind}-{order}'
                profile = {'sourceSha256': sha(source), 'outputs': {}}
                for encoding in ('ascii', 'binary'):
                    output = root / f'{key}-{encoding}.dxf'
                    cmd = [args.candidate, str(source), '-o', str(output), '-y',
                           '--dwg-eed-layer-byte-order=' + order]
                    if version == 'r13':
                        cmd.append('--allow-partial-dwg-read')
                    if encoding == 'binary':
                        cmd.append('-b')
                    result = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False)
                    if result.returncode:
                        raise RuntimeError(f'{key}-{encoding}: {result.stderr[-1200:]!r}')
                    doc = ezdxf.readfile(output)
                    dictionary = (doc.rootdict if kind == 'nod' else doc.rootdict.get(
                        'ACAD_GROUP' if kind == 'group' else 'ACAD_MLINESTYLE'))
                    tags = [(item.code, item.value) for item in dictionary.get_xdata('ACAD')]
                    lines = sorted((tuple(line.dxf.start), tuple(line.dxf.end))
                                   for line in doc.modelspace().query('LINE'))
                    if tags != [(1000, 'HELLO'), (1070, 42), (1003, '0')] or lines != geometry:
                        raise ValueError(f'{key}-{encoding} dictionary or geometry mismatch')
                    profile['outputs'][encoding] = {'sha256': sha(output), 'tags': tags,
                                                    'lineGeometryMatched': True}
                report['profiles'][key] = profile
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
