#!/usr/bin/env python3
"""Verify hash-bound R13/R14 TABLE grids; no native semantics are inferred."""
import argparse
import hashlib
import json
import struct
from pathlib import Path

from audit_ac1024_sortents_boundary import Bits, crc16, require

PROFILES = {
    "53558a8ae4bf666f906c9c2e4d3c3ee7ef61690e3b0f4d4a858e4e1bf555fc89": {
        "version": "r13", "frameStart": 142856, "frameSize": 4475,
        "previewSize": 2964, "bodyEnd": 34236, "commonEnd": 23833,
        "gridEnd": 25932, "extension": 0x105C, "nextSize": 25,
        "nextType": 7,
    },
    "aec626c5680584c763418497495950089854ff9682af36076851fd793d9e2c0b": {
        "version": "r14", "frameStart": 63008, "frameSize": 5051,
        "previewSize": 3496, "bodyEnd": 38842, "commonEnd": 28089,
        "gridEnd": 30188, "extension": 0xCAA, "nextSize": 992,
        "nextType": 532,
    },
}


def read_frame(data, start, expected_size):
    require(start + 2 + expected_size + 2 <= len(data), "truncated frame")
    size = struct.unpack_from("<H", data, start)[0]
    require(size == expected_size, "unexpected frame size")
    end = start + 2 + size
    require(struct.unpack_from("<H", data, end)[0] == crc16(data[start:end]),
            "frame CRC mismatch")
    return data[start + 2:end]


def verify(data, profile):
    frame_start = profile["frameStart"]
    frame_size = profile["frameSize"]
    raw = read_frame(data, frame_start, frame_size)
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
    proxy_size = field("proxyBytes", little32, profile["previewSize"])
    require(b.bit + proxy_size * 8 <= len(raw) * 8, "truncated proxy bytes")
    b.bit += proxy_size * 8
    field("dataEndBit", little32, profile["bodyEnd"])
    field("entityMode", lambda: b.take(2), 2)
    field("reactorCount", b.bl, 0)
    field("byLayer", lambda: b.take(1), 1)
    field("noLinks", lambda: b.take(1), 0)
    field("color", b.bs, 256)
    field("linetypeScale", bitdouble, 1.0)
    field("invisible", b.bs, 0)
    require(b.bit == profile["commonEnd"], "common entity boundary mismatch")
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
    require(b.bit == profile["gridEnd"], "grid boundary mismatch")
    b.bit = profile["bodyEnd"]
    for name, expected in (("extensionDictionary", (3, 2, profile["extension"])),
                           ("layer", (5, 1, 0x8A)),
                           ("previous", (8, 0, 0)),
                           ("next", (10, 2, 0x14E)),
                           ("block", (5, 2, 0x4F3)),
                           ("tableStyle", (5, 2, 0x29B))):
        field(name, b.handle, expected)
    following_start = frame_start + 2 + frame_size + 2
    following = read_frame(data, following_start, profile["nextSize"])
    following_bits = Bits(following)
    require(following_bits.bs() == profile["nextType"]
            and following_bits.handle() == (0, 2, 0x640),
            "following physical frame identity mismatch")
    return {"version": profile["version"], "frameOffset": frame_start, "bodyBytes": frame_size,
            "bodySha256": hashlib.sha256(raw).hexdigest(),
            "followingFrameOffset": following_start,
            "followingFrameHandle": "0x640", "spans": spans}


def self_test(data, profile, proof):
    # Keep CRC valid when changing a value or handle role, so rejection tests
    # the field mapping rather than only the outer checksum.
    frame_start = profile["frameStart"]
    end = frame_start + 2 + profile["frameSize"]
    starts = {row["name"]: row["startBit"] for row in proof["spans"]}
    candidates = []
    for label, relative_bit in (("wrong block role", starts["block"] + 3),
                                ("wrong style role", starts["tableStyle"] + 3),
                                ("wrong column count", starts["columnCount"] + 2 + 7)):
        changed = bytearray(data)
        absolute_bit = (frame_start + 2) * 8 + relative_bit
        changed[absolute_bit // 8] ^= 0x80 >> (absolute_bit % 8)
        struct.pack_into("<H", changed, end, crc16(changed[frame_start:end]))
        candidates.append((label, changed))
    candidates.append(("truncated following frame", data[:end + 3]))
    for label, candidate in candidates:
        try:
            verify(candidate, profile)
        except (ValueError, struct.error, IndexError):
            continue
        raise ValueError(f"{label} was accepted")
    verify(data, profile)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    data = args.source.read_bytes()
    source_hash = hashlib.sha256(data).hexdigest()
    profile = PROFILES.get(source_hash)
    require(profile is not None, "source hash mismatch")
    result = verify(data, profile)
    if args.self_test:
        self_test(data, profile, result)
    result["sourceSha256"] = source_hash
    result["controlsPassed"] = bool(args.self_test)
    result["limits"] = "Selected prefix and handles only; cells, native behavior and output remain unqualified."
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
