#!/usr/bin/env python3
"""Compare the selected AC1012 HEADER prefix with an independent decoder.

The walk follows ODA v5.4.1 section 9 through the AC1012 alignment bits. It is
a bounded physical comparison for these witnesses, not full HEADER admission.
"""
import argparse
import json
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

    bitdouble = lambda: bits.bit_double(require_finite=True)
    rawdouble = lambda: bits.raw_double(require_finite=True)
    text = lambda: bits.ascii_text(255)

    def handle(name, role):
        first = bits.bit
        code, width, value = bits.handle()
        require(code == role, f"unexpected handle role: {name}")
        peer_handle = peer.get(name)
        require(isinstance(peer_handle, list) and peer_handle == [code, width, value, value],
                f"peer handle differs: {name}")
        fields[name] = {"startBit": first, "endBit": bits.bit, "value": [code, width, value]}

    for index in range(1, 5):
        observe(f"unit{index}_ratio", bitdouble)
    for index in range(1, 5):
        observe(f"unit{index}_name", text)
    observe("unknown_8", bits.bl)
    observe("unknown_9", bits.bl)
    observe("unknown_10", bits.bs)
    handle("VX_TABLE_RECORD", 5)
    require(fields["VX_TABLE_RECORD"]["value"] == [5, 0, 0], "non-null viewport handle")
    for name in FLAGS:
        observe(name, lambda: bits.take(1))
    for name in SHORTS:
        observe(name, bits.bs)
    for name in DOUBLES:
        observe(name, bitdouble)
    observe("MENU", text)
    for name in ("TDUCREATE", "TDUUPDATE", "TDINDWG", "TDUSRTIMER"):
        observe(name, lambda: [bits.bl(), bits.bl()])
    observe("CECOLOR", bits.bs)
    handle("HANDSEED", 0)
    for name in ("CLAYER", "TEXTSTYLE", "CELTYPE", "DIMSTYLE", "CMLSTYLE"):
        handle(name, 5)
    for prefix in ("P", ""):
        for name in ("INSBASE", "EXTMIN", "EXTMAX"):
            observe(prefix + name, lambda: [bitdouble() for _ in range(3)])
        for name in ("LIMMIN", "LIMMAX"):
            observe(prefix + name, lambda: [rawdouble() for _ in range(2)])
        observe(prefix + "ELEVATION", bitdouble)
        for name in ("UCSORG", "UCSXDIR", "UCSYDIR"):
            observe(prefix + name, lambda: [bitdouble() for _ in range(3)])
        handle(prefix + "UCSNAME", 5)
    for name in "DIMTOL DIMLIM DIMTIH DIMTOH DIMSE1 DIMSE2 DIMALT DIMTOFL DIMSAH DIMTIX DIMSOXD".split():
        observe(name, lambda: bits.take(1))
    for name in "DIMALTD DIMZIN".split():
        observe(name, lambda: bits.take(8))
    for name in "DIMSD1 DIMSD2".split():
        observe(name, lambda: bits.take(1))
    for name in "DIMTOLJ DIMJUST DIMFIT".split():
        observe(name, lambda: bits.take(8))
    observe("DIMUPT", lambda: bits.take(1))
    for name in "DIMTZIN DIMALTZ DIMALTTZ DIMTAD".split():
        observe(name, lambda: bits.take(8))
    for name in "DIMUNIT DIMAUNIT DIMDEC DIMTDEC DIMALTU DIMALTTD".split():
        observe(name, bits.bs)
    handle("DIMTXSTY", 5)
    for name in "DIMSCALE DIMASZ DIMEXO DIMDLI DIMEXE DIMRND DIMDLE DIMTP DIMTM DIMTXT DIMCEN DIMTSZ DIMALTF DIMLFAC DIMTVP DIMTFAC DIMGAP".split():
        observe(name, bitdouble)
    for name in "DIMPOST DIMAPOST DIMBLK_T DIMBLK1_T DIMBLK2_T".split():
        observe(name, text)
    for name in "DIMCLRD DIMCLRE DIMCLRT".split():
        observe(name, bits.bs)
    for name in "BLOCK_CONTROL_OBJECT LAYER_CONTROL_OBJECT STYLE_CONTROL_OBJECT LTYPE_CONTROL_OBJECT VIEW_CONTROL_OBJECT UCS_CONTROL_OBJECT VPORT_CONTROL_OBJECT APPID_CONTROL_OBJECT DIMSTYLE_CONTROL_OBJECT VX_CONTROL_OBJECT".split():
        handle(name, 3)
    for name in "DICTIONARY_ACAD_GROUP DICTIONARY_ACAD_MLINESTYLE".split():
        handle(name, 5)
    handle("DICTIONARY_NAMED_OBJECT", 3)
    for name in "BLOCK_RECORD_PSPACE BLOCK_RECORD_MSPACE LTYPE_BYLAYER LTYPE_BYBLOCK LTYPE_CONTINUOUS".split():
        handle(name, 5)
    end = bits.bit
    padding = envelope["payloadSize"] * 8 - end
    require(0 <= padding <= 7, "unaccounted HEADER payload")
    padding_value = bits.take(padding)
    return {"startBit": 0, "endBit": end, "fieldCount": len(fields), "fields": fields,
            "paddingBits": padding, "paddingValue": padding_value}


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
        "wrong-current-layer-role": [original["fields"]["CLAYER"]["startBit"] + 3],
        "wrong-root-dictionary-role": [original["fields"]["DICTIONARY_NAMED_OBJECT"]["startBit"] + 3],
        "changed-dimaso": [original["fields"]["DIMASO"]["startBit"]],
        "changed-dimtih": [original["fields"]["DIMTIH"]["startBit"]],
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
    if original["paddingBits"]:
        padded = audit_prefix(changed_bit(data, envelope, original["endBit"]), peer)
        require(padded["fields"] == original["fields"] and padded["endBit"] == original["endBit"],
                "padding mutation changed decoded fields")
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
