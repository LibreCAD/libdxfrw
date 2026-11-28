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
import tempfile

from generate_ac1012_dictionary_witness import Bits, crc16, executable_identity, model_and_frame, require, run, sha
from legacy_dwg_dictionary_wire import VERSIONS, decode_dictionary_frame, locate_object_frame


def dictionary_default_source(dwgread: str, source: Path) -> dict:
    """Observe a selected legacy DICTIONARYWDFLT through its final padding."""
    trace = run([dwgread, "-v4", str(source)])
    data = source.read_bytes()
    versions = [name for name in ("r13", "r14") if data.startswith(VERSIONS[name])]
    require(len(versions) == 1, "unexpected dictionary-default version")
    address, frame = locate_object_frame(data, (trace.stdout + trace.stderr).decode(errors="replace"),
                                         versions[0], 0x12, 500)
    row = decode_dictionary_frame(frame, versions[0], 0x12,
                                  expected_entries={"NORMAL": 0x13}, with_default=True)
    require(row["cloning"] == 1 and row["hardOwner"] == 0 and not row["reactors"]
            and row["owner"]["value"] == 0xA and row["default"]["value"] == 0x13,
            "legacy dictionary-default body or handle roles mismatch")
    return {"frameOffset": address, "frameSize": len(frame), "frameSha256": row["frameSha256"],
            "crcValid": True, "cloningStartBit": row["cloningStartBit"], "cloning": row["cloning"],
            "hardOwner": row["hardOwner"], "name": "NORMAL", "bodyEndBit": row["dataEndBit"],
            "owner": row["owner"], "extensionDictionary": row["xdict"], "entry": row["entries"][0]["handle"],
            "default": row["default"], "defaultStartBit": row["defaultStartBit"],
            "eed": [{"appid": e["appid"], "rawHex": e["hex"]} for e in row["eed"]],
            "paddingBits": row["paddingBits"]}


def malformed_controls(dwgread: str, candidate: str, source: Path, root: Path,
                       default_source: dict) -> dict:
    """Corrupt source self-handles with valid CRCs and test refusal/retry."""
    trace = run([dwgread, "-v4", str(source)])
    text = (trace.stdout + trace.stderr).decode(errors="replace")
    result = {}
    original = source.read_bytes()
    versions = [name for name in ("r13", "r14") if original.startswith(VERSIONS[name])]
    require(len(versions) == 1, "unexpected legacy source version")
    for label, handle, objtype in (("dictionary", 0x12, 500), ("cellmap", 0xC1, 508)):
        address, frame = locate_object_frame(original, text, versions[0], handle, objtype)
        size = len(frame)
        bits = Bits(frame)
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
