#!/usr/bin/env python3
"""Compare bounded modeler revision suffixes in hash-pinned AC1027/ODA pairs.

Diagnostic only: ODA output supplies the proposed GUID; reverse candidates need
not be unique and neither the revision flag nor native semantic parity is proved.
"""
import argparse
import json
import subprocess
import sys
import uuid
from pathlib import Path

from ac1027_acds_source_compare import Bits, checked, dxf_records, one, require, MAX_BYTES


def rawbits(data):
    return ''.join(f'{b:08b}' for b in data)


def enc(value, short):
    if value == 0: return '10'
    if short and value == 256: return '11'
    if value <= 255: return '01' + rawbits(bytes([value]))
    return '00' + rawbits(value.to_bytes(2 if short else 4, 'little'))


def candidates(bits, end):
    tail = end - 3 - 64
    require(tail >= 1 and bits[end-3:end] == '100', 'unexpected footer')
    result = []
    for a in (2, 10, 34):
        for b in (2, 10, 18):
            for c in (2, 10, 18):
                start = tail-a-b-c
                if start < 1: continue
                reader = Bits(bytes())
                reader.bits, reader.pos = bits, start
                try:
                    x, y, z = reader.long(), reader.short(), reader.short()
                    if reader.pos != tail or enc(x, False)+enc(y, True)+enc(z, True) != bits[start:tail]:
                        continue
                    result.append(start-1)
                except (ValueError, OverflowError):
                    continue
    return sorted(set(result))


def verify(raw, handle_bits, uid, flag, handles, common_end=None):
    bits = rawbits(raw)
    end = len(bits)-handle_bits
    require(0 < end <= len(bits), 'invalid object/handle boundary')
    u = uuid.UUID(uid)
    body = enc(u.time_low, False)+enc(u.time_mid, True)+enc(u.time_hi_version, True)+rawbits(u.bytes[8:])
    pattern = str(flag)+body+'100'
    start = end-len(pattern)
    require(flag in (0, 1) and start >= 0 and bits[start:end] == pattern, 'revision suffix mismatch')
    reader = Bits(raw)
    reader.pos = end
    spans = []
    for expected in handles:
        pos = reader.pos
        actual = reader.handle()
        require(actual == expected, f'following handle mismatch at {pos}: {actual} != {expected}')
        spans.append([pos, reader.pos, *actual])
    require(0 <= len(bits)-reader.pos < 8, 'unexpected trailing handle data')
    starts = candidates(bits, end)
    require(start in starts, 'peer-bound GUID missing from reverse candidates')
    result = {'object_end': end, 'revision_flag_bit': start, 'guid_bits': [start+1, end-3],
            'footer_bits': [end-3, end], 'candidate_flag_starts': starts,
            'handle_spans': spans, 'padding_bits': len(bits)-reader.pos}
    if common_end is not None:
        require(0 <= common_end < start, 'invalid common-entity boundary')
        b = Bits(raw)
        b.pos = common_end
        empty = b.take(1)
        forward = {'common_end': common_end, 'acis_empty': empty,
                   'acis_empty_span': [common_end, b.pos]}
        if not empty:
            forward['unknown_bit'] = b.take(1)
            at = b.pos
            forward['oda_version'] = b.short()
            forward['oda_version_span'] = [at, b.pos]
            forward['version_crosses_peer_guid_boundary'] = b.pos > start
        result['tentative_common_prefix'] = forward
    return result


def projection(data):
    records = dxf_records(data)
    layers = {}
    for section, rec in records:
        if section == 'TABLES' and rec[0][1] == 'LAYER':
            name = one(rec, 2)
            require(name not in layers, 'duplicate layer name')
            layers[name] = int(one(rec, 5), 16)
    entities = {}
    for section, rec in records:
        if section != 'ENTITIES' or rec[0][1] not in ('REGION', '3DSOLID', 'BODY'): continue
        identity = int(one(rec, 5), 16)
        require(identity not in entities and (100, 'AcDbModelerGeometry') in rec, 'bad entity identity/class')
        reactors, inside = [], False
        for code, value in rec:
            if code == 102 and value == '{ACAD_REACTORS':
                require(not inside, 'nested reactor group')
                inside = True
            elif code == 102 and value == '}' and inside: inside = False
            elif code == 330 and inside: reactors.append(int(value, 16))
        require(not inside, 'unclosed reactor group')
        layer = one(rec, 8)
        require(layer in layers, 'missing output layer')
        history = [int(v, 16) for c, v in rec if c == 350]
        require(len(history) <= 1, 'ambiguous history')
        handles = [(4, h) for h in reactors]+[(5, layers[layer])]+[(3, h) for h in history]
        entities[identity] = (rec[0][1], one(rec, 2), int(one(rec, 290)), handles)
    require(entities, 'no output modelers')
    return entities


