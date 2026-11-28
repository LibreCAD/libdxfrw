#!/usr/bin/env python3
"""Opt-in, hash-bound FIELD source-span differential; no fixture is bundled.

This checks only the frozen local AC1021 sample and its supplied ODA DXF and
ACadSharp reports. It does not change the DWG parser or establish support.
"""
import argparse
import hashlib
import re
import subprocess
import sys
from pathlib import Path

SOURCE_SHA = "3c95e8a82d5c8c251483fabc61efa8ca904f4e4bc8c2fce4786d74e812284d07"
MAX_BYTES = 20 * 1024 * 1024


def require(condition, message):
    if not condition:
        raise ValueError(message)


def checked_bytes(path, expected_sha):
    path = Path(path)
    require(path.is_file() and path.stat().st_size <= MAX_BYTES,
            f"missing or oversized input: {path}")
    data = path.read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    require(digest == expected_sha.lower(), f"digest mismatch: {path}: {digest}")
    return data


def bit_bytes(bits, position, count):
    require(position >= 0 and count >= 0 and position + 8 * count <= len(bits),
            "byte range escapes frame")
    return bytes(int(bits[p:p + 8], 2)
                 for p in range(position, position + 8 * count, 8))


def bit_long(bits, position):
    require(0 <= position and position + 2 <= len(bits), "missing BL tag")
    tag = bits[position:position + 2]
    position += 2
    if tag == "10":
        return 0, position
    if tag == "01":
        return bit_bytes(bits, position, 1)[0], position + 8
    if tag == "00":
        return int.from_bytes(bit_bytes(bits, position, 4), "little", signed=True), position + 32
    raise ValueError("unsupported BL selector")


def handle(bits, position):
    require(0 <= position and position + 8 <= len(bits), "missing handle header")
    code, length = int(bits[position:position + 4], 2), int(bits[position + 4:position + 8], 2)
    require(length <= 4, "handle length exceeds 32-bit source profile")
    return (code, int.from_bytes(bit_bytes(bits, position + 8, length), "big")), position + 8 + length * 8


def inspect_frame(data, field, owner, target, name, flags, owner_encoding='relative'):
    require(len(data) <= 1024 * 1024, "oversized FIELD frame")
    bits = ''.join(f"{value:08b}" for value in data)
    require(len(bits) >= 64 and bits[:18] == '001111110000000001', "unexpected FIELD type framing")
    object_end = int.from_bytes(bit_bytes(bits, 18, 4), "little")
    require(17 <= object_end <= len(bits) and bits[object_end - 1] == '1',
            "invalid AC1021 object or missing string stream")
    low_size = int.from_bytes(bit_bytes(bits, object_end - 17, 2), 'little')
    require((low_size & 0x8000) == 0, "extended string size outside frozen profile")
    body_end = object_end - 17 - low_size
    require(0 <= body_end < object_end - 17, "invalid string-stream bounds")
    matching = []
    for child_start in (287, 295):
        try:
            observed_flags, p = bit_long(bits, child_start)
            dtype, p = bit_long(bits, p)
            count, p = bit_long(bits, p)
            if observed_flags != flags or dtype != 4 or count not in (26, 32):
                continue
            value_start = p
            value = bit_bytes(bits, p, count)
            p += count * 8
            unit, p = bit_long(bits, p)
            if (value.decode('utf-16le') == name + '\0' and unit == 0
                    and p == body_end):
                matching.append((child_start, value_start, count, body_end))
        except (ValueError, UnicodeDecodeError):
            continue
    require(len(matching) == 1, f"{field:X}: in-body string span is absent or ambiguous")

    h1, p = handle(bits, object_end)
    h2, p = handle(bits, p)
    h3, p = handle(bits, p)
    require(owner_encoding in ('relative', 'absolute') and owner == field - 1,
            f"{field:X}: unsupported owner profile")
    expected_owner = (8, 0) if owner_encoding == 'relative' else (4, owner)
    require(h1 == expected_owner, f"{field:X}: {owner_encoding} owner role mismatch")
    require(h2 == (4, target) and h3 == (4, target),
            f"{field:X}: FIELD value/object soft-pointer mismatch")
    require(0 <= len(bits) - p < 8, f"{field:X}: unexplained handle tail")
    return matching[0]


def records_from_dxf(data):
    lines = data.decode('utf-8', errors='strict').splitlines()
    require(len(lines) % 2 == 0, 'odd DXF group stream')
    records, record = {}, []
    for i in range(0, len(lines), 2):
        code, value = int(lines[i].strip()), lines[i + 1].strip()
        if code == 0:
            if record and record[0] == (0, 'FIELD'):
                h = next((int(v, 16) for c, v in record if c == 5), None)
                require(h is not None and h not in records, 'missing/duplicate FIELD handle')
                records[h] = record
            record = [(code, value)]
        else:
            record.append((code, value))
    return records


def first(record, code):
    return next((value for c, value in record if c == code), None)


