#!/usr/bin/env python3
"""Compare the selected AC1012 HEADER prefix with an independent decoder.

The prefix follows ODA v5.4.1 section 9 through CELTSCALE. It is a bounded
physical comparison for these generated witnesses, not full HEADER admission.
"""
import argparse
import json
import math
from pathlib import Path
import struct
import tempfile

from audit_ac1012_pair_header import audit as audit_envelope
from audit_ac1024_sortents_boundary import Bits, crc16
from generate_ac1012_dictionary_witness import require, run, sha


FLAGS = """DIMASO DIMSHO DIMSAV PLINEGEN ORTHOMODE REGENMODE FILLMODE
QTEXTMODE PSLTSCALE LIMCHECK BLIPMODE USRTIMER SKPOLY ANGDIR SPLFRAME ATTREQ
ATTDIA MIRRTEXT WORLDVIEW WIREFRAME TILEMODE PLIMCHECK VISRETAIN DELOBJ
DISPSILH PELLIPSE""".split()
SHORTS = """PROXYGRAPHICS DRAGMODE TREEDEPTH LUNITS LUPREC AUNITS AUPREC
OSMODE ATTMODE COORDS PDMODE PICKSTYLE USERI1 USERI2 USERI3 USERI4 USERI5
SPLINESEGS SURFU SURFV SURFTYPE SURFTAB1 SURFTAB2 SPLINETYPE SHADEDGE
SHADEDIF UNITMODE MAXACTVP ISOLINES CMLJUST TEXTQLTY""".split()
DOUBLES = """LTSCALE TEXTSIZE TRACEWID SKETCHINC FILLETRAD THICKNESS ANGBASE
PDSIZE PLINEWID USERR1 USERR2 USERR3 USERR4 USERR5 CHAMFERA CHAMFERB
CHAMFERC CHAMFERD FACETRES CMLSCALE CELTSCALE""".split()


def audit_prefix(data, peer):
    envelope = audit_envelope(data)
    start = envelope["sectionOffset"] + 20
    bits = Bits(data[start:start + envelope["payloadSize"]])
    fields = {}

    def observe(name, read):
        first = bits.bit
        value = read()
        require(name in peer and peer[name] == value, f"peer mismatch: {name}")
        fields[name] = {"startBit": first, "endBit": bits.bit, "value": value}

    def bitdouble():
        tag = bits.take(2)
        if tag == 0:
            value = struct.unpack("<d", bytes(bits.take(8) for _ in range(8)))[0]
            require(math.isfinite(value), "non-finite bitdouble")
            return value
        require(tag in (1, 2), "invalid bitdouble tag")
        return 1.0 if tag == 1 else 0.0

    def text():
        length = bits.bs()
        require(length <= 255, "unbounded text")
        try:
            return bytes(bits.take(8) for _ in range(length)).decode("ascii")
        except UnicodeDecodeError as error:
            raise ValueError("non-ASCII witness text") from error

    for index in range(1, 5):
        observe(f"unit{index}_ratio", bitdouble)
    for index in range(1, 5):
        observe(f"unit{index}_name", text)
    observe("unknown_8", bits.bl)
    observe("unknown_9", bits.bl)
    observe("unknown_10", bits.bs)
    first = bits.bit
    code, width, value = bits.handle()
    require(code == 5 and width == 0 and value == 0, "unexpected viewport handle role")
    require(peer.get("VX_TABLE_RECORD") == [5, 0, 0, 0], "peer viewport handle differs")
    fields["VX_TABLE_RECORD"] = {"startBit": first, "endBit": bits.bit, "value": [code, width, value]}
    for name in FLAGS:
        observe(name, lambda: bits.take(1))
    for name in SHORTS:
        observe(name, bits.bs)
    for name in DOUBLES:
        observe(name, bitdouble)
    return {"startBit": 0, "endBit": bits.bit, "fieldCount": len(fields), "fields": fields}


def changed_bit(data, envelope, bit):
    result = bytearray(data)
    section = envelope["sectionOffset"]
    position = section + 20 + bit // 8
    result[position] ^= 1 << (7 - bit % 8)
    crc_at = section + 20 + envelope["payloadSize"]
    struct.pack_into("<H", result, crc_at, crc16(result[section + 16:crc_at]))
    audit_envelope(result)  # The independent prefix check must see the change.
    return result


def check_mutations(data, peer, original):
    envelope = audit_envelope(data)
    cases = {
        "wrong-handle-role": [original["fields"]["VX_TABLE_RECORD"]["startBit"] + 3],
        "changed-dimaso": [original["fields"]["DIMASO"]["startBit"]],
        "invalid-bitdouble-tag": [original["fields"]["unit1_ratio"]["startBit"],
                                  original["fields"]["unit1_ratio"]["startBit"] + 1],
    }
    for label, positions in cases.items():
        changed = data
        for bit in positions:
            changed = changed_bit(changed, envelope, bit)
        try:
            audit_prefix(changed, peer)
        except ValueError:
            pass
        else:
            raise ValueError(f"prefix mutation accepted: {label}")
    require(audit_prefix(data, peer) == original, "prefix retry changed")
    return sorted(cases)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("packet", type=Path)
    parser.add_argument("--dwgread", default="dwgread")
    args = parser.parse_args()
    manifest = json.loads((args.packet / "manifest.json").read_text())
    require(manifest.get("profile") == "populated-links", "wrong packet profile")
    result = {}
    with tempfile.TemporaryDirectory(prefix="ac1012-header-prefix-") as temporary:
        for side in ("destination", "source"):
            path = args.packet / f"{side}.dwg"
            data = path.read_bytes()
            require(sha(data) == manifest["samples"][side]["files"][path.name], "source hash mismatch")
            decoded = Path(temporary) / f"{side}.json"
            run([args.dwgread, "-O", "minJSON", str(path), "-o", str(decoded)])
            peer = json.loads(decoded.read_text())["HEADER"]
            result[side] = audit_prefix(data, peer)
            result[side]["rejectedMutations"] = check_mutations(data, peer, result[side])
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
