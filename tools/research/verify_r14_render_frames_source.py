#!/usr/bin/env python3
"""Verify three external R14 render frames without copying the drawing.

This is source-specific field-span and framing evidence, not a general
render schema, producer-semantics, output, or support qualification.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

from audit_ac1024_sortents_boundary import Bits, crc16, require
from generate_ac1012_dictionary_witness import Bits as LegacyBits

SOURCE_SHA256 = "ab655649e8a1059d0557fb167eeb8d9a5d802afc8313d60d4bcabdce790bac4c"
FRAME = 1243059
NEXT_FRAME = 1243090
RAPID_FRAME = 1242831
MENTAL_FRAME = 1242922
ROOT_DICTIONARY = 1240933
MENTAL_DICTIONARY = 1504068
MENTAL_XRECORD = 1512884


def frame(data, start, size):
    require(start >= 0 and start + 2 + size + 2 <= len(data), "truncated frame")
    require(struct.unpack_from("<H", data, start)[0] == size, "frame size mismatch")
    end = start + 2 + size
    require(struct.unpack_from("<H", data, end)[0] == crc16(data[start:end]),
            "frame CRC mismatch")
    return data[start + 2:end]


def verify_dictionary(data, start, size, own, body_end, count, r14_byte,
                      expected_owner, expected_xdict, expected_entries, padding):
    raw = frame(data, start, size)
    bits = LegacyBits(raw)
    require(bits.bs() == 42 and bits.handle(0, own=True)["value"] == own,
            "dictionary type or identity mismatch")
    require(bits.bs() == 0 and bits.little(4) == body_end
            and bits.bl() == 0 and bits.bl() == count and bits.take(8) == r14_byte,
            "dictionary prefix mismatch")
    names = []
    for _ in range(count):
        size_in_bytes = bits.bs()
        value = bytes(bits.take(8) for _ in range(size_in_bytes))
        require(value and value[-1] == 0, "invalid dictionary name")
        names.append(value[:-1].decode("cp932"))
    require(len(set(names)) == count and bits.position == body_end,
            "dictionary names or data boundary mismatch")
    owner = bits.handle(4, own)
    xdict = bits.handle(3, own)
    entries = {name: bits.handle(2, own)["value"] for name in names}
    require(owner["value"] == expected_owner and xdict["value"] == expected_xdict
            and all(entries.get(name) == value for name, value in expected_entries.items())
            and len(raw)*8 - bits.position == padding, "dictionary graph or tail mismatch")
    return {"frameOffset": start, "frameSize": size, "bodySha256": hashlib.sha256(raw).hexdigest(),
            "handle": own, "owner": owner, "extensionDictionary": xdict,
            "entryCount": count, "selectedEntries": expected_entries, "r14Byte": r14_byte}


def verify_preset(data, start, size, kind):
    raw = frame(data, start, size)
    bits = Bits(raw)
    fields = []

    def field(name, reader, expected):
        offset = bits.bit
        value = reader()
        require(value == expected, f"{kind}.{name}: unexpected value {value!r}")
        fields.append({"name": name, "startBit": offset, "endBit": bits.bit,
                       "value": value})

    def text():
        length = bits.bs()
        value = bytes(bits.take(8) for _ in range(length))
        require(not value or value[-1] == 0, "unterminated TV")
        return value[:-1].decode("cp932") if value else ""

    def double():
        tag = bits.take(2)
        if tag == 0:
            return struct.unpack("<d", bytes(bits.take(8) for _ in range(8)))[0]
        require(tag in (1, 2), "invalid bitdouble tag")
        return 1.0 if tag == 1 else 0.0

    one_bit = lambda: bits.take(1)
    mental = kind == "mentalray"
    field("type", bits.bs, 518 if mental else 517)
    field("self", bits.handle, (0, 2, 0x5E8 if mental else 0x216))
    field("eedSize", bits.bs, 0)
    body_end = 1002 if mental else 653
    field("objectDataEnd", lambda: sum(bits.take(8) << (8*i) for i in range(4)), body_end)
    field("reactorCount", bits.bl, 1)
    field("classVersion", bits.bl, 2)
    field("name", text, "高" if mental else "コーヒー ブレイク品質")
    for name, expected in zip(("fog", "fogBackground", "backfaces", "environmentImage"),
                              (1, 1 if mental else 0, 1, 1)):
        field(name, one_bit, expected)
    field("environmentFilename", text, "")
    field("description", text, "" if mental else "10 分のレンダリング")
    field("displayIndex", bits.bl, 0)
    if not mental:
        for name, expected in zip(("version", "target", "level", "time", "lighting", "filter"),
                                  (3, 1, 1, 10, 1, 2)):
            field(name, bits.bl, expected)
        field("filterWidth", double, 3.0)
        field("filterHeight", double, 3.0)
        field("predefined", one_bit, 1)
    else:
        for name, expected in zip(("version", "sampling1", "sampling2"), (2, 0, 2)):
            field(name, bits.bl, expected)
        field("filter", bits.bs, 3)
        for name, expected in zip(("filterWidth", "filterHeight", "contrast1", "contrast2",
                                   "contrast3", "contrast4"),
                                  (4.0, 4.0, 0.05000000074505806, 0.05000000074505806,
                                   0.05000000074505806, 0.05000000074505806)):
            field(name, double, expected)
        field("shadowMode", bits.bs, 0)
        field("shadowMaps", one_bit, 0)
        field("rayTracing", one_bit, 1)
        for index in range(3):
            field(f"rayDepth{index+1}", bits.bl, 7)
        field("globalIllumination", one_bit, 0)
        field("giSampleCount", bits.bl, 500)
        field("giRadiusEnabled", one_bit, 0)
        field("giRadius", double, 1.0)
        field("photonsPerLight", bits.bl, 10000)
        for index in range(3):
            field(f"photonDepth{index+1}", bits.bl, 5)
        field("finalGathering", one_bit, 0)
        field("finalGatheringRays", bits.bl, 500)
        for index in range(3):
            field(f"radiusState{index+1}", one_bit, 0)
        for name, expected in (("radius1", 0.1), ("radius2", 1.0), ("luminance", 1500.0)):
            field(name, double, expected)
        field("diagnosticsMode", bits.bs, 0)
        field("diagnosticsGridMode", bits.bs, 0)
        field("diagnosticsGridFloat", double, 10.0)
        field("diagnosticsPhotonMode", bits.bs, 0)
        field("diagnosticsBspMode", bits.bs, 0)
        field("exportMi", one_bit, 0)
        field("mentalDescription", text, "")
        field("tileSize", bits.bl, 32)
        field("tileOrder", bits.bs, 0)
        field("memoryLimit", bits.bl, 1048)
        field("diagnosticsSamples", one_bit, 0)
        field("energyMultiplier", double, 1.0)
    require(bits.bit == body_end, f"{kind}: data/handle boundary mismatch")
    field("owner", bits.handle, (4, 1, 0xC))
    field("reactor", bits.handle, (4, 1, 0xC))
    field("extensionDictionary", bits.handle, (3, 2, 0x5E9) if mental else (3, 0, 0))
    require(len(raw)*8 - bits.bit == (6 if mental else 3), "unexpected handle tail or padding")
    return {"kind": kind, "frameOffset": start, "frameSize": size,
            "bodySha256": hashlib.sha256(raw).hexdigest(), "fields": fields}


def verify_xrecord(data):
    # ODA v5.4.1 section 20.4.104: R13/R14 XRECORD and short-valued tags.
    raw = frame(data, MENTAL_XRECORD, 29)
    bits = LegacyBits(raw)
    require(bits.bs() == 541 and bits.handle(0, own=True)["value"] == 0x5EA,
            "XRECORD identity mismatch")
    require(bits.bs() == 0 and bits.little(4) == 192 and bits.bl() == 1
            and bits.bl() == 12, "XRECORD prefix mismatch")
    tags = [(bits.little(2), bits.little(2)) for _ in range(3)]
    require(tags == [(70, 2), (71, 4), (72, 0)] and bits.position == 192,
            "XRECORD data or boundary mismatch")
    owner = bits.handle(4, 0x5EA)
    reactor = bits.handle(4, 0x5EA)
    xdict = bits.handle(3, 0x5EA)
    require(owner == {"role": 8, "length": 0, "value": 0x5E9}
            and reactor == {"role": 4, "length": 2, "value": 0x5E9}
            and xdict == {"role": 3, "length": 0, "value": 0}
            and bits.position == len(raw) * 8, "XRECORD handle tail mismatch")
    return {"frameOffset": MENTAL_XRECORD, "frameSize": len(raw),
            "bodySha256": hashlib.sha256(raw).hexdigest(), "tags": tags,
            "owner": owner, "reactor": reactor, "extensionDictionary": xdict}


def verify(data):
    require(data[:6] == b"AC1014", "not AC1014")
    root = verify_dictionary(data, ROOT_DICTIONARY, 647, 0xC, 4584, 28, 0,
                             0, 0x592,
                             {"ACAD_RENDER_ACTIVE_RAPIDRT": 0x216,
                              "ACAD_RENDER_ACTIVE_SETTINGS": 0x5E8,
                              "ACAD_RENDER_GLOBAL": 0x463}, 0)
    extension = verify_dictionary(data, MENTAL_DICTIONARY, 50, 0x5E9, 354, 1, 1,
                                  0x5E8, 0,
                                  {"ADSK_XREC_MENTALRAYSETTINGSINFO": 0x5EA}, 6)
    rapid = verify_preset(data, RAPID_FRAME, 87, "rapidrt")
    mental = verify_preset(data, MENTAL_FRAME, 133, "mentalray")
    require(RAPID_FRAME + 2 + 87 + 2 == MENTAL_FRAME
            and MENTAL_FRAME + 2 + 133 + 2 == FRAME,
            "render frame boundaries are not consecutive")
    raw = frame(data, FRAME, 27)
    require(FRAME + 2 + len(raw) + 2 == NEXT_FRAME, "next frame boundary mismatch")
    bits = Bits(raw)
    fields = []

    def field(name, reader, expected):
        start = bits.bit
        value = reader()
        require(value == expected, f"{name}: unexpected value {value!r}")
        fields.append({"name": name, "startBit": start, "endBit": bits.bit,
                       "value": value})

    field("type", bits.bs, 539)
    field("self", bits.handle, (0, 2, 0x463))
    field("eedSize", bits.bs, 0)
    field("objectDataEnd", lambda: sum(bits.take(8) << (8*i) for i in range(4)), 173)
    field("reactorCount", bits.bl, 1)
    field("classVersion", bits.bl, 2)
    field("procedure", bits.bl, 0)
    field("destination", bits.bl, 0)
    field("saveEnabled", lambda: bits.take(1), 0)
    field("filenameLength", bits.bs, 0)
    field("imageWidth", bits.bl, 500)
    field("imageHeight", bits.bl, 500)
    field("predefinedPresetsFirst", lambda: bits.take(1), 1)
    field("highLevelInfo", lambda: bits.take(1), 1)
    require(bits.bit == 173, "data/handle boundary mismatch")
    field("owner", bits.handle, (4, 1, 0xC))
    field("reactor", bits.handle, (4, 1, 0xC))
    field("extensionDictionary", bits.handle, (3, 0, 0))
    require(len(raw)*8 - bits.bit == 3, "unexpected handle tail or padding")
    following = frame(data, NEXT_FRAME, 255)
    next_bits = Bits(following)
    require(next_bits.bs() == 42 and next_bits.handle() == (0, 1, 0xB6),
            "following object identity mismatch")
    return {"sourceSha256": hashlib.sha256(data).hexdigest(), "frameOffset": FRAME,
            "frameSize": len(raw), "bodySha256": hashlib.sha256(raw).hexdigest(),
            "nextFrameOffset": NEXT_FRAME, "nextFrameSize": len(following),
            "nextFrameType": 42, "nextFrameHandle": 0xB6, "fields": fields,
            "rapidrt": rapid, "mentalray": mental,
            "rootDictionary": root, "mentalExtensionDictionary": extension,
            "mentalExtensionXrecord": verify_xrecord(data)}


def rewrite_crc(data, start, size):
    end = start + 2 + size
    data[end:end+2] = struct.pack("<H", crc16(data[start:end]))


def self_test(data):
    verify(data)
    bad = []
    truncated = data[:NEXT_FRAME + 2 + 255 + 1]
    bad.append(truncated)
    corrupt_crc = bytearray(data)
    corrupt_crc[FRAME + 2 + 27] ^= 1
    bad.append(corrupt_crc)
    wrong_role = bytearray(data)
    # Owner handle begins at body bit 173; toggle its low role-code bit.
    bit = 173 + 3
    wrong_role[FRAME + 2 + bit // 8] ^= 0x80 >> (bit % 8)
    rewrite_crc(wrong_role, FRAME, 27)
    bad.append(wrong_role)
    wrong_body = bytearray(data)
    # Flip the source procedure selector, with a correct recomputed CRC.
    bit = 96
    wrong_body[FRAME + 2 + bit // 8] ^= 0x80 >> (bit % 8)
    rewrite_crc(wrong_body, FRAME, 27)
    bad.append(wrong_body)
    wrong_next_crc = bytearray(data)
    wrong_next_crc[NEXT_FRAME + 2 + 255] ^= 1
    bad.append(wrong_next_crc)
    wrong_preset = bytearray(data)
    wrong_preset[RAPID_FRAME + 2 + 20] ^= 1
    bad.append(wrong_preset)
    wrong_preset_version = bytearray(data)
    bit = 86 + 9  # Last payload bit of RAPIDRT class-version BL.
    wrong_preset_version[RAPID_FRAME + 2 + bit // 8] ^= 0x80 >> (bit % 8)
    rewrite_crc(wrong_preset_version, RAPID_FRAME, 87)
    bad.append(wrong_preset_version)
    wrong_mental_xdict = bytearray(data)
    bit = 1002 + 16 + 16 + 3  # Low role bit of the xdictionary handle.
    wrong_mental_xdict[MENTAL_FRAME + 2 + bit // 8] ^= 0x80 >> (bit % 8)
    rewrite_crc(wrong_mental_xdict, MENTAL_FRAME, 133)
    bad.append(wrong_mental_xdict)
    wrong_dictionary_crc = bytearray(data)
    wrong_dictionary_crc[ROOT_DICTIONARY + 2 + 647] ^= 1
    bad.append(wrong_dictionary_crc)
    wrong_xrecord_tag = bytearray(data)
    wrong_xrecord_tag[MENTAL_XRECORD + 2 + 12] ^= 1
    rewrite_crc(wrong_xrecord_tag, MENTAL_XRECORD, 29)
    bad.append(wrong_xrecord_tag)
    for mutated in bad:
        try:
            verify(mutated)
        except ValueError:
            continue
        raise AssertionError("malformed control accepted")
    return len(bad)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    data = args.source.read_bytes()
    require(hashlib.sha256(data).hexdigest() == SOURCE_SHA256, "source hash mismatch")
    result = verify(data)
    if args.self_test:
        result["negativeControlsRejected"] = self_test(data)
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
