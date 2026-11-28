#!/usr/bin/env python3
"""Record raw native ASCII-DXF observations; no CAD semantic inference."""
import argparse
import hashlib
import json
import pathlib
import sys

LIMIT = 64 * 1024 * 1024


def read_bounded(path):
    with path.open('rb') as f:
        data = f.read(LIMIT + 1)
    if len(data) > LIMIT:
        raise ValueError('input exceeds 64 MiB limit')
    return data


def pairs_from_bytes(data):
    if data.startswith(b'AutoCAD Binary DXF'):
        raise ValueError('binary DXF is not supported; export ASCII DXF')
    try:
        text = data.decode('utf-8-sig')
        encoding = 'utf-8'
    except UnicodeDecodeError:
        text = data.decode('cp1252')
        encoding = 'cp1252-fallback-unverified'
    lines = text.splitlines()
    if len(lines) % 2:
        raise ValueError('truncated DXF code/value pair')
    pairs = []
    for i in range(0, len(lines), 2):
        try:
            code = int(lines[i].strip())
        except ValueError as exc:
            raise ValueError('invalid group code at line %d' % (i + 1)) from exc
        if not 0 <= code <= 1071:
            raise ValueError('out-of-range group code at line %d' % (i + 1))
        pairs.append((code, lines[i + 1]))
    return encoding, pairs


def inspect(data):
    encoding, pairs = pairs_from_bytes(data)
    section = None
    records = []
    current = None
    versions = []
    sections = set()
    eof = False
    for i, (code, value) in enumerate(pairs):
        val = value.strip()
        if code == 9 and val == '$ACADVER' and section == 'HEADER':
            if i + 1 >= len(pairs) or pairs[i + 1][0] != 1:
                raise ValueError('invalid $ACADVER value')
            versions.append(pairs[i + 1][1].strip())
        if code == 0 and val == 'SECTION':
            if section is not None or i + 1 >= len(pairs) or pairs[i + 1][0] != 2:
                raise ValueError('malformed SECTION')
            section = pairs[i + 1][1].strip()
            if section in sections:
                raise ValueError('duplicate section')
            sections.add(section)
            current = None
        elif code == 0 and val == 'ENDSEC':
            if section is None:
                raise ValueError('orphan ENDSEC')
            section = None
            current = None
        elif code == 0 and val == 'EOF':
            if section is not None or i != len(pairs) - 1:
                raise ValueError('misplaced EOF')
            eof = True
        elif code == 0 and section in ('OBJECTS', 'TABLES', 'BLOCKS'):
            current = {'section': section, 'type': val, 'pairs': []}
            if val in ('XRECORD', 'SORTENTSTABLE', 'DICTIONARY', 'BLOCK_RECORD', 'BLOCK'):
                records.append(current)
            else:
                current = None
        elif current is not None:
            current['pairs'].append([code, value])
    if section is not None:
        raise ValueError('unterminated SECTION')
    if not eof or not {'HEADER', 'OBJECTS'}.issubset(sections) or len(versions) != 1:
        raise ValueError('missing EOF, required sections, or unique $ACADVER')
    return {'encoding': encoding, 'acadver': versions, 'records': records}


def self_test():
    sample = (b'0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1024\n0\nENDSEC\n'
              b'0\nSECTION\n2\nOBJECTS\n0\nXRECORD\n5\n249\n370\n-3\n'
              b'0\nSORTENTSTABLE\n5\nA\n331\nB\n5\nC\n0\nENDSEC\n0\nEOF\n')
    result = inspect(sample)
    assert result['acadver'] == ['AC1024']
    assert result['records'][0]['pairs'][-1] == [370, '-3']
    assert result['records'][1]['pairs'][-2:] == [[331, 'B'], [5, 'C']]
    for invalid in (b'0\nSECTION\n2\nOBJECTS\n0\nXRECORD\n5\n', b'X\nfoo\n',
                    b'AutoCAD Binary DXF\r\n\x1a\x00', b'0\nEOF\n',
                    sample.replace(b'1\nAC1024', b'999\nAC1024'),
                    sample[:-6]):
        try:
            inspect(invalid)
        except ValueError:
            continue
        raise AssertionError('malformed input accepted')
    print('self-test passed')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--self-test', action='store_true')
    parser.add_argument('--dwg', type=pathlib.Path)
    parser.add_argument('--dxf', type=pathlib.Path)
    parser.add_argument('--out', type=pathlib.Path)
    args = parser.parse_args()
    if args.self_test:
        self_test()
        return
    if not all((args.dwg, args.dxf, args.out)):
        parser.error('--dwg, --dxf and --out are required')
    dwg, dxf = read_bounded(args.dwg), read_bounded(args.dxf)
    if not dwg.startswith(b'AC') or len(dwg) < 6:
        raise ValueError('DWG magic missing')
    result = inspect(dxf)
    result.update({'dwg_magic': dwg[:6].decode('ascii'),
                   'dwg_sha256': hashlib.sha256(dwg).hexdigest(),
                   'dxf_sha256': hashlib.sha256(dxf).hexdigest(),
                   'limitations': 'raw DXF observations only; no native semantic or source-pair proof'})
    # Exclusive creation prevents overwriting existing host files.
    with args.out.open('x', encoding='utf-8') as out:
        json.dump(result, out, indent=2, ensure_ascii=True)
        out.write('\n')
    print('Wrote raw observations to', args.out)


if __name__ == '__main__':
    try:
        main()
    except (ValueError, OSError, UnicodeError) as exc:
        print('error:', exc, file=sys.stderr)
        sys.exit(1)
