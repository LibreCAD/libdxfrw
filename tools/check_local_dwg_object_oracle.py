#!/usr/bin/env python3
"""Check locally generated DWG OBJECTS through an independent JSON oracle.

The checker deliberately keeps this lane advisory: it validates the object
carriers that the local writer owns, but it never changes the format-support
ledger or stores the generated drawings.  The writer and oracle are invoked
with argument lists (no shell) and every subprocess is timeout-bounded.
"""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
import tempfile
from pathlib import Path


VERSIONS = {
    13: "AC1015",
    14: "AC1018",
    15: "AC1021",
    16: "AC1024",
    17: "AC1027",
    18: "AC1032",
}

GROUP_HANDLE = 0xA600
DICTIONARY_HANDLE = 0xA601
XRECORD_HANDLE = 0xA602
PLOTSETTINGS_HANDLE = 0xA603
LAYOUT_HANDLE = 0xA700
MALFORMED_HANDLE = 0xA701
MLINESTYLE_HANDLE = 0xA800
MALFORMED_STYLE_HANDLE = 0xA801
MLEADERSTYLE_HANDLE = 0xA900
MALFORMED_MLEADERSTYLE_HANDLE = 0xA901
DICTIONARYVAR_HANDLE = 0xB000
MALFORMED_DICTIONARYVAR_HANDLE = 0xB001
DICTIONARYWDFLT_HANDLE = 0xB100
MALFORMED_DICTIONARYWDFLT_HANDLE = 0xB101
SORTENTSTABLE_HANDLE = 0xB200
MALFORMED_SORTENTSTABLE_HANDLE = 0xB201
FIELDLIST_HANDLE = 0xB300
MALFORMED_FIELDLIST_HANDLE = 0xB301
FIELD_HANDLE = 0xB400
MALFORMED_FIELD_HANDLE = 0xB401
RASTERVARIABLES_HANDLE = 0xB500
MALFORMED_RASTERVARIABLES_HANDLE = 0xB501
WIPEOUTVARIABLES_HANDLE = 0xB600
MALFORMED_WIPEOUTVARIABLES_HANDLE = 0xB601
VISUALSTYLE_HANDLE = 0xC000
MALFORMED_VISUALSTYLE_HANDLE = 0xC001
RENDERSETTINGS_HANDLE = 0xC100
MALFORMED_RENDERSETTINGS_HANDLE = 0xC101


def parse_json_output(text: str) -> dict:
    start = text.find("{")
    if start < 0:
        raise ValueError("oracle output has no JSON object")
    payload = json.loads(text[start:])
    if not isinstance(payload, dict):
        raise ValueError("oracle JSON root is not an object")
    return payload


def record_handle(record: dict) -> int | None:
    value = record.get("handle")
    if not isinstance(value, list) or len(value) < 3:
        return None
    try:
        return int(value[2])
    except (TypeError, ValueError):
        return None


def owner_handle(record: dict) -> int | None:
    value = record.get("ownerhandle")
    if not isinstance(value, list) or len(value) < 3:
        return None
    try:
        return int(value[2])
    except (TypeError, ValueError):
        return None


def find_record(records: list[dict], object_name: str, handle: int) -> dict:
    matches = [
        record for record in records
        if isinstance(record, dict)
        and record.get("object") == object_name
        and record_handle(record) == handle
    ]
    if len(matches) != 1:
        raise ValueError(
            f"expected one {object_name} frame at handle 0x{handle:X}, "
            f"found {len(matches)}"
        )
    return matches[0]


