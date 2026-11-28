#!/usr/bin/env python3
"""Inspect two hash-pinned AC1032 associative-network bodies and handle tails.

The OBJECTS argument is extracted by dwg_ac1032_object_section_probe.cpp;
that production decompressor is shared with libdxfrw, while this bit walker
independently reads its output using the ODA bit primitives. This is a
bounded source diagnostic, not an independent section decoder or admission
of the containing drawing for complete output.
"""
import argparse
import hashlib
import json
from pathlib import Path

SOURCE_HASH = "685bf22ed533121238c62c07357bf03fa6ddb40983cff7967c481fbb244b6b4f"
OBJECTS_HASH = "0d76bd5ae9e762cae2b8c972874d3cc908e41639501b8100e5eea5580175a5fb"


def checked(path, expected):
    data = Path(path).read_bytes()
    if hashlib.sha256(data).hexdigest() != expected:
        raise ValueError("unexpected bytes: " + str(path))
    return data


class Bits:
    def __init__(self, frame, start, end):
        if not (0 <= start <= end <= len(frame) * 8):
            raise ValueError("invalid frame bit bounds")
        self.frame, self.pos, self.end = frame, start, end

    def one(self):
        if self.pos >= self.end:
            raise ValueError("truncated bit field")
        value = (self.frame[self.pos // 8] >> (7 - self.pos % 8)) & 1
        self.pos += 1
        return value

    def byte(self):
        value = 0
        for _ in range(8):
            value = (value << 1) | self.one()
        return value

    def bitshort_or_long(self, kind):
        code = (self.one() << 1) | self.one()
        if code == 0:
            return int.from_bytes(bytes(self.byte() for _ in range(2 if kind == "BS" else 4)), "little")
        if code == 1:
            return self.byte()
        if code == 2:
            return 0
        if kind == "BS":
            return 256
        raise ValueError("invalid bitlong code")

    def handle(self):
        first = self.byte()
        length = first & 15
        if length > 4:
            raise ValueError("unsupported handle width")
        value = 0
        for _ in range(length):
            value = (value << 8) | self.byte()
        return {"code": first >> 4, "size": length, "raw_ref": value}


def inspect(data, *, handle, offset, size, body_end, fields, handles):
    if offset < 0 or size <= 0 or offset + size > len(data):
        raise ValueError("object outside decoded OBJECTS section")
    frame = data[offset:offset + size]
    bit = Bits(frame, 48, body_end)
    body = []
    for name, kind, expected in fields:
        start = bit.pos
        value = bit.one() if kind == "B" else bit.bitshort_or_long(kind)
        if value != expected:
            raise ValueError(f"{handle} {name}: observed {value}, expected {expected}")
        body.append({"field": name, "kind": kind, "start_bit": start,
                     "end_bit": bit.pos, "value": value})
    if bit.pos != body_end:
        raise ValueError("body prefix did not end at expected position")
    references = []
    for name, start, code, size_, raw_ref in handles:
        ref = Bits(frame, start, size * 8)
        value = ref.handle()
        if value != {"code": code, "size": size_, "raw_ref": raw_ref}:
            raise ValueError(f"{handle} {name}: unexpected handle encoding")
        references.append({"field": name, "start_bit": start,
                           "end_bit": ref.pos, **value})
    return {"handle": handle, "decoded_section_object_start": offset,
            "object_size": size, "fields": body, "handles": references}


def fields(action_index, is_owned):
    return [("class_version", "BS", 2), ("geometry_status", "BL", 0),
            ("action_index", "BL", action_index), ("max_assoc_dep_index", "BL", 0),
            ("num_deps", "BL", 0), ("reserved_before_owned_params", "BS", 0),
            ("num_owned_params", "BL", 0), ("reserved_before_values", "BS", 0),
            ("num_values", "BL", 0), ("network_version", "BS", 0),
            ("network_action_index", "BL", 1), ("num_actions", "BL", 1),
            ("action_is_owned", "B", is_owned), ("num_owned_actions", "BL", 0)]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source_dwg", help="hash-pinned source drawing")
    parser.add_argument("objects_bin", help="decoded OBJECTS section from the extractor")
    args = parser.parse_args()
    checked(args.source_dwg, SOURCE_HASH)
    data = checked(args.objects_bin, OBJECTS_HASH)
    if len(data) != 33803:
        raise ValueError("unexpected OBJECTS section length")
    child = inspect(data, handle="0x2A5", offset=33119, size=25, body_end=107,
                    fields=fields(1, 1), handles=[
                        ("owner", 108, 8, 0, 0), ("reactor", 116, 4, 2, 0x2A4),
                        ("owning_network", 140, 4, 2, 0x2A7),
                        ("action_body", 164, 3, 0, 0), ("action", 172, 3, 2, 0x2A8)])
    root = inspect(data, handle="0x2A7", offset=14822, size=22, body_end=99,
                   fields=fields(0, 0), handles=[
                       ("owner", 100, 8, 0, 0), ("reactor", 108, 4, 2, 0x2A6),
                       ("owning_network", 132, 4, 0, 0),
                       ("action_body", 140, 3, 0, 0), ("action", 148, 4, 2, 0x2A5)])
    print(json.dumps({"source_sha256": SOURCE_HASH, "objects_sha256": OBJECTS_HASH,
                      "profiles": [child, root]}, indent=2))


if __name__ == "__main__":
    main()
