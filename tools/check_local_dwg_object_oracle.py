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
RENDERENVIRONMENT_HANDLE = 0xC200
MALFORMED_RENDERENVIRONMENT_HANDLE = 0xC201
RENDERGLOBAL_HANDLE = 0xC300
MALFORMED_RENDERGLOBAL_HANDLE = 0xC301
RENDERENTRY_HANDLE = 0xC400
MALFORMED_RENDERENTRY_HANDLE = 0xC401
RAPIDRTRENDERSETTINGS_HANDLE = 0xC600
MALFORMED_RAPIDRTRENDERSETTINGS_HANDLE = 0xC601
MENTALRAYRENDERSETTINGS_HANDLE = 0xC700
MALFORMED_MENTALRAYRENDERSETTINGS_HANDLE = 0xC701
MATERIAL_HANDLE = 0xC800
MALFORMED_MATERIAL_HANDLE = 0xC801
DBCOLOR_HANDLE = 0xC900
MALFORMED_DBCOLOR_HANDLE = 0xC901
LIGHTLIST_HANDLE = 0xCA00
MALFORMED_LIGHTLIST_HANDLE = 0xCA01
SCALE_HANDLE = 0xCB00
MALFORMED_SCALE_HANDLE = 0xCB01
IDBUFFER_HANDLE = 0xCC00
MALFORMED_IDBUFFER_HANDLE = 0xCC01
LAYER_INDEX_HANDLE = 0xCD00
MALFORMED_LAYER_INDEX_HANDLE = 0xCD01
SPATIAL_INDEX_HANDLE = 0xCE00
MALFORMED_SPATIAL_INDEX_HANDLE = 0xCE01
TABLESTYLE_HANDLE = 0xCF00
MALFORMED_TABLESTYLE_HANDLE = 0xCF01
SPATIAL_FILTER_HANDLE = 0xD000
MALFORMED_SPATIAL_FILTER_HANDLE = 0xD001
GEODATA_HANDLE = 0xD100
MALFORMED_GEODATA_HANDLE = 0xD101
GEODATA_V2_HANDLE = 0xD200
UNDERLAY_PDF_HANDLE = 0xD300
UNDERLAY_DGN_HANDLE = 0xD400
UNDERLAY_DWF_HANDLE = 0xD500
MALFORMED_UNDERLAY_HANDLE = 0xD301
POINTCLOUD_DEFINITION_HANDLE = 0xD600
POINTCLOUD_DEFINITION_EX_HANDLE = 0xD601
POINTCLOUD_REACTOR_HANDLE = 0xD602
POINTCLOUD_REACTOR_EX_HANDLE = 0xD603
MALFORMED_POINTCLOUD_HANDLE = 0xD604
POINTCLOUD_COLORMAP_HANDLE = 0xD800
MALFORMED_POINTCLOUD_COLORMAP_HANDLE = 0xD801
NAVISWORKS_MODEL_DEF_HANDLE = 0xD900
MALFORMED_NAVISWORKS_MODEL_DEF_HANDLE = 0xD901
SUNSTUDY_HANDLE = 0xDA00
MALFORMED_SUNSTUDY_HANDLE = 0xDA01
MOTIONPATH_HANDLE = 0xDB00
MALFORMED_MOTIONPATH_HANDLE = 0xDB01
CURVE_PATH_HANDLE = 0xDC00
MALFORMED_CURVE_PATH_HANDLE = 0xDC01
POINT_PATH_HANDLE = 0xDD00
MALFORMED_POINT_PATH_HANDLE = 0xDD01
OBJECT_PTR_HANDLE = 0xDE00
MALFORMED_OBJECT_PTR_HANDLE = 0xDE01
PARTIAL_VIEWING_INDEX_HANDLE = 0xDF00
MALFORMED_PARTIAL_VIEWING_INDEX_HANDLE = 0xDF01
SOLID_BACKGROUND_HANDLE = 0xE000
GRADIENT_BACKGROUND_HANDLE = 0xE100
GROUNDPLANE_BACKGROUND_HANDLE = 0xE200
IMAGE_BACKGROUND_HANDLE = 0xE300
IBL_BACKGROUND_HANDLE = 0xE400
SKYLIGHT_BACKGROUND_HANDLE = 0xE500
MALFORMED_BACKGROUND_HANDLE = 0xE600
SECTION_MANAGER_HANDLE = 0xE700
SECTION_SETTINGS_HANDLE = 0xE800
MALFORMED_SECTION_HANDLE = 0xE900
TVDEVICEPROPERTIES_HANDLE = 0xEA00
VXCONTROL_HANDLE = 0xEB00
VXTABLERECORD_HANDLE = 0xEC00
MALFORMED_TVDEVICEPROPERTIES_HANDLE = 0xEA01
MALFORMED_VXCONTROL_HANDLE = 0xEB01
MALFORMED_VXTABLERECORD_HANDLE = 0xEC01
IMAGE_HANDLE = 0xD700
MALFORMED_IMAGE_HANDLE = 0xD710
RTEXT_HANDLE = 0xED00
ARCALIGNEDTEXT_HANDLE = 0xED01
HELIX_HANDLE = 0xEE00
CAMERA_HANDLE = 0xEF00
GEOPOSITIONMARKER_HANDLE = 0xF300
DIMASSOC_HANDLE = 0xF000
EVALUATION_GRAPH_HANDLE = 0xF100
BLOCKREPRESENTATIONDATA_HANDLE = 0xF200

RENDER_SETTINGS_KINDS = {
    "Settings": ("RENDERSETTINGS", RENDERSETTINGS_HANDLE, 556),
    "Environment": ("RENDERENVIRONMENT", RENDERENVIRONMENT_HANDLE, 550),
    "Global": ("RENDERGLOBAL", RENDERGLOBAL_HANDLE, 551),
    "Entry": ("RENDERENTRY", RENDERENTRY_HANDLE, 549),
    "RapidRT": ("RAPIDRTRENDERSETTINGS", RAPIDRTRENDERSETTINGS_HANDLE, 558),
    "MentalRay": ("MENTALRAYRENDERSETTINGS",
                   MENTALRAYRENDERSETTINGS_HANDLE, 557),
}


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


def check_pointcloud_entities(records: list[dict], version_name: str) -> list[dict]:
    """Qualify the opaque LibreDWG entity frames by type and handle.

    LibreDWG 0.14 does not decode these classes into named entities, but its
    JSON reader retains the class type and allocated handle.  Keep that
    identity evidence separate from the libdxfrw self-read payload checks.
    """
    expected = []
    if version_name not in {"AC1015", "AC1018"}:
        expected.append((533, 0xD925))
    if version_name in {"AC1027", "AC1032"}:
        expected.append((534, 0xD926))
    candidates = [
        record for record in records
        if isinstance(record, dict)
        and record.get("entity") == "UNKNOWN_ENT"
        and record.get("type") in {533, 534}
    ]
    if len(candidates) != len(expected):
        raise ValueError("POINTCLOUD entity frame count mismatch")
    frames = []
    for object_type, handle in expected:
        matches = [
            record for record in candidates
            if record.get("type") == object_type
            and record_handle(record) == handle
        ]
        if len(matches) != 1:
            raise ValueError(
                f"POINTCLOUD entity type/handle mismatch for 0x{handle:X}")
        frames.append({
            "entity": "POINTCLOUD" if object_type == 533 else "POINTCLOUDEX",
            "type": object_type,
            "handle": handle,
            "oracleEntity": "UNKNOWN_ENT",
        })
    return frames


def check_tolerance_entity(records: list[dict]) -> dict:
    """Qualify the bounded TOLERANCE entity emitted by the local writer."""
    matches = [
        record for record in records
        if isinstance(record, dict)
        and record.get("entity") == "TOLERANCE"
        and record.get("type") == 46
    ]
    if len(matches) != 1:
        raise ValueError("TOLERANCE entity frame count mismatch")
    tolerance = matches[0]
    if (record_handle(tolerance) is None
            or tolerance.get("text_value") != "LOCAL_TOLERANCE"
            or tolerance.get("ins_pt") != [73.0, 74.0, 0.0]
            or tolerance.get("x_direction") != [1.0, 0.0, 0.0]
            or tolerance.get("extrusion") != [0.0, 0.0, 1.0]
            or tolerance.get("dimstyle") != [5, 1, 21, 21]):
        raise ValueError("TOLERANCE bounded identity or payload mismatch")
    return {
        "entity": "TOLERANCE",
        "type": 46,
        "handle": record_handle(tolerance),
        "text": "LOCAL_TOLERANCE",
    }


def check_helix_entity(records: list[dict]) -> dict:
    """Qualify the bounded HELIX spline/trailer entity emitted locally."""
    matches = [
        record for record in records
        if isinstance(record, dict)
        and record.get("entity") == "HELIX"
        and record.get("type") == 503
    ]
    if len(matches) != 1:
        raise ValueError("HELIX entity frame count mismatch")
    helix = matches[0]
    if (record_handle(helix) != HELIX_HANDLE
            or helix.get("scenario") != 1
            or helix.get("degree") != 2
            or helix.get("knots") != [0.0, 0.0, 0.0, 1.0, 1.0, 1.0]
            or helix.get("ctrl_pts") != [
                {"x": 73.0, "y": 74.0, "z": 0.0},
                {"x": 75.0, "y": 76.0, "z": 0.0},
                {"x": 77.0, "y": 78.0, "z": 0.0},
            ]
            or helix.get("major_version") != 1
            or helix.get("maint_version") != 2
            or helix.get("axis_base_pt") != [70.0, 71.0, 0.0]
            or helix.get("start_pt") != [72.0, 73.0, 0.0]
            or helix.get("axis_vector") != [0.0, 0.0, 1.0]
            or helix.get("radius") != 4.5
            or helix.get("turns") != 3.25
            or helix.get("turn_height") != 2.75
            or helix.get("handedness") != 1
            or helix.get("constraint_type") != 2):
        raise ValueError("HELIX identity or bounded payload mismatch")
    return {
        "entity": "HELIX", "type": 503, "handle": HELIX_HANDLE,
        "radius": 4.5, "turns": 3.25, "turnHeight": 2.75,
    }


def check_camera_entity(records: list[dict], version_name: str) -> dict:
    """Qualify CAMERA identity and its null optional VIEW reference."""
    if version_name == "AC1015":
        return {"supported": False}
    matches = [
        record for record in records
        if isinstance(record, dict)
        and record.get("entity") == "CAMERA"
        and record.get("type") == 542
    ]
    if len(matches) != 1:
        raise ValueError("CAMERA entity frame count mismatch")
    camera = matches[0]
    if (record_handle(camera) != CAMERA_HANDLE
            or camera.get("view") != [5, 0, 0, 0]):
        raise ValueError("CAMERA identity or VIEW reference mismatch")
    return {
        "supported": True, "entity": "CAMERA", "type": 542,
        "handle": CAMERA_HANDLE, "view": 0,
    }


def check_geo_position_marker(records: list[dict], version_name: str) -> dict:
    """Qualify the fixed-type GEOPOSITIONMARKER entity body."""
    if version_name in {"AC1015", "AC1018", "AC1021", "AC1024"}:
        return {"supported": False}
    matches = [
        record for record in records
        if isinstance(record, dict)
        and record_handle(record) == GEOPOSITIONMARKER_HANDLE
        and record.get("type") == 1164
    ]
    if len(matches) != 1:
        raise ValueError("GEOPOSITIONMARKER frame count mismatch")
    marker = matches[0]
    return {
        "supported": True, "object": "GEOPOSITIONMARKER", "type": 1164,
        "handle": GEOPOSITIONMARKER_HANDLE,
        "oracleObject": marker.get("object"),
    }