def check_objects(payload: dict, version_name: str) -> dict:
    header = payload.get("FILEHEADER")
    if not isinstance(header, dict) or header.get("version") != version_name:
        raise ValueError(f"FILEHEADER version is not {version_name}")
    records = payload.get("OBJECTS")
    if not isinstance(records, list):
        raise ValueError("oracle JSON has no OBJECTS list")

    group = find_record(records, "GROUP", GROUP_HANDLE)
    if (group.get("name") != "LOCAL_GROUP"
            or not isinstance(group.get("groups"), list)
            or len(group["groups"]) != 1
            or owner_handle(group) != 0x0C):
        raise ValueError("GROUP name, owner, or member stream mismatch")

    dictionary = find_record(records, "DICTIONARY", DICTIONARY_HANDLE)
    if (dictionary.get("numitems") != 14
            or dictionary.get("is_hardowner") != 1
            or owner_handle(dictionary) != 0x0C):
        raise ValueError("custom DICTIONARY count, owner, or cloning mismatch")

    xrecord = find_record(records, "XRECORD", XRECORD_HANDLE)
    xdata = xrecord.get("xdata")
    if (owner_handle(xrecord) != DICTIONARY_HANDLE
            or not isinstance(xdata, list)
            or [40, 1.25] not in xdata
            or [1, "LOCAL_XRECORD"] not in xdata
            or [310, "010203"] not in xdata):
        raise ValueError("XRECORD owner or bounded payload mismatch")

    plot = find_record(records, "PLOTSETTINGS", PLOTSETTINGS_HANDLE)
    if (owner_handle(plot) != DICTIONARY_HANDLE
            or plot.get("printer_cfg_file") != "LOCAL_PAGE"
            or plot.get("paper_size") != "LOCAL_PRINTER"
            or plot.get("canonical_media_name") != "A4"):
        raise ValueError("PLOTSETTINGS owner or bounded field mismatch")

    layout = find_record(records, "LAYOUT", LAYOUT_HANDLE)
    if (owner_handle(layout) != DICTIONARY_HANDLE
            or layout.get("layout_name") != "LOCAL_LAYOUT"):
        raise ValueError("LAYOUT owner or name mismatch")

    mline = find_record(records, "MLINESTYLE", MLINESTYLE_HANDLE)
    lines = mline.get("lines")
    if (owner_handle(mline) != DICTIONARY_HANDLE
            or mline.get("name") != "LOCAL_MLINESTYLE"
            or mline.get("description") != "LOCAL_MLINESTYLE_DESC"
            or mline.get("start_angle") != 0.0
            or mline.get("end_angle") != 1.5707963267949
            or not isinstance(lines, list)
            or len(lines) != 1
            or not isinstance(lines[0], dict)
            or lines[0].get("offset") != 0.5
            or (lines[0].get("lt_index") != 0
                and lines[0].get("lt_ltype") != [0, 0, 0, 0])):
        raise ValueError("MLINESTYLE owner, style, or element payload mismatch")

    mleader = find_record(records, "MLEADERSTYLE", MLEADERSTYLE_HANDLE)
    if (owner_handle(mleader) != DICTIONARY_HANDLE
            or mleader.get("class_version") != 2
            or mleader.get("content_type") != 2
            or mleader.get("description") != "LOCAL_MLEADERSTYLE_DESC"
            or mleader.get("landing_gap") != 0.25
            or mleader.get("text_default") != "LOCAL_MLEADER_TEXT"
            or mleader.get("text_height") != 2.5
            or mleader.get("line_type") != [0, 0, 0, 0]
            or mleader.get("arrow_head") != [0, 0, 0, 0]
            or mleader.get("text_style") != [0, 0, 0, 0]
            or mleader.get("block") != [0, 0, 0, 0]):
        raise ValueError("MLEADERSTYLE owner, style, or handle-stream mismatch")

    dictionary_var = find_record(records, "DICTIONARYVAR", DICTIONARYVAR_HANDLE)
    if (owner_handle(dictionary_var) != DICTIONARY_HANDLE
            or dictionary_var.get("schema") != 7
            or dictionary_var.get("strvalue") != "LOCAL_DICTIONARYVAR_VALUE"):
        raise ValueError("DICTIONARYVAR owner or bounded payload mismatch")

    sortents = find_record(records, "SORTENTSTABLE", SORTENTSTABLE_HANDLE)
    sort_entries = sortents.get("sort_ents")
    entity_entries = sortents.get("ents")
    block_owner = sortents.get("block_owner")
    if (owner_handle(sortents) != 0x17
            or block_owner != [4, 1, 0x17, 0x17]
            or not isinstance(sort_entries, list)
            or len(sort_entries) != 1
            or not isinstance(sort_entries[0], list)
            or len(sort_entries[0]) < 3
            or not isinstance(entity_entries, list)
            or len(entity_entries) != 1
            or not isinstance(entity_entries[0], list)
            or len(entity_entries[0]) < 3
            or sort_entries[0][2] <= 0
            or sort_entries[0][2] != entity_entries[0][2]):
        raise ValueError("SORTENTSTABLE owner or handle stream mismatch")

    field_list = find_record(records, "FIELDLIST", FIELDLIST_HANDLE)
    fields = field_list.get("fields")
    if (owner_handle(field_list) != DICTIONARY_HANDLE
            or field_list.get("unknown") != 0
            or not isinstance(fields, list)
            or len(fields) != 1
            or not isinstance(fields[0], list)
            or len(fields[0]) < 3
            or fields[0][2] != FIELD_HANDLE):
        raise ValueError("FIELDLIST owner or flag mismatch")

    field = find_record(records, "FIELD", FIELD_HANDLE)
    if (owner_handle(field) != DICTIONARY_HANDLE
            or field.get("id") != "ACAD"
            or field.get("code") != "LOCAL_FIELD_CODE"
            or field.get("value.data_type") != 1
            or field.get("value.data_long") != 42
            or field.get("value_string") != "42"
            or field.get("value_string_length") != 2):
        raise ValueError("FIELD owner or bounded payload mismatch")

    raster = find_record(records, "RASTERVARIABLES", RASTERVARIABLES_HANDLE)
    if (owner_handle(raster) != DICTIONARY_HANDLE
            or raster.get("class_version") != 1
            or raster.get("image_frame") != 1
            or raster.get("image_quality") != 2
            or raster.get("units") != 3):
        raise ValueError("RASTERVARIABLES owner or scalar mismatch")

    wipeout = find_record(
        records, "WIPEOUTVARIABLES", WIPEOUTVARIABLES_HANDLE)
    if (owner_handle(wipeout) != DICTIONARY_HANDLE
            or wipeout.get("display_frame") != 1):
        raise ValueError("WIPEOUTVARIABLES owner or scalar mismatch")

    visual = find_record(records, "VISUALSTYLE", VISUALSTYLE_HANDLE)
    if (owner_handle(visual) != DICTIONARY_HANDLE
            or visual.get("type") != 560
            or visual.get("description") != "LOCAL_VISUALSTYLE_DESC"
            or visual.get("style_type") != 1
            or visual.get("face_lighting_model") != 2
            or visual.get("face_opacity") != 0.75
            or visual.get("edge_model") != 1
            or visual.get("edge_isolines") != 3
            or visual.get("display_settings") != 4):
        raise ValueError("VISUALSTYLE owner or bounded legacy fields mismatch")
    if version_name in {"AC1024", "AC1027", "AC1032"}:
        if (visual.get("ext_lighting_model") != 1
                or visual.get("display_brightness") != 0.8):
            raise ValueError("VISUALSTYLE R2010b fields mismatch")
    if version_name in {"AC1027", "AC1032"}:
        if (visual.get("b_prop1c") != 1
                or visual.get("bl_prop25") != 5
                or visual.get("bd_prop26") != 1.25
                or visual.get("bd_prop27") != 2.5
                or not isinstance(visual.get("c_prop29"), dict)
                or visual["c_prop29"].get("rgb") != "c3000007"
                or visual.get("bd_prop34") != 3.5
                or visual.get("bd_prop38") != 4.5
                or visual.get("bd_prop39") != 5.5):
            raise ValueError("VISUALSTYLE R2013b fields mismatch")

    render = find_record(records, "RENDERSETTINGS", RENDERSETTINGS_HANDLE)
    if (owner_handle(render) != DICTIONARY_HANDLE
            or render.get("type") != 556
            or render.get("name") != "LOCAL_RENDERSETTINGS"
            or render.get("description") != "LOCAL_RENDER_DESC"
            or render.get("fog_enabled") != 1
            or render.get("fog_background_enabled") != 0
            or render.get("backfaces_enabled") != 1
            or render.get("environ_image_enabled") != 0
            or render.get("display_index") != 2):
        raise ValueError("RENDERSETTINGS owner or bounded base fields mismatch")
    if version_name in {"AC1015", "AC1018", "AC1021", "AC1024"}:
        if render.get("class_version") != 1:
            raise ValueError("RENDERSETTINGS class version mismatch")
    render_discrepancies = []
    if version_name == "AC1027":
        if render.get("has_predefined") != 0:
            raise ValueError("RENDERSETTINGS predefined flag mismatch")
    elif version_name == "AC1032" and "has_predefined" not in render:
        render_discrepancies.append(
            "LibreDWG 0.14 omits RENDERSETTINGS has_predefined for AC1032")
    elif version_name == "AC1032" and render.get("has_predefined") != 0:
        raise ValueError("RENDERSETTINGS predefined flag mismatch")

    dictionary_default = find_record(
        records, "DICTIONARYWDFLT", DICTIONARYWDFLT_HANDLE)
    if (owner_handle(dictionary_default) != DICTIONARY_HANDLE
            or dictionary_default.get("numitems") != 1
            or dictionary_default.get("cloning") != 1
            or dictionary_default.get("is_hardowner") != 1):
        raise ValueError("DICTIONARYWDFLT owner or header mismatch")
    expected_items = {
        "LOCAL_DEFAULT": [3, 2, DICTIONARYVAR_HANDLE, DICTIONARYVAR_HANDLE]
    }
    expected_default = [5, 2, DICTIONARYVAR_HANDLE, DICTIONARYVAR_HANDLE]
    if (dictionary_default.get("items") != expected_items
            or dictionary_default.get("defaultid") != expected_default):
        if version_name in {"AC1015", "AC1018"}:
            raise ValueError("DICTIONARYWDFLT item/default payload mismatch")
        # LibreDWG 0.14 currently reports the R2007+ string/handle members of
        # this carrier as empty/zero while retaining its type/header/owner.
        # Keep the bounded frame qualification, but surface the independent
        # reader discrepancy instead of silently treating it as parity.
        oracle_discrepancies = render_discrepancies + [
            "LibreDWG 0.14 omits DICTIONARYWDFLT item/default handles for "
            + version_name
        ]
    else:
        oracle_discrepancies = render_discrepancies

    malformed_handles = {
        MALFORMED_HANDLE: "XRECORD",
        MALFORMED_STYLE_HANDLE: "MLINESTYLE",
        MALFORMED_MLEADERSTYLE_HANDLE: "MLEADERSTYLE",
        MALFORMED_DICTIONARYVAR_HANDLE: "DICTIONARYVAR",
        MALFORMED_DICTIONARYWDFLT_HANDLE: "DICTIONARYWDFLT",
        MALFORMED_SORTENTSTABLE_HANDLE: "SORTENTSTABLE",
        MALFORMED_FIELDLIST_HANDLE: "FIELDLIST",
        MALFORMED_FIELD_HANDLE: "FIELD",
        MALFORMED_RASTERVARIABLES_HANDLE: "RASTERVARIABLES",
        MALFORMED_WIPEOUTVARIABLES_HANDLE: "WIPEOUTVARIABLES",
        MALFORMED_VISUALSTYLE_HANDLE: "VISUALSTYLE",
        MALFORMED_RENDERSETTINGS_HANDLE: "RENDERSETTINGS",
    }
    if any(record_handle(record) in malformed_handles
           and record.get("object") == malformed_handles[record_handle(record)]
           for record in records if isinstance(record, dict)):
        raise ValueError("rolled-back malformed object was published")

    return {
        "version": version_name,
        "objectHandles": {
            "GROUP": GROUP_HANDLE,
            "DICTIONARY": DICTIONARY_HANDLE,
            "XRECORD": XRECORD_HANDLE,
            "PLOTSETTINGS": PLOTSETTINGS_HANDLE,
            "LAYOUT": LAYOUT_HANDLE,
            "MLINESTYLE": MLINESTYLE_HANDLE,
            "MLEADERSTYLE": MLEADERSTYLE_HANDLE,
            "DICTIONARYVAR": DICTIONARYVAR_HANDLE,
            "SORTENTSTABLE": SORTENTSTABLE_HANDLE,
            "FIELDLIST": FIELDLIST_HANDLE,
            "FIELD": FIELD_HANDLE,
            "RASTERVARIABLES": RASTERVARIABLES_HANDLE,
            "WIPEOUTVARIABLES": WIPEOUTVARIABLES_HANDLE,
            "VISUALSTYLE": VISUALSTYLE_HANDLE,
            "RENDERSETTINGS": RENDERSETTINGS_HANDLE,
            "DICTIONARYWDFLT": DICTIONARYWDFLT_HANDLE,
        },
        "objectStatus": "qualified",
        "oracleDiscrepancies": oracle_discrepancies,
    }


