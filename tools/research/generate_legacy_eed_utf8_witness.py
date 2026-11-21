#!/usr/bin/env python3
"""Generate scratch R13/R14 non-ASCII EED controls and check ODA reopen.

Requires ezdxf, GNU LibreDWG, and an ODA converter with desktop access.
The files are written only to a newly created scratch directory. This observes
one producer's byte profile; it does not qualify dictionary output semantics.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import re
import tempfile

from generate_ac1012_dictionary_witness import Bits, crc16, executable_identity, model_and_frame, require, run, sha


def dictionary_default_source(dwgread: str, source: Path) -> dict:
    """Observe a selected legacy DICTIONARYWDFLT through its final padding."""
    trace = run([dwgread, "-v4", str(source)])
    blocks = re.split(r"(?=Object number:)", (trace.stdout + trace.stderr).decode(errors="replace"))
    matches = [block for block in blocks if re.search(r"\bhandle: 0\.[0-9]+\.12\s", block)]
    require(len(matches) == 1, "dictionary-default peer frame is not unique")
    location = re.search(r"Size: (\d+) \[MS\], Type: (\d+) \[BS\], Address: (\d+)", matches[0])
    require(location is not None, "dictionary-default frame location missing")
    size, objtype, address = map(int, location.groups())
    data = source.read_bytes()
    require(objtype == 500 and address >= 2 and address + size + 2 <= len(data)
            and data[address - 2:address] == size.to_bytes(2, "little"), "frame bounds/type")
    require(int.from_bytes(data[address + size:address + size + 2], "little")
            == crc16(data[address - 2:address + size]), "dictionary-default CRC")
    frame = data[address:address + size]
    bits = Bits(frame)
    require(bits.bs() == objtype and bits.handle(0, own=True)["value"] == 0x12, "self handle")
    eed = []
    while True:
        length = bits.bs()
        if not length:
            break
        require(length <= 1024 and len(eed) < 32, "EED bounds")
        app = bits.handle(5)
        eed.append({"appid": app, "rawHex": bytes(bits.take(8) for _ in range(length)).hex()})
    end = bits.little(4)
    require(bits.bl() == 0 and bits.bl() == 1, "reactor or entry count")
    cloning_start = bits.position
    cloning = bits.bs()
    hard_owner = bits.take(8)
    count = bits.bs()
    name = bytes(bits.take(8) for _ in range(count)).rstrip(b"\0")
    require((cloning, hard_owner, name, bits.position) == (1, 0, b"NORMAL", end),
            "legacy dictionary-default body or end-bit mismatch")
    owner = bits.handle(4, 0x12)
    extension = bits.handle(3, 0x12)
    item = bits.handle(2, 0x12)
    default_start = bits.position
    default = bits.handle(5, 0x12)
    require(owner["value"] == 0xA and item["value"] == default["value"] == 0x13
            and len(frame) * 8 - bits.position < 8, "dictionary-default handle roles or tail")
    return {"frameOffset": address, "frameSize": size, "frameSha256": sha(frame),
            "crcValid": True, "cloningStartBit": cloning_start, "cloning": cloning,
            "hardOwner": hard_owner, "name": name.decode("ascii"), "bodyEndBit": end,
            "owner": owner, "extensionDictionary": extension, "entry": item,
            "default": default, "defaultStartBit": default_start,
            "eed": eed, "paddingBits": len(frame) * 8 - bits.position}


def malformed_controls(dwgread: str, candidate: str, source: Path, root: Path,
                       default_source: dict) -> dict:
    """Corrupt source self-handles with valid CRCs and test refusal/retry."""
    trace = run([dwgread, "-v4", str(source)])
    blocks = re.split(r"(?=Object number:)", (trace.stdout + trace.stderr).decode(errors="replace"))
    result = {}
    original = source.read_bytes()
    for label, handle, objtype in (("dictionary", 0x12, 500), ("cellmap", 0xC1, 508)):
        pattern = re.compile(r"\bhandle: 0\.[0-9]+\." + format(handle, "X") + r"\s")
        matches = [block for block in blocks if pattern.search(block)]
        require(len(matches) == 1, f"{label} peer frame is not unique")
        location = re.search(r"Size: (\d+) \[MS\], Type: (\d+) \[BS\], Address: (\d+)", matches[0])
        require(location is not None, f"{label} peer frame location missing")
        size, found_type, address = map(int, location.groups())
        require(found_type == objtype and address >= 2 and address + size + 2 <= len(original),
                f"{label} frame bounds or type invalid")
        require(original[address - 2:address] == size.to_bytes(2, "little"),
                f"{label} unexpected frame size encoding")
        require(int.from_bytes(original[address + size:address + size + 2], "little")
                == crc16(original[address - 2:address + size]), f"{label} CRC mismatch")
        bits = Bits(original[address:address + size])
        require(bits.bs() == objtype and bits.handle(0, own=True)["value"] == handle,
                f"{label} wire identity mismatch")
        bit = bits.position - 1
        damaged = bytearray(original)
        damaged[address + bit // 8] ^= 0x80 >> (bit % 8)
        damaged[address + size:address + size + 2] = crc16(
            damaged[address - 2:address + size]).to_bytes(2, "little")
        path = root / ("bad-" + label + ".dwg")
        path.write_bytes(damaged)
        run([candidate, "--legacy-malformed", str(path), str(source), label])
        result[label] = {"sha256": sha(damaged), "frameOffset": address,
                         "frameSize": size, "flippedBit": bit, "recomputedCrc": True,
                         "badFrameRejectedAndCorrectedRetryPassed": True}
    address, size = default_source["frameOffset"], default_source["frameSize"]
    role_bit = default_source["defaultStartBit"] + 3
    damaged = bytearray(original)
    damaged[address + role_bit // 8] ^= 0x80 >> (role_bit % 8)
    damaged[address + size:address + size + 2] = crc16(
        damaged[address - 2:address + size]).to_bytes(2, "little")
    bad_role = root / "bad-default-role.dwg"
    bad_role.write_bytes(damaged)
    run([candidate, "--legacy-bad-default-role", str(bad_role), str(source)])
    result["defaultRole"] = {"sha256": sha(damaged), "flippedBit": role_bit,
        "recomputedCrc": True, "rawOnlyFallbackAndCorrectedRetryPassed": True}
    return result


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--oda", required=True)
    parser.add_argument("--dwgread", default="dwgread")
    parser.add_argument("--candidate-test", help="optional libdxfrw external-profile test executable")
    parser.add_argument("--output-dir", type=Path, required=True)
    args = parser.parse_args()
    import ezdxf

    args.output_dir.mkdir(parents=True, exist_ok=True)
    root = Path(tempfile.mkdtemp(prefix="legacy-eed-utf8-", dir=args.output_dir))
    incoming = root / "input"
    incoming.mkdir()
    document = ezdxf.new("R2018")
    document.appids.new("CODEPAGE_TEST")
    line = document.modelspace().add_line((1, 2, 3), (4, 6, 9))
    line.set_xdata("CODEPAGE_TEST", [(1000, "ASCII"), (1000, "café"),
                                     (1000, "€漢"), (1070, 42)])
    dictionary = document.rootdict.add_new_dict("TEST_DICT")
    dictionary.set_xdata("CODEPAGE_TEST", [(1000, "café"),
                                           (1000, "€漢"), (1070, -42)])
    source = incoming / "seed.dxf"
    document.saveas(source)
    appid = int(document.appids.get("CODEPAGE_TEST").dxf.handle, 16)
    result = {"sourceSha256": sha(source.read_bytes()), "ezdxfVersion": ezdxf.__version__,
              "oda": executable_identity(args.oda), "dwgread": executable_identity(args.dwgread),
              "sourceAppId": appid,
              "sourceLine": line.dxf.handle, "sourceDictionary": dictionary.dxf.handle,
              "versions": {}}
    expected_eed = bytes.fromhex("0005002d636166c3a90006002de282ace6bca246d6ff")
    for name, target in (("r13", "ACAD13"), ("r14", "ACAD14")):
        produced, reopened = root / name, root / (name + "-reopened")
        produced.mkdir(); reopened.mkdir()
        run([args.oda, str(incoming), str(produced), target, "DWG", "0", "1", "*.dxf"])
        dwg = produced / "seed.dwg"
        model = model_and_frame(args.dwgread, dwg, name)
        dictionary_default = dictionary_default_source(args.dwgread, dwg)
        chunks = model["wire"]["eed"]
        require(len(chunks) >= 1 and chunks[0]["appid"]["value"] == appid
                and bytes.fromhex(chunks[0]["rawHex"]) == expected_eed,
                f"{name} EED bytes differ from expected UTF-8 profile")
        run([args.oda, str(produced), str(reopened), "ACAD2018", "DXF", "0", "1", "*.dwg"])
        dxf = reopened / "seed.dxf"
        restored = ezdxf.readfile(dxf)
        restored_default = restored.entitydb.get("12")
        require(restored_default is not None and restored_default.dxftype() == "ACDBDICTIONARYWDFLT"
                and restored_default.dxf.cloning == 1 and restored_default.dxf.default == "13"
                and list(restored_default.keys()) == ["Normal"],
                f"{name} producer reopen changed the default dictionary")
        target_dictionary = restored.rootdict.get("TEST_DICT")
        require(target_dictionary is not None, "reopened dictionary missing")
        values = [(tag.code, tag.value) for tag in target_dictionary.get_xdata("CODEPAGE_TEST")]
        require(values == [(1000, "café"), (1000, "€漢"), (1070, -42)],
                f"{name} producer reopen changed the EED values")
        lines = list(restored.modelspace().query("LINE"))
        require(len(lines) == 1 and [(tag.code, tag.value) for tag in
                lines[0].get_xdata("CODEPAGE_TEST")] ==
                [(1000, "ASCII"), (1000, "café"), (1000, "€漢"), (1070, 42)],
                f"{name} producer reopen changed line EED")
        result["versions"][name] = {"dwgSha256": model["sha256"],
            "frameSha256": model["wire"]["frameSha256"], "frameOffset": model["frameOffset"],
            "frameSize": model["frameSize"], "frameCrcValid": model["crcValid"],
            "eedRawHex": chunks[0]["rawHex"], "reopenedDxfSha256": sha(dxf.read_bytes()),
            "dictionaryDefault": dictionary_default}
        if args.candidate_test:
            run([args.candidate_test, "--legacy-eed-utf8-source", str(dwg)])
            result["versions"][name]["candidateSourceTestPassed"] = True
            if name == "r14":
                result["versions"][name]["malformedControls"] = malformed_controls(
                    args.dwgread, args.candidate_test, dwg, root, dictionary_default)
    if args.candidate_test:
        result["candidateTest"] = executable_identity(args.candidate_test)
    (root / "receipt.json").write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps({"root": str(root), **result}, indent=2))


if __name__ == "__main__":
    main()
