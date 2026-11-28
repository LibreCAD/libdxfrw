#!/usr/bin/env python3
"""Make a scratch DWG with a substituted literal HEADER suffix.

This is an experiment tool, not a format repair or an output qualification.
It only handles a suffix, or its trailing part, that occurs exactly once as a
contiguous literal in the specified R2004-family compressed data page. It
preserves the validated HEADER envelope and recomputes the page checksums. The
caller supplies a decoded logical HEADER and the compressed page's physical
offset; the caller must independently decompress and verify each result.
"""

import argparse
import hashlib
import struct
import zlib
from pathlib import Path


BEGIN = bytes.fromhex("cf7b1f23fdde38a95f7c68b84e6d335f")
END = bytes.fromhex("3084e0dc0221c756a0839747b192cca0")


def crc16(data):
    crc = 0xC0C1
    for byte in data:
        crc ^= byte
        for _ in range(8):
            crc = (crc >> 1) ^ (0xA001 if crc & 1 else 0)
    return crc


def checksum(data, seed=0):
    return zlib.adler32(data, seed) & 0xFFFFFFFF


def run(source, decoded, destination, payload_offset, variant, preserve_prefix):
    drawing = bytearray(source.read_bytes())
    section = decoded.read_bytes()
    if len(section) < 38 or not section.startswith(BEGIN):
        raise ValueError("invalid HEADER beginning sentinel or length")
    end = section.find(END)
    if end < 22 or section.find(END, end + 1) != -1:
        raise ValueError("HEADER ending sentinel is missing or ambiguous")
    declared = struct.unpack_from("<I", section, 16)[0]
    crc_pos = end - 2
    if not (20 + declared == crc_pos or
            (len(section) >= 24 and section[20:24] == bytes(4)
             and 24 + declared == crc_pos)):
        raise ValueError("HEADER size does not terminate at its CRC")
    if crc16(section[16:crc_pos]) != struct.unpack_from("<H", section, crc_pos)[0]:
        raise ValueError("HEADER CRC mismatch")
    suffix = section[end + len(END):]
    if not suffix:
        raise ValueError("HEADER has no suffix to test")
    if preserve_prefix < 0 or preserve_prefix >= len(suffix):
        raise ValueError("suffix prefix must leave at least one byte to replace")
    literal = suffix[preserve_prefix:]
    page_offset = payload_offset - 32
    if page_offset < 0 or payload_offset > len(drawing):
        raise ValueError("page offset is outside the file")
    mask = 0x4164536B ^ page_offset
    header = bytearray().join(
        struct.pack("<I", struct.unpack_from("<I", drawing, page_offset + i)[0]
                    ^ (mask & 0xFFFFFFFF)) for i in range(0, 32, 4)
    )
    fields = struct.unpack("<8I", header)
    if fields[0] != 0x4163043B:
        raise ValueError("not an R2004-family data-page header")
    compressed_size = fields[2]
    if compressed_size > len(drawing) - payload_offset:
        raise ValueError("compressed page extends past the file")
    payload = bytes(drawing[payload_offset:payload_offset + compressed_size])
    if checksum(payload) != fields[7]:
        raise ValueError("original page data checksum mismatch")
    struct.pack_into("<I", header, 24, 0)
    if checksum(header, fields[7]) != fields[6]:
        raise ValueError("original page header checksum mismatch")
    offset = payload.find(literal)
    if offset < 0 or payload.find(literal, offset + 1) >= 0:
        raise ValueError("chosen suffix portion is not one unique contiguous literal")
    replacement = (bytes(len(literal)) if variant == "zero" else
                   bytes((index * 73 + 19) & 0xFF
                         for index in range(preserve_prefix, len(suffix))))
    start = payload_offset + offset
    drawing[start:start + len(literal)] = replacement
    data_checksum = checksum(drawing[payload_offset:payload_offset + compressed_size])
    struct.pack_into("<I", header, 28, data_checksum)
    struct.pack_into("<I", header, 24, checksum(header, data_checksum))
    for i in range(0, 32, 4):
        struct.pack_into("<I", drawing, page_offset + i,
                         struct.unpack_from("<I", header, i)[0] ^ (mask & 0xFFFFFFFF))
    # Exclusive creation prevents replacing the source or an existing result.
    with destination.open("xb") as output:
        output.write(drawing)
    print(f"suffix_length={len(suffix)} preserved_prefix={preserve_prefix} "
          f"literal_offset=0x{start:x} "
          f"sha256={hashlib.sha256(drawing).hexdigest()}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("decoded_header", type=Path)
    parser.add_argument("destination", type=Path)
    parser.add_argument("--payload-offset", type=lambda value: int(value, 0), required=True)
    parser.add_argument("--variant", choices=("zero", "pattern"), required=True)
    parser.add_argument("--preserve-suffix-prefix", type=int, default=0)
    args = parser.parse_args()
    run(args.source, args.decoded_header, args.destination, args.payload_offset,
        args.variant, args.preserve_suffix_prefix)


if __name__ == "__main__":
    main()