def run(writer: str, oracle: str, timeout: float) -> dict:
    records = []
    with tempfile.TemporaryDirectory(prefix="libdxfrw-object-oracle-") as directory:
        output_dir = Path(directory)
        try:
            generated = subprocess.run(
                [writer, "--keep-dir", str(output_dir)],
                capture_output=True,
                text=True,
                timeout=timeout,
                check=False,
            )
        except subprocess.TimeoutExpired:
            return {"status": "timeout", "stage": "writer"}
        if generated.returncode != 0:
            return {
                "status": "writer-failed",
                "returncode": generated.returncode,
            }

        for code, version_name in VERSIONS.items():
            path = output_dir / f"libdxfrw-local-roundtrip-{code}.dwg"
            if not path.exists():
                records.append({"version": version_name, "status": "missing-output"})
                continue
            try:
                result = subprocess.run(
                    [oracle, "-O", "JSON", str(path)],
                    capture_output=True,
                    text=True,
                    timeout=timeout,
                    check=False,
                )
            except subprocess.TimeoutExpired:
                records.append({"version": version_name, "status": "timeout"})
                continue
            if result.returncode != 0:
                records.append({
                    "version": version_name,
                    "status": "oracle-failed",
                    "returncode": result.returncode,
                })
                continue
            try:
                summary = check_objects(parse_json_output(result.stdout), version_name)
                records.append(summary | {"status": "qualified"})
            except (ValueError, json.JSONDecodeError) as exc:
                records.append({
                    "version": version_name,
                    "status": "mismatch",
                    "reason": str(exc),
                })

    status = "complete" if records and all(
        record.get("status") == "qualified" for record in records
    ) else "incomplete"
    return {"status": status, "records": records, "writer": writer, "oracle": oracle}


