#!/usr/bin/env python3
"""Independently census AC1012/AC1014 frame types from a bounded object map.

This single-data-page observer is for generated research samples. It does not
prove native acceptance, semantics, or completeness of unaddressable bytes.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile

from legacy_dwg_dictionary_wire import Bits, crc16, require
from verify_oda_legacy_layer_graph import class_frame


def modular(data, pos, end, signed):
    value = 0
    for index in range(5):
        require(pos < end, 'truncated modular value')
        byte = data[pos]
        pos += 1
        if not byte & 0x80:
            negative = signed and bool(byte & 0x40)
            value |= (byte & (0x3f if signed else 0x7f)) << (7 * index)
            require(not negative or value != 0, 'negative zero modular value')
            return (-value if negative else value), pos
        value |= (byte & 0x7f) << (7 * index)
    raise ValueError('oversized modular value')


def census(data):
    require(len(data) >= 25 and data[:6] in (b'AC1012', b'AC1014'),
            'expected AC1012 or AC1014 source')
    count = int.from_bytes(data[21:25], 'little')
    require(count <= 64 and 25 + count * 9 <= len(data), 'bad section locator count')
    locators = []
    for index in range(count):
        offset = 25 + index * 9
        if data[offset] == 2:
            locators.append((int.from_bytes(data[offset + 1:offset + 5], 'little'),
                             int.from_bytes(data[offset + 5:offset + 9], 'little')))
    require(len(locators) == 1, 'missing or duplicate object-map locator')
    start, length = locators[0]
    end = start + length
    require(start >= 25 + count * 9 and length >= 8 and end <= len(data), 'bad object-map bounds')
    size = int.from_bytes(data[start:start + 2], 'big')
    require(2 < size <= 2032 and start + size + 6 == end, 'expected one object-map data page')
    require(int.from_bytes(data[start + size:start + size + 2], 'big')
            == crc16(data[start:start + size]), 'object-map data CRC mismatch')
    term = start + size + 2
    require(data[term:term + 2] == b'\x00\x02'
            and int.from_bytes(data[term + 2:term + 4], 'big') == crc16(data[term:term + 2]),
            'object-map terminator or CRC mismatch')
    pos = start + 2
    map_end = start + size
    previous_handle = previous_location = 0
    records = []
    locations = set()
    while pos < map_end:
        handle_delta, pos = modular(data, pos, map_end, False)
        location_delta, pos = modular(data, pos, map_end, True)
        previous_handle += handle_delta
        previous_location += location_delta
        require(handle_delta > 0 and 0 < previous_handle <= 0xffffffff
                and 0 <= previous_location < len(data)
                and previous_location not in locations, 'invalid map identity or address')
        locations.add(previous_location)
        address = previous_location
        require(address + 4 <= len(data), 'truncated frame prefix')
        body_size = int.from_bytes(data[address:address + 2], 'little')
        require(0 < body_size < 0x8000 and address + 2 + body_size + 2 <= len(data),
                'unsupported or truncated object frame')
        frame_end = address + 2 + body_size
        require(int.from_bytes(data[frame_end:frame_end + 2], 'little')
                == crc16(data[address:frame_end]), 'object frame CRC mismatch')
        bits = Bits(data[address + 2:frame_end])
        kind = bits.bs()
        own = bits.handle(0, own=True)
        require(own['value'] == previous_handle, 'map/frame handle mismatch')
        records.append({'handle': previous_handle, 'offset': address, 'type': kind,
                        'end': frame_end + 2,
                        'sha256': hashlib.sha256(data[address:frame_end + 2]).hexdigest()})
    require(pos == map_end and records, 'invalid object-map boundary')
    spans = sorted((row['offset'], row['end']) for row in records)
    require(all(a[1] <= b[0] for a, b in zip(spans, spans[1:])), 'overlapping object frames')
    require(all(not (a < end and b > start) for a, b in spans), 'object overlaps map section')
    return {'mapOffset': start, 'mapSize': length,
            'mapSha256': hashlib.sha256(data[start:end]).hexdigest(), 'frames': records}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--sha256', required=True)
    parser.add_argument('--dwgread', required=True)
    parser.add_argument('--class-name', required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    require(not args.output.resolve().is_relative_to(Path(__file__).resolve().parents[2]),
            'output must be outside repository')
    data = args.source.read_bytes()
    require(hashlib.sha256(data).hexdigest() == args.sha256, 'source hash mismatch')
    with tempfile.TemporaryDirectory(prefix='ac1012-class-census-') as temp:
        output = Path(temp) / 'libredwg.json'
        result = subprocess.run([args.dwgread, '-O', 'JSON', '-o', str(output), str(args.source)],
                                capture_output=True, check=False)
        require(result.returncode == 0 and output.is_file(), 'LibreDWG decode failed')
        peer = json.loads(output.read_text())
    selected = [row for row in peer['CLASSES'] if row.get('dxfname') == args.class_name]
    require(len(selected) == 1, 'missing or ambiguous class')
    class_proof = class_frame(args.source, peer['CLASSES'], selected[0]['number'])
    proof = census(data)
    mapped = {row['handle']: row['type'] for row in proof['frames']}
    decoded = {row['handle'][-1]: row['type'] for row in peer['OBJECTS']}
    require(len(decoded) == len(peer['OBJECTS']) and mapped == decoded,
            'physical map and peer decoded census differ')
    count = sum(row['type'] == selected[0]['number'] for row in proof['frames'])
    version = data[:6].decode('ascii')
    report = {'schema': ('libdxfrw.ac1012-class-census.v1' if version == 'AC1012'
                         else 'libdxfrw.ac1014-class-census.v1'),
              'sourceVersion': version, 'sourceSha256': args.sha256,
              'classSection': class_proof, 'selectedClassMapFrameCount': count,
              'objectMap': proof, 'peerDecodedCount': len(decoded), 'supportClaimEffect': 'none',
              'limits': ['Single data page, bounded short object frames, AC1012/AC1014 only.',
                         'Absence from the addressable object map does not prove native semantics or output CLASS behavior.']}
    with args.output.open('x', encoding='utf-8') as stream:
        json.dump(report, stream, indent=2, sort_keys=True)
        stream.write('\n')
    print(f'{version} class census: PASS ({len(mapped)} mapped frames; {count} selected)')


if __name__ == '__main__':
    main()
