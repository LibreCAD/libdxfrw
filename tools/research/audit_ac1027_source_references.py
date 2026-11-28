#!/usr/bin/env python3
"""Hash-bind libdxfrw's retained-frame/common-link diagnostic for one DWG."""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import re
import sys
from collections import Counter
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
COVER_AC1027_SHA256 = (
    "3a6c23151ebec25320722f0ecaaf20a10073c60079d3616181395f1c73deee42"
)
sys.path.insert(0, str(REPO / "tools"))
from bounded_process import run_bounded  # noqa: E402

SUMMARY = re.compile(
    r"^opaqueSourceFrameInventory schema=(\d+) sourceVersion=(\S+) "
    r"sourceCallbacks=(\d+) captured=(\d+) retained=(\d+) "
    r"standardCaptured=(\d+) acdsHistoryCaptured=(\d+).*?"
    r"sourceHashBoundByUtility=(\d+)$",
    re.MULTILINE,
)
FRAME = re.compile(
    r"^ (opaque|standard|acds) handle=(\d+) type=(\d+) record=(\S*) "
    r"class=(\S*) custom=(\d+) "
    r"(?:payloadMatched=\d+ opaquePayload=\d+ )?"
    r"owner=(\d+) blockOwner=(\d+) "
    r"xdict=(\d+) reactors=([^ ]*) commonEvidence=(\d+) "
    r"offset=(\d+) size=(\d+) dataStorage=(\d+) rawBodyBytes=(\d+)$",
    re.MULTILINE,
)
SORTENTS_INVENTORY = re.compile(
    r"^opaqueSortEntsMembershipInventory tables=(\d+) entries=(\d+)$",
    re.MULTILINE,
)
SORTENTS_SOURCE = re.compile(
    r"^ sortentsSource table=(\d+) version=(\S+) encodedType=(\d+) "
    r"owner=(\d+) complete=(\d+) entries=(\d+)$",
    re.MULTILINE,
)
SORTENTS_ENTRY = re.compile(
    r"^ sortentsEntry table=(\d+) ordinal=(\d+) entity=(\d+) "
    r"sort=(\d+) fallback=(\d+)$",
    re.MULTILINE,
)
SORTENTS_SPAN = re.compile(
    r"^sortentsSourceSpan coordinateFrame=(\S+) handle=(\d+) version=(\d+) "
    r"bodyStart=(\d+) bodyEnd=(\d+) count=(\d+) countSpan=(\d+):(\d+) "
    r"sortSpans=(.*?) handleStreamStart=(\d+) handleStreamEnd=(\d+) "
    r"ownerSpan=(\d+):(\d+):(\d+) entitySpans=(.*)$",
    re.MULTILINE,
)
BODY_FRAME = re.compile(
    r"^ typedBodyFrame type=(\S+) handle=(\d+)(?: |$)", re.MULTILINE
)
MLINESTYLE_FIELDS = re.compile(
    r"^ typedMLineStyleFields handle=(\d+) nameHex=([0-9a-f]*) "
    r"descriptionHex=([0-9a-f]*) flags=(-?\d+) fillColor=(-?\d+) "
    r"fillColor24=(-?\d+) startAngleDegrees=(\S+) "
    r"endAngleDegrees=(\S+) elementCount=(\d+)$",
    re.MULTILINE,
)
MLINESTYLE_ELEMENT = re.compile(
    r"^ typedMLineStyleElement handle=(\d+) index=(\d+) "
    r"offset=(\S+) color=(-?\d+) color24=(-?\d+) "
    r"linetypeIndex=(-?\d+)$",
    re.MULTILINE,
)
MODELER_HISTORY_PROFILE = re.compile(
    r"^ typedBodyFrame type=3DSOLID handle=(\d+) layerName=(.*?) "
    r"layerMatches=(\d+) historyPresent=(\d+) historyCode=(\d+) "
    r"historySize=(\d+) historyTarget=(\d+) historyStartBit=(\d+) "
    r"historyBitSize=(\d+) historyComplete=(\d+)$",
    re.MULTILINE,
)
MATERIAL_PROFILE = re.compile(
    r"^ typedBodyFrame type=MATERIAL handle=(\d+) .*?"
    r"semanticFieldsComplete=(\d+) rawTailValidated=(\d+) .*?"
    r"dictionaryKey=(ByLayer|ByBlock|Global) bodyHandleFields=0 "
    r"bodyReferenceRule=LIBREDWG-dwg2\.spec-MATERIAL$",
    re.MULTILINE,
)
MATERIAL_FIELDS = re.compile(
    r"^ typedMaterialFields handle=(\d+) name=(ByLayer|ByBlock|Global) "
    r"rawVersion=17 ambient=0/1/0 diffuse=0/1/0 refractionSource=0 "
    r"channelFlags=127 mapModels=diffuse,specular,reflection,opacity,bump,refraction$",
    re.MULTILINE,
)
NO_BODY_HANDLE_PROFILE = re.compile(
    r"^ typedBodyFrame type=(SCALE|DICTIONARYVAR|RASTERVARIABLES|MLINESTYLE|ACDBPLACEHOLDER|VISUALSTYLE|MATERIAL|SUN) "
    r"handle=(\d+) .*?"
    r"bodyHandleFields=0 bodyReferenceRule="
    r"(ODA-20\.4\.(?:92|74|91|73)|LIBREDWG-dwg2\.spec-(?:PLACEHOLDER|VISUALSTYLE|MATERIAL|SUN))$",
    re.MULTILINE,
)
HANDLE_BODY_PROFILE = re.compile(
    r"^ typedBodyFrame type=(LAYOUT|DICTIONARY|DICTIONARYWDFLT) handle=(\d+) .*?"
    r"bodyHandleFields=(\d+) bodyReferenceRule=(ODA-20\.4\.(?:84|44|45))"
    r"(?: defaultEntryRole=hard-pointer)?$",
    re.MULTILINE,
)
TABLESTYLE_PROFILE = re.compile(
    r"^ typedBodyProfile type=TABLESTYLE handle=(\d+) "
    r"bodyHandleFields=(\d+) tableCellStyleOwnerRole=hard-owner "
    r"tableCellStyleOwnerTarget=(\d+) nestedTextStyleFields=(\d+) "
    r"nestedBorderLineTypeFields=(\d+) "
    r"rule=(ODA-20\.4\.101\.2\+ODA-20\.4\.101\.3\+ODA-20\.4\.101\.4) "
    r"complete=1$",
    re.MULTILINE,
)
MLEADERSTYLE_PROFILE = re.compile(
    r"^ typedBodyProfile type=MLEADERSTYLE handle=(\d+) "
    r"bodyHandleFields=(\d+) rule=(ODA-20\.4\.87) "
    r"dictionaryKey=(ACAD_MLEADERSTYLE) dictionaryHandle=(\d+) complete=1$",
    re.MULTILINE,
)
MLEADERSTYLE_HANDLE_FIELD = re.compile(
    r"^ typedBodyHandleField owner=(\d+) role=(lineType|arrowHead|textStyle|block) "
    r"code=(\d+) target=(\d+) startBit=(\d+) bitSize=(\d+) complete=1$",
    re.MULTILINE,
)
DICTIONARY_MEMBERSHIP = re.compile(
    r"^ typedBodyFrame type=(SCALE|DICTIONARYVAR|MLINESTYLE|ACDBPLACEHOLDER|MATERIAL) handle=(\d+) .*?"
    r"dictionaryOwner=(\d+) dictionaryKey=(.*?) "
    r"bodyHandleFields=0 bodyReferenceRule="
    r"(ODA-20\.4\.(?:92|74|73)|LIBREDWG-dwg2\.spec-(?:PLACEHOLDER|MATERIAL))$",
    re.MULTILINE,
)
NOD_ENTRY = re.compile(r"^ NOD key=(.*?) target=(\d+)(?: |$)", re.MULTILINE)
BODY_REFERENCE = re.compile(
    r"^ typedBodyReference owner=(\d+) (?:role=(.*?) )?target=(\d+)$",
    re.MULTILINE,
)
LAYER_FRAME = re.compile(
    r"^ typedBodyFrame type=LAYER handle=(\d+) name=(.*?)$", re.MULTILINE
)
LAYER_HANDLE_FIELD = re.compile(
    r"^ typedLayerHandleField owner=(\d+) role=(xrefBlock|plotStyle|material|lineType|unknown) "
    r"code=(\d+) target=(\d+) startBit=(\d+) bitSize=(\d+) complete=(\d+)$",
    re.MULTILINE,
)
DIMSTYLE_FRAME = re.compile(
    r"^ typedBodyFrame type=DIMSTYLE handle=(\d+) name=(.*?)$", re.MULTILINE
)
DIMSTYLE_HANDLE_FIELD = re.compile(
    r"^ typedDimstyleHandleField owner=(\d+) "
    r"role=(xrefBlock|dimtxsty|leaderBlock|dimblk|dimblk1|dimblk2|"
    r"dimltype|dimltex1|dimltex2) code=(\d+) target=(\d+) "
    r"startBit=(\d+) bitSize=(\d+) complete=(\d+)$",
    re.MULTILINE,
)
VPORT_HANDLE_FIELD = re.compile(
    r"^ typedVportHandleField owner=(\d+) role=(control|reactor\[\d+\]|"
    r"xdict|xrefBlock|background|visualStyle|sun|namedUcs|baseUcs) "
    r"code=(\d+) target=(\d+) startBit=(\d+) bitSize=(\d+) complete=(\d+)$",
    re.MULTILINE,
)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def peer_handle(value: object) -> int | None:
    if isinstance(value, list) and value:
        tail = value[-1]
        return int(tail) if isinstance(tail, int) else None
    return None


def normalize_null_handle(value: int | None) -> int | None:
    return None if value in (None, 0, 0xFFFFFFFF) else value


def verify_sortents_entry_pairs(
    source_entries: list[dict[str, object]],
    peer_entities: list[object],
    peer_sorts: list[object],
) -> list[dict[str, object]]:
    if (
        len(source_entries) != len(peer_entities)
        or len(source_entries) != len(peer_sorts)
    ):
        raise SystemExit("SORTENTSTABLE source and peer entry counts differ")
    pairs = []
    for ordinal, (source_entry, peer_entity, peer_sort) in enumerate(zip(
        source_entries, peer_entities, peer_sorts
    )):
        if (
            not isinstance(peer_entity, list) or not peer_entity
            or not isinstance(peer_sort, list) or not peer_sort
            or peer_entity[0] != 4 or peer_sort[0] != 0
        ):
            raise SystemExit("LibreDWG SORTENTSTABLE entry has the wrong handle code")
        source_entity = int(source_entry["entityHandle"])
        source_sort = int(source_entry["sortHandle"])
        peer_entity_handle = peer_handle(peer_entity)
        peer_sort_handle = peer_handle(peer_sort)
        if (
            int(source_entry["ordinal"]) != ordinal
            or normalize_null_handle(source_entity)
                != normalize_null_handle(peer_entity_handle)
            or normalize_null_handle(source_sort)
                != normalize_null_handle(peer_sort_handle)
            or bool(source_entry["sortFallsBackToEntity"])
                != (normalize_null_handle(peer_sort_handle) is None)
        ):
            raise SystemExit(
                f"SORTENTSTABLE source/peer body reference mismatch at entry {ordinal}"
            )
        pairs.append({
            "ordinal": ordinal,
            "entityHandle": normalize_null_handle(source_entity),
            "sortHandle": normalize_null_handle(source_sort),
            "sortFallsBackToEntity": bool(source_entry["sortFallsBackToEntity"]),
        })
    return pairs


