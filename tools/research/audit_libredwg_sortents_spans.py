#!/usr/bin/env python3
"""Compare libdxfrw SORTENTSTABLE spans with LibreDWG's independent trace."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import shutil
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO / "tools"))
from bounded_process import run_bounded  # noqa: E402

SORTENT_BLOCK = re.compile(
    r"^Object number:.*?\nAdd object SORTENTSTABLE \[(\d+)\] "
    r"Decode object SORTENTSTABLE\n(.*?)(?=^={10,}\s*$)",
    re.MULTILINE | re.DOTALL,
)
OBJECT_HANDLE = re.compile(r"^handle: .*?\.([0-9A-Fa-f]+) \[H ", re.MULTILINE)
BODY_END = re.compile(r"^bitsize: (\d+) \[RL\]", re.MULTILINE)
HANDLE_BOUNDS = re.compile(
    r"^ Hdlsize: .*?hdl_dat: @(\d+)\.(\d+) - @(\d+)\.(\d+) \(",
    re.MULTILINE,
)
BODY_START = re.compile(r"^is_xdic_missing: .*? @(\d+)\.(\d+)$", re.MULTILINE)
ENTRY_COUNT = re.compile(
    r"^num_ents: (\d+) \[BL \d+\] @(\d+)\.(\d+)$", re.MULTILINE
)
SORT_FIELD = re.compile(
    r"^sort_ents\[(\d+)\]: \((\d+)\.(\d+)\.([0-9A-Fa-f]+)\) "
    r"abs:([0-9A-Fa-f]+).*? @(\d+)\.(\d+)$",
    re.MULTILINE,
)
OWNER_FIELD = re.compile(
    r"^block_owner: \((\d+)\.(\d+)\.([0-9A-Fa-f]+)\) "
    r"abs:([0-9A-Fa-f]+).*? @(\d+)\.(\d+)$",
    re.MULTILINE,
)
ENTITY_FIELD = re.compile(
    r"^ents\[(\d+)\]: \((\d+)\.(\d+)\.([0-9A-Fa-f]+)\) "
    r"abs:([0-9A-Fa-f]+).*? @(\d+)\.(\d+)$",
    re.MULTILINE,
)
TRAILING_PADDING = re.compile(r"^ padding: ([+-]\d+)\s*$", re.MULTILINE)
VERSION = re.compile(r"file's version code is: (AC\d+)")


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def bit_position(byte: str | int, bit: str | int) -> int:
    return int(byte) * 8 + int(bit)


def parse_trace(trace: str) -> dict[int, dict[str, object]]:
    profiles: dict[int, dict[str, object]] = {}
    for match in SORTENT_BLOCK.finditer(trace):
        _object_index, block = match.groups()
        handle_match = OBJECT_HANDLE.search(block)
        body_end_match = BODY_END.search(block)
        handle_bounds_match = HANDLE_BOUNDS.search(block)
        body_start_match = BODY_START.search(block)
        count_match = ENTRY_COUNT.search(block)
        owner_match = OWNER_FIELD.search(block)
        padding_match = TRAILING_PADDING.search(block)
        if not all((handle_match, body_end_match, handle_bounds_match,
                    body_start_match, count_match, owner_match)):
            raise SystemExit("LibreDWG SORTENTSTABLE trace lacks required cursor fields")

        table_handle = int(handle_match.group(1), 16)
        if table_handle in profiles:
            raise SystemExit("LibreDWG trace repeats a SORTENTSTABLE handle")
        body_end = int(body_end_match.group(1))
        handle_start = bit_position(*handle_bounds_match.groups()[:2])
        handle_end = bit_position(*handle_bounds_match.groups()[2:])
        body_start = bit_position(*body_start_match.groups())
        count, count_end_byte, count_end_bit = count_match.groups()
        count = int(count)
        count_end = bit_position(count_end_byte, count_end_bit)
        padding = int(padding_match.group(1)) if padding_match else 0
        if padding < 0 or padding >= 8:
            raise SystemExit("LibreDWG trace reports unsupported trailing object padding")
        handle_fields_end = handle_end - padding

        sorts = []
        for field in SORT_FIELD.finditer(block):
            ordinal, code, size, _relative, target, byte, bit = field.groups()
            sorts.append({
                "ordinal": int(ordinal), "code": int(code),
                "encodedBytes": int(size),
                "targetHandle": int(target, 16),
                "startBit": bit_position(byte, bit),
            })
        sorts.sort(key=lambda item: item["ordinal"])

        owner_code, owner_size, _owner_relative, owner_target, owner_byte, owner_bit = (
            owner_match.groups()
        )
        owner = {
            "code": int(owner_code), "targetHandle": int(owner_target, 16),
            "encodedBytes": int(owner_size),
            "startBit": bit_position(owner_byte, owner_bit),
        }

        entities = []
        for field in ENTITY_FIELD.finditer(block):
            ordinal, code, size, _relative, target, byte, bit = field.groups()
            entities.append({
                "ordinal": int(ordinal), "code": int(code),
                "encodedBytes": int(size),
                "targetHandle": int(target, 16),
                "startBit": bit_position(byte, bit),
            })
        entities.sort(key=lambda item: item["ordinal"])

        if len(sorts) != count or len(entities) != count:
            raise SystemExit(
                f"LibreDWG SORTENTSTABLE {table_handle:#x} count/list mismatch"
            )
        if [item["ordinal"] for item in sorts] != list(range(count)):
            raise SystemExit("LibreDWG SORTENTSTABLE sort fields are unordered")
        if [item["ordinal"] for item in entities] != list(range(count)):
            raise SystemExit("LibreDWG SORTENTSTABLE entity fields are unordered")
        if not (body_start <= count_end <= body_end == handle_start < handle_end):
            raise SystemExit("LibreDWG SORTENTSTABLE body/handle stream bounds are inconsistent")
        if sorts and sorts[0]["startBit"] != count_end:
            raise SystemExit("LibreDWG count end does not meet first sort-handle start")
        if any(item["code"] != 0 for item in sorts):
            raise SystemExit("LibreDWG SORTENTSTABLE sort reference has nonzero H code")
        if owner["startBit"] < handle_start or owner["startBit"] >= handle_fields_end:
            raise SystemExit("LibreDWG SORTENTSTABLE owner field is outside handle data")
        if entities and entities[0]["startBit"] <= owner["startBit"]:
            raise SystemExit("LibreDWG SORTENTSTABLE entity list precedes its owner")
        if any(left["startBit"] >= right["startBit"]
               for left, right in zip(sorts, sorts[1:])):
            raise SystemExit("LibreDWG SORTENTSTABLE sort fields overlap or reorder")
        if any(left["startBit"] >= right["startBit"]
               for left, right in zip(entities, entities[1:])):
            raise SystemExit("LibreDWG SORTENTSTABLE entity fields overlap or reorder")

        profiles[table_handle] = {
            "tableHandle": table_handle,
            "entryCount": count,
            "bodyStartBit": body_start,
            "bodyEndBit": body_end,
            "entryCountSpan": [body_start, count_end],
            "sortFields": sorts,
            "handleStreamStartBit": handle_start,
            "handleStreamEndBit": handle_end,
            "blockOwnerField": owner,
            "entityFields": entities,
            "trailingPaddingBits": padding,
            "handleFieldsEndBit": handle_fields_end,
        }
    if not profiles:
        raise SystemExit("LibreDWG trace contains no SORTENTSTABLE decoder blocks")

    for profile in profiles.values():
        sorts = profile["sortFields"]
        entities = profile["entityFields"]
        for field in sorts:
            field["endBit"] = field["startBit"] + 8 + field["encodedBytes"] * 8
        owner = profile["blockOwnerField"]
        owner["endBit"] = owner["startBit"] + 8 + owner["encodedBytes"] * 8
        for field in entities:
            field["endBit"] = field["startBit"] + 8 + field["encodedBytes"] * 8
    return profiles


def compare_with_libdxfrw(
    peer_profiles: dict[int, dict[str, object]],
    source_profiles: list[dict[str, object]],
) -> list[dict[str, object]]:
    expected = {
        int(str(profile["tableHandle"]), 0): profile for profile in source_profiles
    }
    if set(peer_profiles) != set(expected):
        raise SystemExit("LibreDWG and libdxfrw SORTENTSTABLE profile identities differ")
    result = []
    for handle, peer in sorted(peer_profiles.items()):
        source = expected[handle]
        count_span = source.get("entryCountSpanBits", source.get("entryCountSpan"))
        if not isinstance(count_span, list) or len(count_span) != 2:
            count_span = (
                [source["bodyStartBit"], source["sortHandleFields"][0]["startBit"]]
                if source["sortHandleFields"] else None
            )
        equal_fields = (
            peer["entryCount"] == source["entryCount"]
            and peer["bodyStartBit"] == source["bodyStartBit"]
            and peer["bodyEndBit"] == source["bodyEndBit"]
            and (count_span is None or peer["entryCountSpan"] == count_span)
            and peer["handleStreamStartBit"] == source["handleStreamStartBit"]
            and peer["handleStreamEndBit"] == source["handleStreamEndBit"]
        )
        if not equal_fields:
            raise SystemExit(f"LibreDWG top-level SORTENTSTABLE span mismatch {handle:#x}")
        source_sorts = source["sortHandleFields"]
        source_entities = source["entityHandleFields"]
        if len(peer["sortFields"]) != len(source_sorts):
            raise SystemExit(f"SORTENTSTABLE sort span count mismatch {handle:#x}")
        for peer_field, source_field in zip(peer["sortFields"], source_sorts):
            if (
                peer_field["ordinal"] != source_field["ordinal"]
                or peer_field["code"] != source_handle_code(source_field)
                or peer_field["targetHandle"] != source_field["targetHandle"]
                or peer_field["startBit"] != source_field["startBit"]
                or peer_field["endBit"] != source_field["endBit"]
            ):
                raise SystemExit(
                    f"LibreDWG sort-handle span/role/value mismatch {handle:#x}"
                )
        if len(peer["entityFields"]) != len(source_entities):
            raise SystemExit(f"SORTENTSTABLE entity span count mismatch {handle:#x}")
        for peer_field, source_field in zip(peer["entityFields"], source_entities):
            if (
                peer_field["ordinal"] != source_field["ordinal"]
                or peer_field["code"] != source_handle_code(source_field)
                or peer_field["targetHandle"] != source_field["targetHandle"]
                or peer_field["startBit"] != source_field["startBit"]
                or peer_field["endBit"] != source_field["endBit"]
            ):
                raise SystemExit(
                    f"LibreDWG entity-handle span/role/value mismatch {handle:#x}"
                )
        source_owner = source["blockOwnerField"]
        peer_owner = peer["blockOwnerField"]
        if (
            peer_owner["code"] != source_handle_code(source_owner)
            or peer_owner["targetHandle"] != source_owner["targetHandle"]
            or peer_owner["startBit"] != source_owner["startBit"]
            or peer_owner["endBit"] != source_owner["endBit"]
        ):
            raise SystemExit(f"LibreDWG owner-handle span/role/value mismatch {handle:#x}")
        result.append({
            "tableHandle": handle,
            "entryCount": peer["entryCount"],
            "sortFieldSpanCount": len(peer["sortFields"]),
            "entityFieldSpanCount": len(peer["entityFields"]),
            "allBodyAndHandleFieldSpansMatch": True,
            "ownerHandleFieldMatches": True,
            "peerDecodedTargetsMatch": True,
            "countFieldSpanComparedToParser": count_span is not None,
        })
    return result


def source_handle_code(field: dict[str, object]) -> int:
    role = str(field.get("wireRole", field.get("role", "")))
    match = re.search(r"ODA handle code (\d+)", role)
    if match is None:
        raise SystemExit("libdxfrw SORTENTSTABLE receipt lacks a decoded handle code")
    return int(match.group(1))


def self_test() -> None:
    trace = """Object number: 1/1, Size: 25 [MS], Type: 505 [BS], Address: 100