def check_express_text_entities(records: list[dict], version_name: str) -> dict:
    """Qualify RTEXT/ARCALIGNEDTEXT identity and bounded oracle payload.

    LibreDWG decodes the RTEXT stream consistently.  Its ARCALIGNEDTEXT
    decoder follows a different versioned string-stream layout after AC1018,
    so only the AC1015/AC1018 payload is asserted; newer versions retain
    independent type/handle identity evidence while local self-read remains
    authoritative for the full arc payload.
    """
    rtext_matches = [
        record for record in records
        if isinstance(record, dict)
        and record.get("entity") == "RTEXT"
        and record.get("type") == 521
    ]
    arc_matches = [
        record for record in records
        if isinstance(record, dict)
        and record.get("entity") == "ARCALIGNEDTEXT"
        and record.get("type") == 522
    ]
    if len(rtext_matches) != 1 or len(arc_matches) != 1:
        raise ValueError("RTEXT/ARCALIGNEDTEXT entity frame count mismatch")
    rtext = rtext_matches[0]
    arc = arc_matches[0]
    if (record_handle(rtext) != RTEXT_HANDLE
            or rtext.get("text_value") != "LOCAL_RTEXT"
            or rtext.get("pt") != [90.0, 91.0, 0.0]
            or rtext.get("extrusion") != [0.0, 0.0, 1.0]
            or rtext.get("height") != 2.0
            or rtext.get("flags") != 1):
        raise ValueError("RTEXT identity or bounded payload mismatch")
    if record_handle(arc) != ARCALIGNEDTEXT_HANDLE:
        raise ValueError("ARCALIGNEDTEXT identity mismatch")
    arc_payload_qualified = version_name in {"AC1015", "AC1018"}
    if arc_payload_qualified and (
            arc.get("text_value") != "LOCAL_ARC_TEXT"
            or arc.get("center") != [100.0, 100.0, 0.0]
            or arc.get("radius") != 10.0
            or arc.get("start_angle") != 0.25
            or arc.get("end_angle") != 1.25
            or arc.get("text_size") != "2"
            or arc.get("xscale") != "1"
            or arc.get("char_spacing") != "1"):
        raise ValueError("ARCALIGNEDTEXT bounded payload mismatch")
    return {
        "rtext": {
            "entity": "RTEXT", "type": 521,
            "handle": RTEXT_HANDLE, "text": "LOCAL_RTEXT",
        },
        "arcAlignedText": {
            "entity": "ARCALIGNEDTEXT", "type": 522,
            "handle": ARCALIGNEDTEXT_HANDLE,
            "text": arc.get("text_value") if arc_payload_qualified else None,
            "payloadQualified": arc_payload_qualified,
        },
    }


def check_associative_objects(records: list[dict], version_name: str) -> dict:
    """Qualify the AC1021+ DIMASSOC/EVALUATION_GRAPH object identities."""
    if version_name in {"AC1015", "AC1018"}:
        return {"supported": False}
    type_map = {
        "AC1021": (566, 567),
        "AC1024": (565, 566),
        "AC1027": (565, 566),
        "AC1032": (565, 566),
    }
    dim_type, graph_type = type_map[version_name]
    dim = find_record(records, "DIMASSOC", DIMASSOC_HANDLE)
    graph = find_record(records, "EVALUATION_GRAPH", EVALUATION_GRAPH_HANDLE)
    if (dim.get("type") != dim_type
            or owner_handle(dim) != 0x0C
            or dim.get("associativity") != 1
            or graph.get("type") != graph_type
            or owner_handle(graph) != 0x0C
            or graph.get("first_nodeid") != 96
            or graph.get("first_nodeid_copy") != 97):
        raise ValueError("DIMASSOC/EVALUATION_GRAPH identity mismatch")
    return {
        "supported": True,
        "dimensionAssociation": {
            "object": "DIMASSOC", "handle": DIMASSOC_HANDLE,
            "type": dim_type, "associativity": 1,
        },
        "evaluationGraph": {
            "object": "EVALUATION_GRAPH", "handle": EVALUATION_GRAPH_HANDLE,
            "type": graph_type, "firstNodeId": 96, "firstNodeIdCopy": 97,
        },
    }


def check_block_representation(records: list[dict], version_name: str) -> dict:
    """Qualify fixed BLOCKREPRESENTATIONDATA where LibreDWG exposes it."""
    if version_name in {"AC1015", "AC1018"}:
        return {"supported": False}
    matches = [
        record for record in records
        if isinstance(record, dict)
        and record_handle(record) == BLOCKREPRESENTATIONDATA_HANDLE
        and record.get("type") == 1120
    ]
    if len(matches) != 1 or owner_handle(matches[0]) != 0x0C:
        raise ValueError("BLOCKREPRESENTATIONDATA identity mismatch")
    return {
        "supported": True,
        "object": "BLOCKREPRESENTATIONDATA",
        "handle": BLOCKREPRESENTATIONDATA_HANDLE,
        "type": 1120,
        "oracleObject": matches[0].get("object"),
    }