def self_test_sortents_peer_fields() -> None:
    source = [{
        "ordinal": 0, "entityHandle": 42, "sortHandle": 99,
        "sortFallsBackToEntity": False,
    }]
    peer_entities = [[4, 1, 42, 42]]
    peer_sorts = [[0, 1, 99, 99]]
    if len(verify_sortents_entry_pairs(source, peer_entities, peer_sorts)) != 1:
        raise SystemExit("generated SORTENTSTABLE peer control did not match")
    for entities, sorts in (
        ([[4, 1, 43, 43]], peer_sorts),
        (peer_entities, [[0, 1, 98, 98]]),
        ([[5, 1, 42, 42]], peer_sorts),
    ):
        try:
            verify_sortents_entry_pairs(source, entities, sorts)
        except SystemExit:
            continue
        raise SystemExit("generated SORTENTSTABLE peer mutation was admitted")


def decode_span_list(
    value: str, label: str, wire_role: str
) -> list[dict[str, int | str]]:
    if not value:
        return []
    result = []
    for row in value.split(","):
        parts = row.split(":")
        if len(parts) != 4:
            raise SystemExit(f"malformed SORTENTSTABLE {label} span row")
        ordinal, start, end, target = map(int, parts)
        if ordinal != len(result) or start < 0 or end <= start:
            raise SystemExit(f"invalid SORTENTSTABLE {label} span bounds/order")
        result.append({
            "ordinal": ordinal, "startBit": start, "endBit": end,
            "targetHandle": target, "wireRole": wire_role,
        })
    if any(left["endBit"] > right["startBit"]
           for left, right in zip(result, result[1:])):
        raise SystemExit(f"overlapping SORTENTSTABLE {label} fields")
    return result


def verify_sortents_source_spans(
    matches: list[tuple[str, ...]],
    profiles: dict[int, dict[str, object]],
    expected_version: str,
) -> list[dict[str, object]]:
    fields_by_handle: dict[int, dict[str, object]] = {}
    for match in matches:
        (coordinate_frame, handle, version_code, body_start, body_end, count,
         count_start, count_end, sort_rows, handle_stream_start, handle_stream_end,
         owner_start, owner_end, owner_target, entity_rows) = match
        handle_value = int(handle)
        profile = profiles.get(handle_value)
        if profile is None or handle_value in fields_by_handle:
            raise SystemExit("SORTENTSTABLE span row is missing or duplicated")
        entries = profile["entries"]
        count_value = int(count)
        count_start_value, count_end_value = int(count_start), int(count_end)
        body_start_value = int(body_start)
        body_end_value = int(body_end)
        if (
            coordinate_frame != "objectDecompressedBitstreamBits"
            or profile["version"] != expected_version
            or count_value != int(profile["declaredEntryCount"])
            or count_start_value != body_start_value
            or count_end_value <= count_start_value
            or count_end_value > body_end_value
        ):
            raise SystemExit("SORTENTSTABLE count span does not match its source profile")
        sort_spans = decode_span_list(
            sort_rows, "sort-handle", "null-reference (ODA handle code 0 enforced by parser)"
        )
        entity_spans = decode_span_list(
            entity_rows, "entity-handle", "soft-pointer (ODA handle code 4 enforced by parser)"
        )
        if (
            len(sort_spans) != count_value or len(entity_spans) != count_value
            or any(int(item["targetHandle"]) != int(entry["sortHandle"])
                   for item, entry in zip(sort_spans, entries))
            or any(int(item["targetHandle"]) != int(entry["entityHandle"])
                   for item, entry in zip(entity_spans, entries))
            or any(int(item["endBit"]) > body_end_value for item in sort_spans)
            or (sort_spans and sort_spans[0]["startBit"] < count_end_value)
        ):
            raise SystemExit("SORTENTSTABLE field spans do not match decoded ordered handles")
        handle_stream_start_value = int(handle_stream_start)
        handle_stream_end_value = int(handle_stream_end)
        owner_start_value, owner_end_value = int(owner_start), int(owner_end)
        if (
            handle_stream_start_value < 0
            or handle_stream_end_value <= handle_stream_start_value
            or owner_start_value < handle_stream_start_value
            or owner_end_value <= owner_start_value
            or owner_end_value > handle_stream_end_value
            or int(owner_target) != int(profile["blockOwnerHandle"])
            or (entity_spans and entity_spans[0]["startBit"] < owner_end_value)
            or any(int(item["endBit"]) > handle_stream_end_value
                   for item in entity_spans)
        ):
            raise SystemExit("SORTENTSTABLE handle-stream spans violate field order/identity")
        fields_by_handle[handle_value] = {
            "tableHandle": handle_value,
            "version": expected_version,
            "versionEnum": int(version_code),
            "coordinateFrame": coordinate_frame,
            "bodyStartBit": body_start_value,
            "bodyEndBit": body_end_value,
            "entryCount": count_value,
            "entryCountSpan": [count_start_value, count_end_value],
            "sortHandleFields": sort_spans,
            "handleStreamStartBit": handle_stream_start_value,
            "handleStreamEndBit": handle_stream_end_value,
            "blockOwnerField": {
                "startBit": owner_start_value,
                "endBit": owner_end_value,
                "targetHandle": int(owner_target),
                "role": "soft-pointer (ODA handle code 4 enforced by parser)",
            },
            "entityHandleFields": entity_spans,
            "spanOrigin": "libdxfrw parser cursor observations; no independent peer bit-offset comparison",
        }
    if set(fields_by_handle) != set(profiles):
        raise SystemExit("SORTENTSTABLE field-span rows do not cover all parsed tables")
    return [fields_by_handle[handle] for handle in sorted(fields_by_handle)]


def self_test_sortents_source_spans() -> None:
    profile = {
        10: {
            "version": "AC1021", "blockOwnerHandle": 7,
            "declaredEntryCount": 1,
            "entries": [{"sortHandle": 20, "entityHandle": 9}],
        }
    }
    row = (
        "objectDecompressedBitstreamBits", "10", "15", "80", "96", "1",
        "80", "88", "0:88:96:20", "100", "140", "120", "128", "7",
        "0:128:136:9",
    )
    if len(verify_sortents_source_spans([row], profile, "AC1021")) != 1:
        raise SystemExit("generated SORTENTSTABLE parser span control did not match")
    bad_rows = (
        row[:6] + ("81",) + row[7:],  # count begins after the observed body start
        row[:8] + ("0:87:96:20",) + row[9:],  # sort field precedes count end
        row[:13] + ("8",) + row[14:],  # owner target differs
        row[:14] + ("0:127:136:9",),  # entity overlaps the owner field
        row[:9] + ("121",) + row[10:],  # owner starts before handle stream
        row[:4] + ("95",) + row[5:],  # sort field exceeds the object body
        row[:10] + ("127",) + row[11:],  # owner exceeds handle-stream end
    )
    for index, bad in enumerate(bad_rows):
        try:
            verify_sortents_source_spans([bad], profile, "AC1021")
        except SystemExit:
            continue
        raise SystemExit(
            "generated SORTENTSTABLE parser span mutation was admitted: "
            f"{index}"
        )


def verify_layer_peer_fields(
    source_fields: list[dict[str, object]], peer_layer: dict[str, object]
) -> list[dict[str, object]]:
    peer_field_names = {
        "xrefBlock": "xref", "plotStyle": "plotstyle",
        "material": "material", "lineType": "ltype",
    }
    matched_fields = []
    for source_field in source_fields:
        role = str(source_field["role"])
        peer_name = peer_field_names.get(role)
        if peer_name is None:
            continue  # LibreDWG does not expose ODA's unknown trailing H.
        wire = peer_layer.get(peer_name)
        target = peer_handle(wire)
        raw_role = int(wire[0]) if isinstance(wire, list) and wire else -1
        if (
            target != int(source_field["targetHandle"])
            or raw_role != int(source_field["code"])
        ):
            raise SystemExit(
                f"LibreDWG LAYER {role} identity/role differs from source-bound profile"
            )
        matched_fields.append({
            "role": role,
            "peerField": peer_name,
            "peerRoleCode": raw_role,
            "peerTargetHandle": target,
            "matchesSource": True,
        })
    return matched_fields


def self_test_layer_peer_fields() -> None:
    source_fields: list[dict[str, object]] = [
        {"role": "plotStyle", "code": 5, "targetHandle": 15},
        {"role": "material", "code": 5, "targetHandle": 70},
        {"role": "lineType", "code": 5, "targetHandle": 22},
    ]
    peer_layer: dict[str, object] = {
        "plotstyle": [5, 15], "material": [5, 70], "ltype": [5, 22],
    }
    if len(verify_layer_peer_fields(source_fields, peer_layer)) != 3:
        raise SystemExit("generated LAYER/LTYPE peer control did not match")

    wrong_role = [dict(field) for field in source_fields]
    wrong_role[-1]["code"] = 4
    wrong_target = [dict(field) for field in source_fields]
    wrong_target[-1]["targetHandle"] = 23
    for label, fields in (("role", wrong_role), ("target", wrong_target)):
        try:
            verify_layer_peer_fields(fields, peer_layer)
        except SystemExit:
            continue
        raise SystemExit(f"generated LAYER/LTYPE {label} mutation was admitted")


def _verify_layer_peer_fields_for_profile(
    source_fields: list[dict[str, object]], peer_layer: dict[str, object]
) -> list[dict[str, object]]:
    matched_fields = verify_layer_peer_fields(source_fields, peer_layer)
    if len(matched_fields) != 4:
        raise SystemExit("LibreDWG did not independently expose four LAYER fields")
    return matched_fields