def self_test() -> None:
    payload = {
        "FILEHEADER": {"version": "AC1024"},
        "OBJECTS": [
            {"object": "GROUP", "handle": [0, 1, GROUP_HANDLE],
             "ownerhandle": [4, 1, 0x0C, 0x0C], "name": "LOCAL_GROUP",
             "groups": [[5, 1, 0x1234]]},
            {"object": "DICTIONARY", "handle": [0, 1, DICTIONARY_HANDLE],
             "ownerhandle": [4, 1, 0x0C, 0x0C], "numitems": 14,
             "is_hardowner": 1},
            {"object": "XRECORD", "handle": [0, 1, XRECORD_HANDLE],
             "ownerhandle": [4, 1, DICTIONARY_HANDLE, DICTIONARY_HANDLE],
             "xdata": [[40, 1.25], [1, "LOCAL_XRECORD"], [310, "010203"]]},
            {"object": "PLOTSETTINGS", "handle": [0, 1, PLOTSETTINGS_HANDLE],
             "ownerhandle": [4, 1, DICTIONARY_HANDLE, DICTIONARY_HANDLE],
             "printer_cfg_file": "LOCAL_PAGE", "paper_size": "LOCAL_PRINTER",
             "canonical_media_name": "A4"},
            {"object": "LAYOUT", "handle": [0, 1, LAYOUT_HANDLE],
             "ownerhandle": [4, 1, DICTIONARY_HANDLE, DICTIONARY_HANDLE],
             "layout_name": "LOCAL_LAYOUT"},
            {"object": "MLINESTYLE", "handle": [0, 1, MLINESTYLE_HANDLE],
             "ownerhandle": [4, 1, DICTIONARY_HANDLE, DICTIONARY_HANDLE],
             "name": "LOCAL_MLINESTYLE",
             "description": "LOCAL_MLINESTYLE_DESC",
             "start_angle": 0.0, "end_angle": 1.5707963267949,
             "lines": [{"offset": 0.5, "lt_index": 0}]},
            {"object": "MLEADERSTYLE", "handle": [0, 1, MLEADERSTYLE_HANDLE],
             "ownerhandle": [4, 1, DICTIONARY_HANDLE, DICTIONARY_HANDLE],
             "class_version": 2, "content_type": 2,
             "description": "LOCAL_MLEADERSTYLE_DESC", "landing_gap": 0.25,
             "text_default": "LOCAL_MLEADER_TEXT", "text_height": 2.5,
             "line_type": [0, 0, 0, 0], "arrow_head": [0, 0, 0, 0],
             "text_style": [0, 0, 0, 0], "block": [0, 0, 0, 0]},
            {"object": "DICTIONARYVAR", "handle": [0, 1, DICTIONARYVAR_HANDLE],
             "ownerhandle": [4, 1, DICTIONARY_HANDLE, DICTIONARY_HANDLE],
             "schema": 7, "strvalue": "LOCAL_DICTIONARYVAR_VALUE"},
            {"object": "DICTIONARYWDFLT",
             "handle": [0, 1, DICTIONARYWDFLT_HANDLE],
             "ownerhandle": [4, 1, DICTIONARY_HANDLE, DICTIONARY_HANDLE],
             "numitems": 1, "cloning": 1, "is_hardowner": 1,
             "items": {"LOCAL_DEFAULT": [3, 2, DICTIONARYVAR_HANDLE,
                                           DICTIONARYVAR_HANDLE]},
             "defaultid": [5, 2, DICTIONARYVAR_HANDLE, DICTIONARYVAR_HANDLE]},
            {"object": "SORTENTSTABLE",
             "handle": [0, 1, SORTENTSTABLE_HANDLE],
             "ownerhandle": [4, 1, 0x17, 0x17],
             "sort_ents": [[0, 2, 0x1234, 0x1234]],
             "block_owner": [4, 1, 0x17, 0x17],
             "ents": [[4, 2, 0x1234, 0x1234]]},
            {"object": "FIELDLIST", "handle": [0, 1, FIELDLIST_HANDLE],
             "ownerhandle": [4, 1, DICTIONARY_HANDLE, DICTIONARY_HANDLE],
             "unknown": 0, "fields": [[4, 2, FIELD_HANDLE, FIELD_HANDLE]]},
            {"object": "FIELD", "handle": [0, 1, FIELD_HANDLE],
             "ownerhandle": [4, 1, DICTIONARY_HANDLE, DICTIONARY_HANDLE],
             "id": "ACAD", "code": "LOCAL_FIELD_CODE",
             "value.data_type": 1, "value.data_long": 42,
             "value_string": "42", "value_string_length": 2},
            {"object": "RASTERVARIABLES",
             "handle": [0, 1, RASTERVARIABLES_HANDLE],
             "ownerhandle": [4, 1, DICTIONARY_HANDLE, DICTIONARY_HANDLE],
             "class_version": 1, "image_frame": 1, "image_quality": 2,
             "units": 3},
            {"object": "WIPEOUTVARIABLES",
             "handle": [0, 1, WIPEOUTVARIABLES_HANDLE],
             "ownerhandle": [4, 1, DICTIONARY_HANDLE, DICTIONARY_HANDLE],
             "display_frame": 1},
            {"object": "VISUALSTYLE",
             "handle": [0, 1, VISUALSTYLE_HANDLE],
             "ownerhandle": [4, 1, DICTIONARY_HANDLE, DICTIONARY_HANDLE],
             "type": 560, "description": "LOCAL_VISUALSTYLE_DESC",
             "style_type": 1, "face_lighting_model": 2,
             "face_opacity": 0.75, "edge_model": 1, "edge_isolines": 3,
             "display_settings": 4, "ext_lighting_model": 1,
             "display_brightness": 0.8, "b_prop1c": 1,
             "bl_prop25": 5, "bd_prop26": 1.25, "bd_prop27": 2.5,
             "c_prop29": {"rgb": "c3000007"}, "bd_prop34": 3.5,
             "bd_prop38": 4.5,
             "bd_prop39": 5.5},
            {"object": "RENDERSETTINGS",
             "handle": [0, 1, RENDERSETTINGS_HANDLE],
             "ownerhandle": [4, 1, DICTIONARY_HANDLE, DICTIONARY_HANDLE],
             "type": 556, "class_version": 1,
             "name": "LOCAL_RENDERSETTINGS",
             "description": "LOCAL_RENDER_DESC", "fog_enabled": 1,
             "fog_background_enabled": 0, "backfaces_enabled": 1,
             "environ_image_enabled": 0, "display_index": 2},
        ],
    }
    check_objects(payload, "AC1024")
    try:
        bad = json.loads(json.dumps(payload))
        bad["OBJECTS"].append({"object": "XRECORD", "handle": [0, 1, MALFORMED_HANDLE]})
        check_objects(bad, "AC1024")
    except ValueError:
        pass
    else:
        raise AssertionError("malformed object was not rejected")
    try:
        bad = json.loads(json.dumps(payload))
        bad["OBJECTS"].append({"object": "MLINESTYLE",
                                "handle": [0, 1, MALFORMED_STYLE_HANDLE]})
        check_objects(bad, "AC1024")
    except ValueError:
        pass
    else:
        raise AssertionError("malformed MLINESTYLE was not rejected")
    try:
        bad = json.loads(json.dumps(payload))
        bad["OBJECTS"].append({"object": "MLEADERSTYLE",
                                "handle": [0, 1, MALFORMED_MLEADERSTYLE_HANDLE]})
        check_objects(bad, "AC1024")
    except ValueError:
        pass
    else:
        raise AssertionError("malformed MLEADERSTYLE was not rejected")
    try:
        bad = json.loads(json.dumps(payload))
        bad["OBJECTS"].append({"object": "DICTIONARYVAR",
                                "handle": [0, 1, MALFORMED_DICTIONARYVAR_HANDLE]})
        check_objects(bad, "AC1024")
    except ValueError:
        pass
    else:
        raise AssertionError("malformed DICTIONARYVAR was not rejected")
    try:
        bad = json.loads(json.dumps(payload))
        bad["OBJECTS"].append({"object": "DICTIONARYWDFLT",
                                "handle": [0, 1,
                                            MALFORMED_DICTIONARYWDFLT_HANDLE]})
        check_objects(bad, "AC1024")
    except ValueError:
        pass
    else:
        raise AssertionError("malformed DICTIONARYWDFLT was not rejected")
    try:
        bad = json.loads(json.dumps(payload))
        bad["OBJECTS"].append({"object": "SORTENTSTABLE",
                                "handle": [0, 1,
                                            MALFORMED_SORTENTSTABLE_HANDLE]})
        check_objects(bad, "AC1024")
    except ValueError:
        pass
    else:
        raise AssertionError("malformed SORTENTSTABLE was not rejected")
    try:
        bad = json.loads(json.dumps(payload))
        bad["OBJECTS"].append({"object": "FIELDLIST",
                                "handle": [0, 1, MALFORMED_FIELDLIST_HANDLE]})
        check_objects(bad, "AC1024")
    except ValueError:
        pass
    else:
        raise AssertionError("malformed FIELDLIST was not rejected")
    try:
        bad = json.loads(json.dumps(payload))
        bad["OBJECTS"].append({"object": "FIELD",
                                "handle": [0, 1, MALFORMED_FIELD_HANDLE]})
        check_objects(bad, "AC1024")
    except ValueError:
        pass
    else:
        raise AssertionError("malformed FIELD was not rejected")
    try:
        bad = json.loads(json.dumps(payload))
        bad["OBJECTS"].append({"object": "RASTERVARIABLES",
                                "handle": [0, 1,
                                            MALFORMED_RASTERVARIABLES_HANDLE]})
        check_objects(bad, "AC1024")
    except ValueError:
        pass
    else:
        raise AssertionError("malformed RASTERVARIABLES was not rejected")
    try:
        bad = json.loads(json.dumps(payload))
        bad["OBJECTS"].append({"object": "WIPEOUTVARIABLES",
                                "handle": [0, 1,
                                            MALFORMED_WIPEOUTVARIABLES_HANDLE]})
        check_objects(bad, "AC1024")
    except ValueError:
        pass
    else:
        raise AssertionError("malformed WIPEOUTVARIABLES was not rejected")
    try:
        bad = json.loads(json.dumps(payload))
        bad["OBJECTS"].append({"object": "VISUALSTYLE",
                                "handle": [0, 1,
                                            MALFORMED_VISUALSTYLE_HANDLE]})
        check_objects(bad, "AC1024")
    except ValueError:
        pass
    else:
        raise AssertionError("malformed VISUALSTYLE was not rejected")
    try:
        bad = json.loads(json.dumps(payload))
        bad["OBJECTS"].append({"object": "RENDERSETTINGS",
                                "handle": [0, 1,
                                            MALFORMED_RENDERSETTINGS_HANDLE]})
        check_objects(bad, "AC1024")
    except ValueError:
        pass
    else:
        raise AssertionError("malformed RENDERSETTINGS was not rejected")
    print("local DWG object oracle: PASS")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--writer")
    parser.add_argument("--oracle")
    parser.add_argument("--timeout", type=float, default=10.0)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args(argv)
    if args.self_test:
        self_test()
        return 0
    if not args.writer or not args.oracle:
        parser.error("--writer and --oracle are required unless --self-test is used")
    print(json.dumps(run(args.writer, args.oracle, args.timeout), indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    sys.exit(main())
