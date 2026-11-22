#!/usr/bin/env python3
"""Verify one hash-bound R13 TABLE grid; no native semantics are inferred."""
import argparse
import hashlib
import json
import struct
from pathlib import Path

from audit_ac1024_sortents_boundary import Bits, crc16, require

SOURCE_HASH = "53558a8ae4bf666f906c9c2e4d3c3ee7ef61690e3b0f4d4a858e4e1bf555fc89"
FRAME_START = 142856
FRAME_SIZE = 4475


def read_frame(data, start, expected_size):
    require(start + 2 + expected_size + 2 <= len(data), "truncated frame")
    size = struct.unpack_from("<H", data, start)[0]
    require(size == expected_size, "unexpected frame size")
    end = start + 2 + size
    require(struct.unpack_from("<H", data, end)[0] == crc16(data[start:end]),
            "frame CRC mismatch")
    return data[start + 2:end]


def verify(data):
    raw = read_frame(data, FRAME_START, FRAME_SIZE)
    b = Bits(raw)
    spans = []

    def field(name, reader, expected):
        start = b.bit
        value = reader()
        require(value == expected, f"{name}: {value!r} != {expected!r}")
        spans.append({"name": name, "startBit": start, "endBit": b.bit,
                      "value": value})
        return value

    def bitdouble():
        tag = b.take(2)
        if tag == 0:
            return struct.unpack("<d", bytes(b.take(8) for _ in range(8)))[0]
        require(tag in (1, 2), "invalid bitdouble tag")
        return 1.0 if tag == 1 else 0.0

    def little32():
        return sum(b.take(8) << (8 * index) for index in range(4))

    field("type", b.bs, 528)
    field("self", b.handle, (0, 2, 0x4F2))
    field("eedSize", b.bs, 0)
    field("proxyPresent", lambda: b.take(1), 1)
    proxy_size = field("proxyBytes", little32, 2964)
    require(b.bit + proxy_size * 8 <= len(raw) * 8, "truncated proxy bytes")
    b.bit += proxy_size * 8
    field("dataEndBit", little32, 34236)
    field("entityMode", lambda: b.take(2), 2)
    field("reactorCount", b.bl, 0)
    field("byLayer", lambda: b.take(1), 1)
    field("noLinks", lambda: b.take(1), 0)
    field("color", b.bs, 256)
    field("linetypeScale", bitdouble, 1.0)
    field("invisible", b.bs, 0)
    require(b.bit == 23833, "common entity boundary mismatch")
    for name, expected in (("x", 3298.7947769117454), ("y", 839.6133480853896),
                           ("z", 0.0), ("xscale", 1.0), ("yscale", 1.0),
                           ("zscale", 1.0), ("rotation", 0.0), ("normalX", 0.0),
                           ("normalY", 0.0), ("normalZ", 1.0)):
        field(name, bitdouble, expected)
    field("hasAttribs", lambda: b.take(1), 0)
    field("tableFlag", b.bs, 22)
    for name, expected in (("horizontalX", 1.0), ("horizontalY", 0.0),
                           ("horizontalZ", 0.0)):
        field(name, bitdouble, expected)
    field("columnCount", b.bl, 9)
    field("rowCount", b.bl, 20)
    for index in range(9):
        field(f"columnWidth{index}", bitdouble, 63.5)
    for index in range(20):
        field(f"rowHeight{index}", bitdouble, 11.0 if index == 0 else 9.0)
    require(b.bit == 25932, "grid boundary mismatch")
    b.bit = 34236
    for name, expected in (("extensionDictionary", (3, 2, 0x105C)),
                           ("layer", (5, 1, 0x8A)),
                           ("previous", (8, 0, 0)),
                           ("next", (10, 2, 0x14E)),
                           ("block", (5, 2, 0x4F3)),
                           ("tableStyle", (5, 2, 0x29B))):
        field(name, b.handle, expected)
    following_start = FRAME_START + 2 + FRAME_SIZE + 2
    following = read_frame(data, following_start, 25)
    following_bits = Bits(following)
    require(following_bits.bs() == 7 and following_bits.handle() == (0, 2, 0x640),
            "following physical frame identity mismatch")
    return {"frameOffset": FRAME_START, "bodyBytes": FRAME_SIZE,
            "bodySha256": hashlib.sha256(raw).hexdigest(),
            "followingFrameOffset": following_start,
            "followingFrameHandle": "0x640", "spans": spans}


def self_test(data):
    # Keep CRC valid when changing a value or handle role, so rejection tests
    # the field mapping rather than only the outer checksum.
    end = FRAME_START + 2 + FRAME_SIZE
    candidates = []
    for label, relative_bit in (("wrong block role", 34308 + 3),
                                ("wrong style role", 34332 + 3),
                                ("wrong column count", 23998 + 2 + 7)):
        changed = bytearray(data)
        absolute_bit = (FRAME_START + 2) * 8 + relative_bit
        changed[absolute_bit // 8] ^= 0x80 >> (absolute_bit % 8)
        struct.pack_into("<H", changed, end, crc16(changed[FRAME_START:end]))
        candidates.append((label, changed))
    candidates.append(("truncated following frame", data[:end + 3]))
    for label, candidate in candidates:
        try:
            verify(candidate)
        except (ValueError, struct.error, IndexError):
            continue
        raise ValueError(f"{label} was accepted")
    verify(data)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    data = args.source.read_bytes()
    require(hashlib.sha256(data).hexdigest() == SOURCE_HASH, "source hash mismatch")
    result = verify(data)
    if args.self_test:
        self_test(data)
    result["sourceSha256"] = SOURCE_HASH
    result["controlsPassed"] = bool(args.self_test)
    result["limits"] = "Selected prefix and handles only; cells, native behavior and output remain unqualified."
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
