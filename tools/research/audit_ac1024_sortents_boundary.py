#!/usr/bin/env python3
"""Check two adjacent AC1024 frames in a hash-bound decoded OBJECTS section.

Section decompression uses libdxfrw. The CRC and bit walker are separate from
its frame reader. This is source evidence, not export or semantic admission.
"""
import argparse
import hashlib
import json
import math
import struct
from pathlib import Path


def require(condition, message):
    if not condition:
        raise ValueError(message)


def digest(data):
    return hashlib.sha256(data).hexdigest()


def read_checked(path, expected):
    data = path.read_bytes()
    require(digest(data) == expected, f'unexpected hash: {path}')
    return data


def crc16(data):
    crc = 0xC0C1
    for byte in data:
        crc ^= byte
        for _ in range(8):
            crc = (crc >> 1) ^ ((crc & 1) * 0xA001)
    return crc


class Bits:
    def __init__(self, data):
        self.data = data
        self.bit = 0

    def take(self, count):
        require(self.bit + count <= len(self.data) * 8, 'truncated bit field')
        result = 0
        for index in range(self.bit, self.bit + count):
            result = (result << 1) | ((self.data[index // 8] >> (7 - index % 8)) & 1)
        self.bit += count
        return result

    def handle(self):
        code, size = self.take(4), self.take(4)
        require(size <= 4, 'unsupported handle width')
        return code, size, self.take(8 * size)

    def bs(self):
        tag = self.take(2)
        if tag == 0:
            return self.take(8) | (self.take(8) << 8)
        return self.take(8) if tag == 1 else 0 if tag == 2 else 256

    def bl(self):
        tag = self.take(2)
        if tag == 0:
            return sum(self.take(8) << (8 * index) for index in range(4))
        if tag == 1:
            return self.take(8)
        require(tag == 2, 'invalid bitlong tag')
        return 0

    def raw_double(self, require_finite=False):
        value = struct.unpack('<d', bytes(self.take(8) for _ in range(8)))[0]
        require(not require_finite or math.isfinite(value), 'non-finite double')
        return value

    def bit_double(self, require_finite=False):
        tag = self.take(2)
        if tag == 0:
            return self.raw_double(require_finite)
        require(tag in (1, 2), 'invalid bit-double tag')
        return 1.0 if tag == 1 else 0.0

    def ascii_text(self, max_length):
        length = self.bs()
        require(length <= max_length, 'text exceeds bounded profile')
        return bytes(self.take(8) for _ in range(length)).decode('ascii')


def frame(data, start, expected_size, expected_hbits, expected_type, expected_handle):
    require(start + 3 < len(data), 'truncated frame prefix')
    require(struct.unpack_from('<H', data, start)[0] == expected_size, 'unexpected frame size')
    pos, hbits, shift = start + 2, 0, 0
    while True:
        require(pos < len(data), 'truncated handle-size prefix')
        value = data[pos]
        pos += 1
        hbits |= (value & 127) << shift
        shift += 7
        if not value & 128:
            break
        require(shift <= 35, 'oversized handle-size prefix')
    require(hbits == expected_hbits, 'unexpected handle-stream size')
    end = pos + expected_size
    require(end + 2 <= len(data), 'truncated body or CRC')
    stored_crc = struct.unpack_from('<H', data, end)[0]
    require(stored_crc == crc16(data[start:end]), 'frame CRC mismatch')
    body = data[pos:end]
    bits = Bits(body)
    require(bits.take(2) == 1, 'unexpected BOT tag')
    require(bits.take(8) + 0x1F0 == expected_type, 'unexpected object type')
    require(bits.handle() == (0, 2, expected_handle), 'unexpected self handle')
    require(bits.bs() == 0 and bits.bl() == 1 and bits.take(1) == 1,
            'unexpected EED/reactor/xdictionary prefix')
    return {'frame_start': start, 'prefix_bytes': pos - start, 'body_start': pos,
            'body_size': expected_size, 'handle_bits': hbits,
            'object_data_bits': expected_size * 8 - hbits, 'body_end': end,
            'crc': hex(stored_crc), 'next_frame_start': end + 2,
            'type': expected_type, 'self_handle': hex(expected_handle),
            'body_sha256': digest(body)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('source', 'section', 'raw', 'trace'):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    source = read_checked(args.source, 'fad5a9f95ac72c1ef572b2b69f831ee08492135acd5da5ff76af01b2ec506881')
    data = read_checked(args.section, '8797ff454e743a063270cebdd51f669e6920157da9c1ab316c00c968bff64f7f')
    raw = read_checked(args.raw, 'e7d559d47f3084d2b924e197bb7f77624d9d0456d3477d0c9bbd681b9e1c8c37')
    trace_bytes = read_checked(args.trace, '44b9c4ac7e13e03e4245439af3aae77014783d3fd3acf18a7344779a89ac87a8')
    trace = trace_bytes.decode('utf-8', errors='replace')
    first = frame(data, 418479, 46, 214, 529, 0x236)
    second = frame(data, first['next_frame_start'], 16, 70, 530, 0x237)
    require(data[first['body_start']:first['body_end']] == raw, 'raw frame differs')
    require(first['crc'] == '0x2ae1' and second['crc'] == '0x79c1', 'peer CRC mismatch')
    for value in ('check_CRC 418479-418529 = 50: 2AE1 == 2AE1',
                  '< Next object: 196 Handleoff: 0x1 [UMC] Offset: 52 [MC] @418531',
                  'Object number: 196/C4, Size: 16 [MS], Hdlsize: 0x46 [UMC] , Type: 530 [BOT], Address: 418534',
                  'check_CRC 418531-418550 = 19: 79C1 == 79C1'):
        require(value in trace, f'peer trace lacks {value}')
    # Corruptions and all byte-prefix truncations must fail for each frame.
    for start, size, hbits, kind, handle, points, prefix in (
            (418479, 46, 214, 529, 0x236, (418479, 418483, 418528, 418529, 418530), 4),
            (418531, 16, 70, 530, 0x237, (418531, 418534, 418549, 418550, 418551), 3)):
        for point in points:
            changed = bytearray(data)
            changed[point] ^= 1
            try:
                frame(changed, start, size, hbits, kind, handle)
            except ValueError:
                pass
            else:
                raise ValueError(f'changed frame byte {point} accepted')
        for end in range(start, start + prefix + size + 2):
            try:
                frame(data[:end], start, size, hbits, kind, handle)
            except (ValueError, struct.error, IndexError):
                pass
            else:
                raise ValueError(f'truncated frame at {end} accepted')
    print(json.dumps({'mutation_checks': 10, 'prefix_truncation_checks': 73,
                      'source_sha256': digest(source), 'decoded_objects_sha256': digest(data),
                      'trace_sha256': digest(trace_bytes), 'section_bytes': len(data),
                      'frames': [first, second],
                      'limits': 'Section decompression uses libdxfrw. Producer identity, named-block semantics, strict output and native qualification remain open.'}, indent=2))


if __name__ == '__main__':
    main()
