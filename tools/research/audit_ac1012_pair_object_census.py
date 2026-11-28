#!/usr/bin/env python3
"""Reconcile all object frames in the selected generated AC1012 pair.

Requires the previously audited handoff packet, GNU LibreDWG and the focused
libdxfrw external-test executable. This is not a whole-DWG or native proof.
"""
import argparse
import json
from pathlib import Path
import tempfile

from audit_ac1012_pair_header import audit as audit_header
from audit_ac1012_pair_header_prefix import audit_prefix
from generate_ac1012_dictionary_merge_samples import read_object_frame
from generate_ac1012_dictionary_witness import require, run, sha


def check_header_targets(prefix: dict, objects: dict) -> dict:
    expected = {"CLAYER": (51, "0"), "TEXTSTYLE": (53, "Standard"),
                "CELTYPE": (57, "BYLAYER"), "DIMSTYLE": (69, "Standard"),
                "CMLSTYLE": (73, "Standard")}
    targets = {}
    for name, (kind, label) in expected.items():
        handle = prefix["fields"][name]["value"][2]
        target = objects.get(handle)
        require(target is not None and target.get("type") == kind and target.get("name") == label,
                f"wrong HEADER {name} target")
        targets[name] = {"handle": handle, "type": kind, "name": label}
    seed = prefix["fields"]["HANDSEED"]["value"][2]
    require(seed > max(objects), "HANDSEED does not exceed source objects")
    return targets


def audit(packet: Path, side: str, manifest: dict, dwgread: str, probe: str) -> dict:
    path = packet / f"{side}.dwg"
    record = manifest["samples"][side]
    source_sha = sha(path.read_bytes())
    require(source_sha == record["files"][f"{side}.dwg"], "source hash mismatch")
    with tempfile.TemporaryDirectory(prefix="ac1012-census-") as temporary:
        decoded = Path(temporary) / "source.json"
        run([dwgread, "-O", "minJSON", str(path), "-o", str(decoded)])
        drawing = json.loads(decoded.read_text())
        rows = drawing["OBJECTS"]
    by_handle = {}
    for row in rows:
        handle = row["handle"][-1]
        require(handle not in by_handle, "duplicate decoded source handle")
        by_handle[handle] = row
    require(len(rows) == 32 and len(by_handle) == 32, "unexpected source object census")
    styles = record["styleFrameSha256"]
    unique = "SOURCE_ONLY" if side == "source" else "DEST_ONLY"
    expected = {
        1: (48, record["blockControlFrameSha256"]),
        2: (50, record["layerControlFrameSha256"]),
        3: (52, record["styleAndAppidFrameSha256"]["styleControl"]),
        5: (56, record["linetypeFrameSha256"]["linetypeControl"]),
        6: (60, record["remainingControlFrameSha256"]["viewControl"]),
        7: (62, record["remainingControlFrameSha256"]["ucsControl"]),
        8: (64, record["remainingControlFrameSha256"]["vportControl"]),
        9: (66, record["styleAndAppidFrameSha256"]["appidControl"]),
        10: (68, record["remainingControlFrameSha256"]["dimstyleControl"]),
        11: (70, record["remainingControlFrameSha256"]["viewportEntityControl"]),
        12: (42, record["rootFrameSha256"]),
        13: (42, record["emptyChildFrameSha256"]["ACAD_GROUP"]),
        16: (51, record["layerFrameSha256"]),
        17: (53, record["styleAndAppidFrameSha256"]["style"]),
        18: (67, record["styleAndAppidFrameSha256"]["appid"]),
        20: (57, record["linetypeFrameSha256"]["byBlock"]),
        21: (57, record["linetypeFrameSha256"]["byLayer"]),
        22: (57, record["linetypeFrameSha256"]["continuous"]),
        23: (42, record["dictionaryFrameSha256"]),
        24: (73, styles["Standard"]),
        25: (42, record["emptyChildFrameSha256"]["ACAD_PLOTSETTINGS"]),
        31: (49, None), 32: (49, None),
        33: (4, record["blockEntityFrameSha256"]["21"]),
        34: (5, record["blockEntityFrameSha256"]["22"]),
        36: (4, record["blockEntityFrameSha256"]["24"]),
        37: (5, record["blockEntityFrameSha256"]["25"]),
        38: (65, record["vportFrameSha256"]),
        44: (19, record["lineFrameSha256"]),
        45: (69, record["dimstyleFrameSha256"]),
        46: (73, styles["SAME_KEY"]), 47: (73, styles[unique]),
    }
    require(set(expected) == set(by_handle), "missing or extra source object")
    prefix = audit_prefix(path.read_bytes(), drawing["HEADER"])
    targets = check_header_targets(prefix, by_handle)
    damaged = dict(by_handle)
    layer = targets["CLAYER"]["handle"]
    damaged[layer] = dict(damaged[layer], type=0)
    try:
        check_header_targets(prefix, damaged)
    except ValueError:
        pass
    else:
        raise ValueError("wrong current-layer target type was accepted")
    require(check_header_targets(prefix, by_handle) == targets, "HEADER target retry changed")
    # The probe verifies both retained BLOCK_HEADER bodies and their frame
    # binding, generation, reference roles, target types, refusal and retry.
    probe_result = run([probe, "--legacy-block-pair-wire", str(path)])
    frames = {}
    for handle, (kind, expected_sha) in sorted(expected.items()):
        require(by_handle[handle].get("type") == kind, f"wrong object type {handle:X}")
        offset, raw = read_object_frame(path, dwgread, handle, kind)
        digest = sha(raw)
        require(expected_sha is None or digest == expected_sha, f"frame mismatch {handle:X}")
        frames[f"{handle:X}"] = {"type": kind, "offset": offset, "frameSha256": digest,
                                  "binding": "shared-block-probe" if expected_sha is None else "audited-packet"}
    return {"sourceSha256": source_sha, "headerEnvelope": audit_header(path.read_bytes()),
            "headerPrefix": prefix, "headerTargets": targets,
            "wrongHeaderTargetRejectedAndRetryPassed": True,
            "frameCount": len(frames),
            "probeOutputSha256": sha(probe_result.stdout + probe_result.stderr), "frames": frames}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("packet", type=Path)
    parser.add_argument("--adapter-probe", required=True)
    parser.add_argument("--dwgread", default="dwgread")
    args = parser.parse_args()
    manifest = json.loads((args.packet / "manifest.json").read_text())
    require(manifest.get("profile") == "populated-links", "wrong handoff profile")
    require(sha((args.packet / "source-receipt.json").read_bytes()) == manifest["sourceReceiptSha256"],
            "handoff receipt hash mismatch")
    print(json.dumps({side: audit(args.packet, side, manifest, args.dwgread, args.adapter_probe)
                      for side in ("source", "destination")}, indent=2))


if __name__ == "__main__":
    main()