def self_test():
    for flag, uid in [(0, '00000000-0000-0000-0000-000000000000'),
                      (1, '11223344-5566-7788-99aa-bbccddeeff00')]:
        u = uuid.UUID(uid)
        prefix = '1010101010101010'
        suffix = str(flag)+enc(u.time_low, False)+enc(u.time_mid, True)+enc(u.time_hi_version, True)+rawbits(u.bytes[8:])+'100'
        handle = '01010001' + '10001010'
        bits = prefix+suffix+handle
        bits += '0' * (-len(bits) % 8)
        raw = bytes(int(bits[i:i+8], 2) for i in range(0, len(bits), 8))
        end = len(prefix+suffix)
        hs = len(bits)-end
        result = verify(raw, hs, uid, flag, [(5, 0x8a)])
        require(result['revision_flag_bit'] == len(prefix), 'wrong flag position')
        for pos in (len(prefix), len(prefix)+3, end-5, end-2, end+9):
            changed = bytearray(raw)
            changed[pos//8] ^= 1 << (7-pos%8)
            try: verify(bytes(changed), hs, uid, flag, [(5, 0x8a)])
            except ValueError: continue
            raise ValueError(f'mutation accepted at bit {pos}')
        try: verify(raw[:-1], hs, uid, flag, [(5, 0x8a)])
        except ValueError: continue
        raise ValueError('truncated frame accepted')
    print('PASS: zero/nonzero UUID, source bits, footer, handle, mutation and truncation')


def main():
    if sys.argv[1:] == ['--self-test']:
        self_test()
        return
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('source', 'oda-dxf', 'probe'): parser.add_argument('--'+name, type=Path, required=True)
    for name in ('source-sha256', 'oda-sha256'): parser.add_argument('--'+name, required=True)
    args = parser.parse_args()
    require(checked(args.source, args.source_sha256).startswith(b'AC1027'), 'expected AC1027 source')
    peer = projection(checked(args.oda_dxf, args.oda_sha256))
    run = subprocess.run([str(args.probe), str(args.source)], capture_output=True, check=True, timeout=60)
    require(len(run.stdout) <= MAX_BYTES, 'probe output too large')
    frames, common_ends = {}, {}
    for line in run.stdout.decode('ascii').splitlines():
        row = line.split()
        if row and row[0] == 'REVISION_FRAME':
            require(len(row) == 4, 'bad frame row')
            identity = int(row[1], 16)
            require(identity not in frames, 'duplicate frame')
            frames[identity] = (bytes.fromhex(row[3]), int(row[2]))
        elif row and row[0] == 'COMMON_END':
            require(len(row) == 3, 'bad common end row')
            identity = int(row[1], 16)
            require(identity not in common_ends, 'duplicate common end')
            common_ends[identity] = int(row[2])
    require(set(frames) == set(peer) == set(common_ends), 'source/peer entity census mismatch')
    for identity, (kind, uid, flag, handles) in sorted(peer.items()):
        result = verify(*frames[identity], uid, flag, handles, common_ends[identity])
        result.update(handle=f'{identity:X}', kind=kind, peer_uid=uid, peer_290=flag)
        print(json.dumps(result, sort_keys=True))
    print('PASS: peer-bound suffix and following handles; candidates are not semantic proof')


if __name__ == '__main__':
    try: main()
    except (OSError, ValueError, subprocess.SubprocessError) as exc:
        print(f'FAIL: {exc}', file=sys.stderr)
        sys.exit(1)
