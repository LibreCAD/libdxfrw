#!/usr/bin/env python3
"""Package the locally generated, ODA-reopened AC1012 merge controls.

The package is scratch-only. Physical source checks do not establish AutoCAD
normal-open behavior or the dictionary's native duplicate-merge semantics.
"""
import argparse
import json
import math
import os
from pathlib import Path
import shutil
import tempfile

from audit_ac1012_oda_merge_pair import PROFILES, audit
from audit_ac1012_generated_root import audit as audit_generated_root
from audit_ac1012_pair_block_entities import audit as audit_block_entities
from generate_ac1012_dictionary_witness import executable_identity, require, run, sha
from generate_ac1012_dictionary_merge_samples import (check_single_line_graph,
    observe_populated_style_frame, observe_single_line_frame, read_dictionary_frame,
    read_model, read_object_frame)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("run", type=Path, help="source-profile scratch run")
    parser.add_argument("output", type=Path, help="new scratch handoff directory")
    parser.add_argument("--dwgread", default="dwgread")
    parser.add_argument("--profile", choices=("oda", "explicit-links", "populated-links"), default="oda")
    args = parser.parse_args()
    require(not os.path.lexists(args.output), "output already exists")
    require(args.output.parent.is_dir(), "output parent does not exist")
    dwgread = shutil.which(args.dwgread)
    require(dwgread is not None, "dwgread executable not found")
    require(b"0.14" in run([dwgread, "--version"]).stdout, "dwgread 0.14 required")
    root = Path(__file__).resolve().parents[2]
    receipt_names = {"oda": "p6-ac1012-oda-root-case-pair-2026-11-25.json",
                     "explicit-links": "p6-ac1012-explicit-entity-links-2026-11-27.json",
                     "populated-links": "p6-ac1012-populated-style-wire-2026-11-27.json"}
    receipt_name = receipt_names[args.profile]
    receipt_path = root / "docs/research" / receipt_name
    receipt_bytes = receipt_path.read_bytes()
    receipt = json.loads(receipt_bytes)
    guide = root / "tools/research/AC1012_DICTIONARY_NATIVE_WITNESS.md"
    with tempfile.TemporaryDirectory(prefix="ac1012-populated-packet-", dir=args.output.parent) as tmp:
        packet = Path(tmp) / "packet"
        packet.mkdir()
        records = {}
        for side in PROFILES:
            expected = receipt["samples"][side]
            if args.profile == "oda":
                files = ((f"{side}.dwg", args.run / expected["path"], expected["sha256"]),
                         (f"{side}-oda-reopen.dxf", args.run / "allcase-reopen" / f"{side}.dxf",
                          expected["reopenedDxfSha256"]))
            else:
                files = ((f"{side}.dwg", args.run / "oda-input" / f"{side}.dwg", expected["dwgSha256"]),
                         (f"{side}-oda-reopen.dxf", args.run / "oda-output" / f"{side}.dxf",
                          expected["odaDxfSha256"]))
            hashes = {}
            for name, source, digest in files:
                data = source.read_bytes()
                require(sha(data) == digest, f"receipt hash mismatch: {source}")
                (packet / name).write_bytes(data)
                hashes[name] = digest
            if args.profile == "oda":
                observed = audit(packet / f"{side}.dwg", side, dwgread, Path(tmp), False, True)
                require(observed["sha256"] == hashes[f"{side}.dwg"], "audited source changed")
                records[side] = {"files": hashes, "stylesDegrees": PROFILES[side],
                                 "rootHandle": observed["rootHandle"],
                                 "dictionaryHandle": observed["dictionaryHandle"],
                                 "dictionaryFrameSha256": observed["frame"]["frameSha256"],
                                 "rootFrameSha256": observed["rootFrame"]["frameSha256"]}
            else:
                scratch = Path(tmp) / f"audit-{side}.dwg"
                shutil.copyfile(packet / f"{side}.dwg", scratch)
                model = read_model(scratch, dwgread, "DEST_ONLY" if side == "destination" else "SOURCE_ONLY")
                require(model["sha256"] == hashes[f"{side}.dwg"], "audited source changed")
                frame, _ = read_dictionary_frame(scratch, dwgread, model["dictionaryHandle"],
                                                  model["dictionaryEntries"])
                require(model["dictionaryOwner"] == model["rootHandle"]
                        and frame["parent"]["value"] == model["rootHandle"]
                        and len(frame["reactors"]) == 1
                        and frame["reactors"][0]["value"] == model["rootHandle"]
                        and frame["xdictionary"]["value"] == 0,
                        "style dictionary ownership or reactors differ")
                for name, angle in expected["styles"].items():
                    require(name in model["namedStyles"] and math.isclose(
                        model["namedStyles"][name]["startAngle"], math.radians(angle),
                        abs_tol=1e-10), f"style {name} differs from the receipt")
                drawing = json.loads(scratch.with_suffix(".json").read_text())
                _, line_handle, layer_handle = check_single_line_graph(drawing)
                _, line_frame = read_object_frame(scratch, dwgread, line_handle, 19)
                line = observe_single_line_frame(line_frame, line_handle, layer_handle)
                require(line["frameSha256"] == expected["line"]["wire"]["frameSha256"],
                        "LINE frame differs from the receipt")
                root_proof = audit_generated_root(packet / f"{side}.dwg", side, expected, dwgread)
                style_frames = {}
                block_entity_frames = {}
                if args.profile == "populated-links":
                    block_entities = audit_block_entities(scratch, dwgread)
                    require(block_entities["sourceSha256"] == model["sha256"],
                            "BLOCK/ENDBLK source changed")
                    block_entity_frames = {
                        handle: item["proof"]["frameSha256"]
                        for handle, item in block_entities["objects"].items()}
                    for name, row in expected["styleFrames"].items():
                        _, style_raw = read_object_frame(scratch, dwgread, row["handle"], 73)
                        style = observe_populated_style_frame(style_raw, row["handle"], name,
                                                              model["dictionaryHandle"],
                                                              row["startAngleRadians"])
                        require(style["frameSha256"] == row["frameSha256"],
                                f"style {name} frame differs from the receipt")
                        style_frames[name] = style["frameSha256"]
                records[side] = {"files": hashes, "stylesDegrees": expected["styles"],
                                 "rootHandle": model["rootHandle"],
                                 "dictionaryHandle": model["dictionaryHandle"],
                                 "dictionaryFrameSha256": frame["frameSha256"],
                                 "lineFrameSha256": line["frameSha256"],
                                 "rootFrameSha256": root_proof["root"]["frameSha256"],
                                 "emptyChildFrameSha256": {
                                     name: value["wire"]["frameSha256"]
                                     for name, value in root_proof["emptyChildren"].items()},
                                 "styleFrameSha256": style_frames}
                if block_entity_frames:
                    records[side]["blockEntityFrameSha256"] = block_entity_frames
                    records[side]["blockControlFrameSha256"] = block_entities["blockControl"]["proof"]["frameSha256"]
                    records[side]["layerControlFrameSha256"] = block_entities["layerControl"]["proof"]["frameSha256"]
                    records[side]["layerFrameSha256"] = block_entities["layer"]["proof"]["frameSha256"]
                    records[side]["linetypeFrameSha256"] = {
                        name: block_entities[name]["proof"]["frameSha256"]
                        for name in ("linetypeControl", "byBlock", "byLayer", "continuous")}
                    records[side]["styleAndAppidFrameSha256"] = {
                        name: block_entities[name]["proof"]["frameSha256"]
                        for name in ("styleControl", "style", "appidControl", "appid")}
                    records[side]["remainingControlFrameSha256"] = {
                        name: block_entities[name]["proof"]["frameSha256"]
                        for name in ("viewControl", "ucsControl", "vportControl",
                                     "dimstyleControl", "viewportEntityControl")}
        shutil.copyfile(guide, packet / "NATIVE_WITNESS.md")
        (packet / "source-receipt.json").write_bytes(receipt_bytes)
        readme = ("Locally generated AC1012 controls screened by ODA; native behavior is unobserved.\n"
                  "Check manifest hashes before and after testing; work on copies only.\n"
                  "destination: SAME_KEY 15 degrees, DEST_ONLY 22 degrees.\n"
                  "source: SAME_KEY 37 degrees, SOURCE_ONLY 51 degrees.\n"
                  "The DXF files are ODA reopens, not AutoCAD output.\n"
                  "Screen normal-open and style validity before native merge testing.\n"
                  "See NATIVE_WITNESS.md; repository-relative links refer to libdxfrw.\n"
                  "Root, EED, complete graph, and native semantics remain unqualified.\n")
        (packet / "README.txt").write_text(readme)
        manifest = {"schema": 1, "profile": args.profile,
                    "scope": "selected physical controls; native semantics unobserved",
                    "sourceReceiptSha256": sha(receipt_bytes),
                    "dwgread": executable_identity(dwgread),
                    "guideSha256": sha((packet / "NATIVE_WITNESS.md").read_bytes()),
                    "readmeSha256": sha(readme.encode()), "samples": records}
        (packet / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
        require(not os.path.lexists(args.output), "output already exists")
        packet.rename(args.output)
    print(args.output)


if __name__ == "__main__":
    main()