def verify_mlinestyle_fields(
    source: dict[str, object],
    source_elements: list[dict[str, object]],
    peer: dict[str, object],
) -> dict[str, object]:
    try:
        name = bytes.fromhex(str(source["nameHex"])).decode("utf-8")
        description = bytes.fromhex(str(source["descriptionHex"])).decode("utf-8")
    except (ValueError, UnicodeDecodeError) as error:
        raise SystemExit("MLINESTYLE source text hex is invalid") from error
    peer_lines = peer.get("lines")
    if not isinstance(peer_lines, list):
        raise SystemExit("LibreDWG MLINESTYLE has no line-element list")
    expected_lines: list[dict[str, object]] = []
    for item in peer_lines:
        if not isinstance(item, dict):
            raise SystemExit("LibreDWG MLINESTYLE line element is malformed")
        color = item.get("color")
        if not isinstance(color, dict) or color.get("rgb") != "c0000000":
            raise SystemExit("MLINESTYLE line color is outside the BYLAYER profile")
        expected_lines.append(item)
    fill_color = peer.get("fill_color")
    if not isinstance(fill_color, dict) or fill_color.get("rgb") != "c0000000":
        raise SystemExit("MLINESTYLE fill color is outside the BYLAYER profile")

    numeric_checks = (
        ("name", name, peer.get("name")),
        ("description", description, peer.get("description")),
        ("flags", int(source["flags"]), peer.get("flag")),
        ("fillColor", int(source["fillColor"]), 256),
        ("fillColor24", int(source["fillColor24"]), -1),
        ("elementCount", len(source_elements), len(expected_lines)),
    )
    for field, actual, expected in numeric_checks:
        if actual != expected:
            raise SystemExit(f"physical MLINESTYLE {field} differs from LibreDWG profile")
    angle_matches = []
    for field, actual_key, peer_key in (
        ("startAngle", "startAngleDegrees", "start_angle"),
        ("endAngle", "endAngleDegrees", "end_angle"),
    ):
        radians = peer.get(peer_key)
        if not isinstance(radians, (int, float)) or not math.isfinite(float(radians)):
            raise SystemExit(f"LibreDWG MLINESTYLE {peer_key} is not finite")
        degrees = math.degrees(float(radians))
        actual = float(source[actual_key])
        if not math.isclose(actual, degrees, rel_tol=0.0, abs_tol=1e-12):
            raise SystemExit(
                f"physical MLINESTYLE {field} degrees differ from LibreDWG radians"
            )
        angle_matches.append({
            "field": field,
            "libdxfrwDegrees": actual,
            "libreDwgRadians": float(radians),
            "degreesFromPeer": degrees,
            "matches": True,
        })

    line_matches = []
    for index, (actual, expected) in enumerate(zip(source_elements, expected_lines)):
        if int(actual["color"]) != 256 or int(actual["color24"]) != -1:
            raise SystemExit(
                f"physical MLINESTYLE line {index} color differs from BYLAYER profile"
            )
        offset = float(expected.get("offset"))
        actual_offset = float(actual["offset"])
        line_type_index = int(expected.get("lt.index"))
        actual_line_type_index = int(actual["linetypeIndex"])
        if (not math.isclose(actual_offset, offset, rel_tol=0.0, abs_tol=1e-12)
                or actual_line_type_index != line_type_index):
            raise SystemExit(
                f"physical MLINESTYLE line {index} offset or linetype differs from LibreDWG"
            )
        line_matches.append({
            "index": index,
            "libdxfrwOffset": actual_offset,
            "libreDwgOffset": offset,
            "libdxfrwColor": 256,
            "libreDwgColor": "c0000000 (BYLAYER)",
            "libdxfrwLinetypeIndex": actual_line_type_index,
            "libreDwgLinetypeIndex": line_type_index,
            "matches": True,
        })
    if len(source_elements) != len(expected_lines):
        raise SystemExit("physical MLINESTYLE element count differs from LibreDWG")
    return {
        "handle": int(source["handle"]),
        "name": name,
        "description": description,
        "flags": int(source["flags"]),
        "fillColorMapping": "libdxfrw 256/-1 matches LibreDWG c0000000 BYLAYER",
        "angleMatches": angle_matches,
        "lineMatches": line_matches,
    }


def self_test_mlinestyle_fields() -> None:
    source = {
        "handle": 24, "nameHex": "5374616e64617264", "descriptionHex": "",
        "flags": 0, "fillColor": 256, "fillColor24": -1,
        "startAngleDegrees": 90.0, "endAngleDegrees": 90.0,
    }
    elements = [
        {"offset": 0.5, "color": 256, "color24": -1, "linetypeIndex": 32767},
        {"offset": -0.5, "color": 256, "color24": -1, "linetypeIndex": 32767},
    ]
    peer = {
        "name": "Standard", "description": "", "flag": 0,
        "fill_color": {"rgb": "c0000000"},
        "start_angle": math.pi / 2.0, "end_angle": math.pi / 2.0,
        "lines": [
            {"offset": 0.5, "color": {"rgb": "c0000000"}, "lt.index": 32767},
            {"offset": -0.5, "color": {"rgb": "c0000000"}, "lt.index": 32767},
        ],
    }
    if len(verify_mlinestyle_fields(source, elements, peer)["lineMatches"]) != 2:
        raise SystemExit("generated MLINESTYLE semantic control did not match")
    wrong_angle = dict(source, startAngleDegrees=89.0)
    try:
        verify_mlinestyle_fields(wrong_angle, elements, peer)
    except SystemExit:
        pass
    else:
        raise SystemExit("generated MLINESTYLE angle mutation was admitted")
    wrong_line = [dict(elements[0], linetypeIndex=0), elements[1]]
    try:
        verify_mlinestyle_fields(source, wrong_line, peer)
    except SystemExit:
        pass
    else:
        raise SystemExit("generated MLINESTYLE linetype mutation was admitted")
    wrong_offset = [dict(elements[0], offset=0.75), elements[1]]
    try:
        verify_mlinestyle_fields(source, wrong_offset, peer)
    except SystemExit:
        pass
    else:
        raise SystemExit("generated MLINESTYLE offset mutation was admitted")


