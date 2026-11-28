#!/usr/bin/env python3
"""Audit extracted AC1027 HEADER sections; never classify suffix bytes."""
import argparse
import hashlib
import json
from pathlib import Path

BEGIN = bytes.fromhex("cf7b1f23fdde38a95f7c68b84e6d335f")
END = bytes.fromhex("3084e0dc0221c756a0839747b192cca0")


def crc16(data):
    value = 0xc0c1
    for byte in data:
        value ^= byte
        for _ in range(8):
            value = (value >> 1) ^ (0xa001 if value & 1 else 0)
    return value


def inspect(path):
    data = path.read_bytes()
    if len(data) < 42 or data[:16] != BEGIN or data.count(END) != 1:
        raise ValueError("missing or ambiguous HEADER sentinel")
    end = data.index(END)
    payload_end = end - 2
    count = int.from_bytes(data[16:20], "little")
    if payload_end not in (count + 20, count + 24):
        raise ValueError("declared HEADER size does not locate trailer")
    if payload_end < 20 or crc16(data[16:payload_end]) != int.from_bytes(data[payload_end:end], "little"):
        raise ValueError("HEADER CRC mismatch")
    trailer_end = end + len(END)
    suffix = data[trailer_end:]
    return {"bytes": len(data), "section_sha256": hashlib.sha256(data).hexdigest(),
            "size_word": count, "payload_end": payload_end,
            "stored_crc": int.from_bytes(data[payload_end:end], "little"),
            "trailer_end": trailer_end, "suffix_bytes": len(suffix),
            "suffix_sha256": hashlib.sha256(suffix).hexdigest()}, suffix


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("resave", type=Path)
    parser.add_argument("reference", type=Path, help="another extracted section with a suffix")
    parser.add_argument("--require-empty-source-suffix", action="store_true",
                        help="reject a nonempty source suffix")
    for label in ("source", "resave", "reference"):
        parser.add_argument("--expected-" + label + "-sha256", metavar="SHA256",
                            help="require the extracted " + label + " section digest")
    args = parser.parse_args()
    source, _ = inspect(args.source)
    resave, suffix = inspect(args.resave)
    reference, other = inspect(args.reference)
    if args.require_empty_source_suffix and source["suffix_bytes"] != 0:
        raise ValueError("source suffix is nonempty")
    for label, result in (("source", source), ("resave", resave),
                          ("reference", reference)):
        expected = getattr(args, "expected_" + label + "_sha256")
        if expected is not None and result["section_sha256"] != expected.lower():
            raise ValueError(label + " section SHA-256 mismatch")
    if len(suffix) != len(other) or not suffix:
        raise ValueError("suffix lengths differ or are empty")
    bits = {(a ^ b) & 1 for a, b in zip(suffix, other)}
    if len(bits) != 1:
        raise ValueError("suffix LSB relation is not uniform")
    print(json.dumps({"source": source, "resave": resave, "reference": reference,
                      "suffix_lsb_xor": bits.pop(),
                      "interpretation": "observation only; suffix remains unclaimed; converter lineage is external"}, indent=2))


if __name__ == "__main__":
    main()
