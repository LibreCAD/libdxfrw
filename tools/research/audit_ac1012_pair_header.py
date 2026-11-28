#!/usr/bin/env python3
"""Audit the HEADER envelope in the locally generated AC1012 witness pair.

This checks physical framing and CRCs, not individual header-variable values
or AutoCAD's interpretation. The input packet and all mutations stay local.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

from audit_ac1024_sortents_boundary import crc16


FILE_SENTINEL = bytes.fromhex("95a04e2899821ae55e41e05f9d3a4d00")
HEADER_BEGIN = bytes.fromhex("cf7b1f23fdde38a95f7c68b84e6d335f")
HEADER_END = bytes.fromhex("3084e0dc0221c756a0839747b192cca0")
FILE_CRC_XOR = {3: 0xA598, 4: 0x8101, 5: 0x3CC4, 6: 0x8461}


def require(condition, message):
    if not condition:
        raise ValueError(message)


def file_crc(data):
    value = 0
    for byte in data:
        value ^= byte
        for _ in range(8):
            value = (value >> 1) ^ (0xA001 if value & 1 else 0)
    return value


def audit(data):
    require(len(data) >= 25 and data[:6] == b"AC1012", "not an AC1012 file")
    count = struct.unpack_from("<I", data, 21)[0]
    require(count in FILE_CRC_XOR, "unsupported locator count")
    crc_offset = 25 + count * 9
    first_section = crc_offset + 18
    require(first_section <= len(data), "truncated file header")
    locators = {}
    for index in range(count):
        kind, offset, size = struct.unpack_from("<BII", data, 25 + index * 9)
        require(kind not in locators, "duplicate locator")
        require(offset >= first_section and offset <= len(data) and size <= len(data) - offset,
                "locator outside file")
        locators[kind] = (offset, size)
    require(0 in locators, "missing HEADER locator")
    spans = sorted((offset, offset + size) for offset, size in locators.values() if size)
    require(all(left[1] <= right[0] for left, right in zip(spans, spans[1:])),
            "overlapping section locators")
    stored = struct.unpack_from("<H", data, crc_offset)[0]
    require(stored == file_crc(data[:crc_offset]) ^ FILE_CRC_XOR[count], "file header CRC mismatch")
    require(data[crc_offset + 2:first_section] == FILE_SENTINEL, "file header sentinel mismatch")
    offset, size = locators[0]
    require(size >= 38, "short HEADER envelope")
    section = data[offset:offset + size]
    require(section[:16] == HEADER_BEGIN, "HEADER begin sentinel mismatch")
    payload = struct.unpack_from("<I", section, 16)[0]
    require(payload == size - 38, "HEADER size does not cover its envelope")
    crc_offset = 20 + payload
    stored = struct.unpack_from("<H", section, crc_offset)[0]
    require(stored == crc16(section[16:crc_offset]), "HEADER CRC mismatch")
    require(section[crc_offset + 2:] == HEADER_END, "HEADER end sentinel mismatch")
    return {"fileSha256": hashlib.sha256(data).hexdigest(), "locatorCount": count,
            "sectionOffset": offset, "sectionSize": size, "payloadSize": payload,
            "sectionSha256": hashlib.sha256(section).hexdigest(),
            "sectionCrc": f"{stored:04x}", "unclaimedBytes": 0}


def check_mutations(data):
    count = struct.unpack_from("<I", data, 21)[0]
    require(count >= 2 and data[25] == 0, "unexpected witness locator order")
    file_crc_offset = 25 + 9 * count
    offset = struct.unpack_from("<I", data, 26)[0]
    size = struct.unpack_from("<I", data, 30)[0]
    mutations = {}
    for name, pos in (("file-crc", file_crc_offset), ("file-sentinel", file_crc_offset + 2),
                      ("header-begin", offset), ("header-payload", offset + 20),
                      ("header-size", offset + 16), ("header-end", offset + size - 1)):
        changed = bytearray(data)
        changed[pos] ^= 1
        mutations[name] = changed
    # Repair the outer CRC so malformed locators reach the structural checks.
    for name, position, replacement in (("duplicate-locator", 34, b"\x00"),
                                        ("out-of-range-locator", 26, b"\xff\xff\xff\xff")):
        changed = bytearray(data)
        changed[position:position + len(replacement)] = replacement
        struct.pack_into("<H", changed, file_crc_offset,
                         file_crc(changed[:file_crc_offset]) ^ FILE_CRC_XOR[count])
        mutations[name] = changed
    for name, changed in mutations.items():
        try:
            audit(changed)
        except ValueError:
            pass
        else:
            raise ValueError(f"mutation accepted: {name}")
    for length in range(len(data)):
        try:
            audit(data[:length])
        except ValueError:
            pass
        else:
            raise ValueError(f"truncation accepted at {length}")
    require(audit(data)["unclaimedBytes"] == 0, "clean retry failed")
    return sorted(mutations)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("packet", type=Path, help="existing populated-links witness packet")
    args = parser.parse_args()
    manifest = json.loads((args.packet / "manifest.json").read_text())
    require(manifest.get("profile") == "populated-links", "wrong packet profile")
    result = {}
    for side in ("destination", "source"):
        name = f"{side}.dwg"
        data = (args.packet / name).read_bytes()
        require(hashlib.sha256(data).hexdigest() == manifest["samples"][side]["files"][name],
                "packet source hash mismatch")
        result[side] = audit(data)
        result[side]["rejectedMutations"] = check_mutations(data)
        result[side]["truncationPositionsRejected"] = len(data)
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
