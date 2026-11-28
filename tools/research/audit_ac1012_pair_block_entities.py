#!/usr/bin/env python3
"""Audit block, layer and linetype frames of the generated AC1012 witness pair.

This is a bounded source-wire observer, not native cloning evidence.
"""
import argparse
import json
from pathlib import Path
import struct

from generate_ac1012_dictionary_merge_samples import read_object_frame
from generate_ac1012_dictionary_witness import Bits, require, sha


def observe_control(frame: bytes, profile: str = "block") -> dict:
    """Bind the selected BLOCK, LAYER or LTYPE control table frame."""
    bits = Bits(frame)
    spans = {}

    def field(label, read):
        start = bits.position
        value = read()
        spans[label] = {"startBit": start, "endBit": bits.position, "value": value}
        return value

    profiles = {"block": (1, 48, 0, (("model", 3, 0x1f), ("paper", 3, 0x20))),
                "layer": (2, 50, 1, (("entry", 2, 0x10),)),
                "linetype": (5, 56, 1, (("entry", 2, 0x16),
                                        ("byBlock", 3, 0x14), ("byLayer", 3, 0x15)))}
    handle, kind, count, slots = profiles[profile]
    require(field("type", bits.bs) == kind
            and field("self", lambda: bits.handle(0, own=True))["value"] == handle
            and field("eedSize", bits.bs) == 0, "unexpected control prefix")
    boundary = field("dataEnd", lambda: bits.little(4))
    require(field("reactorCount", bits.bl) == 0 and field("count", bits.bl) == count
            and bits.position == boundary, "unexpected control body")
    for label, role, expected in (("owner", 4, 0), ("extension", 3, 0)) + slots:
        value = field(label, lambda: bits.handle(role, handle))
        require(value["role"] == role and value["value"] == expected,
                f"wrong control {label}")
    require(len(frame) * 8 - bits.position < 8, "unexpected control tail")
    return {"frameSha256": sha(frame), "spans": spans, "paddingBits": len(frame) * 8 - bits.position}


def observe_layer(frame: bytes) -> dict:
    """Bind the exact generated R13 LAYER 0 body and its reference roles."""
    bits = Bits(frame)
    spans = {}

    def field(label, read):
        start = bits.position
        value = read()
        spans[label] = {"startBit": start, "endBit": bits.position, "value": value}
        return value

    require(field("type", bits.bs) == 51
            and field("self", lambda: bits.handle(0, own=True))["value"] == 0x10
            and field("eedSize", bits.bs) == 0, "unexpected LAYER prefix")
    boundary = field("dataEnd", lambda: bits.little(4))
    require(field("reactorCount", bits.bl) == 0, "unexpected LAYER reactors")
    length = field("nameLength", bits.bs)
    require(length == 1 and field("name", lambda: bits.take(8)) == ord("0")
            and field("xref64", lambda: bits.take(1)) == 1
            and field("xrefIndexPlusOne", bits.bs) == 0
            and field("xrefDependent", lambda: bits.take(1)) == 0
            and field("legacyFlags", lambda: bits.take(4)) == 0
            and field("color", bits.bs) == 7 and bits.position == boundary,
            "unexpected LAYER body")
    for label, role, expected, raw_role in (("owner", 4, 2, 12), ("extension", 3, 0, 3),
                                             ("xref", 5, 0, 5), ("linetype", 5, 0x16, 5)):
        value = field(label, lambda: bits.handle(role, 0x10))
        require(value["role"] == raw_role and value["value"] == expected,
                f"wrong LAYER {label}")
    require(len(frame) * 8 - bits.position < 8, "unexpected LAYER tail")
    return {"frameSha256": sha(frame), "spans": spans, "paddingBits": len(frame) * 8 - bits.position}