def verify_peer_profiles(
    peer_json_path: Path,
    peer_schema_path: Path,
    source_hash: str,
    frames: list[dict[str, object]],
    layer_names: dict[int, str],
    layer_handle_fields: dict[int, list[dict[str, object]]],
    dimstyle_names: dict[int, str],
    dimstyle_handle_fields: dict[int, list[dict[str, object]]],
    vport_handle_fields: dict[int, list[dict[str, object]]],
    mlinestyle_fields: dict[int, dict[str, object]],
    mlinestyle_elements: dict[int, list[dict[str, object]]],
    sortents_profiles: dict[int, dict[str, object]],
) -> dict[str, object]:
    peer_json = json.loads(peer_json_path.read_text())
    peer_objects = peer_json.get("OBJECTS")
    if not isinstance(peer_objects, list):
        raise SystemExit("LibreDWG minJSON has no OBJECTS array")
    peer_by_handle: dict[int, dict[str, object]] = {}
    for obj in peer_objects:
        if not isinstance(obj, dict):
            continue
        handle = peer_handle(obj.get("handle"))
        if handle is None:
            continue
        if handle in peer_by_handle:
            raise SystemExit("LibreDWG minJSON repeats an object handle")
        peer_by_handle[handle] = obj

    matched_sortents = []
    sortents_frames = {
        int(frame["handle"]): frame for frame in frames
        if frame["recordName"] == "SORTENTSTABLE"
        or frame["className"] == "AcDbSortentsTable"
    }
    if sortents_profiles and set(sortents_profiles) != set(sortents_frames):
        raise SystemExit(
            "source SORTENTSTABLE membership rows do not cover every captured table"
        )
    for handle, profile in sorted(sortents_profiles.items()):
        obj = peer_by_handle.get(handle)
        peer_block_owner = peer_handle(obj.get("block_owner")) if obj else None
        peer_block_owner_ref = obj.get("block_owner") if obj else None
        peer_entities = obj.get("ents") if obj else None
        peer_sorts = obj.get("sort_ents") if obj else None
        if (
            peer_entities is None and peer_sorts is None
            and int(profile["declaredEntryCount"]) == 0
        ):
            peer_entities = []
            peer_sorts = []
        if (
            obj is None or obj.get("object") != "SORTENTSTABLE"
            or peer_block_owner != int(profile["blockOwnerHandle"])
            or not isinstance(peer_entities, list)
            or not isinstance(peer_sorts, list)
            or len(peer_entities) != len(peer_sorts)
            or len(peer_entities) != len(profile["entries"])
            or not isinstance(peer_block_owner_ref, list)
            or not peer_block_owner_ref or peer_block_owner_ref[0] != 4
        ):
            raise SystemExit(
                "LibreDWG SORTENTSTABLE identity/owner/count mismatch for "
                f"{handle:#x}"
            )
        pairs = verify_sortents_entry_pairs(
            profile["entries"], peer_entities, peer_sorts
        )
        matched_sortents.append({
            "handle": handle,
            "blockOwnerHandle": peer_block_owner,
            "entryCount": len(pairs),
            "entries": pairs,
            "matchesLibdxfrwAndLibreDWG": True,
            "roleRule": "ODA-20.4.93",
        })

    record_to_peer_object = {
        "VPORT": "VPORT", "DIMSTYLE": "DIMSTYLE",
        "MLINESTYLE": "MLINESTYLE", "ACDBPLACEHOLDER": "PLACEHOLDER",
        "VISUALSTYLE": "VISUALSTYLE", "MATERIAL": "MATERIAL",
        "TABLESTYLE": "TABLESTYLE", "MLEADERSTYLE": "MLEADERSTYLE",
        "SORTENTSTABLE": "SORTENTSTABLE", "SUN": "SUN",
    }
    class_to_peer_object = {
        "AcDbVisualStyle": "VISUALSTYLE",
        "AcDbMaterial": "MATERIAL",
        "AcDbTableStyle": "TABLESTYLE",
        "AcDbMLeaderStyle": "MLEADERSTYLE",
        "AcDbSortentsTable": "SORTENTSTABLE",
        "AcDbSun": "SUN",
    }
    fixed_type_to_peer_object = {
        65: "VPORT", 69: "DIMSTYLE", 73: "MLINESTYLE", 80: "PLACEHOLDER",
    }
    rows = []
    for frame in frames:
        object_type = int(frame["encodedType"])
        expected_object = (
            record_to_peer_object.get(str(frame["recordName"] or ""))
            or class_to_peer_object.get(str(frame["className"] or ""))
            or fixed_type_to_peer_object.get(object_type)
        )
        if expected_object is None:
            continue
        obj = peer_by_handle.get(int(frame["handle"]))
        expected_dxf = "ACDBPLACEHOLDER" if expected_object == "PLACEHOLDER" else None
        reactors = obj.get("reactors", []) if obj else []
        peer_reactors = [
            handle for handle in (peer_handle(item) for item in reactors)
            if handle is not None
        ]
        peer_owner = peer_handle(obj.get("ownerhandle")) if obj else None
        if (
            obj is None
            or obj.get("object") != expected_object
            or obj.get("dxfname") != expected_dxf
            or int(obj.get("size", -1)) != int(frame["objectSize"])
            or peer_owner != int(frame["ownerHandle"])
            or peer_reactors != frame["reactorHandles"]
        ):
            raise SystemExit(
                f"LibreDWG minJSON identity/common-link mismatch for "
                f"{expected_object} handle {frame['handle']}"
            )
        rows.append({
            "handle": int(frame["handle"]),
            "encodedType": object_type,
            "object": expected_object,
            "dxfName": expected_dxf,
            "size": int(obj["size"]),
            "ownerHandle": peer_owner,
            "reactorHandles": peer_reactors,
            "matchesLibdxfrwFrame": True,
        })

        if expected_object == "MLEADERSTYLE":
            expected_fields = {
                "line_type": [5, 1, 20, 20],
                "arrow_head": [5, 0, 0, 0],
                "text_style": [5, 1, 17, 17],
                "block": [5, 0, 0, 0],
            }
            if any(obj.get(field) != value
                   for field, value in expected_fields.items()):
                raise SystemExit(
                    "LibreDWG minJSON MLEADERSTYLE body handle roles/targets "
                    "differ from the source-bound profile"
                )

    schema_text = peer_schema_path.read_text(errors="replace")
    schema_blocks: dict[str, str] = {}
    sortents_start = schema_text.find("DWG_OBJECT (SORTENTSTABLE)")
    sortents_end = schema_text.find("DWG_OBJECT_END", sortents_start)
    if sortents_start < 0 or sortents_end < 0:
        raise SystemExit("LibreDWG dwg2.spec lacks SORTENTSTABLE schema")
    sortents_block = schema_text[sortents_start:sortents_end]
    object_stream = sortents_block.find("START_OBJECT_HANDLE_STREAM")
    sort_stream = sortents_block.find("HANDLE_VECTOR (sort_ents")
    owner_stream = sortents_block.find("FIELD_HANDLE (block_owner, 4, 0)")
    entity_stream = sortents_block.find("HANDLE_VECTOR_N (ents")
    if not (
        "FIELD_BL (num_ents, 0);" in sortents_block
        and 0 <= sort_stream < object_stream < owner_stream < entity_stream
        and "HANDLE_VECTOR (sort_ents, num_ents, 0, 0);" in sortents_block
        and (
            "HANDLE_VECTOR_N (ents, FIELD_VALUE (num_ents), 4, 0);"
            in sortents_block
        )
    ):
        raise SystemExit("LibreDWG SORTENTSTABLE body/handle-stream roles changed")
    schema_blocks["SORTENTSTABLE"] = hashlib.sha256(
        sortents_block.encode()
    ).hexdigest()
    for name in ("PLACEHOLDER", "VISUALSTYLE", "SUN"):
        start = schema_text.find(f"DWG_OBJECT ({name})")
        end = schema_text.find("DWG_OBJECT_END", start)
        if start < 0 or end < 0:
            raise SystemExit(f"LibreDWG dwg2.spec lacks {name} object block")
        block = schema_text[start:end]
        body = block.split("START_OBJECT_HANDLE_STREAM", 1)[0]
        if "START_OBJECT_HANDLE_STREAM" not in block or re.search(
            r"\b(?:FIELD_H|HANDLE_VECTOR)\b", body
        ):
            raise SystemExit(
                f"LibreDWG {name} body schema has handle fields before the common handle stream"
            )
        if name == "PLACEHOLDER" and "no own data members" not in body:
            raise SystemExit("LibreDWG PLACEHOLDER schema no longer declares an empty body")
        schema_blocks[name] = hashlib.sha256(block.encode()).hexdigest()

    material_macro_start = schema_text.find("#define MAT_COLOR")
    material_start = schema_text.find("DWG_OBJECT (MATERIAL)", material_macro_start)
    material_end = schema_text.find("DWG_OBJECT_END", material_start)
    if min(material_macro_start, material_start, material_end) < 0:
        raise SystemExit("LibreDWG dwg2.spec lacks MATERIAL body macros/object")
    material_block = schema_text[material_macro_start:material_end]
    material_body = material_block.split("START_OBJECT_HANDLE_STREAM", 1)[0]
    if "START_OBJECT_HANDLE_STREAM" not in material_block or re.search(
        r"\b(?:FIELD_H|HANDLE_VECTOR)\b", material_body
    ):
        raise SystemExit(
            "LibreDWG MATERIAL color/map schema has body-handle fields before common handles"
        )
    schema_blocks["MATERIAL"] = hashlib.sha256(
        material_block.encode()
    ).hexdigest()

    tablestyle_start = schema_text.find("DWG_OBJECT (TABLESTYLE)")
    tablestyle_end = schema_text.find("DWG_OBJECT_END", tablestyle_start)
    if tablestyle_start < 0 or tablestyle_end < 0:
        raise SystemExit("LibreDWG dwg2.spec lacks TABLESTYLE object block")
    tablestyle_block = schema_text[tablestyle_start:tablestyle_end]
    if (
        "FIELD_HANDLE (cellstyle, DWG_HDL_HARDOWN, 0);" not in tablestyle_block
        or "CellStyle_fields (sty.cellstyle);" not in tablestyle_block
        or "START_OBJECT_HANDLE_STREAM;" not in tablestyle_block
        or "FIELD_HANDLE (fmt.text_style, 3, 340);" not in schema_text
        or "SUB_FIELD_HANDLE (sty.borders[rcount2],ltype, 3, 340);"
           not in schema_text
    ):
        raise SystemExit("LibreDWG TABLESTYLE schema handle roles changed")
    schema_blocks["TABLESTYLE"] = hashlib.sha256(
        tablestyle_block.encode()
    ).hexdigest()

    layer_schema_path = peer_schema_path.with_name("dwg.spec")
    if not layer_schema_path.is_file():
        raise SystemExit("LibreDWG peer directory lacks dwg.spec for LAYER profile")
    layer_schema_text = layer_schema_path.read_text(errors="replace")
    layer_start = layer_schema_text.find("DWG_TABLE (LAYER)")
    layer_end = layer_schema_text.find("DWG_OBJECT_END", layer_start)
    if layer_start < 0 or layer_end < 0:
        raise SystemExit("LibreDWG dwg.spec lacks the LAYER table profile")
    layer_block = layer_schema_text[layer_start:layer_end]
    expected_layer_fields = (
        "FIELD_HANDLE (plotstyle, 5, 0);",
        "FIELD_HANDLE (material, 5, 0);",
        "FIELD_HANDLE (ltype, 5, 6);",
    )
    if any(field not in layer_block for field in expected_layer_fields):
        raise SystemExit("LibreDWG LAYER handle-role fields changed")
    schema_blocks["LAYER"] = hashlib.sha256(layer_block.encode()).hexdigest()

    dimstyle_start = layer_schema_text.find("DWG_TABLE (DIMSTYLE)")
    dimstyle_end = layer_schema_text.find("DWG_OBJECT_END", dimstyle_start)
    if dimstyle_start < 0 or dimstyle_end < 0:
        raise SystemExit("LibreDWG dwg.spec lacks the DIMSTYLE table profile")
    dimstyle_block = layer_schema_text[dimstyle_start:dimstyle_end]
    expected_dimstyle_fields = (
        "FIELD_HANDLE (DIMTXSTY, 5, 340);",
        "FIELD_HANDLE (DIMLDRBLK, 5, 341);",
        "FIELD_HANDLE (DIMBLK, 5, 342);",
        "FIELD_HANDLE (DIMBLK1, 5, 343);",
        "FIELD_HANDLE (DIMBLK2, 5, 344);",
        "FIELD_HANDLE (DIMLTYPE, 5, 345);",
        "FIELD_HANDLE (DIMLTEX1, 5, 346);",
        "FIELD_HANDLE (DIMLTEX2, 5, 347);",
    )
    if any(field not in dimstyle_block for field in expected_dimstyle_fields):
        raise SystemExit("LibreDWG DIMSTYLE handle-role fields changed")
    schema_blocks["DIMSTYLE"] = hashlib.sha256(
        dimstyle_block.encode()
    ).hexdigest()

    matched_layer = None
    if source_hash == COVER_AC1027_SHA256:
        peer_layers = [
            obj for obj in peer_objects
            if isinstance(obj, dict) and obj.get("object") == "LAYER"
            and peer_handle(obj.get("handle")) == 0x10
        ]
        source_fields = layer_handle_fields.get(0x10, [])
        if len(peer_layers) != 1 or layer_names.get(0x10) != "0":
            raise SystemExit("LibreDWG minJSON lacks the hash-bound Cover LAYER 0")
        peer_layer = peer_layers[0]
        matched_fields = _verify_layer_peer_fields_for_profile(
            source_fields, peer_layer
        )
        matched_layer = {
            "handle": 0x10,
            "name": "0",
            "sourceBoundFields": source_fields,
            "matchedPeerFields": matched_fields,
            "unmatchedOdaField": "unknown trailing hard-pointer field",
        }

    matched_dimstyle = None
    if source_hash == COVER_AC1027_SHA256:
        peer_dimstyles = [
            obj for obj in peer_objects
            if isinstance(obj, dict) and obj.get("object") == "DIMSTYLE"
            and peer_handle(obj.get("handle")) == 0x27
        ]
        source_fields = dimstyle_handle_fields.get(0x27, [])
        if len(peer_dimstyles) != 1 or dimstyle_names.get(0x27) != "ISO-25":
            raise SystemExit(
                "LibreDWG minJSON lacks the hash-bound Cover DIMSTYLE ISO-25"
            )
        peer_dimstyle = peer_dimstyles[0]
        peer_field_names = {
            "xrefBlock": "xref", "dimtxsty": "DIMTXSTY",
            "leaderBlock": "DIMLDRBLK", "dimblk": "DIMBLK",
            "dimblk1": "DIMBLK1", "dimblk2": "DIMBLK2",
            "dimltype": "DIMLTYPE", "dimltex1": "DIMLTEX1",
            "dimltex2": "DIMLTEX2",
        }
        expected_roles = ("xrefBlock", "dimtxsty", "leaderBlock", "dimblk",
                          "dimblk1", "dimblk2", "dimltype", "dimltex1",
                          "dimltex2")
        matched_fields = []
        for source_field in source_fields:
            role = str(source_field["role"])
            peer_name = peer_field_names[role]
            wire = peer_dimstyle.get(peer_name)
            target = peer_handle(wire)
            raw_role = int(wire[0]) if isinstance(wire, list) and wire else -1
            if (
                target != int(source_field["targetHandle"])
                or raw_role != int(source_field["code"])
            ):
                raise SystemExit(
                    f"LibreDWG DIMSTYLE {role} role/target differs from source profile"
                )
            matched_fields.append({
                "role": role,
                "peerField": peer_name,
                "peerRoleCode": raw_role,
                "peerTargetHandle": target,
                "matchesSource": True,
            })
        if tuple(str(field["role"]) for field in source_fields) != expected_roles:
            raise SystemExit("Cover DIMSTYLE body handle field order/profile changed")
        if len(matched_fields) != len(expected_roles):
            raise SystemExit("LibreDWG did not independently expose all DIMSTYLE fields")
        matched_dimstyle = {
            "handle": 0x27,
            "name": "ISO-25",
            "sourceBoundFields": source_fields,
            "matchedPeerFields": matched_fields,
        }

    matched_vport = None
    if source_hash == COVER_AC1027_SHA256:
        peer_vports = [
            obj for obj in peer_objects
            if isinstance(obj, dict) and obj.get("object") == "VPORT"
            and peer_handle(obj.get("handle")) == 0x29
        ]
        source_fields = vport_handle_fields.get(0x29, [])
        if len(peer_vports) != 1:
            raise SystemExit("LibreDWG minJSON lacks the hash-bound Cover VPORT 0x29")
        peer_vport = peer_vports[0]
        peer_field_names = {
            "control": "ownerhandle", "xrefBlock": "xref",
            "background": "background", "visualStyle": "visualstyle",
            "sun": "sun", "namedUcs": "named_ucs", "baseUcs": "base_ucs",
        }
        expected_roles = (
            "control", "xrefBlock", "background", "visualStyle", "sun",
            "namedUcs", "baseUcs",
        )
        matched_fields = []
        for source_field in source_fields:
            role = str(source_field["role"])
            if role.startswith("reactor[") or role == "xdict":
                continue
            peer_name = peer_field_names.get(role)
            if peer_name is None:
                raise SystemExit(f"unmapped VPORT source handle role {role}")
            wire = peer_vport.get(peer_name)
            target = peer_handle(wire)
            raw_role = int(wire[0]) if isinstance(wire, list) and wire else -1
            if (
                target != int(source_field["targetHandle"])
                or raw_role != int(source_field["code"])
            ):
                raise SystemExit(
                    f"LibreDWG VPORT {role} role/target differs from source profile"
                )
            matched_fields.append({
                "role": role,
                "peerField": peer_name,
                "peerRoleCode": raw_role,
                "peerTargetHandle": target,
                "matchesSource": True,
            })
        if tuple(str(field["role"]) for field in matched_fields) != expected_roles:
            raise SystemExit("Cover VPORT body handle field order/profile changed")
        matched_vport = {
            "handle": 0x29,
            "name": "*Active",
            "sourceBoundFields": source_fields,
            "matchedPeerFields": matched_fields,
            "independentPeerLimit": "LibreDWG exposes resolved fields and raw role codes, not physical bit spans",
        }

    source_mlinestyle_handles = {
        int(frame["handle"]) for frame in frames
        if int(frame["encodedType"]) == 73
    }
    if (set(mlinestyle_fields) != source_mlinestyle_handles
            or set(mlinestyle_elements) - source_mlinestyle_handles):
        raise SystemExit(
            "MLINESTYLE typed rows do not cover exactly the retained type-73 frames"
        )
    matched_mlinestyles = []
    for handle in sorted(source_mlinestyle_handles):
        peer_mlinestyle = peer_by_handle.get(handle)
        if peer_mlinestyle is None or peer_mlinestyle.get("object") != "MLINESTYLE":
            raise SystemExit(
                f"LibreDWG minJSON lacks source MLINESTYLE handle {handle:#x}"
            )
        matched_fields = verify_mlinestyle_fields(
            mlinestyle_fields[handle], mlinestyle_elements.get(handle, []),
            peer_mlinestyle,
        )
        matched_mlinestyles.append({
            **matched_fields,
            "sourceFrameHandle": handle,
            "sourceFrameSize": next(
                int(frame["objectSize"]) for frame in frames
                if int(frame["handle"]) == handle
            ),
            "peerSize": int(peer_mlinestyle["size"]),
            "independentPeerLimit": (
                "LibreDWG minJSON independently exposes semantic fields but "
                "not their physical source bit spans or arbitrary trailing bytes"
            ),
        })
    matched_mlinestyle = (
        matched_mlinestyles[0]
        if source_hash == COVER_AC1027_SHA256 and len(matched_mlinestyles) == 1
        else None
    )

    mleaderstyle_start = schema_text.find("DWG_OBJECT (MLEADERSTYLE)")
    mleaderstyle_end = schema_text.find("DWG_OBJECT_END", mleaderstyle_start)
    if mleaderstyle_start < 0 or mleaderstyle_end < 0:
        raise SystemExit("LibreDWG dwg2.spec lacks MLEADERSTYLE object block")
    mleaderstyle_block = schema_text[mleaderstyle_start:mleaderstyle_end]
    expected_mleader_handles = (
        "FIELD_HANDLE (line_type, 5, 340);",
        "FIELD_HANDLE (arrow_head, 5, 341);",
        "FIELD_HANDLE (text_style, 5, 342);",
        "FIELD_HANDLE (block, 5, 343);",
    )
    if (
        "START_OBJECT_HANDLE_STREAM;" not in mleaderstyle_block
        or mleaderstyle_block.count("FIELD_HANDLE") != 4
        or any(field not in mleaderstyle_block for field in expected_mleader_handles)
    ):
        raise SystemExit("LibreDWG MLEADERSTYLE schema handle roles changed")
    schema_blocks["MLEADERSTYLE"] = hashlib.sha256(
        mleaderstyle_block.encode()
    ).hexdigest()

    source_types = Counter(int(frame["encodedType"]) for frame in frames)
    peer_types = Counter(int(obj.get("type", -1)) for obj in peer_by_handle.values())
    if source_hash == COVER_AC1027_SHA256 and any(
        peer_types[object_type] != source_types[object_type]
        for object_type in (73, 80, 501, 502, 504, 506)
    ):
        raise SystemExit("LibreDWG minJSON profile frame counts differ from Cover source inventory")
    return {
        "createdBy": peer_json.get("created_by"),
        "objectCountsByEncodedType": {
            str(object_type): peer_types[object_type]
            for object_type in (69, 73, 80, 501, 502, 504, 506)
        },
        "matchedFrames": rows,
        "schemaPath": str(peer_schema_path),
        "schemaSha256": sha256(peer_schema_path),
        "layerSchemaPath": str(layer_schema_path),
        "layerSchemaSha256": sha256(layer_schema_path),
        "schemaBlockSha256": schema_blocks,
        "schemaBodyHandleFieldsBeforeCommonStream": False,
        "sourceSha256": source_hash,
        "matchedLayer": matched_layer,
        "matchedDimstyle": matched_dimstyle,
        "matchedVport": matched_vport,
        "matchedMLineStyle": matched_mlinestyle,
        "matchedMLineStyles": matched_mlinestyles,
        "matchedSortEntsTables": matched_sortents,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--executable", type=Path)
    parser.add_argument("--source", type=Path)
    parser.add_argument("--peer-json", type=Path)
    parser.add_argument("--peer-schema", type=Path)
    parser.add_argument("--timeout", type=float, default=30.0)
    args = parser.parse_args()
    if args.self_test:
        self_test_layer_peer_fields()
        self_test_mlinestyle_fields()
        self_test_sortents_peer_fields()
        self_test_sortents_source_spans()
        print(
            "generated LAYER/LTYPE, MLINESTYLE, and SORTENTSTABLE mutation "
            "controls passed"
        )
        return 0
    if args.executable is None or args.source is None:
        parser.error("--executable and --source are required unless --self-test is used")
    executable = args.executable.resolve(strict=True)
    source = args.source.resolve(strict=True)
    if not executable.is_file() or not source.is_file():
        parser.error("executable and source must be regular files")
    if (args.peer_json is None) != (args.peer_schema is None):
        parser.error("--peer-json and --peer-schema must be supplied together")

    source_before = sha256(source)
    result = run_bounded(
        [str(executable), "--opaque-dwg-source-inventory", str(source)],
        timeout=args.timeout,
        output_limit=16 * 1024 * 1024,
        cwd=REPO,
    )
    source_after = sha256(source)
    combined = (result.stdout + b"\n" + result.stderr).decode(
        "utf-8", errors="replace"
    )
    summary = SUMMARY.search(combined)
    if result.returncode != 0 or source_before != source_after or summary is None:
        sys.stderr.write(combined[-12000:])
        raise SystemExit("source inventory failed, changed its input, or lacked its summary")

    frames = []
    for match in FRAME.finditer(combined):
        (kind, handle, object_type, record, class_name, custom, owner,
         block_owner, xdict, reactors, evidence, offset, size, data_storage,
         raw_body_bytes) = match.groups()
        frames.append({
            "captureKind": kind,
            "handle": int(handle),
            "encodedType": int(object_type),
            "recordName": record or None,
            "className": class_name or None,
            "customClass": bool(int(custom)),
            "ownerHandle": int(owner),
            "blockOwnerHandle": int(block_owner),
            "extensionDictionaryHandle": int(xdict),
            "reactorHandles": [int(item) for item in reactors.split(",") if item],
            "commonLinkEvidence": int(evidence),
            "sourceOffset": int(offset),
            "objectSize": int(size),
            "hasDataStorage": bool(int(data_storage)),
            "rawBodyBytes": int(raw_body_bytes),
        })
    sortents_inventory = SORTENTS_INVENTORY.search(combined)
    sortents_profiles: dict[int, dict[str, object]] = {}
    expected_version = summary.group(2)
    for match in SORTENTS_SOURCE.finditer(combined):
        handle, sort_version, encoded_type, block_owner, complete, count = (
            match.groups()
        )
        handle_value = int(handle)
        if handle_value in sortents_profiles:
            raise SystemExit("source inventory repeats a SORTENTSTABLE membership")
        if sort_version != expected_version:
            raise SystemExit("SORTENTSTABLE profile version differs from its DWG source")
        sortents_profiles[handle_value] = {
            "version": sort_version,
            "encodedType": int(encoded_type),
            "blockOwnerHandle": int(block_owner),
            "complete": bool(int(complete)),
            "declaredEntryCount": int(count),
            "entries": [],
        }
    for match in SORTENTS_ENTRY.finditer(combined):
        table, ordinal, entity, sort, fallback = map(int, match.groups())
        profile = sortents_profiles.get(table)
        if profile is None:
            raise SystemExit("SORTENTSTABLE body row has no source table profile")
        profile["entries"].append({
            "ordinal": ordinal,
            "entityHandle": entity,
            "sortHandle": sort,
            "sortFallsBackToEntity": bool(fallback),
        })
    sortents_frames = {
        int(frame["handle"]) for frame in frames
        if frame["recordName"] == "SORTENTSTABLE"
        or frame["className"] == "AcDbSortentsTable"
    }
    if set(sortents_profiles) != sortents_frames:
        raise SystemExit(
            "SORTENTSTABLE source membership inventory does not cover all captured frames"
        )
    if sortents_inventory is not None:
        declared_tables, declared_entries = map(int, sortents_inventory.groups())
        observed_entries = sum(len(p["entries"]) for p in sortents_profiles.values())
        if (
            declared_tables != len(sortents_profiles)
            or declared_entries != observed_entries
            or any(
                not p["complete"]
                or len(p["entries"]) != p["declaredEntryCount"]
                for p in sortents_profiles.values()
            )
        ):
            raise SystemExit("SORTENTSTABLE source membership inventory is incomplete")
    elif sortents_profiles:
        raise SystemExit("SORTENTSTABLE source rows lack their inventory summary")
    sortents_source_spans = verify_sortents_source_spans(
        SORTENTS_SPAN.findall(combined), sortents_profiles, expected_version
    )
    if len({frame["handle"] for frame in frames}) != len(frames):
        raise SystemExit("source inventory contains duplicate captured frame handles")

    typed_frames = {
        int(handle): name for name, handle in BODY_FRAME.findall(combined)
    }
    modeler_history_profiles = {
        int(handle): {
            "layerName": layer_name,
            "layerMatches": int(layer_matches),
            "historyPresent": bool(int(history_present)),
            "historyCode": int(history_code),
            "historySize": int(history_size),
            "historyTarget": int(history_target),
            "historyStartBit": int(history_start),
            "historyBitSize": int(history_bits),
            "historyComplete": bool(int(history_complete)),
        }
        for (handle, layer_name, layer_matches, history_present,
             history_code, history_size, history_target, history_start,
             history_bits, history_complete)
        in MODELER_HISTORY_PROFILE.findall(combined)
    }
    no_body_handle_profiles = {
        int(handle): rule
        for _record_type, handle, rule in NO_BODY_HANDLE_PROFILE.findall(combined)
    }
    handle_body_profiles = {
        int(handle): {"rule": rule, "fieldCount": int(field_count),
                      "objectType": record_type}
        for record_type, handle, field_count, rule
        in HANDLE_BODY_PROFILE.findall(combined)
    }
    table_style_profiles = {
        int(handle): {
            "fieldCount": int(field_count),
            "ownerTarget": int(owner_target),
            "textStyleFieldCount": int(text_style_fields),
            "borderLineTypeFieldCount": int(line_type_fields),
            "rule": rule,
        }
        for handle, field_count, owner_target, text_style_fields,
            line_type_fields, rule in TABLESTYLE_PROFILE.findall(combined)
    }
    for handle, profile in table_style_profiles.items():
        handle_body_profiles[handle] = {
            "rule": profile["rule"],
            "fieldCount": profile["fieldCount"],
            "objectType": "TABLESTYLE",
        }
    mleader_style_profiles = {
        int(handle): {
            "fieldCount": int(field_count),
            "rule": rule,
            "dictionaryKey": dictionary_key,
            "dictionaryHandle": int(dictionary_handle),
        }
        for handle, field_count, rule, dictionary_key, dictionary_handle
        in MLEADERSTYLE_PROFILE.findall(combined)
    }
    for handle, profile in mleader_style_profiles.items():
        handle_body_profiles[handle] = {
            "rule": profile["rule"],
            "fieldCount": profile["fieldCount"],
            "objectType": "MLEADERSTYLE",
        }
    dictionary_memberships = {
        int(handle): {
            "objectType": record_type,
            "dictionaryHandle": int(dictionary),
            "dictionaryKey": key,
            "bodyReferenceRule": rule,
        }
        for record_type, handle, dictionary, key, rule
        in DICTIONARY_MEMBERSHIP.findall(combined)
    }
    dictionary_membership_edges = [
        {"sourceFrameHandle": handle, **membership, "targetHandle": handle}
        for handle, membership in sorted(dictionary_memberships.items())
    ]
    named_object_dictionary_entries = [
        {"key": key, "targetHandle": int(target)}
        for key, target in NOD_ENTRY.findall(combined)
    ]
    typed_references = []
    for owner, role, target in BODY_REFERENCE.findall(combined):
        typed_references.append({
            "ownerHandle": int(owner),
            "role": role or "unlabeled",
            "targetHandle": int(target),
        })
    layer_names = {
        int(handle): name for handle, name in LAYER_FRAME.findall(combined)
    }
    layer_handle_fields: dict[int, list[dict[str, object]]] = {}
    for owner, role, code, target, start_bit, bit_size, complete in (
        LAYER_HANDLE_FIELD.findall(combined)
    ):
        layer_handle_fields.setdefault(int(owner), []).append({
            "role": role,
            "code": int(code),
            "targetHandle": int(target),
            "startBit": int(start_bit),
            "bitSize": int(bit_size),
            "complete": bool(int(complete)),
        })
    for owner, fields in layer_handle_fields.items():
        starts = [int(field["startBit"]) for field in fields]
        if (
            owner not in layer_names
            or len({str(field["role"]) for field in fields}) != len(fields)
            or starts != sorted(starts)
            or any(
                not field["complete"] or int(field["bitSize"]) <= 0
                for field in fields
            )
        ):
            raise SystemExit(f"LAYER {owner:#x} handle-field spans are incomplete or unordered")
    dimstyle_names = {
        int(handle): name
        for handle, name in DIMSTYLE_FRAME.findall(combined)
    }
    dimstyle_handle_fields: dict[int, list[dict[str, object]]] = {}
    for owner, role, code, target, start_bit, bit_size, complete in (
        DIMSTYLE_HANDLE_FIELD.findall(combined)
    ):
        dimstyle_handle_fields.setdefault(int(owner), []).append({
            "role": role,
            "code": int(code),
            "targetHandle": int(target),
            "startBit": int(start_bit),
            "bitSize": int(bit_size),
            "complete": bool(int(complete)),
        })
    for owner, fields in dimstyle_handle_fields.items():
        starts = [int(field["startBit"]) for field in fields]
        if (
            owner not in dimstyle_names
            or len({str(field["role"]) for field in fields}) != len(fields)
            or starts != sorted(starts)
            or any(
                not field["complete"] or int(field["bitSize"]) <= 0
                for field in fields
            )
        ):
            raise SystemExit(
                f"DIMSTYLE {owner:#x} handle-field spans are incomplete or unordered"
            )
    vport_handle_fields: dict[int, list[dict[str, object]]] = {}
    for owner, role, code, target, start_bit, bit_size, complete in (
        VPORT_HANDLE_FIELD.findall(combined)
    ):
        vport_handle_fields.setdefault(int(owner), []).append({
            "role": role,
            "code": int(code),
            "targetHandle": int(target),
            "startBit": int(start_bit),
            "bitSize": int(bit_size),
            "complete": bool(int(complete)),
        })
    for owner, fields in vport_handle_fields.items():
        starts = [int(field["startBit"]) for field in fields]
        if (
            len({str(field["role"]) for field in fields}) != len(fields)
            or starts != sorted(starts)
            or any(
                not field["complete"] or int(field["bitSize"]) <= 0
                for field in fields
            )
        ):
            raise SystemExit(
                f"VPORT {owner:#x} handle-field spans are incomplete or unordered"
            )
    mlinestyle_fields: dict[int, dict[str, object]] = {}
    for match in MLINESTYLE_FIELDS.finditer(combined):
        (handle, name_hex, description_hex, flags, fill_color, fill_color24,
         start_angle, end_angle, element_count) = match.groups()
        owner = int(handle)
        if owner in mlinestyle_fields:
            raise SystemExit(f"MLINESTYLE {owner:#x} has duplicate typed field rows")
        mlinestyle_fields[owner] = {
            "handle": owner,
            "nameHex": name_hex,
            "descriptionHex": description_hex,
            "flags": int(flags),
            "fillColor": int(fill_color),
            "fillColor24": int(fill_color24),
            "startAngleDegrees": float(start_angle),
            "endAngleDegrees": float(end_angle),
            "elementCount": int(element_count),
        }
    mlinestyle_elements: dict[int, list[dict[str, object]]] = {}
    for match in MLINESTYLE_ELEMENT.finditer(combined):
        handle, index, offset, color, color24, line_type_index = match.groups()
        owner = int(handle)
        mlinestyle_elements.setdefault(owner, []).append({
            "index": int(index),
            "offset": float(offset),
            "color": int(color),
            "color24": int(color24),
            "linetypeIndex": int(line_type_index),
        })
    for owner, fields in mlinestyle_fields.items():
        elements = mlinestyle_elements.get(owner, [])
        indices = [int(item["index"]) for item in elements]
        if (len(elements) != int(fields["elementCount"])
                or indices != list(range(len(elements)))):
            raise SystemExit(
                f"MLINESTYLE {owner:#x} element rows are incomplete or unordered"
            )
    source_bound_modeler_history_handles: set[int] = set()
    source_bound_dimstyle_handles: set[int] = set()
    if source_before == COVER_AC1027_SHA256:
        expected_history = modeler_history_profiles.get(0x6F)
        layer_edges = [
            item for item in typed_references
            if item["ownerHandle"] == 0x6F and item["role"] == "layer"
        ]
        if (
            len(modeler_history_profiles) != 1
            or expected_history != {
                "layerName": "0",
                "layerMatches": 1,
                "historyPresent": True,
                "historyCode": 3,
                "historySize": 0,
                "historyTarget": 0,
                "historyStartBit": 209,
                "historyBitSize": 8,
                "historyComplete": True,
            }
            or layer_edges != [{
                "ownerHandle": 0x6F,
                "role": "layer",
                "targetHandle": 0x10,
            }]
        ):
            raise SystemExit(
                "hash-bound Cover 3DSOLID layer/History ID source profile changed"
            )
        layer_fields = layer_handle_fields.get(0x10, [])
        expected_layer_fields = [
            ("xrefBlock", 5, 0, 155, 8),
            ("plotStyle", 5, 15, 163, 16),
            ("material", 5, 70, 179, 16),
            ("lineType", 5, 22, 195, 16),
            ("unknown", 5, 0, 211, 8),
        ]
        observed_layer_fields = [
            (field["role"], field["code"], field["targetHandle"],
             field["startBit"], field["bitSize"])
            for field in layer_fields
        ]
        layer_reference_edges = [
            (item["role"], item["targetHandle"])
            for item in typed_references if item["ownerHandle"] == 0x10
        ]
        expected_layer_edges = [
            ("plotStyle", 15), ("material", 70), ("lineType", 22)
        ]
        if (
            layer_names.get(0x10) != "0"
            or observed_layer_fields != expected_layer_fields
            or layer_reference_edges != expected_layer_edges
        ):
            raise SystemExit(
                "hash-bound Cover LAYER 0 role/span/target source profile changed"
            )
        dimstyle_fields = dimstyle_handle_fields.get(0x27, [])
        expected_dimstyle_fields = [
            ("xrefBlock", 5, 0, 1290, 8),
            ("dimtxsty", 5, 17, 1298, 16),
            ("leaderBlock", 5, 0, 1314, 8),
            ("dimblk", 5, 0, 1322, 8),
            ("dimblk1", 5, 0, 1330, 8),
            ("dimblk2", 5, 0, 1338, 8),
            ("dimltype", 5, 0, 1346, 8),
            ("dimltex1", 5, 0, 1354, 8),
            ("dimltex2", 5, 0, 1362, 8),
        ]
        observed_dimstyle_fields = [
            (field["role"], field["code"], field["targetHandle"],
             field["startBit"], field["bitSize"])
            for field in dimstyle_fields
        ]
        dimstyle_reference_edges = [
            (item["role"], item["targetHandle"])
            for item in typed_references if item["ownerHandle"] == 0x27
        ]
        if (
            dimstyle_names.get(0x27) != "ISO-25"
            or observed_dimstyle_fields != expected_dimstyle_fields
            or dimstyle_reference_edges != [("textStyle", 17)]
        ):
            raise SystemExit(
                "hash-bound Cover DIMSTYLE ISO-25 role/span/target profile changed"
            )
        source_bound_modeler_history_handles.add(0x6F)
        source_bound_dimstyle_handles.add(0x27)
    reference_counts = Counter(item["ownerHandle"] for item in typed_references)
    retained_handles = {frame["handle"] for frame in frames}
    for frame in frames:
        frame["typedBodyReport"] = typed_frames.get(frame["handle"])
        frame["reportedTypedBodyReferenceCount"] = reference_counts[frame["handle"]]
        no_body_rule = no_body_handle_profiles.get(frame["handle"])
        frame["bodyReferenceDisposition"] = (
            "source-bound-modeler-history-profile"
            if frame["handle"] in source_bound_modeler_history_handles
            else "source-bound-dimstyle-handle-profile"
            if frame["handle"] in source_bound_dimstyle_handles
            else "peer-profile-no-body-handles"
            if no_body_rule is not None and no_body_rule.startswith("LIBREDWG-")
            else "spec-profile-no-body-handles"
            if no_body_rule is not None
            else "spec-profile-handle-references"
            if frame["handle"] in handle_body_profiles
            else "selected-typed-report-only"
            if frame["handle"] in typed_frames or reference_counts[frame["handle"]]
            else "not-exhaustively-decoded"
        )
        frame["bodyReferenceRule"] = no_body_handle_profiles.get(frame["handle"])
        if frame["handle"] in source_bound_modeler_history_handles:
            frame["bodyReferenceRule"] = "ODA-20.4.41"
            frame["modelerHistoryProfile"] = modeler_history_profiles[
                frame["handle"]
            ]
        if frame["handle"] in handle_body_profiles:
            handle_profile = handle_body_profiles[frame["handle"]]
            frame["bodyReferenceRule"] = handle_profile["rule"]
            frame["bodyHandleFieldCount"] = handle_profile["fieldCount"]
        frame["dictionaryMembership"] = dictionary_memberships.get(frame["handle"])
        frame["ownerTargetsRetainedObjectFrame"] = (
            frame["ownerHandle"] in retained_handles
            if frame["ownerHandle"] not in (0, 0xFFFFFFFF) else None
        )

    (schema, version, callbacks, captured, opaque_retained,
     standard_retained, acds_retained, utility_bound) = summary.groups()
    declared_frames = (
        int(opaque_retained) + int(standard_retained) + int(acds_retained)
    )
    if int(schema) != 2 or len(frames) != declared_frames or declared_frames != int(captured):
        raise SystemExit(
            f"captured identity rows mismatch: rows={len(frames)} "
            f"declared={declared_frames} callbacksCaptured={captured}"
        )
    if any(int(frame["rawBodyBytes"]) != int(frame["objectSize"])
           for frame in frames):
        raise SystemExit("source inventory has a frame without its full captured body")
    peer_evidence = None
    if args.peer_json is not None and args.peer_schema is not None:
        peer_evidence = verify_peer_profiles(
            args.peer_json.resolve(strict=True),
            args.peer_schema.resolve(strict=True),
            source_before,
            frames,
            layer_names,
            layer_handle_fields,
            dimstyle_names,
            dimstyle_handle_fields,
            vport_handle_fields,
            mlinestyle_fields,
            mlinestyle_elements,
            sortents_profiles,
        )
    peer_sortents_handles = {
        int(item["handle"])
        for item in (peer_evidence or {}).get("matchedSortEntsTables", [])
    }
    for frame in frames:
        if frame["handle"] in peer_sortents_handles:
            frame["bodyReferenceDisposition"] = (
                "peer-profile-exact-body-references"
            )
            frame["bodyReferenceRule"] = "ODA-20.4.93"
            frame["sortEntsEntryCount"] = len(
                sortents_profiles[frame["handle"]]["entries"]
            )
    if version == "AC1027":
        mleader_styles = [
            frame for frame in frames
            if frame["recordName"] == "MLEADERSTYLE"
            or frame["className"] == "AcDbMLeaderStyle"
        ]
        mleader_handle_fields = {
            int(owner): [] for owner, *_ in MLEADERSTYLE_HANDLE_FIELD.findall(combined)
        }
        for owner, role, code, target, start_bit, bit_size in (
            MLEADERSTYLE_HANDLE_FIELD.findall(combined)
        ):
            mleader_handle_fields[int(owner)].append({
                "role": role,
                "code": int(code),
                "target": int(target),
                "startBit": int(start_bit),
                "bitSize": int(bit_size),
            })
        for frame in mleader_styles:
            profile = mleader_style_profiles.get(int(frame["handle"]))
            fields = mleader_handle_fields.get(int(frame["handle"]), [])
            dictionary_edge = any(
                item["ownerHandle"] == frame["ownerHandle"]
                and item["role"] == "dictionary.entry[Standard]"
                and item["targetHandle"] == frame["handle"]
                for item in typed_references
            )
            nod_edge = any(
                entry["key"] == "ACAD_MLEADERSTYLE"
                and entry["targetHandle"] == frame["ownerHandle"]
                for entry in named_object_dictionary_entries
            )
            expected_fields = [
                ("lineType", 5, 20), ("arrowHead", 5, 0),
                ("textStyle", 5, 17), ("block", 5, 0),
            ]
            if (
                frame["className"] != "AcDbMLeaderStyle"
                or frame["recordName"] != "MLEADERSTYLE"
                or frame["typedBodyReport"] != "MLEADERSTYLE"
                or frame["bodyReferenceDisposition"]
                != "spec-profile-handle-references"
                or profile != {
                    "fieldCount": 4,
                    "rule": "ODA-20.4.87",
                    "dictionaryKey": "ACAD_MLEADERSTYLE",
                    "dictionaryHandle": frame["ownerHandle"],
                }
                or len(fields) != 4
                or [(item["role"], item["code"], item["target"])
                    for item in fields] != expected_fields
                or any(item["bitSize"] <= 0 for item in fields)
                or [item["startBit"] for item in fields]
                    != sorted(item["startBit"] for item in fields)
                or frame["handle"] != 108 or frame["ownerHandle"] != 107
                or frame["reactorHandles"] != [107]
                or not dictionary_edge or not nod_edge
            ):
                raise SystemExit(
                    "AC1027 MLEADERSTYLE lacks complete ODA body spans, peer "
                    "handle-role evidence, or rooted dictionary membership"
                )
        table_styles = [
            frame for frame in frames
            if frame["recordName"] == "TABLESTYLE"
            or frame["className"] == "AcDbTableStyle"
        ]
        for frame in table_styles:
            profile = table_style_profiles.get(int(frame["handle"]))
            table_style_dictionary = next((
                item for item in typed_references
                if item["ownerHandle"] == frame["ownerHandle"]
                and item["role"] == "dictionary.entry[Standard]"
                and item["targetHandle"] == frame["handle"]
            ), None)
            nod_edge = any(
                entry["key"] == "ACAD_TABLESTYLE"
                and entry["targetHandle"] == frame["ownerHandle"]
                for entry in named_object_dictionary_entries
            )
            if (
                frame["recordName"] != "TABLESTYLE"
                or frame["className"] != "AcDbTableStyle"
                or frame["typedBodyReport"] != "TABLESTYLE"
                or frame["bodyReferenceDisposition"]
                != "spec-profile-handle-references"
                or profile != {
                    "fieldCount": 29,
                    "ownerTarget": 0,
                    "textStyleFieldCount": 4,
                    "borderLineTypeFieldCount": 24,
                    "rule": "ODA-20.4.101.2+ODA-20.4.101.3+ODA-20.4.101.4",
                }
                or frame["handle"] != 106 or frame["ownerHandle"] != 105
                or frame["reactorHandles"] != [105]
                or not nod_edge or table_style_dictionary is None
            ):
                raise SystemExit(
                    "AC1027 TABLESTYLE lacks complete ODA handle-span, peer "
                    "identity, or rooted dictionary-role evidence"
                )
        fixed_mline_styles = [
            frame for frame in frames if frame["encodedType"] == 73
        ]
        for frame in fixed_mline_styles:
            membership = frame["dictionaryMembership"]
            if (
                frame["typedBodyReport"] != "MLINESTYLE"
                or frame["bodyReferenceDisposition"]
                != "spec-profile-no-body-handles"
                or frame["bodyReferenceRule"] != "ODA-20.4.73"
                or membership is None
                or membership["objectType"] != "MLINESTYLE"
                or membership["dictionaryHandle"] != frame["ownerHandle"]
                or not any(
                    edge["sourceFrameHandle"] == frame["handle"]
                    and edge["dictionaryHandle"] == frame["ownerHandle"]
                    and edge["targetHandle"] == frame["handle"]
                    for edge in dictionary_membership_edges
                )
            ):
                raise SystemExit(
                    "AC1027 type-73 frame lacks its typed identity, ODA body "
                    "profile, or exact ACAD_MLINESTYLE membership edge"
                )
        visual_styles = [
            frame for frame in frames
            if frame["recordName"] == "VISUALSTYLE"
            or frame["className"] == "AcDbVisualStyle"
        ]
        for frame in visual_styles:
            if (
                frame["className"] != "AcDbVisualStyle"
                or frame["typedBodyReport"] != "VISUALSTYLE"
                or frame["bodyReferenceDisposition"]
                != "peer-profile-no-body-handles"
                or frame["bodyReferenceRule"]
                != "LIBREDWG-dwg2.spec-VISUALSTYLE"
            ):
                raise SystemExit(
                    "AC1027 VISUALSTYLE frame lacks its typed identity "
                    "or peer-supported no-body-handle profile"
                )
        materials = [
            frame for frame in frames
            if frame["recordName"] == "MATERIAL"
            or frame["className"] == "AcDbMaterial"
        ]
        material_profiles = {
            int(handle): (int(semantic), int(raw_tail), key)
            for handle, semantic, raw_tail, key in MATERIAL_PROFILE.findall(
                combined
            )
        }
        material_fields = {
            int(handle): name
            for handle, name in MATERIAL_FIELDS.findall(combined)
        }
        for frame in materials:
            membership = frame["dictionaryMembership"]
            if (
                frame["className"] != "AcDbMaterial"
                or frame["typedBodyReport"] != "MATERIAL"
                or frame["bodyReferenceDisposition"]
                != "peer-profile-no-body-handles"
                or frame["bodyReferenceRule"] != "LIBREDWG-dwg2.spec-MATERIAL"
                or membership is None
                or membership["objectType"] != "MATERIAL"
                or membership["dictionaryHandle"] != frame["ownerHandle"]
                or material_profiles.get(int(frame["handle"]))
                != (1, 1, membership["dictionaryKey"])
                or material_fields.get(int(frame["handle"]))
                != membership["dictionaryKey"]
                or not any(
                    edge["sourceFrameHandle"] == frame["handle"]
                    and edge["dictionaryHandle"] == frame["ownerHandle"]
                    and edge["targetHandle"] == frame["handle"]
                    for edge in dictionary_membership_edges
                )
            ):
                raise SystemExit(
                    "AC1027 MATERIAL frame lacks its typed semantic profile, "
                    "peer body profile, or exact ACAD_MATERIAL membership"
                )
        suns = [frame for frame in frames if frame["encodedType"] == 506]
        sun_reports = {
            int(handle): rule
            for _object_type, handle, rule
            in NO_BODY_HANDLE_PROFILE.findall(combined)
            if rule == "LIBREDWG-dwg2.spec-SUN"
        }
        for frame in suns:
            sun_owner_refs = [
                edge for edge in typed_references
                if edge["ownerHandle"] == frame["ownerHandle"]
                and edge["role"] == "sun"
                and edge["targetHandle"] == frame["handle"]
            ]
            if (
                frame["className"] != "AcDbSun"
                or frame["typedBodyReport"] != "SUN"
                or frame["bodyReferenceDisposition"]
                != "peer-profile-no-body-handles"
                or frame["bodyReferenceRule"] != "LIBREDWG-dwg2.spec-SUN"
                or sun_reports.get(int(frame["handle"]))
                != "LIBREDWG-dwg2.spec-SUN"
                or frame["reactorHandles"]
                or frame["extensionDictionaryHandle"] != 0
                or len(sun_owner_refs) != 1
            ):
                raise SystemExit(
                    "AC1027 SUN lacks its source-matched no-body-handle profile "
                    "or exact VPORT hard-owner reference"
                )
            if source_before == COVER_AC1027_SHA256 and (
                frame["handle"] != 109 or frame["ownerHandle"] != 41
                or sun_owner_refs[0]["ownerHandle"] != 41
            ):
                raise SystemExit(
                    "hash-bound Cover SUN/VPORT owner identities changed"
                )
        default_dictionaries = [
            frame for frame in frames if frame["encodedType"] == 500
        ]
        default_profiles = {
            int(handle): int(fields)
            for object_type, handle, fields, _rule
            in HANDLE_BODY_PROFILE.findall(combined)
            if object_type == "DICTIONARYWDFLT"
        }
        for frame in default_dictionaries:
            dictionary_refs = [
                edge for edge in typed_references
                if edge["ownerHandle"] == frame["handle"]
                and edge["role"].startswith("dictionary-entry key=")
            ]
            default_refs = [
                edge for edge in typed_references
                if edge["ownerHandle"] == frame["handle"]
                and edge["role"] == "default-entry"
            ]
            if (
                frame["recordName"] != "ACDBDICTIONARYWDFLT"
                or frame["className"] != "AcDbDictionaryWithDefault"
                or frame["typedBodyReport"] != "DICTIONARYWDFLT"
                or frame["bodyReferenceDisposition"]
                != "spec-profile-handle-references"
                or frame["bodyReferenceRule"] != "ODA-20.4.45"
                or default_profiles.get(int(frame["handle"]))
                != len(dictionary_refs) + 1
                or len(default_refs) != 1
                or not any(
                    edge["targetHandle"] == default_refs[0]["targetHandle"]
                    for edge in dictionary_refs
                )
            ):
                raise SystemExit(
                    "AC1027 DICTIONARYWDFLT lacks ODA default-handle field "
                    "or matching dictionary-entry role evidence: "
                    f"handle={frame['handle']} name={frame['recordName']} "
                    f"class={frame['className']} "
                    f"report={frame['typedBodyReport']} "
                    f"disposition={frame['bodyReferenceDisposition']} "
                    f"rule={frame['bodyReferenceRule']} "
                    f"profile={default_profiles.get(int(frame['handle']))} "
                    f"dictRefs={len(dictionary_refs)} "
                    f"defaultRefs={len(default_refs)}"
                )
            if source_before == COVER_AC1027_SHA256 and (
                frame["handle"] != 14 or frame["ownerHandle"] != 12
                or frame["reactorHandles"] != [12]
                or default_refs[0]["targetHandle"] != 15
                or [entry for entry in named_object_dictionary_entries
                    if entry["targetHandle"] == 14]
                    != [{"key": "ACAD_PLOTSTYLENAME", "targetHandle": 14}]
            ):
                raise SystemExit(
                    "hash-bound Cover DICTIONARYWDFLT common/default roles changed"
                )
        placeholders = [
            frame for frame in frames if frame["encodedType"] == 80
        ]
        if source_before == COVER_AC1027_SHA256 and len(placeholders) != 1:
            raise SystemExit(
                "hash-bound Cover source did not publish exactly one type-80 frame"
            )
        for frame in placeholders:
            if frame["bodyReferenceDisposition"] != "peer-profile-no-body-handles":
                if source_before == COVER_AC1027_SHA256:
                    raise SystemExit(
                        "hash-bound Cover type-80 frame lacks its peer profile"
                    )
                continue
            membership = frame["dictionaryMembership"]
            if (
                frame["handle"] != 15
                or frame["typedBodyReport"] != "ACDBPLACEHOLDER"
                or frame["bodyReferenceDisposition"]
                != "peer-profile-no-body-handles"
                or frame["bodyReferenceRule"]
                != "LIBREDWG-dwg2.spec-PLACEHOLDER"
                or membership is None
                or membership["objectType"] != "ACDBPLACEHOLDER"
                or membership["dictionaryHandle"] != frame["ownerHandle"]
                or not any(
                    edge["sourceFrameHandle"] == frame["handle"]
                    and edge["dictionaryHandle"] == frame["ownerHandle"]
                    and edge["targetHandle"] == frame["handle"]
                    for edge in dictionary_membership_edges
                )
                or not any(
                    item["ownerHandle"] == frame["ownerHandle"]
                    and item["targetHandle"] == frame["handle"]
                    and item["role"] == "default-entry"
                    for item in typed_references
                )
            ):
                raise SystemExit(
                    "AC1027 type-80 frame lacks its source-matched "
                    "PLACEHOLDER peer profile or exact dictionary roles"
                )

    report = {
        "schema": "libdxfrw-ac1027-source-reference-audit-v1",
        "verifier": str(Path(__file__).resolve().relative_to(REPO)),
        "verifierSha256": sha256(Path(__file__).resolve()),
        "executable": str(executable),
        "executableSha256": sha256(executable),
        "source": str(source),
        "sourceSha256Before": source_before,
        "sourceSha256After": source_after,
        "sourceUnchangedDuringRun": source_before == source_after,
        "diagnosticUtilityHashBindsInput": bool(int(utility_bound)),
        "verifierBindsUnchangedSourceBytes": source_before == source_after,
        "command": [str(executable), "--opaque-dwg-source-inventory", str(source)],
        "diagnosticOutputSha256": hashlib.sha256(
            result.stdout + b"\n" + result.stderr
        ).hexdigest(),
        "exitCode": result.returncode,
        "sourceVersion": version,
        "independentPeerEvidence": peer_evidence,
        "opaqueCallbacks": int(callbacks),
        "capturedOpaqueCallbacks": int(captured),
        "retainedOpaqueFrames": int(opaque_retained),
        "retainedStandardObjects": int(standard_retained),
        "retainedAcDsHistoryFrames": int(acds_retained),
        "capturedFrameIdentities": len(frames),
        "typedBodyReferenceRows": len(typed_references),
        "sourceSortEntsTableCount": len(sortents_profiles),
        "sourceSortEntsEntryCount": sum(
            len(profile["entries"]) for profile in sortents_profiles.values()
        ),
        "sourceSortEntsFieldSpanProfileCount": len(sortents_source_spans),
        "sourceSortEntsFieldSpans": sortents_source_spans,
        "peerMatchedSortEntsTableCount": len(peer_sortents_handles),
        "typedLayerHandleFieldRows": sum(
            len(fields) for fields in layer_handle_fields.values()
        ),
        "sourceBoundLayerHandleProfiles": (
            1 if source_before == COVER_AC1027_SHA256 else 0
        ),
        "typedLayerHandleFields": [
            {"ownerHandle": owner, **field}
            for owner, fields in sorted(layer_handle_fields.items())
            for field in fields
        ],
        "typedDimstyleHandleFieldRows": sum(
            len(fields) for fields in dimstyle_handle_fields.values()
        ),
        "sourceBoundDimstyleHandleProfiles": (
            1 if source_before == COVER_AC1027_SHA256 else 0
        ),
        "typedDimstyleHandleFields": [
            {"ownerHandle": owner, **field}
            for owner, fields in sorted(dimstyle_handle_fields.items())
            for field in fields
        ],
        "typedVportHandleFieldRows": sum(
            len(fields) for fields in vport_handle_fields.values()
        ),
        "sourceBoundVportHandleProfiles": (
            1 if source_before == COVER_AC1027_SHA256 else 0
        ),
        "typedVportHandleFields": [
            {"ownerHandle": owner, **field}
            for owner, fields in sorted(vport_handle_fields.items())
            for field in fields
        ],
        "framesWithSelectedTypedBodyReport": sum(
            frame["bodyReferenceDisposition"] == "selected-typed-report-only"
            for frame in frames
        ),
        "framesWithSourceBoundModelerHistoryProfile": sum(
            frame["bodyReferenceDisposition"]
            == "source-bound-modeler-history-profile"
            for frame in frames
        ),
        "framesWithoutExhaustiveBodyReferenceDecode": sum(
            frame["bodyReferenceDisposition"] == "not-exhaustively-decoded"
            for frame in frames
        ),
        "framesWithPeerNoBodyHandleProfile": sum(
            frame["bodyReferenceDisposition"] == "peer-profile-no-body-handles"
            for frame in frames
        ),
        "framesWithPeerExactBodyReferenceProfile": sum(
            frame["bodyReferenceDisposition"]
            == "peer-profile-exact-body-references"
            for frame in frames
        ),
        "typedBodyReferences": typed_references,
        "dictionaryMembershipEdges": dictionary_membership_edges,
        "namedObjectDictionaryEntries": named_object_dictionary_entries,
        "capturedFrames": frames,
        "coverageLimit": (
            "This binds the diagnostic run to unchanged input bytes and verifies "
            "the emitted identity inventory. ODA profiles and source-matched "
            "LibreDWG peer profiles are kept distinct. Typed body rows outside "
            "those bounded profiles are selected reports only; "
            "not-exhaustively-decoded does not mean no references. Common "
            "header values are not independent role proof, and no DXF output route "
            "or semantic support claim is qualified."
        ),
    }
    print(json.dumps(report, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