Add object SORTENTSTABLE [1] Decode object SORTENTSTABLE
bitsize: 121 [RL] @4.2
 Hdlsize: 0x40, hdl_dat: @15.1 - @25.0 (25)
handle: 0.1.A [H 5]
is_xdic_missing: 1 [B 0] @10.7
num_ents: 1 [BL 0] @12.1
sort_ents[0]: (0.2.14) abs:14 [H* 0] @12.1
block_owner: (4.1.7) abs:7 [H 0] @15.1
ents[0]: (4.1.9) abs:9 [H* 0] @17.1
 padding: +6 
 object_map{A} = 1
==========================================
"""
    parsed = parse_trace(trace)
    profile = parsed[10]
    if (
        profile["bodyStartBit"] != 87
        or profile["entryCountSpan"] != [87, 97]
        or profile["sortFields"][0]["targetHandle"] != 20
        or profile["sortFields"][0]["startBit"] != 97
        or profile["sortFields"][0]["endBit"] != 121
        or profile["blockOwnerField"]["targetHandle"] != 7
        or profile["blockOwnerField"]["startBit"] != 121
        or profile["blockOwnerField"]["endBit"] != 137
        or profile["entityFields"][0]["targetHandle"] != 9
        or profile["entityFields"][0]["startBit"] != 137
        or profile["entityFields"][0]["endBit"] != 153
    ):
        raise SystemExit("generated independent trace position control failed")
    source = [{
        "tableHandle": 10, "entryCount": 1, "bodyStartBit": 87,
        "bodyEndBit": 121, "entryCountSpanBits": [87, 97],
        "sortHandleFields": [{
            "ordinal": 0, "targetHandle": 20, "startBit": 97, "endBit": 121,
            "wireRole": "absolute-reference (ODA handle code 0 enforced by parser)",
        }],
        "handleStreamStartBit": 121, "handleStreamEndBit": 200,
        "blockOwnerField": {"targetHandle": 7, "startBit": 121,
                            "endBit": 137,
                            "role": "soft-pointer (ODA handle code 4 enforced by parser)"},
        "entityHandleFields": [{
            "ordinal": 0, "targetHandle": 9, "startBit": 137, "endBit": 153,
            "wireRole": "soft-pointer (ODA handle code 4 enforced by parser)",
        }],
    }]
    if len(compare_with_libdxfrw(parsed, source)) != 1:
        raise SystemExit("generated source/peer physical span match failed")
    mutations = []
    mutated_span = json.loads(json.dumps(source))
    mutated_span[0]["entityHandleFields"][0]["startBit"] += 1
    mutations.append(mutated_span)
    mutated_role = json.loads(json.dumps(source))
    mutated_role[0]["entityHandleFields"][0]["wireRole"] = (
        "soft-pointer (ODA handle code 2 enforced by parser)"
    )
    mutations.append(mutated_role)
    mutated_target = json.loads(json.dumps(source))
    mutated_target[0]["entityHandleFields"][0]["targetHandle"] += 1
    mutations.append(mutated_target)
    for mutation in mutations:
        try:
            compare_with_libdxfrw(parsed, mutation)
        except SystemExit:
            continue
        raise SystemExit("independent span/role/target mutation was admitted")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--source", type=Path)
    parser.add_argument("--libdxfrw-audit", type=Path)
    parser.add_argument("--dwgread", type=Path)
    parser.add_argument("--timeout", type=float, default=60.0)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    if args.self_test:
        self_test()
        print("LibreDWG independent SORTENTSTABLE span controls passed")
        return 0
    if args.source is None or args.libdxfrw_audit is None:
        parser.error("--source and --libdxfrw-audit are required unless --self-test is used")
    source = args.source.resolve(strict=True)
    audit_path = args.libdxfrw_audit.resolve(strict=True)
    dwgread = args.dwgread or (Path(shutil.which("dwgread")) if shutil.which("dwgread") else None)
    if dwgread is None:
        parser.error("LibreDWG dwgread executable not found; supply --dwgread")
    dwgread = dwgread.resolve(strict=True)
    version_result = run_bounded(
        [str(dwgread), "--version"], timeout=5.0,
        output_limit=64 * 1024, cwd=REPO,
    )
    if version_result.returncode != 0:
        raise SystemExit("LibreDWG dwgread --version failed")
    peer_version = (version_result.stdout + version_result.stderr).decode(
        "utf-8", errors="replace"
    ).strip()
    source_hash = sha256_file(source)
    command = [str(dwgread), "-v5", "-O", "minJSON", str(source)]
    result = run_bounded(
        command, timeout=args.timeout, output_limit=32 * 1024 * 1024, cwd=REPO
    )
    trace = result.stderr.decode("utf-8", errors="replace")
    peer_json = result.stdout.decode("utf-8", errors="replace")
    if result.returncode != 0:
        sys.stderr.write(trace[-12000:])
        raise SystemExit("LibreDWG verbose source decode failed")
    parsed_peer_json = json.loads(peer_json)
    version_match = VERSION.search(trace)
    if version_match is None:
        raise SystemExit("LibreDWG trace does not report the source version")
    peer_profiles = parse_trace(trace)
    source_audit = json.loads(audit_path.read_text())
    source_record = source_audit.get("source", {})
    audit_source_hash = source_audit.get(
        "sourceSha256Before", source_record.get("sha256")
    )
    if audit_source_hash != source_hash:
        raise SystemExit("libdxfrw audit is not bound to the same source hash")
    audit_source_version = source_audit.get(
        "sourceVersion", source_record.get("version")
    )
    if audit_source_version != version_match.group(1):
        raise SystemExit("libdxfrw and LibreDWG source versions differ")
    source_profiles = source_audit.get(
        "sourceSortEntsFieldSpans", source_audit.get("matchedProfiles")
    )
    if not isinstance(source_profiles, list):
        raise SystemExit("libdxfrw audit has no SORTENTSTABLE field-span profiles")
    matches = compare_with_libdxfrw(peer_profiles, source_profiles)
    if sha256_file(source) != source_hash:
        raise SystemExit("source bytes changed during LibreDWG trace audit")

    peer_json_objects = parsed_peer_json.get("OBJECTS")
    if not isinstance(peer_json_objects, list):
        raise SystemExit("LibreDWG minJSON output does not contain OBJECTS")
    peer_sortents = {}
    for obj in peer_json_objects:
        if not isinstance(obj, dict) or obj.get("object") != "SORTENTSTABLE":
            continue
        handle_value = obj.get("handle")
        if isinstance(handle_value, list) and handle_value:
            peer_sortents[int(handle_value[-1])] = obj
    if set(peer_sortents) != set(peer_profiles):
        raise SystemExit("LibreDWG minJSON/trace SORTENTSTABLE profiles differ")
    for handle, trace_profile in peer_profiles.items():
        peer_profile = peer_sortents[handle]
        sort_refs = peer_profile.get("sort_ents", [])
        entity_refs = peer_profile.get("ents", [])
        owner_ref = peer_profile.get("block_owner")
        if not isinstance(sort_refs, list) or not isinstance(entity_refs, list):
            raise SystemExit("LibreDWG minJSON lacks SORTENTSTABLE reference arrays")
        if len(sort_refs) != len(trace_profile["sortFields"]):
            raise SystemExit("LibreDWG verbose/minJSON sort-reference counts differ")
        if len(entity_refs) != len(trace_profile["entityFields"]):
            raise SystemExit("LibreDWG verbose/minJSON entity-reference counts differ")
        if not isinstance(owner_ref, list) or len(owner_ref) < 4:
            raise SystemExit("LibreDWG minJSON lacks the SORTENTSTABLE owner reference")
        peer_refs = [*sort_refs, *entity_refs, owner_ref]
        trace_refs = [
            *trace_profile["sortFields"], *trace_profile["entityFields"],
            trace_profile["blockOwnerField"],
        ]
        if any(
            not isinstance(ref, list) or len(ref) < 4
            or int(ref[0]) != traced["code"]
            or int(ref[1]) != traced["encodedBytes"]
            or int(ref[-1]) != traced["targetHandle"]
            for ref, traced in zip(peer_refs, trace_refs)
        ):
            raise SystemExit("LibreDWG verbose trace and minJSON references disagree")

    report = {
        "schema": "libdxfrw.research-receipt.v1",
        "id": "p6-ac1021-imperial-sortents-independent-libredwg-spans-2026-11-14",
        "date": "2026-11-14",
        "status": "INDEPENDENT_LIBREDWG_SORTENTSTABLE_FIELD_SPAN_DIFFERENTIAL_PASS",
        "supportClaimEffect": "none",
        "source": {
            "path": str(source), "sha256": source_hash,
            "version": version_match.group(1), "unchangedDuringRun": True,
            "fixtureFilesAdded": 0,
        },
        "independentPeer": {
            "name": "LibreDWG dwgread",
            "version": peer_version,
            "executable": str(dwgread), "executableSha256": sha256_file(dwgread),
            "verbosity": 5,
            "traceSha256": sha256_bytes(result.stderr),
            "minJsonSha256": sha256_bytes(result.stdout),
            "fieldPositionSource": "LibreDWG v0.14 decoder LOG_INSANE cursor positions and object bitstream bounds",
        },
        "coordinateFrame": "object-relative decompressed DWG bitstream; body/handle streams separately bounded",
        "comparison": {
            "matchedTableCount": len(matches),
            "matchedSortFieldSpanCount": sum(len(p["sortFields"]) for p in peer_profiles.values()),
            "matchedEntityFieldSpanCount": sum(len(p["entityFields"]) for p in peer_profiles.values()),
            "matchedOwnerFieldSpanCount": len(peer_profiles),
            "parserCountSpanProfileCount": sum(
                bool(profile["countFieldSpanComparedToParser"])
                for profile in matches
            ),
            "allDecodedRolesAndTargetsMatch": True,
            "allCapturedParserAndPeerFieldStartsAndEndsMatch": True,
            "verboseTraceAndMinJsonReferenceCodesWidthsAndTargetsMatch": True,
            "allBodyAndHandleStreamBoundsMatch": True,
            "profiles": matches,
        },
        "limitations": [
            "The source witness is one AC1021 producer profile; it does not establish other DWG revisions or producer prevalence.",
            "Matching reference offsets does not resolve recursive target ownership/closure, DXF output, or native draw order.",
            "No DWG/DXF fixture was added; the existing local source was read without modification.",
        ],
        "planImpact": {
            "completedSlice": "Independent LibreDWG v0.14 trace agrees with libdxfrw on every SORTENTSTABLE count/body bound, sort-handle span and target, handle-stream bound, block-owner span and target, and entity-handle span and target for all 16 AC1021 profiles.",
            "remaining": "Recursive target closure, other uncovered frame families, strict output, other producer/revision profiles, and native ordering remain open.",
        },
    }
    rendered = json.dumps(report, indent=2, sort_keys=True) + "\n"
    if args.output:
        args.output.write_text(rendered)
    else:
        print(rendered, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