def selected_fields(records):
    selected = {}
    for h, record in records.items():
        if first(record, 1) != 'AcObjProp.16.2':
            continue
        root_at = next((i for i, (c, _) in enumerate(record) if c == 7), None)
        if root_at is None:
            continue
        root = record[root_at + 1:]
        if first(root, 93) != '4' or first(root, 90) != '2':
            continue
        children = {}
        for i, (code, value) in enumerate(record):
            if code == 6:
                end = next((j for j in range(i + 1, len(record))
                            if record[j][0] in (6, 7)), len(record))
                require(value not in children, f'{h:X}: duplicate child key')
                children[value] = record[i + 1:end]
        prop = children.get('ObjectPropertyName', [])
        ident = children.get('ObjectPropertyId', [])
        name, flag = first(prop, 1), first(prop, 93)
        objects = [v for c, v in record if c == 331]
        require(name is not None and flag in ('0', '2') and first(prop, 90) == '4'
                and first(prop, 94) == '0' and len(objects) == 1
                and first(ident, 93) == '2' and first(ident, 90) == '64'
                and first(ident, 330) == objects[0], f'{h:X}: DXF child/reference shape')
        prefix = record[:next(i for i, (c, _) in enumerate(record) if c == 100)]
        owner = first(prefix, 330)
        require(owner is not None, f'{h:X}: missing DXF owner')
        selected[h] = (int(owner, 16), int(objects[0], 16), name, int(flag))
    require(len(selected) == 42, f'expected 42 frozen FIELD profiles, got {len(selected)}')
    return selected


def acadsharp_fields(data):
    fields, current = {}, None
    for line in data.decode('utf-8', errors='strict').splitlines():
        m = re.match(r'^FIELD handle=0x([0-9A-F]+) ', line)
        if m:
            current = int(m.group(1), 16)
            require(current not in fields, 'duplicate ACadSharp field')
            fields[current] = {}
        elif current is not None and line.startswith('CHILD key=ObjectPropertyName '):
            m = re.search(r' value=(.*?) format=', line)
            require(m is not None, 'invalid ACadSharp child line')
            fields[current]['name'] = m.group(1)
        elif current is not None and line.startswith('CHILDREN '):
            fields[current]['objects'] = line.split(' OBJECTS ', 1)[1].strip()
    return fields


def self_test():
    # Hand-construct a bounded AC1021-shaped frame without any fixture bytes.
    bits = ['0'] * 672
    def put(at, values):
        text = ''.join(f'{value:08b}' for value in values)
        bits[at:at + len(text)] = text
    bits[:18] = '001111110000000001'
    put(18, (600).to_bytes(4, 'little'))
    bits[287:309] = '10' + '01' + f'{4:08b}' + '01' + f'{32:08b}'
    put(309, 'UpdatedDistance\0'.encode('utf-16le'))
    bits[565:567] = '10'
    put(583, (16).to_bytes(2, 'little'))
    bits[599] = '1'
    put(600, bytes.fromhex('80430144f4430144f4'))
    data = bytes(int(''.join(bits[i:i + 8]), 2) for i in range(0, len(bits), 8))
    args = (0x144FA, 0x144F9, 0x144F4, 'UpdatedDistance', 0)
    require(inspect_frame(data, *args) == (287, 309, 32, 567),
            'generated frame did not decode as expected')
    for bit in (299, 309, 608, 625):
        bad = bytearray(data)
        bad[bit // 8] ^= 1 << (7 - bit % 8)
        try:
            inspect_frame(bytes(bad), *args)
        except ValueError:
            continue
        raise ValueError(f'mutated source bit {bit} was accepted')
    try:
        inspect_frame(data[:40], *args)
    except ValueError:
        print('PASS: generated FIELD span and mutation controls')
        return 0
    raise ValueError('truncated source frame was accepted')


def main():
    if sys.argv[1:] == ['--self-test']:
        return self_test()
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', required=True, type=Path)
    parser.add_argument('--raw-probe', required=True, type=Path)
    parser.add_argument('--oda-dxf', required=True, type=Path)
    parser.add_argument('--oda-sha256', required=True)
    parser.add_argument('--acadsharp-log', required=True, type=Path)
    parser.add_argument('--acadsharp-sha256', required=True)
    args = parser.parse_args()
    checked_bytes(args.source, SOURCE_SHA)
    oda = checked_bytes(args.oda_dxf, args.oda_sha256)
    peer = checked_bytes(args.acadsharp_log, args.acadsharp_sha256)
    selected = selected_fields(records_from_dxf(oda))
    sharp = acadsharp_fields(peer)
    completed = subprocess.run([str(args.raw_probe), str(args.source)], capture_output=True,
                               timeout=60, check=True)
    require(len(completed.stdout) <= MAX_BYTES, 'oversized raw probe output')
    frames = {}
    for line in completed.stdout.decode('ascii', errors='strict').splitlines():
        if not line.startswith('RAW '):
            continue
        parts = line.split()
        require(len(parts) == 4, 'invalid raw probe output')
        handle_id, n, data = int(parts[1], 16), int(parts[2]), bytes.fromhex(parts[3])
        require(handle_id not in frames and n == len(data) <= 1024 * 1024,
                'duplicate or invalid raw frame')
        frames[handle_id] = data
    require(len(frames) == 142, f'expected 142 raw FIELD frames, got {len(frames)}')
    lengths = {}
    for field, (owner, target, name, flags) in selected.items():
        require(field in frames and field in sharp, f'{field:X}: missing peer or raw frame')
        require(sharp[field].get('name') == name
                and sharp[field].get('objects') == f'{target:X}',
                f'{field:X}: ACadSharp mismatch')
        _, _, count, _ = inspect_frame(frames[field], field, owner, target, name, flags)
        lengths[count] = lengths.get(count, 0) + 1
    require(lengths == {32: 41, 26: 1}, f'unexpected value byte counts: {lengths}')
    print(f'PASS: {len(selected)} exact FIELD body spans and owner/soft-pointer roles; '
          f'byte counts {lengths}; source={SOURCE_SHA}; oda={args.oda_sha256.lower()}; '
          f'acadsharp={args.acadsharp_sha256.lower()}')
    return 0


if __name__ == '__main__':
    try:
        sys.exit(main())
    except (OSError, ValueError, subprocess.SubprocessError) as exc:
        print(f'FAIL: {exc}', file=sys.stderr)
        sys.exit(1)