def check_objects(payload: dict, version_name: str) -> dict:
    header = payload.get("FILEHEADER")
    if not isinstance(header, dict) or header.get("version") != version_name:
        raise ValueError(f"FILEHEADER version is not {version_name}")
    records = payload.get("OBJECTS")
    if not isinstance(records, list):
        raise ValueError("oracle JSON has no OBJECTS list")

    pointcloud_entities = check_pointcloud_entities(records, version_name)
    tolerance_entity = check_tolerance_entity(records)
    helix_entity = check_helix_entity(records)
    camera_entity = check_camera_entity(records, version_name)
    geo_position_marker = check_geo_position_marker(records, version_name)
    express_text_entities = check_express_text_entities(records, version_name)
    associative_objects = check_associative_objects(records, version_name)
    block_representation = check_block_representation(records, version_name)
    associative_discrepancies = []
    if associative_objects.get("supported"):
        associative_discrepancies.append(
            "LibreDWG 0.14 qualifies DIMASSOC/EVALUATION_GRAPH type/handle/owner "
            "identity and DIMASSOC associativity, but does not retain the local "
            "evaluation-graph node/edge arrays; those payloads remain local-self-read "
            "authoritative")
    block_representation_discrepancies = []
    if not block_representation.get("supported"):
        block_representation_discrepancies.append(
            "LibreDWG 0.14 does not expose the local BLOCKREPRESENTATIONDATA "
            "frame before AC1021; local self-read remains authoritative")
    else:
        block_representation_discrepancies.append(
            "LibreDWG 0.14 exposes BLOCKREPRESENTATIONDATA as UNKNOWN_OBJ; "
            "type/handle/owner identity is qualified while flag/block payload "
            "remains local-self-read authoritative")
    camera_discrepancies = []
    if not camera_entity.get("supported"):
        camera_discrepancies.append(
            "CAMERA emission is intentionally gated off for AC1015 because "
            "the legacy implicit next-entity chain cannot safely carry its "
            "class-542 frame; AC1018+ identity is independently qualified")
    geo_position_marker_discrepancies = []
    if not geo_position_marker.get("supported"):
        geo_position_marker_discrepancies.append(
            "GEOPOSITIONMARKER emission is intentionally gated off before "
            "AC1027 because its versioned marker body is not supported there")
    else:
        geo_position_marker_discrepancies.append(
            "LibreDWG 0.14 exposes the local GEOPOSITIONMARKER as "
            "UNKNOWN_OBJ; type/handle identity is qualified while marker "
            "body fields remain local-self-read authoritative")

    render_matrix = {}
    for kind, (object_name, handle, object_type) in RENDER_SETTINGS_KINDS.items():
        record = find_record(records, object_name, handle)
        if record.get("type") != object_type:
            raise ValueError(
                f"{kind} RENDERSETTINGS type is not {object_type}")
        render_matrix[kind] = {
            "object": object_name,
            "handle": handle,
            "type": object_type,
        }

    group = find_record(records, "GROUP", GROUP_HANDLE)
    if (group.get("name") != "LOCAL_GROUP"
            or not isinstance(group.get("groups"), list)
            or len(group["groups"]) != 1
            or owner_handle(group) != 0x0C):
        raise ValueError("GROUP name, owner, or member stream mismatch")

    dictionary = find_record(records, "DICTIONARY", DICTIONARY_HANDLE)
    expected_dictionary_items = 54 if version_name in {
        "AC1021", "AC1024", "AC1027", "AC1032"} else 52
    if (dictionary.get("numitems") != expected_dictionary_items
            or dictionary.get("is_hardowner") != 1
            or owner_handle(dictionary) != 0x0C):
        raise ValueError("custom DICTIONARY count, owner, or cloning mismatch")

    underlay_types = {
        "AC1015": {"PDFDEFINITION": 530, "DGNDEFINITION": 531,
                    "DWFDEFINITION": 543},
        "AC1018": {"PDFDEFINITION": 530, "DGNDEFINITION": 531,
                    "DWFDEFINITION": 543},
        "AC1021": {"PDFDEFINITION": 530, "DGNDEFINITION": 531,
                    "DWFDEFINITION": 543},
        "AC1024": {"PDFDEFINITION": 528, "DGNDEFINITION": 530,
                    "DWFDEFINITION": 531},
        "AC1027": {"PDFDEFINITION": 528, "DGNDEFINITION": 530,
                    "DWFDEFINITION": 531},
        "AC1032": {"PDFDEFINITION": 528, "DGNDEFINITION": 530,
                    "DWFDEFINITION": 531},
    }[version_name]
    underlay_handles = {
        "PDFDEFINITION": UNDERLAY_PDF_HANDLE,
        "DGNDEFINITION": UNDERLAY_DGN_HANDLE,
        "DWFDEFINITION": UNDERLAY_DWF_HANDLE,
    }
    underlays = {}
    for object_name, handle in underlay_handles.items():
        underlay = find_record(records, object_name, handle)
        expected_stem = object_name.removesuffix("DEFINITION")
        if (underlay.get("type") != underlay_types[object_name]
                or owner_handle(underlay) != DICTIONARY_HANDLE
                or underlay.get("filename") != f"LOCAL_{expected_stem}."
                + ("pdf" if expected_stem == "PDF" else
                   "dgn" if expected_stem == "DGN" else "dwf")
                or underlay.get("name") != f"LOCAL_{expected_stem}_SHEET"):
            raise ValueError(f"{object_name} type, owner, or payload mismatch")
        underlays[object_name] = {
            "object": object_name,
            "handle": handle,
            "type": underlay_types[object_name],
        }
    pointcloud_specs = [
        (POINTCLOUD_DEFINITION_HANDLE, 535, DICTIONARY_HANDLE),
        (POINTCLOUD_DEFINITION_EX_HANDLE, 536, DICTIONARY_HANDLE),
        (POINTCLOUD_REACTOR_HANDLE, 537, POINTCLOUD_DEFINITION_HANDLE),
        (POINTCLOUD_REACTOR_EX_HANDLE, 538, POINTCLOUD_DEFINITION_EX_HANDLE),
    ]
    pointcloud_frames = {}
    for handle, object_type, expected_owner in pointcloud_specs:
        matches = [
            record for record in records
            if isinstance(record, dict)
            and record_handle(record) == handle
            and record.get("type") == object_type
        ]
        if len(matches) != 1 or owner_handle(matches[0]) != expected_owner:
            raise ValueError(
                f"POINTCLOUD definition frame 0x{handle:X} identity/owner mismatch")
        pointcloud_frames[f"0x{handle:X}"] = {
            "handle": handle,
            "type": object_type,
            "owner": expected_owner,
            "object": matches[0].get("object"),
        }
    pointcloud_discrepancies = [
        "LibreDWG 0.14 exposes local POINTCLOUDDEFINITION frames as UNKNOWN_OBJ; type/handle/owner identity is qualified",
        "POINTCLOUDDEFINITION payload fields remain local-self-read authoritative while external point-cloud resources are absent",
    ]
    color_map_matches = [
        record for record in records
        if isinstance(record, dict)
        and record_handle(record) == POINTCLOUD_COLORMAP_HANDLE
        and record.get("type") == 540
    ]
    if len(color_map_matches) != 1 or owner_handle(color_map_matches[0]) != DICTIONARY_HANDLE:
        raise ValueError("POINTCLOUDCOLORMAP identity or owner mismatch")
    pointcloud_discrepancies.append(
        "LibreDWG 0.14 exposes local POINTCLOUDCOLORMAP as UNKNOWN_OBJ; type/handle/owner identity is qualified")
    navisworks_matches = [
        record for record in records
        if isinstance(record, dict)
        and record_handle(record) == NAVISWORKS_MODEL_DEF_HANDLE
        and record.get("type") == 539
    ]
    if len(navisworks_matches) != 1 or owner_handle(navisworks_matches[0]) != DICTIONARY_HANDLE:
        raise ValueError("NAVISWORKSMODELDEF identity or owner mismatch")
    pointcloud_discrepancies.append(
        "LibreDWG 0.14 exposes local NAVISWORKSMODELDEF as UNKNOWN_OBJ; type/handle/owner identity is qualified")
    pointcloud_discrepancies.append(
        "LibreDWG 0.14 exposes POINTCLOUD/POINTCLOUDEX as UNKNOWN_ENT; "
        "type/handle identity is qualified while payload fields remain "
        "local-self-read authoritative")
    sun_study = find_record(records, "SUNSTUDY", SUNSTUDY_HANDLE)
    if (sun_study.get("type") != 548
            or owner_handle(sun_study) != 0xA603
            or sun_study.get("class_version") != 1
            or sun_study.get("setup_name") != "LOCAL_SUNSTUDY"
            or sun_study.get("description") != "LOCAL_SUN_DESC"
            or sun_study.get("dates") != [{"julian_day": 2451545,
                                           "msecs": 3600000}]
            or sun_study.get("hours") != [1, 0, 1]
            or sun_study.get("spacing") != 1.5):
        raise ValueError("SUNSTUDY type, identity, or bounded payload mismatch")
    sun_study_discrepancies = [
        "LibreDWG 0.14 shifts SUNSTUDY owner/reference handle fields for the "
        "local carrier; class/name/date/hour/scalar identity is qualified"
    ]
    motion_path = find_record(records, "MOTIONPATH", MOTIONPATH_HANDLE)
    if (motion_path.get("type") != 552
            or owner_handle(motion_path) != DICTIONARY_HANDLE
            or motion_path.get("class_version") != 2):
        raise ValueError("MOTIONPATH type, identity, or owner mismatch")
    motion_path_discrepancies = [
        "LibreDWG 0.14 misdecodes local MOTIONPATH hard-pointer/frame payload; "
        "type/handle/owner/class identity is qualified"
    ]
    path_objects = []
    for object_name, object_type, handle in (
            ("CURVEPATH", 553, CURVE_PATH_HANDLE),
            ("POINTPATH", 554, POINT_PATH_HANDLE),
            ("OBJECT_PTR", 555, OBJECT_PTR_HANDLE)):
        matches = [
            record for record in records
            if isinstance(record, dict)
            and record_handle(record) == handle
            and record.get("type") == object_type
        ]
        if len(matches) != 1 or owner_handle(matches[0]) != DICTIONARY_HANDLE:
            raise ValueError(f"{object_name} type, handle, or owner mismatch")
        path_objects.append({
            "object": object_name, "handle": handle, "type": object_type,
            "oracleObject": matches[0].get("object"),
        })
    path_discrepancies = [
        "LibreDWG 0.14 exposes CURVEPATH/POINTPATH as UNKNOWN_OBJ and "
        "retains OBJECT_PTR by name; type/handle/owner identity is qualified "
        "while path payload fields remain local-self-read authoritative"
    ]
    partial_viewing_index = find_record(
        records, "PARTIAL_VIEWING_INDEX", PARTIAL_VIEWING_INDEX_HANDLE)
    partial_entries = partial_viewing_index.get("entries")
    if (partial_viewing_index.get("type") != 559
            or owner_handle(partial_viewing_index) != DICTIONARY_HANDLE
            or partial_viewing_index.get("has_entries") != 1
            or not isinstance(partial_entries, list)
            or len(partial_entries) != 2
            or not isinstance(partial_entries[0], dict)
            or partial_entries[0].get("extents_min") != [-1.0, -2.0, -3.0]
            or partial_entries[0].get("extents_max") != [10.0, 20.0, 30.0]):
        raise ValueError(
            "PARTIAL_VIEWING_INDEX type, owner, count, or extent mismatch")
    partial_viewing_index_discrepancies = [
        "LibreDWG 0.14 qualifies PARTIAL_VIEWING_INDEX type/handle/owner, "
        "entry count, and first extent pair; object-reference and later-entry "
        "fields are not stable and remain local-self-read authoritative"
    ]
    background_types = {
        "AC1015": {
            "SOLIDBACKGROUND": 544, "GRADIENTBACKGROUND": 545,
            "GROUNDPLANEBACKGROUND": 546, "IMAGEBACKGROUND": 547,
            "IBLBACKGROUND": 561, "SKYLIGHTBACKGROUND": 565,
        },
        "AC1018": {
            "SOLIDBACKGROUND": 544, "GRADIENTBACKGROUND": 545,
            "GROUNDPLANEBACKGROUND": 546, "IMAGEBACKGROUND": 547,
            "IBLBACKGROUND": 561, "SKYLIGHTBACKGROUND": 565,
        },
        "AC1021": {
            "SOLIDBACKGROUND": 544, "GRADIENTBACKGROUND": 545,
            "GROUNDPLANEBACKGROUND": 546, "IMAGEBACKGROUND": 547,
            "IBLBACKGROUND": 561, "SKYLIGHTBACKGROUND": 565,
        },
        "AC1024": {
            "SOLIDBACKGROUND": 543, "GRADIENTBACKGROUND": 544,
            "GROUNDPLANEBACKGROUND": 545, "IMAGEBACKGROUND": 546,
            "IBLBACKGROUND": 547, "SKYLIGHTBACKGROUND": 561,
        },
        "AC1027": {
            "SOLIDBACKGROUND": 543, "GRADIENTBACKGROUND": 544,
            "GROUNDPLANEBACKGROUND": 545, "IMAGEBACKGROUND": 546,
            "IBLBACKGROUND": 547, "SKYLIGHTBACKGROUND": 561,
        },
        "AC1032": {
            "SOLIDBACKGROUND": 543, "GRADIENTBACKGROUND": 544,
            "GROUNDPLANEBACKGROUND": 545, "IMAGEBACKGROUND": 546,
            "IBLBACKGROUND": 547, "SKYLIGHTBACKGROUND": 561,
        },
    }[version_name]
    background_handles = {
        "SOLIDBACKGROUND": SOLID_BACKGROUND_HANDLE,
        "GRADIENTBACKGROUND": GRADIENT_BACKGROUND_HANDLE,
        "GROUNDPLANEBACKGROUND": GROUNDPLANE_BACKGROUND_HANDLE,
        "IMAGEBACKGROUND": IMAGE_BACKGROUND_HANDLE,
        "IBLBACKGROUND": IBL_BACKGROUND_HANDLE,
        "SKYLIGHTBACKGROUND": SKYLIGHT_BACKGROUND_HANDLE,
    }
    backgrounds = {}
    for object_name, handle in background_handles.items():
        background = find_record(records, "UNKNOWN_OBJ", handle)
        if (background.get("type") != background_types[object_name]
                or owner_handle(background) != DICTIONARY_HANDLE):
            raise ValueError(
                f"{object_name} type, handle, or owner mismatch")
        backgrounds[object_name] = {
            "object": object_name,
            "handle": handle,
            "type": background_types[object_name],
            "oracleObject": background.get("object"),
        }
    background_discrepancies = [
        "LibreDWG 0.14 exposes all six BACKGROUND kinds as UNKNOWN_OBJ; "
        "version-specific type/handle/owner identity is qualified while "
        "kind payload fields remain local-self-read authoritative"
    ]
    sections = {}
    section_discrepancies = []
    if version_name in {"AC1021", "AC1024", "AC1027", "AC1032"}:
        section_manager = find_record(
            records, "SECTION_MANAGER", SECTION_MANAGER_HANDLE)
        section_links = section_manager.get("sections")
        if (section_manager.get("type") != 1321
                or owner_handle(section_manager) != DICTIONARY_HANDLE
                or section_manager.get("is_live") != 1
                or not isinstance(section_links, list)
                or len(section_links) != 1
                or section_links[0] != [5, 2,
                                         SECTION_SETTINGS_HANDLE,
                                         SECTION_SETTINGS_HANDLE]):
            raise ValueError("SECTION_MANAGER type, owner, or link mismatch")
        section_settings = find_record(
            records, "SECTION_SETTINGS", SECTION_SETTINGS_HANDLE)
        section_types = section_settings.get("types")
        if (section_settings.get("type") != 1322
                or owner_handle(section_settings) != DICTIONARY_HANDLE
                or section_settings.get("curr_type") != 1
                or not isinstance(section_types, list)
                or len(section_types) != 1
                or not isinstance(section_types[0], dict)
                or section_types[0].get("type") != 2
                or section_types[0].get("generation") != 3
                or section_types[0].get("destfile") != "LOCAL_SECTION.dwg"
                or section_types[0].get("destblock") != [4, 1, 0x17, 0x17]
                or not isinstance(section_types[0].get("sources"), list)
                or len(section_types[0]["sources"]) != 1
                or not isinstance(section_types[0].get("geom"), list)
                or len(section_types[0]["geom"]) != 1
                or section_types[0]["geom"][0].get("hexindex") != 4
                or section_types[0]["geom"][0].get("flags") != 5
                or section_types[0]["geom"][0].get("layer") != "LOCAL_LAYER"
                or section_types[0]["geom"][0].get("hatch_scale") != 1.25):
            raise ValueError("SECTION_SETTINGS type or bounded payload mismatch")
        sections = {
            "manager": {"object": "SECTION_MANAGER",
                        "handle": SECTION_MANAGER_HANDLE, "type": 1321},
            "settings": {"object": "SECTION_SETTINGS",
                          "handle": SECTION_SETTINGS_HANDLE, "type": 1322},
        }
        section_discrepancies.append(
            "LibreDWG 0.14 preserves SECTION_MANAGER/SECTION_SETTINGS "
            "identity and bounded settings while retaining opaque trailing bits")
    else:
        if any(record.get("object") in {"SECTION_MANAGER", "SECTION_SETTINGS"}
               and record_handle(record) in {
                   SECTION_MANAGER_HANDLE, SECTION_SETTINGS_HANDLE}
               for record in records if isinstance(record, dict)):
            raise ValueError("unsupported SECTION objects were published")
        section_discrepancies.append(
            "SECTION_MANAGER/SECTION_SETTINGS are intentionally gated off before AC1021")
    tv_vx = {}
    tv_vx_discrepancies = []
    modern_tv_vx = version_name in {"AC1021", "AC1024", "AC1027", "AC1032"}
    if modern_tv_vx:
        tv_device = find_record(
            records, "TVDEVICEPROPERTIES", TVDEVICEPROPERTIES_HANDLE)
        if (tv_device.get("type") != 1326
                or owner_handle(tv_device) != DICTIONARY_HANDLE
                or tv_device.get("flags") != 1
                or tv_device.get("max_regen_threads") != 2
                or tv_device.get("use_lut_palette") != 3
                or tv_device.get("alt_hlt") != 4
                or tv_device.get("alt_hltcolor") != 5
                or tv_device.get("geom_shader_usage") != 6
                or tv_device.get("blending_mode") != 7
                or tv_device.get("antialiasing_level") != 0.25
                or tv_device.get("bd2") != 0.75):
            raise ValueError("TVDEVICEPROPERTIES type, owner, or payload mismatch")
        tv_vx["TVDEVICEPROPERTIES"] = {
            "object": "TVDEVICEPROPERTIES",
            "handle": TVDEVICEPROPERTIES_HANDLE,
            "type": 1326,
        }
    else:
        # AC1015/AC1018 use a compact, file-local remap for the 1320s custom
        # classes.  LibreDWG names TVDEVICEPROPERTIES but reports the VX
        # carriers as UNKNOWN_OBJ; qualify their physical identity by handle,
        # owner, and a valid custom type instead of requiring the modern
        # ordinals.
        legacy_specs = {
            "TVDEVICEPROPERTIES": TVDEVICEPROPERTIES_HANDLE,
            "VXCONTROL": VXCONTROL_HANDLE,
            "VXTABLERECORD": VXTABLERECORD_HANDLE,
        }
        for object_name, handle in legacy_specs.items():
            matches = [
                record for record in records
                if isinstance(record, dict)
                and record_handle(record) == handle
            ]
            if len(matches) != 1 or owner_handle(matches[0]) != DICTIONARY_HANDLE:
                raise ValueError(
                    f"{object_name} legacy type, handle, or owner mismatch")
            observed_type = matches[0].get("type")
            if not isinstance(observed_type, int) or observed_type < 500:
                raise ValueError(f"{object_name} legacy custom type mismatch")
            if object_name == "TVDEVICEPROPERTIES":
                if matches[0].get("object") != object_name:
                    raise ValueError("TVDEVICEPROPERTIES legacy name mismatch")
            elif matches[0].get("object") != object_name:
                tv_vx_discrepancies.append(
                    f"LibreDWG 0.14 exposes local {object_name} as UNKNOWN_OBJ")
            tv_vx[object_name] = {
                "object": object_name,
                "handle": handle,
                "type": observed_type,
                "oracleObject": matches[0].get("object"),
            }
        tv_vx_discrepancies.append(
            "AC1015/AC1018 compact high custom-class ordinals are file-local; "
            "LibreDWG 0.14 qualifies TV/VX handle-owner identity while VX "
            "payload fields remain local-self-read authoritative")
    vx_specs = {
        "VXCONTROL": (VXCONTROL_HANDLE, 1327),
        "VXTABLERECORD": (VXTABLERECORD_HANDLE, 1328),
    }
    for object_name, (handle, object_type) in vx_specs.items():
        if version_name not in {"AC1021", "AC1024", "AC1027", "AC1032"}:
            continue
        matches = [
            record for record in records
            if isinstance(record, dict)
            and record_handle(record) == handle
            and record.get("type") == object_type
        ]
        if len(matches) != 1 or owner_handle(matches[0]) != DICTIONARY_HANDLE:
            raise ValueError(f"{object_name} type, handle, or owner mismatch")
        if matches[0].get("object") != object_name:
            tv_vx_discrepancies.append(
                f"LibreDWG 0.14 exposes local {object_name} as UNKNOWN_OBJ")
        tv_vx[object_name] = {
            "object": object_name,
            "handle": handle,
            "type": object_type,
            "oracleObject": matches[0].get("object"),
        }
    tv_vx_discrepancies.append(
        "VXCONTROL/VXTABLERECORD payload fields remain local-self-read "
        "authoritative while LibreDWG retains type/handle/owner identity")
    image_discrepancies = [
        "LibreDWG 0.14 does not expose the local IMAGE/IMAGEDEF entity and "
        "fixed-object frames in JSON; local self-read remains the authoritative "
        "IMAGE/IMAGEDEF/IMAGEDEF_REACTOR check"
    ]
    if version_name == "AC1015":
        image_discrepancies.append(
            "IMAGE/IMAGEDEF/IMAGEDEF_REACTOR emission is intentionally gated "
            "off for AC1015 pending a compatible legacy image wire layout")

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

    environment = find_record(
        records, "RENDERENVIRONMENT", RENDERENVIRONMENT_HANDLE)
    if (owner_handle(environment) != DICTIONARY_HANDLE
            or environment.get("type") != 550
            or environment.get("class_version") != 1
            or environment.get("fog_enabled") != 1
            or environment.get("fog_background_enabled") != 0
            or environment.get("fog_color_r") != 10
            or environment.get("fog_color_g") != 20
            or environment.get("fog_color_b") != 30
            or environment.get("fog_density_near") != 0.1
            or environment.get("fog_density_far") != 0.9
            or environment.get("fog_distance_near") != 2.0
            or environment.get("fog_distance_far") != 3.0
            or environment.get("environ_image_enabled") != 1
            or environment.get("environ_image_filename")
                != "LOCAL_RENDER_ENVIRONMENT"):
        raise ValueError("RENDERENVIRONMENT owner or bounded fields mismatch")

    global_settings = find_record(records, "RENDERGLOBAL", RENDERGLOBAL_HANDLE)
    if (owner_handle(global_settings) != DICTIONARY_HANDLE
            or global_settings.get("type") != 551
            or global_settings.get("class_version") != 1
            or global_settings.get("procedure") != 7
            or global_settings.get("destination") != 8
            or global_settings.get("save_filename") != "LOCAL_RENDER_GLOBAL"):
        raise ValueError("RENDERGLOBAL owner or bounded fields mismatch")

    entry = find_record(records, "RENDERENTRY", RENDERENTRY_HANDLE)
    if (owner_handle(entry) != DICTIONARY_HANDLE
            or entry.get("type") != 549
            or entry.get("class_version") != 1
            or entry.get("image_file_name") != "LOCAL_RENDER_ENTRY"
            or entry.get("dimension_x") != 11
            or entry.get("dimension_y") != 12
            or entry.get("start_year") != 1
            or entry.get("start_month") != 2
            or entry.get("start_day") != 3
            or entry.get("start_minute") != 4
            or entry.get("start_second") != 5
            or entry.get("start_msec") != 6
            or entry.get("render_time") != 1.5
            or entry.get("memory_amount") != 13
            or entry.get("material_count") != 14
            or entry.get("light_count") != 15
            or entry.get("triangle_count") != 16
            or entry.get("display_index") != 17):
        raise ValueError("RENDERENTRY owner or bounded fields mismatch")

    rapid = find_record(
        records, "RAPIDRTRENDERSETTINGS", RAPIDRTRENDERSETTINGS_HANDLE)
    if (owner_handle(rapid) != DICTIONARY_HANDLE
            or rapid.get("type") != 558
            or rapid.get("name") != "LOCAL_RENDER_RAPIDRT"
            or rapid.get("fog_enabled") != 1
            or rapid.get("fog_background_enabled") != 0
            or rapid.get("backfaces_enabled") != 1
            or rapid.get("display_index") != 9):
        raise ValueError("RAPIDRTRENDERSETTINGS owner or base fields mismatch")
    rapid_discrepancies = []
    if version_name in {"AC1015", "AC1018", "AC1032"}:
        expected_class_version = 2 if version_name == "AC1032" else 1
        if (rapid.get("class_version") != expected_class_version
                or rapid.get("rapidrt_version") != 2
                or rapid.get("render_target") != 3
                or rapid.get("render_level") != 4
                or rapid.get("render_time") != 5
                or rapid.get("lighting_model") != 6
                or rapid.get("filter_type") != 7
                or rapid.get("filter_width") != 0.25
                or rapid.get("filter_height") != 0.75):
            raise ValueError("RAPIDRTRENDERSETTINGS bounded fields mismatch")
    else:
        rapid_discrepancies.append(
            "LibreDWG 0.14 misdecodes RapidRT render fields for " + version_name)

    mental = find_record(
        records, "MENTALRAYRENDERSETTINGS", MENTALRAYRENDERSETTINGS_HANDLE)
    if (owner_handle(mental) != DICTIONARY_HANDLE
            or mental.get("type") != 557
            or mental.get("name") != "LOCAL_RENDER_MENTALRAY"
            or mental.get("description") != "LOCAL_MENTAL_DESC"
            or mental.get("fog_enabled") != 1
            or mental.get("fog_background_enabled") != 0
            or mental.get("backfaces_enabled") != 1
            or mental.get("environ_image_enabled") != 0
            or mental.get("display_index") != 2):
        raise ValueError("MENTALRAYRENDERSETTINGS owner or base fields mismatch")
    mental_discrepancies = []
    mental_expected = {
        "mr_version": 3,
        "sampling1": 4,
        "sampling2": 5,
        "sampling_mr_filter": 1,
        "sampling_filter1": 0.1,
        "sampling_filter2": 0.2,
        "sampling_contrast_color1": 0.3,
        "sampling_contrast_color2": 0.4,
        "sampling_contrast_color3": 0.5,
        "sampling_contrast_color4": 0.6,
        "shadow_mode": 2,
        "ray_trace_depth1": 6,
        "ray_trace_depth2": 7,
        "ray_trace_depth3": 8,
        "gi_sample_count": 9,
        "gi_sample_radius": 0.7,
        "gi_photons_per_light": 10,
        "photon_trace_depth1": 11,
        "photon_trace_depth2": 12,
        "photon_trace_depth3": 13,
        "fg_ray_count": 14,
        "fg_sample_radius1": 0.8,
        "fg_sample_radius2": 0.9,
        "light_luminance_scale": 1.0,
        "diagnostics_mode": 3,
        "diagnostics_grid_mode": 4,
        "diagnostics_grid_float": 1.1,
        "diagnostics_photon_mode": 5,
        "diagnostics_bsp_mode": 6,
        "energy_multiplier": 1.2,
    }
    if version_name in {"AC1015", "AC1018", "AC1021", "AC1024"}:
        mental_expected["class_version"] = 1
        mental_expected.update({
            "shadow_maps_enabled": 1,
            "ray_tracing_enabled": 0,
            "global_illumination_enabled": 1,
        })
    elif version_name == "AC1027":
        # LibreDWG 0.14 omits the class version and inverts these three
        # legacy flags while decoding the otherwise stable payload.
        mental_discrepancies.extend([
            "LibreDWG 0.14 omits MENTALRAYRENDERSETTINGS class_version for AC1027",
            "LibreDWG 0.14 inverts MENTALRAYRENDERSETTINGS shadow/ray/global flags for AC1027",
        ])
    else:
        # The R2018 reader currently loses alignment after the common header;
        # retain the bounded frame qualification and report the discrepancy.
        mental_discrepancies.append(
            "LibreDWG 0.14 misdecodes MENTALRAYRENDERSETTINGS payload for AC1032")
    if version_name != "AC1032":
        for key, expected in mental_expected.items():
            if mental.get(key) != expected:
                raise ValueError(
                    "MENTALRAYRENDERSETTINGS field mismatch: " + key)

    material = find_record(records, "MATERIAL", MATERIAL_HANDLE)
    if (owner_handle(material) != DICTIONARY_HANDLE
            or material.get("type") != 507
            or material.get("name") != "LOCAL_MATERIAL"
            or material.get("description") != "LOCAL_MATERIAL_DESC"):
        raise ValueError("MATERIAL owner or identity fields mismatch")
    material_discrepancies = [
        "MATERIAL visual-property fields are intentionally identity-only in libdxfrw"
    ]
    dbcolor_discrepancies = []
    if version_name == "AC1015":
        dbcolor_discrepancies.append(
            "DBCOLOR is intentionally unsupported before AC1018")
    else:
        dbcolor = find_record(records, "DBCOLOR", DBCOLOR_HANDLE)
        color = dbcolor.get("color")
        if (owner_handle(dbcolor) != DICTIONARY_HANDLE
                or dbcolor.get("type") != 563
                or not isinstance(color, dict)
                or color.get("index") != 256
                or color.get("rgb") != "c2123456"
                or color.get("flag") != 3):
            raise ValueError("DBCOLOR owner or bounded color fields mismatch")
        if version_name == "AC1018":
            if (color.get("name") != "LOCAL_COLOR"
                    or color.get("book_name") != "LOCAL_BOOK"):
                raise ValueError("DBCOLOR color-book fields mismatch")
        else:
            dbcolor_discrepancies.append(
                "LibreDWG 0.14 truncates DBCOLOR color-book names for "
                + version_name)

    light_list = find_record(records, "LIGHTLIST", LIGHTLIST_HANDLE)
    lights = light_list.get("lights")
    if (owner_handle(light_list) != DICTIONARY_HANDLE
            or light_list.get("type") != 508
            or light_list.get("class_version") != 1
            or not isinstance(lights, list)
            or len(lights) != 1):
        raise ValueError("LIGHTLIST owner, header, or count mismatch")
    light_list_discrepancies = []
    if version_name in {"AC1015", "AC1018"}:
        light_list_discrepancies.append(
            "LibreDWG 0.14 omits LIGHTLIST member name/handle for "
            + version_name)
    else:
        if lights[0].get("name") != "LOCAL_LIGHT":
            raise ValueError("LIGHTLIST member name mismatch")
        light_list_discrepancies.append(
            "LibreDWG 0.14 omits LIGHTLIST member handle for " + version_name)

    scale = find_record(records, "SCALE", SCALE_HANDLE)
    if (owner_handle(scale) != DICTIONARY_HANDLE
            or scale.get("type") != 509
            or scale.get("flag") != 0
            or scale.get("name") != "LOCAL_SCALE"
            or scale.get("paper_units") != 1.0
            or scale.get("drawing_units") != 48.0
            or scale.get("is_unit_scale") != 0):
        raise ValueError("SCALE owner or bounded ratio fields mismatch")

    id_buffer = find_record(records, "IDBUFFER", IDBUFFER_HANDLE)
    object_ids = id_buffer.get("obj_ids")
    if (owner_handle(id_buffer) != DICTIONARY_HANDLE
            or id_buffer.get("type") != 510
            or id_buffer.get("unknown") != 0
            or not isinstance(object_ids, list)
            or len(object_ids) != 1
            or not isinstance(object_ids[0], list)
            or len(object_ids[0]) < 3
            or object_ids[0][2] <= 0):
        raise ValueError("IDBUFFER owner, count, or handle mismatch")

    layer_index = find_record(records, "LAYER_INDEX", LAYER_INDEX_HANDLE)
    layer_entries = layer_index.get("entries")
    if (owner_handle(layer_index) != DICTIONARY_HANDLE
            or layer_index.get("type") != 511
            or layer_index.get("last_updated") != [100, 200]
            or not isinstance(layer_entries, list)
            or len(layer_entries) != 1
            or not isinstance(layer_entries[0], dict)
            or layer_entries[0].get("numlayers") != 1
            or layer_entries[0].get("name") != "LOCAL_LAYER"
            or not isinstance(layer_entries[0].get("handle"), list)
            or len(layer_entries[0]["handle"]) < 3
            or layer_entries[0]["handle"][2] <= 0):
        raise ValueError("LAYER_INDEX owner, count, name, or handle mismatch")

    spatial_index = find_record(records, "SPATIAL_INDEX", SPATIAL_INDEX_HANDLE)
    if (owner_handle(spatial_index) != DICTIONARY_HANDLE
            or spatial_index.get("type") != 517
            or spatial_index.get("last_updated") != [300, 400]):
        raise ValueError("SPATIAL_INDEX owner or timestamp mismatch")
    spatial_index_discrepancies = [
        "SPATIAL_INDEX opaque spatial payload is intentionally empty in local fixture"
    ]

    table_style_discrepancies = []
    if version_name in {"AC1015", "AC1018", "AC1021"}:
        table_style = find_record(records, "TABLESTYLE", TABLESTYLE_HANDLE)
        row_styles = table_style.get("rowstyles")
        if (owner_handle(table_style) != DICTIONARY_HANDLE
                or table_style.get("type") != 526
                or table_style.get("name") != "LOCAL_TABLESTYLE"
                or not isinstance(row_styles, list)
                or len(row_styles) != 3
                or any(not isinstance(row, dict)
                       or not isinstance(row.get("borders"), list)
                       or len(row["borders"]) != 6 for row in row_styles)):
            raise ValueError("TABLESTYLE owner, identity, or row payload mismatch")
        if version_name == "AC1021":
            table_style_discrepancies.append(
                "LibreDWG 0.14 misdecodes TABLESTYLE AC1021 row scalar fields")
    else:
        if any(record.get("object") == "TABLESTYLE"
               and record_handle(record) == TABLESTYLE_HANDLE
               for record in records if isinstance(record, dict)):
            raise ValueError("unsupported TABLESTYLE was published")
        table_style_discrepancies.append(
            "TABLESTYLE is intentionally unsupported after AC1021")

    spatial_filter = find_record(records, "SPATIAL_FILTER", SPATIAL_FILTER_HANDLE)
    boundary = spatial_filter.get("clip_verts")
    expected_spatial_filter_type = 527 if version_name in {
        "AC1015", "AC1018", "AC1021"} else 526
    if (owner_handle(spatial_filter) != DICTIONARY_HANDLE
            or spatial_filter.get("type") != expected_spatial_filter_type
            or not isinstance(boundary, list)
            or len(boundary) != 2
            or boundary[0] != [1.0, 2.0]
            or boundary[1] != [3.0, 4.0]
            or spatial_filter.get("extrusion") != [0.0, 0.0, 1.0]
            or spatial_filter.get("origin") != [10.0, 20.0, 30.0]
            or spatial_filter.get("display_boundary_on") != 1
            or spatial_filter.get("front_clip_on") != 1
            or spatial_filter.get("front_clip_z") != 5.0
            or spatial_filter.get("back_clip_on") != 0
            or not isinstance(spatial_filter.get("inverse_transform"), list)
            or len(spatial_filter["inverse_transform"]) != 12
            or not isinstance(spatial_filter.get("transform"), list)
            or len(spatial_filter["transform"]) != 12):
        raise ValueError("SPATIAL_FILTER owner or bounded payload mismatch")

    geodata = find_record(records, "GEODATA", GEODATA_HANDLE)
    expected_geodata_type = 528 if version_name in {
        "AC1015", "AC1018", "AC1021"} else 527
    if (geodata.get("type") != expected_geodata_type
            or record_handle(geodata) != GEODATA_HANDLE
            or owner_handle(geodata) != 0x17
            or geodata.get("xdicobjhandle") != [3, 2,
                                                   DICTIONARY_HANDLE,
                                                   DICTIONARY_HANDLE]):
        raise ValueError("GEODATA type, handle, owner, or xDictionary mismatch")
    geodata_discrepancies = []
    if version_name in {"AC1015", "AC1018"}:
        geodata_discrepancies.append(
            "LibreDWG 0.14 does not preserve the local version-1 GEODATA "
            "host/body fields for AC1015/AC1018; type, handle, owner, and "
            "xDictionary identity are qualified")
    else:
        if geodata.get("host_block") != [4, 1, 0x17, 0x17]:
            raise ValueError("GEODATA host-block handle mismatch")
        geodata_discrepancies.append(
            "LibreDWG 0.14 decodes local version-1 GEODATA body fields "
            "differently from libdxfrw; type/handle/owner/xDictionary "
            "identity is qualified")
        if version_name == "AC1021":
            geodata_discrepancies.append(
                "LibreDWG 0.14 omits the local version-1 GEODATA north "
                "direction field for AC1021")

    geodata_v2 = find_record(records, "GEODATA", GEODATA_V2_HANDLE)
    if (geodata_v2.get("type") != expected_geodata_type
            or record_handle(geodata_v2) != GEODATA_V2_HANDLE):
        raise ValueError("GEODATA v2 type or handle mismatch")
    if version_name in {"AC1024", "AC1027", "AC1032"}:
        if (owner_handle(geodata_v2) != 0x17
                or geodata_v2.get("host_block") != [4, 1, 0x17, 0x17]
                or geodata_v2.get("xdicobjhandle") != [3, 2,
                                                       DICTIONARY_HANDLE,
                                                       DICTIONARY_HANDLE]
                or geodata_v2.get("class_version") != 2
                or geodata_v2.get("coord_type") != 2
                or geodata_v2.get("design_pt") != [100.0, 200.0, 300.0]
                or geodata_v2.get("ref_pt") != [10.0, 20.0, 30.0]
                or geodata_v2.get("unit_scale_horiz") != 1.5
                or geodata_v2.get("units_value_horiz") != 2
                or geodata_v2.get("unit_scale_vert") != 2.5
                or geodata_v2.get("units_value_vert") != 3
                or geodata_v2.get("up_dir") != [0.0, 0.0, 1.0]
                or geodata_v2.get("north_dir") != [0.0, 1.0]
                or geodata_v2.get("scale_est") != 1
                or geodata_v2.get("user_scale_factor") != 1.25
                or geodata_v2.get("do_sea_level_corr") != 1
                or geodata_v2.get("sea_level_elev") != 4.5
                or geodata_v2.get("coord_proj_radius") != 6.5
                or geodata_v2.get("coord_system_def") != "LOCAL_COORD_SYS_V2"
                or geodata_v2.get("geo_rss_tag") != "LOCAL_GEO_TAG_V2"
                or geodata_v2.get("observation_from_tag") != "LOCAL_FROM_V2"
                or geodata_v2.get("observation_to_tag") != "LOCAL_TO_V2"
                or geodata_v2.get("observation_coverage_tag")
                    != "LOCAL_COVERAGE_V2"):
            raise ValueError("GEODATA v2 bounded payload mismatch")
    else:
        geodata_discrepancies.append(
            "LibreDWG 0.14 does not decode version-2 GEODATA fields reliably "
            "before AC1024")
    if version_name == "AC1021":
        if (geodata.get("has_civil_data") != 0
                or geodata_v2.get("has_civil_data") != 0):
            raise ValueError("GEODATA civil-data disposition changed")
        geodata_discrepancies.append(
            "GEODATA civil-data tail is intentionally unsupported for the "
            "local AC1021 carrier; no public DRW_GeoData fields model it")

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
        oracle_discrepancies = render_discrepancies + rapid_discrepancies + [
            "LibreDWG 0.14 omits DICTIONARYWDFLT item/default handles for "
            + version_name
        ]
    else:
        oracle_discrepancies = render_discrepancies + rapid_discrepancies

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
        MALFORMED_RENDERENVIRONMENT_HANDLE: "RENDERENVIRONMENT",
        MALFORMED_RENDERGLOBAL_HANDLE: "RENDERGLOBAL",
        MALFORMED_RENDERENTRY_HANDLE: "RENDERENTRY",
        MALFORMED_RAPIDRTRENDERSETTINGS_HANDLE: "RAPIDRTRENDERSETTINGS",
        MALFORMED_MENTALRAYRENDERSETTINGS_HANDLE: "MENTALRAYRENDERSETTINGS",
        MALFORMED_MATERIAL_HANDLE: "MATERIAL",
        MALFORMED_DBCOLOR_HANDLE: "DBCOLOR",
        MALFORMED_LIGHTLIST_HANDLE: "LIGHTLIST",
        MALFORMED_SCALE_HANDLE: "SCALE",
        MALFORMED_IDBUFFER_HANDLE: "IDBUFFER",
        MALFORMED_LAYER_INDEX_HANDLE: "LAYER_INDEX",
        MALFORMED_SPATIAL_INDEX_HANDLE: "SPATIAL_INDEX",
        MALFORMED_TABLESTYLE_HANDLE: "TABLESTYLE",
        MALFORMED_SPATIAL_FILTER_HANDLE: "SPATIAL_FILTER",
        MALFORMED_GEODATA_HANDLE: "GEODATA",
        MALFORMED_UNDERLAY_HANDLE: "PDFDEFINITION",
        MALFORMED_POINTCLOUD_HANDLE: "POINTCLOUDDEFINITION",
        MALFORMED_POINTCLOUD_COLORMAP_HANDLE: "POINTCLOUDCOLORMAP",
        MALFORMED_NAVISWORKS_MODEL_DEF_HANDLE: "NAVISWORKSMODELDEF",
        MALFORMED_SUNSTUDY_HANDLE: "SUNSTUDY",
        MALFORMED_MOTIONPATH_HANDLE: "MOTIONPATH",
        MALFORMED_CURVE_PATH_HANDLE: "CURVEPATH",
        MALFORMED_POINT_PATH_HANDLE: "POINTPATH",
        MALFORMED_OBJECT_PTR_HANDLE: "OBJECT_PTR",
        MALFORMED_PARTIAL_VIEWING_INDEX_HANDLE: "PARTIAL_VIEWING_INDEX",
        MALFORMED_BACKGROUND_HANDLE: "UNKNOWN_OBJ",
        MALFORMED_SECTION_HANDLE: "SECTION_SETTINGS",
        MALFORMED_TVDEVICEPROPERTIES_HANDLE: "TVDEVICEPROPERTIES",
        MALFORMED_VXCONTROL_HANDLE: "UNKNOWN_OBJ",
        MALFORMED_VXTABLERECORD_HANDLE: "UNKNOWN_OBJ",
        MALFORMED_IMAGE_HANDLE: "IMAGE",
    }
    if any(record_handle(record) in malformed_handles
           and record.get("object") == malformed_handles[record_handle(record)]
           for record in records if isinstance(record, dict)):
        raise ValueError("rolled-back malformed object was published")

    return {
        "version": version_name,
        "renderSettingsKinds": render_matrix,
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
            "RENDERENVIRONMENT": RENDERENVIRONMENT_HANDLE,
            "RENDERGLOBAL": RENDERGLOBAL_HANDLE,
            "RENDERENTRY": RENDERENTRY_HANDLE,
            "RAPIDRTRENDERSETTINGS": RAPIDRTRENDERSETTINGS_HANDLE,
            "MENTALRAYRENDERSETTINGS": MENTALRAYRENDERSETTINGS_HANDLE,
            "MATERIAL": MATERIAL_HANDLE,
            "DBCOLOR": DBCOLOR_HANDLE,
            "LIGHTLIST": LIGHTLIST_HANDLE,
            "SCALE": SCALE_HANDLE,
            "IDBUFFER": IDBUFFER_HANDLE,
            "LAYER_INDEX": LAYER_INDEX_HANDLE,
            "SPATIAL_INDEX": SPATIAL_INDEX_HANDLE,
            "TABLESTYLE": TABLESTYLE_HANDLE,
            "SPATIAL_FILTER": SPATIAL_FILTER_HANDLE,
            "GEODATA": GEODATA_HANDLE,
            "GEODATA_V2": GEODATA_V2_HANDLE,
            "PDFDEFINITION": UNDERLAY_PDF_HANDLE,
            "DGNDEFINITION": UNDERLAY_DGN_HANDLE,
            "DWFDEFINITION": UNDERLAY_DWF_HANDLE,
            "POINTCLOUDDEFINITION": POINTCLOUD_DEFINITION_HANDLE,
            "POINTCLOUDDEFINITIONEX": POINTCLOUD_DEFINITION_EX_HANDLE,
            "POINTCLOUDDEFREACTOR": POINTCLOUD_REACTOR_HANDLE,
            "POINTCLOUDDEFREACTOREX": POINTCLOUD_REACTOR_EX_HANDLE,
            "POINTCLOUDCOLORMAP": POINTCLOUD_COLORMAP_HANDLE,
            "NAVISWORKSMODELDEF": NAVISWORKS_MODEL_DEF_HANDLE,
            "SUNSTUDY": SUNSTUDY_HANDLE,
            "MOTIONPATH": MOTIONPATH_HANDLE,
            "CURVEPATH": CURVE_PATH_HANDLE,
            "POINTPATH": POINT_PATH_HANDLE,
            "OBJECT_PTR": OBJECT_PTR_HANDLE,
            "PARTIAL_VIEWING_INDEX": PARTIAL_VIEWING_INDEX_HANDLE,
            "SOLIDBACKGROUND": SOLID_BACKGROUND_HANDLE,
            "GRADIENTBACKGROUND": GRADIENT_BACKGROUND_HANDLE,
            "GROUNDPLANEBACKGROUND": GROUNDPLANE_BACKGROUND_HANDLE,
            "IMAGEBACKGROUND": IMAGE_BACKGROUND_HANDLE,
            "IBLBACKGROUND": IBL_BACKGROUND_HANDLE,
            "SKYLIGHTBACKGROUND": SKYLIGHT_BACKGROUND_HANDLE,
            "SECTION_MANAGER": SECTION_MANAGER_HANDLE,
            "SECTION_SETTINGS": SECTION_SETTINGS_HANDLE,
            "TVDEVICEPROPERTIES": TVDEVICEPROPERTIES_HANDLE,
            "VXCONTROL": VXCONTROL_HANDLE,
            "VXTABLERECORD": VXTABLERECORD_HANDLE,
            "IMAGE": IMAGE_HANDLE,
            "IMAGEDEF_REACTOR": IMAGE_HANDLE + 1,
            "DICTIONARYWDFLT": DICTIONARYWDFLT_HANDLE,
        },
        "underlayDefinitions": underlays,
        "pointCloudDefinitions": pointcloud_frames,
        "pointCloudEntities": pointcloud_entities,
        "toleranceEntity": tolerance_entity,
        "helixEntity": helix_entity,
        "cameraEntity": camera_entity,
        "geoPositionMarker": geo_position_marker,
        "expressTextEntities": express_text_entities,
        "associativeObjects": associative_objects,
        "blockRepresentationData": block_representation,
        "sunStudy": {
            "object": "SUNSTUDY", "handle": SUNSTUDY_HANDLE, "type": 548,
        },
        "motionPath": {
            "object": "MOTIONPATH", "handle": MOTIONPATH_HANDLE, "type": 552,
        },
        "pathObjects": path_objects,
        "partialViewingIndex": {
            "object": "PARTIAL_VIEWING_INDEX",
            "handle": PARTIAL_VIEWING_INDEX_HANDLE,
            "type": 559,
            "entryCount": len(partial_entries),
        },
        "backgrounds": backgrounds,
        "sections": sections,
        "tvVxObjects": tv_vx,
        "objectStatus": "qualified",
        "oracleDiscrepancies": (oracle_discrepancies + mental_discrepancies
                                 + material_discrepancies
                                 + dbcolor_discrepancies
                                 + light_list_discrepancies
                                 + spatial_index_discrepancies
                                 + table_style_discrepancies
                                 + geodata_discrepancies
                                 + pointcloud_discrepancies
                                 + sun_study_discrepancies
                                 + motion_path_discrepancies
                                 + path_discrepancies
                                 + partial_viewing_index_discrepancies
                                 + background_discrepancies
                                 + section_discrepancies
                                 + tv_vx_discrepancies
                                 + image_discrepancies
        + associative_discrepancies
        + block_representation_discrepancies
        + camera_discrepancies + geo_position_marker_discrepancies),
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
             "ownerhandle": [4, 1, 0x0C, 0x0C], "numitems": 54,
             "is_hardowner": 1},
            {"object": "SECTION_MANAGER",
             "handle": [0, 1, SECTION_MANAGER_HANDLE],
             "ownerhandle": [4, 1, DICTIONARY_HANDLE, DICTIONARY_HANDLE],
             "type": 1321, "is_live": 1,
             "sections": [[5, 2, SECTION_SETTINGS_HANDLE,
                           SECTION_SETTINGS_HANDLE]]},
            {"object": "SECTION_SETTINGS",
             "handle": [0, 1, SECTION_SETTINGS_HANDLE],
             "ownerhandle": [4, 1, DICTIONARY_HANDLE, DICTIONARY_HANDLE],
             "type": 1322, "curr_type": 1,
             "types": [{
                 "type": 2, "generation": 3,
                 "sources": [[5, 1, 0x1234, 0x1234]],
                 "destblock": [4, 1, 0x17, 0x17],
                 "destfile": "LOCAL_SECTION.dwg",
                 "geom": [{
                     "hexindex": 4, "flags": 5,
                     "layer": "LOCAL_LAYER", "hatch_scale": 1.25,
                 }],
             }]},
            {"object": "TVDEVICEPROPERTIES",
             "handle": [0, 1, TVDEVICEPROPERTIES_HANDLE],
             "ownerhandle": [4, 1, DICTIONARY_HANDLE, DICTIONARY_HANDLE],
             "type": 1326, "flags": 1, "max_regen_threads": 2,
             "use_lut_palette": 3, "alt_hlt": 4, "alt_hltcolor": 5,
             "geom_shader_usage": 6, "blending_mode": 7,
             "antialiasing_level": 0.25, "bd2": 0.75},
            {"object": "UNKNOWN_OBJ",
             "handle": [0, 1, VXCONTROL_HANDLE],
             "ownerhandle": [4, 1, DICTIONARY_HANDLE, DICTIONARY_HANDLE],
             "type": 1327},
            {"object": "UNKNOWN_OBJ",
             "handle": [0, 1, VXTABLERECORD_HANDLE],
             "ownerhandle": [4, 1, DICTIONARY_HANDLE, DICTIONARY_HANDLE],
             "type": 1328},
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
            {"object": "RENDERENVIRONMENT",
             "handle": [0, 1, RENDERENVIRONMENT_HANDLE],
             "ownerhandle": [4, 1, DICTIONARY_HANDLE, DICTIONARY_HANDLE],
             "type": 550, "class_version": 1, "fog_enabled": 1,
             "fog_background_enabled": 0, "fog_color_r": 10,
             "fog_color_g": 20, "fog_color_b": 30,
             "fog_density_near": 0.1, "fog_density_far": 0.9,
             "fog_distance_near": 2.0, "fog_distance_far": 3.0,
             "environ_image_enabled": 1,
             "environ_image_filename": "LOCAL_RENDER_ENVIRONMENT"},
            {"object": "RENDERGLOBAL",
             "handle": [0, 1, RENDERGLOBAL_HANDLE],
             "ownerhandle": [4, 1, DICTIONARY_HANDLE, DICTIONARY_HANDLE],
             "type": 551, "class_version": 1, "procedure": 7,
             "destination": 8, "save_filename": "LOCAL_RENDER_GLOBAL"},
            {"object": "RENDERENTRY",
             "handle": [0, 1, RENDERENTRY_HANDLE],
             "ownerhandle": [4, 1, DICTIONARY_HANDLE, DICTIONARY_HANDLE],
             "type": 549, "class_version": 1,
             "image_file_name": "LOCAL_RENDER_ENTRY",
             "dimension_x": 11, "dimension_y": 12, "start_year": 1,
             "start_month": 2, "start_day": 3, "start_minute": 4,
             "start_second": 5, "start_msec": 6, "render_time": 1.5,
             "memory_amount": 13, "material_count": 14,
             "light_count": 15, "triangle_count": 16,
             "display_index": 17},
            {"object": "RAPIDRTRENDERSETTINGS",
             "handle": [0, 1, RAPIDRTRENDERSETTINGS_HANDLE],
             "ownerhandle": [4, 1, DICTIONARY_HANDLE, DICTIONARY_HANDLE],
             "type": 558, "class_version": 1,
             "name": "LOCAL_RENDER_RAPIDRT", "fog_enabled": 1,
             "fog_background_enabled": 0, "backfaces_enabled": 1,
             "display_index": 9, "rapidrt_version": 2,
             "render_target": 3, "render_level": 4, "render_time": 5,
             "lighting_model": 6, "filter_type": 7,
             "filter_width": 0.25, "filter_height": 0.75},
            {"object": "MENTALRAYRENDERSETTINGS",
             "handle": [0, 1, MENTALRAYRENDERSETTINGS_HANDLE],
             "ownerhandle": [4, 1, DICTIONARY_HANDLE, DICTIONARY_HANDLE],
             "type": 557, "class_version": 1,
             "name": "LOCAL_RENDER_MENTALRAY",
             "description": "LOCAL_MENTAL_DESC", "fog_enabled": 1,
             "fog_background_enabled": 0, "backfaces_enabled": 1,
             "environ_image_enabled": 0, "display_index": 2,
             "mr_version": 3, "sampling1": 4, "sampling2": 5,
             "sampling_mr_filter": 1, "sampling_filter1": 0.1,
             "sampling_filter2": 0.2,
             "sampling_contrast_color1": 0.3,
             "sampling_contrast_color2": 0.4,
             "sampling_contrast_color3": 0.5,
             "sampling_contrast_color4": 0.6, "shadow_mode": 2,
             "shadow_maps_enabled": 1, "ray_tracing_enabled": 0,
             "ray_trace_depth1": 6, "ray_trace_depth2": 7,
             "ray_trace_depth3": 8, "global_illumination_enabled": 1,
             "gi_sample_count": 9, "gi_sample_radius": 0.7,
             "gi_photons_per_light": 10, "photon_trace_depth1": 11,
             "photon_trace_depth2": 12, "photon_trace_depth3": 13,
             "fg_ray_count": 14, "fg_sample_radius1": 0.8,
             "fg_sample_radius2": 0.9, "light_luminance_scale": 1.0,
             "diagnostics_mode": 3, "diagnostics_grid_mode": 4,
             "diagnostics_grid_float": 1.1, "diagnostics_photon_mode": 5,
             "diagnostics_bsp_mode": 6, "energy_multiplier": 1.2},
            {"object": "MATERIAL", "handle": [0, 1, MATERIAL_HANDLE],
             "ownerhandle": [4, 1, DICTIONARY_HANDLE, DICTIONARY_HANDLE],
             "type": 507, "name": "LOCAL_MATERIAL",
             "description": "LOCAL_MATERIAL_DESC"},
            {"object": "DBCOLOR", "handle": [0, 1, DBCOLOR_HANDLE],
             "ownerhandle": [4, 1, DICTIONARY_HANDLE, DICTIONARY_HANDLE],
             "type": 563,
             "color": {"index": 256, "rgb": "c2123456", "flag": 3,
                       "name": "LOCAL_COLOR", "book_name": "LOCAL_BOOK"}},
            {"object": "LIGHTLIST", "handle": [0, 1, LIGHTLIST_HANDLE],
             "ownerhandle": [4, 1, DICTIONARY_HANDLE, DICTIONARY_HANDLE],
             "type": 508, "class_version": 1,
             "lights": [{"handle": [0, 0], "name": "LOCAL_LIGHT"}]},
            {"object": "SCALE", "handle": [0, 1, SCALE_HANDLE],
             "ownerhandle": [4, 1, DICTIONARY_HANDLE, DICTIONARY_HANDLE],
             "type": 509, "flag": 0, "name": "LOCAL_SCALE",
             "paper_units": 1.0, "drawing_units": 48.0,
             "is_unit_scale": 0},
            {"object": "IDBUFFER", "handle": [0, 1, IDBUFFER_HANDLE],
             "ownerhandle": [4, 1, DICTIONARY_HANDLE, DICTIONARY_HANDLE],
             "type": 510, "unknown": 0,
             "obj_ids": [[4, 2, 0x1234, 0x1234]]},
            {"object": "LAYER_INDEX", "handle": [0, 1, LAYER_INDEX_HANDLE],
             "ownerhandle": [4, 1, DICTIONARY_HANDLE, DICTIONARY_HANDLE],
             "type": 511, "last_updated": [100, 200],
             "entries": [{"numlayers": 1, "name": "LOCAL_LAYER",
                          "handle": [5, 2, 0xCC00, 0xCC00]}]},
            {"object": "SPATIAL_INDEX",
             "handle": [0, 1, SPATIAL_INDEX_HANDLE],
             "ownerhandle": [4, 1, DICTIONARY_HANDLE, DICTIONARY_HANDLE],
             "type": 517, "last_updated": [300, 400]},
            {"object": "SPATIAL_FILTER",
             "handle": [0, 1, SPATIAL_FILTER_HANDLE],
             "ownerhandle": [4, 1, DICTIONARY_HANDLE, DICTIONARY_HANDLE],
             "type": 526, "clip_verts": [[1.0, 2.0], [3.0, 4.0]],
             "extrusion": [0.0, 0.0, 1.0],
             "origin": [10.0, 20.0, 30.0],
             "display_boundary_on": 1, "front_clip_on": 1,
             "front_clip_z": 5.0, "back_clip_on": 0,
             "inverse_transform": [0.0] * 12,
             "transform": [0.0] * 12},
            {"object": "GEODATA",
             "handle": [0, 1, GEODATA_HANDLE],
             "ownerhandle": [4, 1, 0x17, 0x17],
             "xdicobjhandle": [3, 2, DICTIONARY_HANDLE, DICTIONARY_HANDLE],
             "host_block": [4, 1, 0x17, 0x17], "type": 527,
             "has_civil_data": 0},
            {"object": "GEODATA", "handle": [0, 1, GEODATA_V2_HANDLE],
             "ownerhandle": [4, 1, 0x17, 0x17],
             "xdicobjhandle": [3, 2, DICTIONARY_HANDLE, DICTIONARY_HANDLE],
             "host_block": [4, 1, 0x17, 0x17], "type": 527,
             "class_version": 2, "coord_type": 2,
             "design_pt": [100.0, 200.0, 300.0],
             "ref_pt": [10.0, 20.0, 30.0],
             "unit_scale_horiz": 1.5, "units_value_horiz": 2,
             "unit_scale_vert": 2.5, "units_value_vert": 3,
             "up_dir": [0.0, 0.0, 1.0], "north_dir": [0.0, 1.0],
             "scale_est": 1, "user_scale_factor": 1.25,
             "do_sea_level_corr": 1, "sea_level_elev": 4.5,
             "coord_proj_radius": 6.5,
             "coord_system_def": "LOCAL_COORD_SYS_V2",
             "geo_rss_tag": "LOCAL_GEO_TAG_V2",
             "observation_from_tag": "LOCAL_FROM_V2",
             "observation_to_tag": "LOCAL_TO_V2",
             "observation_coverage_tag": "LOCAL_COVERAGE_V2",
             "has_civil_data": 0},
            {"object": "PDFDEFINITION",
             "handle": [0, 1, UNDERLAY_PDF_HANDLE],
             "ownerhandle": [4, 1, DICTIONARY_HANDLE, DICTIONARY_HANDLE],
             "type": 528, "filename": "LOCAL_PDF.pdf",
             "name": "LOCAL_PDF_SHEET"},
            {"object": "DGNDEFINITION",
             "handle": [0, 1, UNDERLAY_DGN_HANDLE],
             "ownerhandle": [4, 1, DICTIONARY_HANDLE, DICTIONARY_HANDLE],
             "type": 530, "filename": "LOCAL_DGN.dgn",
             "name": "LOCAL_DGN_SHEET"},
            {"object": "DWFDEFINITION",
             "handle": [0, 1, UNDERLAY_DWF_HANDLE],
             "ownerhandle": [4, 1, DICTIONARY_HANDLE, DICTIONARY_HANDLE],
             "type": 531, "filename": "LOCAL_DWF.dwf",
             "name": "LOCAL_DWF_SHEET"},
            {"object": "UNKNOWN_OBJ",
             "handle": [0, 1, POINTCLOUD_DEFINITION_HANDLE],
             "ownerhandle": [4, 1, DICTIONARY_HANDLE, DICTIONARY_HANDLE],
             "type": 535},
            {"object": "UNKNOWN_OBJ",
             "handle": [0, 1, POINTCLOUD_DEFINITION_EX_HANDLE],
             "ownerhandle": [4, 1, DICTIONARY_HANDLE, DICTIONARY_HANDLE],
             "type": 536},
            {"object": "UNKNOWN_OBJ",
             "handle": [0, 1, POINTCLOUD_REACTOR_HANDLE],
             "ownerhandle": [4, 1, POINTCLOUD_DEFINITION_HANDLE,
                             POINTCLOUD_DEFINITION_HANDLE],
             "type": 537},
            {"object": "UNKNOWN_OBJ",
             "handle": [0, 1, POINTCLOUD_REACTOR_EX_HANDLE],
             "ownerhandle": [4, 1, POINTCLOUD_DEFINITION_EX_HANDLE,
                             POINTCLOUD_DEFINITION_EX_HANDLE],
             "type": 538},
            {"object": "UNKNOWN_OBJ",
             "handle": [0, 1, POINTCLOUD_COLORMAP_HANDLE],
             "ownerhandle": [4, 1, DICTIONARY_HANDLE, DICTIONARY_HANDLE],
             "type": 540},
            {"object": "UNKNOWN_OBJ",
             "handle": [0, 1, NAVISWORKS_MODEL_DEF_HANDLE],
             "ownerhandle": [4, 1, DICTIONARY_HANDLE, DICTIONARY_HANDLE],
             "type": 539},
            {"object": "SUNSTUDY", "handle": [0, 2, SUNSTUDY_HANDLE],
             "ownerhandle": [5, 2, 0xA603, 0xA603], "type": 548,
             "class_version": 1, "setup_name": "LOCAL_SUNSTUDY",
             "description": "LOCAL_SUN_DESC",
             "dates": [{"julian_day": 2451545, "msecs": 3600000}],
             "hours": [1, 0, 1], "spacing": 1.5},
            {"object": "MOTIONPATH", "handle": [0, 2, MOTIONPATH_HANDLE],
             "ownerhandle": [4, 2, DICTIONARY_HANDLE, DICTIONARY_HANDLE],
             "type": 552, "class_version": 2},
            {"object": "UNKNOWN_OBJ", "handle": [0, 2, CURVE_PATH_HANDLE],
             "ownerhandle": [4, 2, DICTIONARY_HANDLE, DICTIONARY_HANDLE],
             "type": 553},
            {"object": "UNKNOWN_OBJ", "handle": [0, 2, POINT_PATH_HANDLE],
             "ownerhandle": [4, 2, DICTIONARY_HANDLE, DICTIONARY_HANDLE],
             "type": 554},
            {"object": "OBJECT_PTR", "handle": [0, 2, OBJECT_PTR_HANDLE],
             "ownerhandle": [4, 2, DICTIONARY_HANDLE, DICTIONARY_HANDLE],
             "type": 555},
            {"object": "PARTIAL_VIEWING_INDEX",
             "handle": [0, 2, PARTIAL_VIEWING_INDEX_HANDLE],
             "ownerhandle": [4, 2, DICTIONARY_HANDLE, DICTIONARY_HANDLE],
             "type": 559, "has_entries": 1,
             "entries": [
                 {"extents_min": [-1.0, -2.0, -3.0],
                  "extents_max": [10.0, 20.0, 30.0], "object": [0, 0]},
                 {"extents_min": [1.0, 1.0, 0.0],
                  "extents_max": [0.0, 0.0, 0.0], "object": [0, 0]},
             ]},
            {"object": "UNKNOWN_OBJ", "handle": [0, 2, SOLID_BACKGROUND_HANDLE],
             "ownerhandle": [4, 2, DICTIONARY_HANDLE, DICTIONARY_HANDLE],
             "type": 543},
            {"object": "UNKNOWN_OBJ", "handle": [0, 2, GRADIENT_BACKGROUND_HANDLE],
             "ownerhandle": [4, 2, DICTIONARY_HANDLE, DICTIONARY_HANDLE],
             "type": 544},
            {"object": "UNKNOWN_OBJ", "handle": [0, 2, GROUNDPLANE_BACKGROUND_HANDLE],
             "ownerhandle": [4, 2, DICTIONARY_HANDLE, DICTIONARY_HANDLE],
             "type": 545},
            {"object": "UNKNOWN_OBJ", "handle": [0, 2, IMAGE_BACKGROUND_HANDLE],
             "ownerhandle": [4, 2, DICTIONARY_HANDLE, DICTIONARY_HANDLE],
             "type": 546},
            {"object": "UNKNOWN_OBJ", "handle": [0, 2, IBL_BACKGROUND_HANDLE],
             "ownerhandle": [4, 2, DICTIONARY_HANDLE, DICTIONARY_HANDLE],
             "type": 547},
            {"object": "UNKNOWN_OBJ", "handle": [0, 2, SKYLIGHT_BACKGROUND_HANDLE],
             "ownerhandle": [4, 2, DICTIONARY_HANDLE, DICTIONARY_HANDLE],
             "type": 561},
            {"entity": "UNKNOWN_ENT", "handle": [0, 2, 0xD925],
             "type": 533},
            {"entity": "TOLERANCE", "handle": [0, 2, 0xEC20],
             "type": 46, "text_value": "LOCAL_TOLERANCE",
             "ins_pt": [73.0, 74.0, 0.0],
             "x_direction": [1.0, 0.0, 0.0],
             "extrusion": [0.0, 0.0, 1.0],
             "dimstyle": [5, 1, 21, 21]},
            {"entity": "HELIX", "handle": [0, 2, HELIX_HANDLE],
             "type": 503, "scenario": 1, "degree": 2,
             "knots": [0.0, 0.0, 0.0, 1.0, 1.0, 1.0],
             "ctrl_pts": [
                 {"x": 73.0, "y": 74.0, "z": 0.0},
                 {"x": 75.0, "y": 76.0, "z": 0.0},
                 {"x": 77.0, "y": 78.0, "z": 0.0}],
             "major_version": 1, "maint_version": 2,
             "axis_base_pt": [70.0, 71.0, 0.0],
             "start_pt": [72.0, 73.0, 0.0],
             "axis_vector": [0.0, 0.0, 1.0],
            "radius": 4.5, "turns": 3.25, "turn_height": 2.75,
            "handedness": 1, "constraint_type": 2},
            {"entity": "CAMERA", "handle": [0, 2, CAMERA_HANDLE],
             "type": 542, "view": [5, 0, 0, 0]},
            {"object": "UNKNOWN_OBJ",
             "handle": [0, 2, GEOPOSITIONMARKER_HANDLE], "type": 1164},
            {"entity": "RTEXT", "handle": [0, 2, RTEXT_HANDLE],
             "type": 521, "text_value": "LOCAL_RTEXT",
             "pt": [90.0, 91.0, 0.0],
             "extrusion": [0.0, 0.0, 1.0], "height": 2.0, "flags": 1},
            {"entity": "ARCALIGNEDTEXT",
             "handle": [0, 2, ARCALIGNEDTEXT_HANDLE], "type": 522,
             "text_value": "LOCAL_ARC_TEXT",
             "center": [100.0, 100.0, 0.0], "radius": 10.0,
             "start_angle": 0.25, "end_angle": 1.25,
             "text_size": "2", "xscale": "1", "char_spacing": "1"},
            {"object": "DIMASSOC",
             "handle": [0, 2, DIMASSOC_HANDLE], "type": 565,
             "ownerhandle": [4, 1, 0x0C, 0x0C], "associativity": 1},
            {"object": "EVALUATION_GRAPH",
             "handle": [0, 2, EVALUATION_GRAPH_HANDLE], "type": 566,
             "ownerhandle": [4, 1, 0x0C, 0x0C],
             "first_nodeid": 96, "first_nodeid_copy": 97},
            {"object": "UNKNOWN_OBJ",
             "handle": [0, 2, BLOCKREPRESENTATIONDATA_HANDLE], "type": 1120,
             "ownerhandle": [4, 1, 0x0C, 0x0C]},
        ],
    }
    summary = check_objects(payload, "AC1024")
    geo_marker_summary = check_geo_position_marker(payload["OBJECTS"], "AC1027")
    if set(summary.get("renderSettingsKinds", {})) != set(RENDER_SETTINGS_KINDS):
        raise AssertionError("aggregate RENDERSETTINGS kind matrix is incomplete")
    if summary.get("pointCloudEntities") != [{
            "entity": "POINTCLOUD", "type": 533, "handle": 0xD925,
            "oracleEntity": "UNKNOWN_ENT"}]:
        raise AssertionError("POINTCLOUD entity identity was not qualified")
    if summary.get("toleranceEntity") != {
            "entity": "TOLERANCE", "type": 46, "handle": 0xEC20,
            "text": "LOCAL_TOLERANCE"}:
        raise AssertionError("TOLERANCE entity identity was not qualified")
    if summary.get("helixEntity") != {
            "entity": "HELIX", "type": 503, "handle": HELIX_HANDLE,
            "radius": 4.5, "turns": 3.25, "turnHeight": 2.75}:
        raise AssertionError("HELIX entity identity was not qualified")
    if summary.get("cameraEntity") != {
            "supported": True, "entity": "CAMERA", "type": 542,
            "handle": CAMERA_HANDLE, "view": 0}:
        raise AssertionError("CAMERA entity identity was not qualified")
    if geo_marker_summary != {
            "supported": True, "object": "GEOPOSITIONMARKER", "type": 1164,
            "handle": GEOPOSITIONMARKER_HANDLE,
            "oracleObject": "UNKNOWN_OBJ"}:
        raise AssertionError("GEOPOSITIONMARKER identity was not qualified")
    if summary.get("expressTextEntities") != {
            "rtext": {"entity": "RTEXT", "type": 521,
                      "handle": RTEXT_HANDLE, "text": "LOCAL_RTEXT"},
            "arcAlignedText": {"entity": "ARCALIGNEDTEXT", "type": 522,
                               "handle": ARCALIGNEDTEXT_HANDLE,
                               "text": None,
                               "payloadQualified": False}}:
        raise AssertionError("RTEXT/ARCALIGNEDTEXT identity was not qualified")
    if summary.get("associativeObjects") != {
            "supported": True,
            "dimensionAssociation": {
                "object": "DIMASSOC", "handle": DIMASSOC_HANDLE,
                "type": 565, "associativity": 1},
            "evaluationGraph": {
                "object": "EVALUATION_GRAPH", "handle": EVALUATION_GRAPH_HANDLE,
                "type": 566, "firstNodeId": 96, "firstNodeIdCopy": 97}}:
        raise AssertionError("DIMASSOC/EVALUATION_GRAPH identity was not qualified")
    if summary.get("blockRepresentationData") != {
            "supported": True, "object": "BLOCKREPRESENTATIONDATA",
            "handle": BLOCKREPRESENTATIONDATA_HANDLE, "type": 1120,
            "oracleObject": "UNKNOWN_OBJ"}:
        raise AssertionError("BLOCKREPRESENTATIONDATA identity was not qualified")
    if [frame["object"] for frame in summary.get("pathObjects", [])] != [
            "CURVEPATH", "POINTPATH", "OBJECT_PTR"]:
        raise AssertionError("path-object identity was not qualified")
    if summary.get("partialViewingIndex") != {
            "object": "PARTIAL_VIEWING_INDEX",
            "handle": PARTIAL_VIEWING_INDEX_HANDLE,
            "type": 559,
            "entryCount": 2}:
        raise AssertionError("PARTIAL_VIEWING_INDEX evidence was not qualified")
    expected_backgrounds = {
        "SOLIDBACKGROUND": (SOLID_BACKGROUND_HANDLE, 543),
        "GRADIENTBACKGROUND": (GRADIENT_BACKGROUND_HANDLE, 544),
        "GROUNDPLANEBACKGROUND": (GROUNDPLANE_BACKGROUND_HANDLE, 545),
        "IMAGEBACKGROUND": (IMAGE_BACKGROUND_HANDLE, 546),
        "IBLBACKGROUND": (IBL_BACKGROUND_HANDLE, 547),
        "SKYLIGHTBACKGROUND": (SKYLIGHT_BACKGROUND_HANDLE, 561),
    }
    actual_backgrounds = {
        name: (frame["handle"], frame["type"])
        for name, frame in summary.get("backgrounds", {}).items()
    }
    if actual_backgrounds != expected_backgrounds:
        raise AssertionError("BACKGROUND identity was not qualified")
    expected_sections = {
        "manager": (SECTION_MANAGER_HANDLE, 1321),
        "settings": (SECTION_SETTINGS_HANDLE, 1322),
    }
    actual_sections = {
        name: (frame["handle"], frame["type"])
        for name, frame in summary.get("sections", {}).items()
    }
    if actual_sections != expected_sections:
        raise AssertionError("SECTION identity was not qualified")
    expected_tv_vx = {
        "TVDEVICEPROPERTIES": (TVDEVICEPROPERTIES_HANDLE, 1326),
        "VXCONTROL": (VXCONTROL_HANDLE, 1327),
        "VXTABLERECORD": (VXTABLERECORD_HANDLE, 1328),
    }
    actual_tv_vx = {
        name: (frame["handle"], frame["type"])
        for name, frame in summary.get("tvVxObjects", {}).items()
    }
    if actual_tv_vx != expected_tv_vx:
        raise AssertionError("TV/VX object identity was not qualified")
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
    try:
        bad = json.loads(json.dumps(payload))
        bad["OBJECTS"].append({"object": "RENDERENVIRONMENT",
                                "handle": [0, 1,
                                            MALFORMED_RENDERENVIRONMENT_HANDLE]})
        check_objects(bad, "AC1024")
    except ValueError:
        pass
    else:
        raise AssertionError("malformed RENDERENVIRONMENT was not rejected")
    try:
        bad = json.loads(json.dumps(payload))
        bad["OBJECTS"].append({"object": "RENDERGLOBAL",
                                "handle": [0, 1,
                                            MALFORMED_RENDERGLOBAL_HANDLE]})
        check_objects(bad, "AC1024")
    except ValueError:
        pass
    else:
        raise AssertionError("malformed RENDERGLOBAL was not rejected")
    try:
        bad = json.loads(json.dumps(payload))
        bad["OBJECTS"].append({"object": "RENDERENTRY",
                                "handle": [0, 1,
                                            MALFORMED_RENDERENTRY_HANDLE]})
        check_objects(bad, "AC1024")
    except ValueError:
        pass
    else:
        raise AssertionError("malformed RENDERENTRY was not rejected")
    try:
        bad = json.loads(json.dumps(payload))
        bad["OBJECTS"].append({"object": "RAPIDRTRENDERSETTINGS",
                                "handle": [0, 1,
                                            MALFORMED_RAPIDRTRENDERSETTINGS_HANDLE]})
        check_objects(bad, "AC1024")
    except ValueError:
        pass
    else:
        raise AssertionError("malformed RAPIDRTRENDERSETTINGS was not rejected")
    try:
        bad = json.loads(json.dumps(payload))
        bad["OBJECTS"].append({"object": "MENTALRAYRENDERSETTINGS",
                                "handle": [0, 1,
                                            MALFORMED_MENTALRAYRENDERSETTINGS_HANDLE]})
        check_objects(bad, "AC1024")
    except ValueError:
        pass
    else:
        raise AssertionError("malformed MENTALRAYRENDERSETTINGS was not rejected")
    try:
        bad = json.loads(json.dumps(payload))
        bad["OBJECTS"].append({"object": "MATERIAL",
                                "handle": [0, 1, MALFORMED_MATERIAL_HANDLE]})
        check_objects(bad, "AC1024")
    except ValueError:
        pass
    else:
        raise AssertionError("malformed MATERIAL was not rejected")
    try:
        bad = json.loads(json.dumps(payload))
        bad["OBJECTS"].append({"object": "DBCOLOR",
                                "handle": [0, 1, MALFORMED_DBCOLOR_HANDLE]})
        check_objects(bad, "AC1024")
    except ValueError:
        pass
    else:
        raise AssertionError("malformed DBCOLOR was not rejected")
    try:
        bad = json.loads(json.dumps(payload))
        bad["OBJECTS"].append({"object": "LIGHTLIST",
                                "handle": [0, 1, MALFORMED_LIGHTLIST_HANDLE]})
        check_objects(bad, "AC1024")
    except ValueError:
        pass
    else:
        raise AssertionError("malformed LIGHTLIST was not rejected")
    try:
        bad = json.loads(json.dumps(payload))
        bad["OBJECTS"].append({"object": "SCALE",
                                "handle": [0, 1, MALFORMED_SCALE_HANDLE]})
        check_objects(bad, "AC1024")
    except ValueError:
        pass
    else:
        raise AssertionError("malformed SCALE was not rejected")
    try:
        bad = json.loads(json.dumps(payload))
        bad["OBJECTS"].append({"object": "IDBUFFER",
                                "handle": [0, 1, MALFORMED_IDBUFFER_HANDLE]})
        check_objects(bad, "AC1024")
    except ValueError:
        pass
    else:
        raise AssertionError("malformed IDBUFFER was not rejected")
    try:
        bad = json.loads(json.dumps(payload))
        bad["OBJECTS"].append({"object": "LAYER_INDEX",
                                "handle": [0, 1, MALFORMED_LAYER_INDEX_HANDLE]})
        check_objects(bad, "AC1024")
    except ValueError:
        pass
    else:
        raise AssertionError("malformed LAYER_INDEX was not rejected")
    try:
        bad = json.loads(json.dumps(payload))
        bad["OBJECTS"].append({"object": "SPATIAL_INDEX",
                                "handle": [0, 1, MALFORMED_SPATIAL_INDEX_HANDLE]})
        check_objects(bad, "AC1024")
    except ValueError:
        pass
    else:
        raise AssertionError("malformed SPATIAL_INDEX was not rejected")
    try:
        bad = json.loads(json.dumps(payload))
        bad["OBJECTS"].append({"object": "PDFDEFINITION",
                                "handle": [0, 1, MALFORMED_UNDERLAY_HANDLE]})
        check_objects(bad, "AC1024")
    except ValueError:
        pass
    else:
        raise AssertionError("malformed UNDERLAYDEFINITION was not rejected")
    try:
        bad = json.loads(json.dumps(payload))
        bad["OBJECTS"].append({"object": "POINTCLOUDDEFINITION",
                                "handle": [0, 1, MALFORMED_POINTCLOUD_HANDLE]})
        check_objects(bad, "AC1024")
    except ValueError:
        pass
    else:
        raise AssertionError("malformed POINTCLOUDDEFINITION was not rejected")
    try:
        bad = json.loads(json.dumps(payload))
        bad["OBJECTS"].append({"object": "POINTCLOUDCOLORMAP",
                                "handle": [0, 1,
                                            MALFORMED_POINTCLOUD_COLORMAP_HANDLE]})
        check_objects(bad, "AC1024")
    except ValueError:
        pass
    else:
        raise AssertionError("malformed POINTCLOUDCOLORMAP was not rejected")
    try:
        bad = json.loads(json.dumps(payload))
        bad["OBJECTS"].append({"object": "NAVISWORKSMODELDEF",
                                "handle": [0, 1,
                                            MALFORMED_NAVISWORKS_MODEL_DEF_HANDLE]})
        check_objects(bad, "AC1024")
    except ValueError:
        pass
    else:
        raise AssertionError("malformed NAVISWORKSMODELDEF was not rejected")
    try:
        bad = json.loads(json.dumps(payload))
        bad["OBJECTS"].append({"object": "SUNSTUDY",
                                "handle": [0, 1, MALFORMED_SUNSTUDY_HANDLE]})
        check_objects(bad, "AC1024")
    except ValueError:
        pass
    else:
        raise AssertionError("malformed SUNSTUDY was not rejected")
    try:
        bad = json.loads(json.dumps(payload))
        bad["OBJECTS"].append({"object": "MOTIONPATH",
                                "handle": [0, 1, MALFORMED_MOTIONPATH_HANDLE]})
        check_objects(bad, "AC1024")
    except ValueError:
        pass
    else:
        raise AssertionError("malformed MOTIONPATH was not rejected")
    try:
        bad = json.loads(json.dumps(payload))
        bad["OBJECTS"].append({"object": "CURVEPATH",
                                "handle": [0, 1, MALFORMED_CURVE_PATH_HANDLE],
                                "type": 553})
        check_objects(bad, "AC1024")
    except ValueError:
        pass
    else:
        raise AssertionError("malformed CURVEPATH was not rejected")
    try:
        bad = json.loads(json.dumps(payload))
        bad["OBJECTS"].append({"object": "POINTPATH",
                                "handle": [0, 1, MALFORMED_POINT_PATH_HANDLE],
                                "type": 554})
        check_objects(bad, "AC1024")
    except ValueError:
        pass
    else:
        raise AssertionError("malformed POINTPATH was not rejected")
    try:
        bad = json.loads(json.dumps(payload))
        bad["OBJECTS"].append({"object": "OBJECT_PTR",
                                "handle": [0, 1, MALFORMED_OBJECT_PTR_HANDLE],
                                "type": 555})
        check_objects(bad, "AC1024")
    except ValueError:
        pass
    else:
        raise AssertionError("malformed OBJECT_PTR was not rejected")
    try:
        bad = json.loads(json.dumps(payload))
        bad["OBJECTS"].append({
            "object": "PARTIAL_VIEWING_INDEX",
            "handle": [0, 1, MALFORMED_PARTIAL_VIEWING_INDEX_HANDLE],
            "type": 559})
        check_objects(bad, "AC1024")
    except ValueError:
        pass
    else:
        raise AssertionError("malformed PARTIAL_VIEWING_INDEX was not rejected")
    try:
        bad = json.loads(json.dumps(payload))
        bad["OBJECTS"].append({
            "object": "UNKNOWN_OBJ",
            "handle": [0, 1, MALFORMED_BACKGROUND_HANDLE]})
        check_objects(bad, "AC1024")
    except ValueError:
        pass
    else:
        raise AssertionError("malformed BACKGROUND was not rejected")
    try:
        bad = json.loads(json.dumps(payload))
        bad["OBJECTS"].append({"entity": "UNKNOWN_ENT",
                                "handle": [0, 2, 0xD927], "type": 533})
        check_objects(bad, "AC1024")
    except ValueError:
        pass
    else:
        raise AssertionError("duplicate POINTCLOUD entity was not rejected")
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
