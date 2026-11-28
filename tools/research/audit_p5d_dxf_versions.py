#!/usr/bin/env python3
"""Check AC1024 HEADER markers in a P5-D ASCII/Binary DXF pair.

This is a bounded header screen, not a whole-file DXF or semantic validator.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
from pathlib import Path

from audit_binary_dxf_layout_nod import SENTINEL, read_group_value


def binary_pairs(data):
    offset = len(SENTINEL)
    while offset < len(data):
        if offset + 2 > len(data):
            raise ValueError('truncated binary group code')
        code = struct.unpack_from('<H', data, offset)[0]
        value, offset = read_group_value(data, offset + 2, code)
        yield code, value


def ascii_pairs(data):
    lines = iter(data.splitlines())
    while True:
        try:
            raw_code = next(lines)
        except StopIteration:
            return
        try:
            raw_value = next(lines)
        except StopIteration:
            raise ValueError('truncated ASCII group/value pair') from None
        try:
            code = int(raw_code.strip())
            value = raw_value.decode('ascii') if code in (0, 1, 2, 9) else raw_value
        except (ValueError, UnicodeDecodeError) as exc:
            raise ValueError('invalid ASCII header group') from exc
        yield code, value


def header_version(pairs):
    iterator = iter(pairs)
    in_header = False
    found = []
    for code, value in iterator:
        if code == 0 and value == 'SECTION':
            section_code, section_name = next(iterator, (None, None))
            if section_code != 2:
                raise ValueError('SECTION missing name')
            in_header = section_name == 'HEADER'
        elif in_header and code == 0 and value == 'ENDSEC':
            if len(found) != 1:
                raise ValueError('HEADER must contain exactly one $ACADVER')
            return found[0]
        elif in_header and code == 9 and value == '$ACADVER':
            value_code, version = next(iterator, (None, None))
            if value_code != 1 or not isinstance(version, str):
                raise ValueError('invalid $ACADVER value')
            found.append(version)
        elif code == 0 and value == 'EOF':
            break
    raise ValueError('missing or incomplete HEADER section')


def audit(path, expected_encoding):
    data = path.read_bytes()
    encoding = 'binary' if data.startswith(SENTINEL) else 'ascii'
    if encoding != expected_encoding:
        raise ValueError(f'expected {expected_encoding}, got {encoding}')
    version = header_version(binary_pairs(data) if encoding == 'binary' else ascii_pairs(data))
    if version != 'AC1024':
        raise ValueError(f'expected AC1024, got {version!r}')
    return {'file': path.name, 'sha256': hashlib.sha256(data).hexdigest(),
            'bytes': len(data), 'encoding': encoding, 'acadver': version}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('ascii_dxf', type=Path)
    parser.add_argument('binary_dxf', type=Path)
    args = parser.parse_args()
    try:
        results = [audit(args.ascii_dxf, 'ascii'), audit(args.binary_dxf, 'binary')]
    except (OSError, ValueError) as exc:
        parser.exit(1, f'P5-D DXF header check failed: {exc}\n')
    print(json.dumps({'result': 'PASS_HEADER_ONLY', 'files': results}, indent=2))


if __name__ == '__main__':
    main()
