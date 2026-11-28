#!/usr/bin/env python3
"""Make the documented scratch-only AC1014 IMAGEDEF zero-size probe."""
import argparse
import hashlib
from pathlib import Path

SOURCE_SHA = "26e46e206b35ab23152aad4ed1d96116d8c97c5dfa67a7df1e5b9b1a8cb3f70e"
FRAME, BODY, BODY_SIZE = 12698, 12700, 62


def crc16(data):
    value = 0xc0c1
    for byte in data:
        value ^= byte
        for _ in range(8):
            value = (value >> 1) ^ (0xa001 if value & 1 else 0)
    return value


def take(bits, pos, count):
    return int(bits[pos:pos + count], 2)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("output", type=Path, help="new scratch path; never overwritten")
    args = parser.parse_args()
    data = bytearray(args.source.read_bytes())
    if hashlib.sha256(data).hexdigest() != SOURCE_SHA or data[:6] != b"AC1014":
        raise ValueError("unexpected source drawing")
    end = BODY + BODY_SIZE
    if len(data) < end + 2 or crc16(data[FRAME:end]) != int.from_bytes(data[end:end + 2], "little"):
        raise ValueError("source object CRC mismatch")
    bits = "".join(f"{byte:08b}" for byte in data[BODY:end])
    if (take(bits, 0, 2) != 0
        or int.from_bytes(bytes((take(bits, 2, 8), take(bits, 10, 8))), "little") != 514
        or take(bits, 18, 8) != 1 or take(bits, 26, 8) != 0x31
        or take(bits, 34, 2) != 2
        or int.from_bytes(bytes(take(bits, 36 + 8 * n, 8) for n in range(4)), "little") != 435):
        raise ValueError("source IMAGEDEF span or identity mismatch")
    for pos in range(36, 68):
        data[BODY + pos // 8] &= ~(1 << (7 - pos % 8))
    data[end:end + 2] = crc16(data[FRAME:end]).to_bytes(2, "little")
    with args.output.open("xb") as output:
        output.write(data)
    print("sha256=" + hashlib.sha256(data).hexdigest())
    print("IMAGEDEF 0x31 object-size 435 -> 0; object CRC resealed")


if __name__ == "__main__":
    main()
