#!/usr/bin/env python3
"""Compare one physical DWG MLINESTYLE profile with LibreDWG minJSON."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
from pathlib import Path

from audit_ac1027_source_references import (
    FRAME,
    MLINESTYLE_ELEMENT,
    MLINESTYLE_FIELDS,
    REPO,
    SUMMARY,
    peer_handle,
    sha256,
    verify_mlinestyle_fields,
)
from bounded_process import run_bounded

BODY = re.compile(
    r"^ typedBodyFrame type=MLINESTYLE handle=(\d+) elements=(\d+) "
    r"owner=(\d+) reactors=(\d+) xdict=(\d+) dictionaryOwner=(\d+) "
    r"dictionaryKey=(\S+) bodyHandleFields=(\d+) bodyReferenceRule=(\S+)$",
    re.MULTILINE,
)
EXPECTED_LATER_FAILURE = re.compile(
    r"^VISUALSTYLE source identity or common-link mismatch at handle \d+$"
)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--executable", type=Path, required=True)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--peer-json", type=Path, required=True)
    parser.add_argument("--timeout", type=float, default=60.0)
    args = parser.parse_args()

    executable = args.executable.resolve(strict=True)
    source = args.source.resolve(strict=True)
    peer_path = args.peer_json.resolve(strict=True)
    source_hash_before = sha256(source)
    peer_json = json.loads(peer_path.read_text())
    result = run_bounded(
        [str(executable), "--opaque-dwg-source-inventory", str(source)],
        timeout=args.timeout,
        output_limit=32 * 1024 * 1024,
        cwd=REPO,
    )
    source_hash_after = sha256(source)
    stderr = result.stderr.decode("utf-8", errors="replace")
    output = (result.stdout + b"\n" + result.stderr).decode(
        "utf-8", errors="replace"
    )

    frames = []
    inventory_frame_count = 0
    inventory_bodies_complete = True
    for match in FRAME.finditer(output):
        (kind, handle, object_type, record, class_name, custom, owner,
         block_owner, xdict, reactors, evidence, offset, size,
         data_storage, raw_body_bytes) = match.groups()
        inventory_frame_count += 1
        inventory_bodies_complete &= int(raw_body_bytes) == int(size)
        if int(object_type) == 73:
            frames.append({
                "captureKind": kind,
                "handle": int(handle),
                "type": int(object_type),
                "ownerHandle": int(owner),
                "reactorHandles": [int(item) for item in reactors.split(",") if item],
                "sourceOffset": int(offset),
                "objectSize": int(size),
                "commonLinkEvidence": int(evidence),
                "hasDataStorage": bool(int(data_storage)),
            })
    fields = {}
    for match in MLINESTYLE_FIELDS.finditer(output):
        (handle, name_hex, description_hex, flags, fill_color, fill_color24,
         start_angle, end_angle, element_count) = match.groups()
        key = int(handle)
        if key in fields:
            raise SystemExit("source diagnostic repeated a typed MLINESTYLE row")
        fields[key] = {
            "handle": key,
            "nameHex": name_hex,
            "descriptionHex": description_hex,
            "flags": int(flags),
            "fillColor": int(fill_color),
            "fillColor24": int(fill_color24),
            "startAngleDegrees": float(start_angle),
            "endAngleDegrees": float(end_angle),
            "elementCount": int(element_count),
        }
    elements: dict[int, list[dict[str, object]]] = {}
    for match in MLINESTYLE_ELEMENT.finditer(output):
        handle, index, offset, color, color24, line_type = match.groups()
        elements.setdefault(int(handle), []).append({
            "index": int(index),
            "offset": float(offset),
            "color": int(color),
            "color24": int(color24),
            "linetypeIndex": int(line_type),
        })
    bodies = {int(match.group(1)): match.groups() for match in BODY.finditer(output)}
    if len(frames) != 1:
        raise SystemExit(f"expected one retained type-73 frame, observed {len(frames)}")
    frame = frames[0]
    handle = int(frame["handle"])
    body = bodies.get(handle)
    if body is None or set(fields) != {handle} or set(elements) != {handle}:
        raise SystemExit("source frame and typed MLINESTYLE diagnostics do not align")
    if source_hash_before != source_hash_after:
        raise SystemExit("source DWG changed during the MLINESTYLE diagnostic")

    summary_match = SUMMARY.search(output)
    if summary_match is None:
        raise SystemExit("source diagnostic omitted its frame-count summary")
    (schema, source_version, callbacks, captured, retained,
     standard_retained, acds_history_retained, utility_bound) = (
        summary_match.groups()
    )
    if int(schema) != 2 or source_version != source.open("rb").read(6).decode(
        "ascii", errors="replace"
    ):
        raise SystemExit("source diagnostic summary version/schema does not match input")
    captured = int(captured)
    retained = int(retained)
    standard_retained = int(standard_retained)
    acds_history_retained = int(acds_history_retained)
    callbacks = int(callbacks)
    inventory_identities = retained + standard_retained + acds_history_retained
    source_inventory_closed = (
        callbacks == captured == inventory_identities
        and inventory_frame_count == inventory_identities
        and inventory_bodies_complete
    )

    expected_later_failure = False
    if result.returncode != 0:
        error_lines = [line for line in stderr.splitlines() if line.strip()]
        failure_lines = [
            line for line in error_lines
            if re.search(r"(?:mismatch|fail(?:ed|ure)|error)", line, re.IGNORECASE)
        ]
        expected_later_failure = bool(
            error_lines and EXPECTED_LATER_FAILURE.fullmatch(error_lines[-1])
            and failure_lines == [error_lines[-1]]
        )
        if not expected_later_failure:
            sys.stderr.write(stderr[-12000:])
            raise SystemExit("source diagnostic failed outside the admitted later VISUALSTYLE check")

    peer_objects = peer_json.get("OBJECTS")
    if not isinstance(peer_objects, list):
        raise SystemExit("LibreDWG minJSON has no OBJECTS array")
    peer_styles = [
        item for item in peer_objects
        if isinstance(item, dict) and item.get("object") == "MLINESTYLE"
        and peer_handle(item.get("handle")) == handle
    ]
    if len(peer_styles) != 1:
        raise SystemExit("LibreDWG minJSON lacks a unique matching MLINESTYLE handle")
    peer = peer_styles[0]
    body_values = tuple(body)
    body_elements = int(body_values[1])
    if (
        int(peer.get("type", -1)) != 73
        or int(peer.get("size", -1)) != int(frame["objectSize"])
        or peer_handle(peer.get("ownerhandle")) != int(frame["ownerHandle"])
        or [value for value in (peer_handle(item) for item in peer.get("reactors", []))
            if value is not None] != frame["reactorHandles"]
        or body_elements != fields[handle]["elementCount"]
        or int(body_values[2]) != int(frame["ownerHandle"])
        or int(body_values[3]) != len(frame["reactorHandles"])
        or int(body_values[5]) != int(frame["ownerHandle"])
        or body_values[6] != "Standard"
        or int(body_values[7]) != 0
        or body_values[8] != "ODA-20.4.73"
    ):
        raise SystemExit("source, typed callback, and LibreDWG frame identity do not agree")
    matched_fields = verify_mlinestyle_fields(fields[handle], elements[handle], peer)
    print(json.dumps({
        "source": str(source),
        "sourceVersion": source.open("rb").read(6).decode("ascii", errors="replace"),
        "sourceSha256": source_hash_before,
        "sourceUnchanged": True,
        "sourceFrame": frame,
        "typedDictionaryMembership": {
            "ownerHandle": int(body_values[5]),
            "key": body_values[6],
            "bodyHandleFields": int(body_values[7]),
            "rule": body_values[8],
        },
        "matchedFields": matched_fields,
        "peerFrameSize": int(peer["size"]),
        "peerCreatedBy": peer_json.get("created_by"),
        "peerJsonPath": str(peer_path),
        "peerJsonSha256": sha256(peer_path),
        "auditExecutable": str(executable),
        "auditExecutableSha256": sha256(executable),
        "diagnosticExitCode": result.returncode,
        "diagnosticOutputSha256": hashlib.sha256(result.stdout).hexdigest(),
        "laterUnrelatedVisualStyleCheckFailed": expected_later_failure,
        "diagnosticAssertionsPassed": result.returncode == 0,
        "sourceCallbackCount": callbacks,
        "capturedCallbackCount": captured,
        "retainedOpaqueFrameCount": retained,
        "retainedStandardObjectFrameCount": standard_retained,
        "retainedAcDsHistoryFrameCount": acds_history_retained,
        "publishedFrameIdentityCount": inventory_identities,
        "printedFrameIdentityCount": inventory_frame_count,
        "allPrintedFrameBodiesComplete": inventory_bodies_complete,
        "sourceHashBoundByDiagnosticUtility": bool(int(utility_bound)),
        "sourceHashBoundExternallyBeforeAndAfter": source_hash_before == source_hash_after,
        "sourceFrameIdentityInventoryClosed": (
            result.returncode == 0 and source_inventory_closed
            and source_hash_before == source_hash_after
        ),
        "wholeDrawingSemanticOrOutputQualified": False,
        "limits": [
            "Only the MLINESTYLE semantic fields are differentially compared; closure of source frame identities does not establish whole-drawing graph or output qualification.",
            "LibreDWG minJSON supplies semantic values and object size but not independent physical subfield bit spans.",
            "The source inventory separates ordinary opaque frames, standard OBJECTS frames, and the ACDS history frame lane; no support claim is promoted."
        ],
    }, indent=2, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