def observe_linetype(frame: bytes, handle: int, name: str, description: str) -> dict:
    """Bind the selected R13 zero-dash standard LTYPE frames."""
    bits = Bits(frame)
    spans = {}

    def field(label, read):
        start = bits.position
        value = read()
        spans[label] = {"startBit": start, "endBit": bits.position, "value": value}
        return value

    def read_text():
        length = bits.bs()
        require(length <= 128, "linetype text exceeds profile")
        return bytes(bits.take(8) for _ in range(length)).decode("ascii")

    require(field("type", bits.bs) == 57
            and field("self", lambda: bits.handle(0, own=True))["value"] == handle
            and field("eedSize", bits.bs) == 0, "unexpected LTYPE prefix")
    boundary = field("dataEnd", lambda: bits.little(4))
    require(field("reactorCount", bits.bl) == 0 and field("name", read_text) == name
            and field("xref64", lambda: bits.take(1)) == 1
            and field("xrefIndexPlusOne", bits.bs) == 0
            and field("xrefDependent", lambda: bits.take(1)) == 0
            and field("description", read_text) == description
            and field("zeroPatternTag", lambda: bits.take(2)) == 2
            and field("alignment", lambda: bits.take(8)) == 65
            and field("dashCount", lambda: bits.take(8)) == 0,
            "unexpected LTYPE body")
    area = field("textArea", lambda: bytes(bits.take(8) for _ in range(256)).hex())
    require(area == "00" * 256 and bits.position == boundary, "unexpected LTYPE string area")
    for label, role, expected, raw_role in (("owner", 4, 5, 12), ("extension", 3, 0, 3),
                                             ("xref", 5, 0, 5)):
        value = field(label, lambda: bits.handle(role, handle))
        require(value["role"] == raw_role and value["value"] == expected,
                f"wrong LTYPE {label}")
    require(len(frame) * 8 - bits.position < 8, "unexpected LTYPE tail")
    return {"frameSha256": sha(frame), "spans": spans, "paddingBits": len(frame) * 8 - bits.position}


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
    offset, raw = read_object_frame(path, dwgread, 1, 48)
    control = observe_control(raw)
    for label in ("owner", "extension", "model", "paper"):
        damaged = bytearray(raw)
        bit = control["spans"][label]["startBit"]
        damaged[bit // 8] ^= 0x80 >> (bit % 8)
        try:
            observe_control(bytes(damaged))
        except ValueError:
            pass
        else:
            raise ValueError(f"wrong BLOCK_CONTROL {label} role accepted")
    for length in range(len(raw)):
        try:
            observe_control(raw[:length])
        except ValueError:
            pass
        else:
            raise ValueError("truncated BLOCK_CONTROL accepted")
    require(observe_control(raw) == control, "unchanged BLOCK_CONTROL retry differs")
    dependencies = {}
    for label, handle, kind, reader, handle_names in (
            ("layerControl", 2, 50, lambda f: observe_control(f, "layer"), ("owner", "extension", "entry")),
            ("layer", 0x10, 51, observe_layer, ("owner", "extension", "xref", "linetype")),
            ("linetypeControl", 5, 56, lambda f: observe_control(f, "linetype"),
             ("owner", "extension", "entry", "byBlock", "byLayer")),
            ("byBlock", 0x14, 57, lambda f: observe_linetype(f, 0x14, "BYBLOCK", ""), ("owner", "extension", "xref")),
            ("byLayer", 0x15, 57, lambda f: observe_linetype(f, 0x15, "BYLAYER", ""), ("owner", "extension", "xref")),
            ("continuous", 0x16, 57, lambda f: observe_linetype(f, 0x16, "CONTINUOUS", "Solid line"), ("owner", "extension", "xref"))):
        location, frame = read_object_frame(path, dwgread, handle, kind)
        layer_proof = reader(frame)
        for field_name in handle_names:
            damaged = bytearray(frame)
            bit = layer_proof["spans"][field_name]["startBit"]
            damaged[bit // 8] ^= 0x80 >> (bit % 8)
            try:
                reader(bytes(damaged))
            except ValueError:
                pass
            else:
                raise ValueError(f"wrong {label} {field_name} role accepted")
        for length in range(len(frame)):
            try:
                reader(frame[:length])
            except ValueError:
                pass
            else:
                raise ValueError(f"truncated {label} accepted")
        require(reader(frame) == layer_proof, f"unchanged {label} retry differs")
        dependencies[label] = {"offset": location, "proof": layer_proof,
                               "wrongRoleAndTruncationRejectedWithRetry": True}
    return {"sourceSha256": sha(path.read_bytes()), "objects": result, **dependencies,
            "blockControl": {"offset": offset, "proof": control,
                             "wrongRoleAndTruncationRejectedWithRetry": True}}


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
