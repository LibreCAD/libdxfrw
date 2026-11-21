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

from generate_ac1012_dictionary_witness import executable_identity, model_and_frame, require, run, sha


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--oda", required=True)
    parser.add_argument("--dwgread", default="dwgread")
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
        chunks = model["wire"]["eed"]
        require(len(chunks) >= 1 and chunks[0]["appid"]["value"] == appid
                and bytes.fromhex(chunks[0]["rawHex"]) == expected_eed,
                f"{name} EED bytes differ from expected UTF-8 profile")
        run([args.oda, str(produced), str(reopened), "ACAD2018", "DXF", "0", "1", "*.dwg"])
        dxf = reopened / "seed.dxf"
        restored = ezdxf.readfile(dxf)
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
            "eedRawHex": chunks[0]["rawHex"], "reopenedDxfSha256": sha(dxf.read_bytes())}
    (root / "receipt.json").write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps({"root": str(root), **result}, indent=2))


if __name__ == "__main__":
    main()
