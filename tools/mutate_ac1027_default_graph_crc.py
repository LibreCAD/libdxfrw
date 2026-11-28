#!/usr/bin/env python3
"""Create bounded inner-CRC negatives for a selected uncompressed AC1027 file.

The caller supplies a hash-bound input, output directory and the independently
observed LAYER frame offset. The outer page checksums are resealed; these are
deliberately corrupt test drawings, not repair or interchange fixtures.
"""
import argparse
import hashlib
from pathlib import Path
import struct
import zlib

BEGIN = bytes.fromhex("cf7b1f23fdde38a95f7c68b84e6d335f")
END = bytes.fromhex("3084e0dc0221c756a0839747b192cca0")


def require(value, message):
    if not value:
        raise ValueError(message)


def u32(data, offset):
    return struct.unpack_from("<I", data, offset)[0]


def crc16(data):
    crc = 0xC0C1
    for byte in data:
        crc ^= byte
        for _ in range(8):
            crc = (crc >> 1) ^ ((crc & 1) * 0xA001)
    return crc


def find_page(data, section):
    found = []
    for offset in range(0x100, len(data) - 32):
        mask = 0x4164536B ^ offset
        if (u32(data, offset) ^ mask) != 0x4163043B:
            continue
        header = bytearray().join(struct.pack("<I", u32(data, offset + i) ^ mask)
                                  for i in range(0, 32, 4))
        fields = struct.unpack("<8I", header)
        if fields[1] != section:
            continue
        require(fields[2] == fields[3] and fields[4] == 0
                and offset + 32 + fields[2] <= len(data), "page is not one uncompressed source page")
        payload = data[offset + 32:offset + 32 + fields[2]]
        require(zlib.adler32(payload, 0) & 0xFFFFFFFF == fields[7], "source page data checksum")
        header[24:28] = bytes(4)
        require(zlib.adler32(header, fields[7]) & 0xFFFFFFFF == fields[6], "source page header checksum")
        found.append((offset, header, fields[2]))
    require(len(found) == 1, "expected one matching source page")
    return found[0]


def reseal(data, offset, header, size):
    start = offset + 32
    checksum = zlib.adler32(data[start:start + size], 0) & 0xFFFFFFFF
    struct.pack_into("<I", header, 28, checksum)
    struct.pack_into("<I", header, 24, zlib.adler32(header, checksum) & 0xFFFFFFFF)
    mask = 0x4164536B ^ offset
    for index in range(0, 32, 4):
        struct.pack_into("<I", data, offset + index, u32(header, index) ^ mask)


def generate(source, digest, destination, layer_offset):
    raw = source.read_bytes()
    require(hashlib.sha256(raw).hexdigest() == digest, "source SHA-256 mismatch")
    require(raw[:6] == b"AC1027", "not AC1027")
    destination.mkdir(parents=True, exist_ok=True)
    page, header, size = find_page(raw, 0)
    start = page + 32
    require(raw[start:start + 16] == BEGIN, "HEADER begin sentinel")
    end = raw.find(END, start + 16, start + size)
    require(end >= start + 26 and raw.find(END, end + 1, start + size) < 0, "HEADER end sentinel")
    crc_at = end - 2
    declared = u32(raw, start + 16)
    require(u32(raw, start + 20) == 0 and start + 24 + declared == crc_at, "unexpected HEADER size profile")
    require(crc16(raw[start + 16:crc_at]) == struct.unpack_from("<H", raw, crc_at)[0], "source HEADER CRC")
    bad = bytearray(raw)
    # Select the legacy size-includes-high-word envelope so a wrong CRC is a
    # warning after successful parsing rather than a rejected ambiguous envelope.
    struct.pack_into("<I", bad, start + 16, declared + 4)
    struct.pack_into("<H", bad, crc_at, crc16(bad[start + 16:crc_at]) ^ 1)
    reseal(bad, page, header, size)
    out = destination / "bad-header-included.dwg"
    with out.open("xb") as stream:
        stream.write(bad)
    print(f"{out} sha256={hashlib.sha256(bad).hexdigest()} inner_crc_offset={crc_at}")
    page, header, size = find_page(raw, 2)
    start = page + 32
    pos = start + layer_offset
    require(0 <= layer_offset and pos + 35 <= start + size, "LAYER offset outside page")
    # Restrict this tool to the observed 30-byte one-byte-handle LAYER frame.
    require(raw[pos:pos + 3] == bytes.fromhex("1e0055"), "unexpected selected LAYER framing")
    crc_at = pos + 3 + 30
    require(crc16(raw[pos:crc_at]) == struct.unpack_from("<H", raw, crc_at)[0], "source LAYER CRC")
    bad = bytearray(raw)
    bad[crc_at] ^= 1
    reseal(bad, page, header, size)
    out = destination / "bad-layer.dwg"
    with out.open("xb") as stream:
        stream.write(bad)
    print(f"{out} sha256={hashlib.sha256(bad).hexdigest()} inner_crc_offset={crc_at}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--sha256", required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--layer-offset", type=int, required=True)
    args = parser.parse_args()
    generate(args.source, args.sha256.lower(), args.output_dir, args.layer_offset)


if __name__ == "__main__":
    main()
