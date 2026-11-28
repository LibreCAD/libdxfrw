#!/usr/bin/env python3
"""Audit BLOCK/ENDBLK frames of the locally generated AC1012 witness pair.

This is a bounded source-wire observer, not native cloning evidence.
"""
import argparse
import json
from pathlib import Path
import struct

from generate_ac1012_dictionary_merge_samples import read_object_frame
from generate_ac1012_dictionary_witness import Bits, require, sha


def observe(frame: bytes, handle: int, kind: int, mode: int, name: str) -> dict:
    bits = Bits(frame)
    spans = {}

    def field(label, read):
        start = bits.position
        value = read()
        spans[label] = {"startBit": start, "endBit": bits.position, "value": value}
        return value

    def bit_double():
        tag = bits.take(2)
        if tag == 0:
            return struct.unpack("<d", bytes(bits.take(8) for _ in range(8)))[0]
        require(tag in (1, 2), "invalid bit-double")
        return 1.0 if tag == 1 else 0.0

    def text_value():
        length = bits.bs()
        require(length <= 128, "block name exceeds profile")
        raw = bytes(bits.take(8) for _ in range(length))
        require(raw.count(b"\0") <= 1 and (b"\0" not in raw or raw.endswith(b"\0")),
                "invalid block name terminator")
        return (raw[:-1] if raw.endswith(b"\0") else raw).decode("ascii")

    require(field("type", bits.bs) == kind
            and field("self", lambda: bits.handle(0, own=True))["value"] == handle
            and field("eedSize", bits.bs) == 0 and field("graphics", lambda: bits.take(1)) == 0,
            "unexpected entity prefix")
    boundary = field("dataEnd", lambda: bits.little(4))
    require(field("entityMode", lambda: bits.take(2)) == mode
            and field("reactorCount", bits.bl) == 0
            and field("byLayerLinetype", lambda: bits.take(1)) == 1
            and field("noLinks", lambda: bits.take(1)) == 0
            and field("color", bits.bs) == 256
            and field("linetypeScale", bit_double) == 1.0
            and field("invisible", bits.bs) == 0, "unexpected common entity values")
    if kind == 4:
        require(field("name", text_value) == name, "unexpected block name")
    require(bits.position == boundary, "entity body boundary mismatch")
    extension = field("extension", lambda: bits.handle(3, handle))
    layer = field("layer", lambda: bits.handle(5, handle))
    previous = field("previous", lambda: bits.handle(4, handle))
    following = field("next", lambda: bits.handle(4, handle))
    require(extension["role"] == 3 and extension["value"] == 0
            and layer["role"] == 5 and layer["value"] == 0x10
            and previous["role"] == 4 and previous["value"] == 0
            and following["role"] == 4 and following["value"] == 0
            and len(frame) * 8 - bits.position < 8, "unexpected common entity handles or tail")
    return {"frameSha256": sha(frame), "spans": spans, "paddingBits": len(frame) * 8 - bits.position}


def audit(path: Path, dwgread: str) -> dict:
    objects = ((0x21, 4, 1, "*PAPER_SPACE"), (0x22, 5, 1, ""),
               (0x24, 4, 2, "*MODEL_SPACE"), (0x25, 5, 2, ""))
    result = {}
    for handle, kind, mode, name in objects:
        offset, raw = read_object_frame(path, dwgread, handle, kind)
        proof = observe(raw, handle, kind, mode, name)
        for label in ("extension", "layer", "previous", "next"):
            damaged = bytearray(raw)
            bit = proof["spans"][label]["startBit"]
            damaged[bit // 8] ^= 0x80 >> (bit % 8)
            try:
                observe(bytes(damaged), handle, kind, mode, name)
            except ValueError:
                pass
            else:
                raise ValueError(f"wrong {label} role accepted for {handle:X}")
        for length in range(len(raw)):
            try:
                observe(raw[:length], handle, kind, mode, name)
            except (ValueError, UnicodeDecodeError):
                pass
            else:
                raise ValueError(f"truncated frame accepted for {handle:X}")
        require(observe(raw, handle, kind, mode, name) == proof, "unchanged frame retry differs")
        result[f"{handle:X}"] = {"offset": offset, "proof": proof,
                                  "wrongRoleAndTruncationRejectedWithRetry": True}
    return {"sourceSha256": sha(path.read_bytes()), "objects": result}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("destination", type=Path)
    parser.add_argument("--dwgread", default="dwgread")
    args = parser.parse_args()
    print(json.dumps({"source": audit(args.source, args.dwgread),
                      "destination": audit(args.destination, args.dwgread)}, indent=2))


if __name__ == "__main__":
    main()
