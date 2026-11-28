#!/usr/bin/env python3
"""Walk one hash-pinned AC1032 constraint-group prefix and handle stream.

The OBJECTS section comes from libdxfrw's production decompressor. Field
order is a candidate from LibreDWG; the independent bounded bit walk checks
its selected physical values and offsets, not full node or output semantics.
"""
import argparse
import json
import struct

from ac1032_forum_assocnetwork_spans import Bits, OBJECTS_HASH, SOURCE_HASH, checked


def bitdouble(bits):
    code = (bits.one() << 1) | bits.one()
    if code == 0:
        return struct.unpack("<d", bytes(bits.byte() for _ in range(8)))[0]
    if code == 1:
        return 1.0
    if code == 2:
        return 0.0
    raise ValueError("invalid bitdouble code")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source_dwg")
    parser.add_argument("objects_bin")
    args = parser.parse_args()
    checked(args.source_dwg, SOURCE_HASH)
    data = checked(args.objects_bin, OBJECTS_HASH)
    offset, size = 33150, 596
    if len(data) != 33803 or offset + size > len(data):
        raise ValueError("invalid decoded source section or frame bounds")
    frame = data[offset:offset + size]
    bits = Bits(frame, 82, 121)
    observed = []
    for name, kind, expected in [
        ("group_version", "BL", 2), ("group_flag", "B", 0),
        ("origin_x", "BD", 0), ("origin_y", "BD", 0), ("origin_z", "BD", 0),
        ("axis_x_x", "BD", 1), ("axis_x_y", "BD", 0), ("axis_x_z", "BD", 0),
        ("axis_y_x", "BD", 0), ("axis_y_y", "BD", 1), ("axis_y_z", "BD", 0),
        ("num_actions", "BL", 2),
    ]:
        start = bits.pos
        value = bits.one() if kind == "B" else (bitdouble(bits) if kind == "BD" else bits.bitshort_or_long(kind))
        if value != expected:
            raise ValueError("unexpected field: " + name)
        observed.append({"field": name, "kind": kind, "start_bit": start,
                         "end_bit": bits.pos, "value": value})
    if bits.pos != 121:
        raise ValueError("candidate prefix did not reach its expected boundary")
    handles = []
    for name, start, code, size_, value in [
        ("owner", 4575, 12, 1, 3), ("owning_network", 4591, 4, 2, 0x2A5),
        ("action_body", 4615, 3, 0, 0), ("group_reference", 4623, 3, 0, 0),
        ("action_0", 4631, 3, 2, 0x2AA), ("action_1", 4655, 3, 2, 0x2A9),
    ]:
        probe = Bits(frame, start, size * 8)
        got = probe.handle()
        if got != {"code": code, "size": size_, "raw_ref": value}:
            raise ValueError("unexpected handle: " + name)
        handles.append({"field": name, "start_bit": start, "end_bit": probe.pos, **got})
    print(json.dumps({"source_sha256": SOURCE_HASH, "objects_sha256": OBJECTS_HASH,
                      "handle": "0x2A8", "decoded_section_object_start": offset,
                      "object_size": size, "fields": observed, "handles": handles}, indent=2))


if __name__ == "__main__":
    main()
