#!/usr/bin/env python3
"""Independently inspect one captured AC1018 TABLE roundtrip XRECORD.

Inputs are the pinned drawing and the 0xbc object body captured from that
same read. Hashes bind this specific observation; they do not authenticate an
arbitrary body-to-drawing association or authorize any output.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

SOURCE_SHA256 = "1ba45d704ed7cc846ea4d6d08c7c78062f8090842ebc9e9a4cdefcb44ea6ab2a"
BODY_SHA256 = "849458d8d7e0d40aa5e9c49d4f7ae0b0f9917e1cc56f8844f78b3bd077b9aa26"
EXPECTED = [
    (102, "ACAD_ROUNDTRIP_2008_TABLE_ENTITY", 45), (360, 0xB9),
    (70, 2), (90, 1), (10, (0.0, 0.0, 0.0)), (90, 0), (90, 2),
    (361, 0xBA), (102, "ACAD_ROUNDTRIP_PRE2007_TABLE", 45),
    (90, 3), (91, 3), (102, "ACAD_ROUNDTRIP_PRE2007_TABLECELL", 45),
    (360, 0xBD),
]


def require(condition, message):
    if not condition:
        raise ValueError(message)


class Cursor:
    def __init__(self, data, position=0):
        self.data = data
        self.position = position

    def bits(self, count):
        require(count >= 0 and self.position + count <= len(self.data) * 8,
                "field escapes bounded object body")
        value = 0
        for bit in range(self.position, self.position + count):
            value = (value << 1) | ((self.data[bit // 8] >> (7 - bit % 8)) & 1)
        self.position += count
        return value

    def raw(self, count):
        return bytes(self.bits(8) for _ in range(count))

    def short(self):
        tag = self.bits(2)
        if tag == 0:
            return int.from_bytes(self.raw(2), "little")
        if tag == 1:
            return self.bits(8)
        return 0 if tag == 2 else 256

    def long(self):
        tag = self.bits(2)
        if tag == 0:
            return int.from_bytes(self.raw(4), "little", signed=True)
        if tag == 1:
            return self.bits(8)
        require(tag == 2, "reserved BL selector")
        return 0

    def handle(self):
        start = self.position
        role, length = self.bits(4), self.bits(4)
        require(length <= 8, "handle length exceeds 64 bits")
        value = int.from_bytes(self.raw(length), "big")
        return {"startBit": start, "endBit": self.position,
                "role": role, "size": length, "value": value}


def inspect(data):
    require(len(data) == 213, "unexpected source body size")
    c = Cursor(data)
    object_type = c.short()
    body_end = int.from_bytes(c.raw(4), "little")
    identity = c.handle()
    eed_bytes = c.short()
    reactors = c.long()
    xdictionary_missing = c.bits(1)
    data_bytes = c.long()
    start = c.position
    require((object_type, body_end, identity["value"], eed_bytes, reactors,
             xdictionary_missing, data_bytes, start)
            == (79, 1667, 0xBC, 0, 1, 1, 197, 81),
            "XRECORD common prefix differs from pinned profile")
    payload = c.raw(data_bytes)
    require(c.position == 1657, "data block boundary differs")
    pos, items = 0, []

    def take(count):
        nonlocal pos
        require(count >= 0 and pos + count <= len(payload),
                "XRECORD pair escapes declared data block")
        out = payload[pos:pos + count]
        pos += count
        return out

    while pos < len(payload):
        item_start = pos
        code = int.from_bytes(take(2), "little")
        if code == 102:
            length = int.from_bytes(take(2), "little")
            page = take(1)[0]
            value = take(length).decode("ascii", errors="strict")
            row = (code, value, page)
        elif code in (360, 361):
            row = (code, int.from_bytes(take(8), "little"))
        elif code == 70:
            row = (code, int.from_bytes(take(2), "little"))
        elif code in (90, 91):
            row = (code, int.from_bytes(take(4), "little", signed=True))
        elif code == 10:
            row = (code, struct.unpack("<ddd", take(24)))
        else:
            raise ValueError("unexpected XRECORD pair code")
        items.append({"value": row, "startBit": start + item_start * 8,
                      "endBit": start + pos * 8})
    require([entry["value"] for entry in items] == EXPECTED,
            "ordered source pair sequence differs")
    clone_start = c.position
    cloning = c.short()
    require(cloning == 1 and c.position == body_end, "cloning or body boundary differs")
    owner, reactor = c.handle(), c.handle()
    require((owner["role"], owner["value"], owner["startBit"], owner["endBit"])
            == (4, 0xBB, 1667, 1683), "owner handle differs")
    require((reactor["role"], reactor["value"], reactor["startBit"], reactor["endBit"])
            == (4, 0xBB, 1683, 1699), "reactor handle differs")
    require(c.bits(5) == 0 and c.position == len(data) * 8, "padding differs")
    return {"objectType": object_type, "handle": identity, "bodyEndBit": body_end,
            "eedBytes": eed_bytes, "reactorCount": reactors,
            "xdictionaryMissing": xdictionary_missing, "dataStartBit": start,
            "dataEndBit": clone_start, "cloning": cloning, "pairs": items,
            "owner": owner, "reactor": reactor, "paddingBits": 5}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", required=True, type=Path)
    parser.add_argument("--body", required=True, type=Path)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    source = args.source.read_bytes()
    body = args.body.read_bytes()
    require(hashlib.sha256(source).hexdigest() == SOURCE_SHA256, "source hash mismatch")
    require(hashlib.sha256(body).hexdigest() == BODY_SHA256, "body hash mismatch")
    proof = inspect(body)
    if args.self_test:
        for bit in (0, 81, 113, 1667, 1683):
            damaged = bytearray(body)
            damaged[bit // 8] ^= 1 << (7 - bit % 8)
            try:
                inspect(bytes(damaged))
            except (ValueError, UnicodeError):
                continue
            raise ValueError("mutation was not refused at bit %d" % bit)
        try:
            inspect(body[:-1])
        except ValueError:
            pass
        else:
            raise ValueError("truncation was not refused")
        proof["selfTest"] = "six mutations/truncation rejected"
    proof["sourceSha256"] = SOURCE_SHA256
    proof["bodySha256"] = BODY_SHA256
    proof["limitation"] = "Pinned body-to-source association requires a separate actual-read capture receipt."
    print(json.dumps(proof, indent=2))


if __name__ == "__main__":
    main()
