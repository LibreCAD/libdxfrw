#!/usr/bin/env python3
"""Audit the bounded root graph of the generated AC1012 witness pair."""
import argparse
import json
import math
from pathlib import Path
import shutil
import tempfile
from unittest import mock

import generate_ac1012_dictionary_merge_samples as merge
from generate_ac1012_dictionary_witness import Bits, observe_dictionary, require, sha


def audit(path: Path, side: str, expected: dict, dwgread: str) -> dict:
    require(sha(path.read_bytes()) == expected["dwgSha256"], "source hash differs from receipt")
    unique = "DEST_ONLY" if side == "destination" else "SOURCE_ONLY"
    with tempfile.TemporaryDirectory(prefix="ac1012-root-audit-") as temporary:
        source = Path(temporary) / f"{side}.dwg"
        shutil.copyfile(path, source)
        model = merge.read_model(source, dwgread, unique)
        drawing = json.loads(source.with_suffix(".json").read_text())
        roots = [obj for obj in drawing["OBJECTS"] if obj.get("object") == "DICTIONARY"
                 and obj.get("handle", [None])[-1] == model["rootHandle"]]
        require(len(roots) == 1, "root dictionary not unique")
        root = roots[0]
        items = root.get("items", {})
        require(set(items) == {"ACAD_GROUP", "ACAD_MLINESTYLE", "ACAD_PLOTSETTINGS"}
                and root.get("numitems") == 3
                and all(v[0] == 2 and v[-1] for v in items.values()),
                "unexpected root entry set or role")
        entries = {name: value[-1] for name, value in items.items()}
        root_frame, raw = merge.read_dictionary_frame(source, dwgread, model["rootHandle"],
                                                      entries, "ACAD_MLINESTYLE")
        require(root_frame["parent"]["role"] == 4 and root_frame["parent"]["value"] == 0
                and not root_frame["reactors"] and not root_frame["eed"]
                and root_frame["xdictionary"]["role"] == 3
                and root_frame["xdictionary"]["value"] == 0
                and root_frame["itemHandle"]["value"] == model["dictionaryHandle"]
                and root_frame["cloningField"] is None, "unexpected root wire profile")

        # Mutate the selected soft-owner role on a frame copy, then retry the
        # unchanged frame through the same shared observer.
        bits = Bits(raw)
        bits.bit = root_frame["dataEndBit"]
        bits.handle(4, model["rootHandle"])
        bits.handle(3, model["rootHandle"])
        selected_bit = None
        for row in root_frame["entries"]:
            start = bits.position
            bits.handle(2, model["rootHandle"])
            if row["name"] == "ACAD_MLINESTYLE":
                selected_bit = start
        require(selected_bit is not None, "selected root handle span missing")
        damaged = bytearray(raw)
        bit = selected_bit + 3
        damaged[bit // 8] ^= 1 << (7 - bit % 8)
        with mock.patch.object(merge, "read_object_frame", return_value=(root_frame["offset"], bytes(damaged))):
            try:
                merge.read_dictionary_frame(source, dwgread, model["rootHandle"],
                                            entries, "ACAD_MLINESTYLE")
            except ValueError:
                pass
            else:
                raise ValueError("wrong root-entry role was accepted")
        with mock.patch.object(merge, "read_object_frame", return_value=(root_frame["offset"], raw)):
            retry, _ = merge.read_dictionary_frame(source, dwgread, model["rootHandle"],
                                                   entries, "ACAD_MLINESTYLE")
        require(retry["frameSha256"] == root_frame["frameSha256"], "root frame retry changed")

        children = {}
        for name in ("ACAD_GROUP", "ACAD_PLOTSETTINGS"):
            handle = entries[name]
            offset, frame = merge.read_object_frame(source, dwgread, handle, 42)
            proof = observe_dictionary(frame, "r13", handle)
            require(proof["owner"]["value"] == model["rootHandle"]
                    and len(proof["reactors"]) == 1
                    and proof["reactors"][0]["role"] == 4
                    and proof["reactors"][0]["value"] == model["rootHandle"]
                    and proof["xdict"]["role"] == 3 and proof["xdict"]["value"] == 0
                    and not proof["eed"] and proof["cloning"] is None,
                    f"{name} empty dictionary graph differs")
            proof.pop("frameHex")
            children[name] = {"handle": handle, "offset": offset, "wire": proof}
        style_frame, _ = merge.read_dictionary_frame(source, dwgread, model["dictionaryHandle"],
                                                     model["dictionaryEntries"])
        require(style_frame["parent"]["value"] == model["rootHandle"]
                and len(style_frame["reactors"]) == 1
                and style_frame["reactors"][0]["value"] == model["rootHandle"]
                and style_frame["xdictionary"]["role"] == 3
                and style_frame["xdictionary"]["value"] == 0
                and not style_frame["eed"], "style dictionary graph differs")
        for name, angle in expected["styles"].items():
            require(name in model["namedStyles"] and math.isclose(
                model["namedStyles"][name]["startAngle"], math.radians(angle), abs_tol=1e-10),
                f"style {name} differs")
        return {"sourceSha256": model["sha256"], "root": root_frame,
                "selectedEntryHandleBit": selected_bit, "wrongRoleRejectedAndRetryPassed": True,
                "emptyChildren": children, "styleDictionary": style_frame,
                "stylesDegrees": expected["styles"]}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("destination", type=Path)
    parser.add_argument("source", type=Path)
    parser.add_argument("--dwgread", default="dwgread")
    parser.add_argument("--receipt", type=Path, default=Path(__file__).resolve().parents[2] /
                        "docs/research/p6-ac1012-explicit-entity-links-2026-11-27.json")
    args = parser.parse_args()
    receipt = json.loads(args.receipt.read_text())
    result = {side: audit(path, side, receipt["samples"][side], args.dwgread)
              for side, path in (("destination", args.destination), ("source", args.source))}
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
