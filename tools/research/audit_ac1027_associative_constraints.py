#!/usr/bin/env python3
"""Bind the LibreDWG 2013 Constraints sample to libdxfrw source captures."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import subprocess
import sys
import tempfile
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO / "tools"))
from bounded_process import run_bounded  # noqa: E402

EXPECTED_SHA256 = (
    "bdd7458bda6c1464faa5836b8aa85640199fbd01c510fa00e25550c043185429"
)
EXPECTED_ASSOCIATIVE = {
    992: (517, "ACDBASSOCNETWORK"),
    994: (517, "ACDBASSOCNETWORK"),
    995: (518, "ACDBASSOC2DCONSTRAINTGROUP"),
    996: (519, "ACDBASSOCGEOMDEPENDENCY"),
    997: (519, "ACDBASSOCGEOMDEPENDENCY"),
}
FRAME = re.compile(
    r"^ (opaque|standard) handle=(\d+) type=(\d+) record=(\S*) "
    r"class=(\S*) custom=(\d+) .*?owner=(\d+) .*?reactors=([^ ]*) "
    r"commonEvidence=(\d+) offset=(\d+) size=(\d+) ",
    re.MULTILINE,
)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def handle(value: object) -> int:
    if not isinstance(value, list) or not value:
        raise ValueError(f"expected LibreDWG handle array, got {value!r}")
    return int(value[-1])


def run(command: list[str], timeout: float):
    result = run_bounded(
        command,
        timeout=timeout,
        output_limit=16 * 1024 * 1024,
        cwd=REPO,
    )
    combined = (result.stdout + b"\n" + result.stderr).decode(
        "utf-8", errors="replace"
    )
    return result.returncode, combined


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--dwgread", type=Path, required=True)
    parser.add_argument("--dwg2dxf", type=Path, required=True)
    parser.add_argument("--adapter-tests", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--timeout", type=float, default=60.0)
    args = parser.parse_args()

    source = args.source.resolve(strict=True)
    dwgread = args.dwgread.resolve(strict=True)
    dwg2dxf = args.dwg2dxf.resolve(strict=True)
    adapter_tests = args.adapter_tests.resolve(strict=True)
    if sha256(source) != EXPECTED_SHA256:
        raise SystemExit("source bytes differ from the pinned LibreDWG sample")
    source_hash = sha256(source)

    with tempfile.TemporaryDirectory(prefix="libdxfrw-ac1027-assoc-") as work:
        peer_path = Path(work) / "peer-minjson.json"
        peer_status, peer_output = run(
            [str(dwgread), "-O", "minJSON", "-o", str(peer_path), str(source)],
            args.timeout,
        )
        if peer_status != 0 or not peer_path.is_file():
            raise SystemExit("LibreDWG minJSON peer extraction failed:\n" + peer_output[-8000:])
        peer = json.loads(peer_path.read_text(encoding="utf-8"))
        peer_json_hash = sha256(peer_path)
        if peer.get("created_by") != "LibreDWG 0.14":
            raise SystemExit(f"unexpected peer producer: {peer.get('created_by')!r}")
        if peer.get("FILEHEADER", {}).get("version") != "AC1027":
            raise SystemExit("peer reports a non-AC1027 source")

        source_inventory_status, output = run(
            [str(adapter_tests), "--opaque-dwg-source-inventory", str(source)],
            args.timeout,
        )
        source_after = sha256(source)
        if source_after != source_hash:
            raise SystemExit("source changed during source inventory")
        if "frameCoverageStatus=2 frameSourceDenominator=observed sourceFrames=160 pending=0 deferred=0 " \
                "staged=0 published=160 quarantined=0 failed=0 unresolved=0 " \
                "unrecognizedDispositions=0" not in output:
            raise SystemExit("source frame census is incomplete or changed")
        inventory = re.search(
            r"^opaqueSourceFrameInventory schema=2 sourceVersion=AC1027 "
            r"sourceCallbacks=(\d+) captured=(\d+) retained=(\d+) "
            r"standardCaptured=(\d+) acdsHistoryCaptured=(\d+).*?$",
            output,
            re.MULTILINE,
        )
        if inventory is None or tuple(map(int, inventory.groups())) != (123, 123, 85, 38, 0):
            raise SystemExit("opaque/standard source capture census changed")
        if "sourceFrameIdentityProjection generation=1 unrepresented=0" not in output:
            raise SystemExit("source-frame to callback identity projection is incomplete")

        captured = {}
        for match in FRAME.finditer(output):
            kind, raw_handle, raw_type, record, class_name, custom, owner, reactors, evidence, offset, size = match.groups()
            numeric_handle = int(raw_handle)
            if numeric_handle in captured:
                raise SystemExit(f"duplicate captured frame handle {numeric_handle}")
            captured[numeric_handle] = {
                "captureKind": kind,
                "type": int(raw_type),
                "recordName": record or None,
                "className": class_name or None,
                "customClass": bool(int(custom)),
                "ownerHandle": int(owner),
                "reactorHandles": [int(v) for v in reactors.split(",") if v],
                "commonEvidence": int(evidence),
                "sourceOffset": int(offset),
                "objectSize": int(size),
            }

        peer_objects = peer.get("OBJECTS")
        if not isinstance(peer_objects, list) or len(peer_objects) != 160:
            raise SystemExit("LibreDWG peer object census is not 160")
        peer_by_handle = {
            handle(obj.get("handle")): obj
            for obj in peer_objects
            if isinstance(obj, dict) and isinstance(obj.get("handle"), list)
        }
        found = {}
        for numeric_handle, (expected_type, expected_record) in EXPECTED_ASSOCIATIVE.items():
            frame = captured.get(numeric_handle)
            peer_object = peer_by_handle.get(numeric_handle)
            if frame is None or peer_object is None:
                raise SystemExit(f"associative handle {numeric_handle} missing from source/peer inventory")
            if (frame["type"] != expected_type
                    or frame["recordName"] != expected_record
                    or frame["className"] != {
                        "ACDBASSOCNETWORK": "AcDbAssocNetwork",
                        "ACDBASSOC2DCONSTRAINTGROUP": "AcDbAssoc2dConstraintGroup",
                        "ACDBASSOCGEOMDEPENDENCY": "AcDbAssocGeomDependency",
                    }[expected_record]
                    or frame["ownerHandle"] != handle(peer_object.get("ownerhandle"))
                    or frame["sourceOffset"] < 0
                    or frame["objectSize"] != int(peer_object.get("size", -1))):
                raise SystemExit(f"captured frame {numeric_handle} differs from peer identity")
            peer_reactors = [handle(item) for item in peer_object.get("reactors", [])]
            if frame["reactorHandles"] != peer_reactors:
                raise SystemExit(f"captured frame {numeric_handle} reactors differ from peer")
            found[numeric_handle] = {
                **frame,
                "peerObject": peer_object.get("object"),
                "peerClassVersion": peer_object.get("class_version"),
            }

        if {h for h, obj in peer_by_handle.items() if obj.get("object") in {
                "ASSOCNETWORK", "ASSOC2DCONSTRAINTGROUP", "ASSOCGEOMDEPENDENCY"}} \
                != set(EXPECTED_ASSOCIATIVE):
            raise SystemExit("LibreDWG associative profile handle set changed")

        nonzero_refs = set()
        for obj in peer_objects:
            if not isinstance(obj, dict) or obj.get("object") not in {
                    "ASSOCNETWORK", "ASSOC2DCONSTRAINTGROUP", "ASSOCGEOMDEPENDENCY"}:
                continue
            for field in ("ownerhandle", "owningnetwork", "dep_on"):
                value = obj.get(field)
                if isinstance(value, list) and value:
                    target = handle(value)
                    if target:
                        nonzero_refs.add(target)
            for action in obj.get("actions", []):
                if isinstance(action, dict):
                    target = handle(action.get("dep"))
                elif isinstance(action, list):
                    target = handle(action)
                else:
                    continue
                if target:
                    nonzero_refs.add(target)
        missing_targets = sorted(nonzero_refs - set(peer_by_handle))
        if missing_targets:
            raise SystemExit(f"associative peer references lack object frames: {missing_targets}")

        refusal = re.search(r"^SCALE source inventory identity or ACAD_SCALELIST edge mismatch at handle \d+", output, re.MULTILINE)
        expected_other_profile_refusal = source_inventory_status != 0 and refusal is not None
        if source_inventory_status != 0 and not expected_other_profile_refusal:
            raise SystemExit("source inventory stopped before its known Cover-specific assertion:\n" + output[-8000:])

        candidate_status, candidate_output = run(
            [str(adapter_tests), "--assocnetwork-ac1027-candidate-profile", str(source)],
            args.timeout,
        )
        candidate_summary = next(
            (line.strip() for line in candidate_output.splitlines()
             if line.startswith("Physical AC1027 class-version-2 associative-network ")),
            "",
        )
        if (candidate_status != 0
                or "base/network index separation" not in candidate_summary):
            raise SystemExit("AC1027 network candidate profile failed:\n" + candidate_output[-8000:])

        cli_output_path = Path(work) / "must-not-publish.dxf"
        cli_status, cli_output = run(
            [str(dwg2dxf), str(source), "-y", "-d", "-v2013", str(cli_output_path)],
            args.timeout,
        )
        if (cli_status == 0 or cli_output_path.exists()
                or "AC1027 DWG source references are not fully qualified" not in cli_output):
            raise SystemExit("strict-complete CLI disposition changed:\n" + cli_output[-8000:])

        report = {
            "schema": "libdxfrw-research-receipt-v1",
            "id": "p4-ac1027-libredwg-constraints-source-callback-profile-2026-11-14",
            "date": "2026-11-14",
            "status": "P4_A_AC1027_SOURCE_AND_CALLBACK_IDENTITY_CAPTURED",
            "supportClaimEffect": "none",
            "source": {
                "repository": "LibreDWG/libredwg",
                "path": "test/test-data/2013/Constraints.dwg",
                "url": "https://github.com/LibreDWG/libredwg/blob/master/test/test-data/2013/Constraints.dwg",
                "sha256": source_hash,
                "formatRevision": "AC1027",
                "producer": peer.get("created_by"),
                "peerMinJsonSha256": peer_json_hash,
                "provenanceLimit": "The file is LibreDWG-authored; no original CAD application or release is claimed.",
                "fixtureCommitted": False,
            },
            "tools": {
                "libredwgPeerVersion": "dwgread 0.14",
                "adapterInventoryExecutable": str(adapter_tests),
                "adapterInventorySha256": sha256(adapter_tests),
                "inventoryReturnCode": source_inventory_status,
                "inventoryLimit": "The inventory prints the complete source identity projection, then exits nonzero at a pre-existing Cover-specific SCALE profile assertion; this receipt does not treat that later assertion as a sample failure or pass.",
                "ac1027CandidateProfileReturnCode": candidate_status,
                "ac1027CandidateProfileOutput": candidate_summary,
                "strictCliReturnCode": cli_status,
                "strictCliOutputPublished": cli_output_path.exists(),
                "strictCliDisposition": "refused before output preparation because AC1027 DWG source references are not fully qualified",
            },
            "frameCensus": {
                "sourceFrames": 160,
                "publishedFrames": 160,
                "entityParseFailures": 0,
                "objectParseFailures": 0,
                "opaqueCallbacks": 123,
                "opaqueCaptured": 123,
                "retainedOpaqueFrames": 85,
                "standardFramesCapturedAndMatched": 38,
                "acdsHistoryFrames": 0,
                "unrepresentedSourceIdentities": 0,
            },
            "associativeProfile": {
                "records": found,
                "peerObjectCount": len(peer_objects),
                "peerAssociativeReferenceTargetsPresent": True,
                "classVersions": {
                    str(h): found[h]["peerClassVersion"]
                    for h in EXPECTED_ASSOCIATIVE
                },
                "networkIndexCandidates": {
                    str(h): {
                        "baseActionIndex": peer_by_handle[h].get("action_index"),
                        "networkVersion": peer_by_handle[h].get("network_version"),
                        "networkActionIndex": peer_by_handle[h].get(
                            "network_action_index"
                        ),
                        "writerQualified": False,
                    }
                    for h in (992, 994)
                },
            },
            "limits": [
                "Producer and peer are the same LibreDWG implementation lineage; minJSON is not independent semantic corroboration.",
                "Only five associative object frame identities and their LibreDWG-described links are inventoried; no source-to-entity semantic or native constraint-evaluation result is established.",
                "Strict-complete DWG-to-DXF export refuses this sample on unqualified AC1027 source references; no output support claim is made.",
                "The external DWG is kept in temporary storage and is not committed as a test fixture.",
            ],
        }
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        print(json.dumps(report, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError, subprocess.TimeoutExpired, json.JSONDecodeError) as error:
        print(f"audit failed: {error}", file=sys.stderr)
        raise SystemExit(1)
