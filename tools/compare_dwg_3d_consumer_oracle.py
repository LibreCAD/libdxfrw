#!/usr/bin/env python3
"""Compare a narrow DWG callback subset with LibreDWG's JSON reader.

This is an optional local qualification helper, not a CTest dependency. It
compares fields by entity handle so callback order is irrelevant. The accepted
scope is intentionally limited to one AutoCAD-authored AC1015 (R2000) planar
3D POLYLINE sample, one locally generated AC1015 topology control, AC1024
(R2010) INSERT placement and SPLINE fit data and one sample's LINE endpoints,
plus AC1021 (R2007) LINE endpoints, 3DFACE corners/edge flags, and ARC,
CIRCLE, ELLIPSE, and two legacy POLYLINE_MESH records in one sample of
unverified provenance. A hash-pinned AutoCAD 2016-authored POINT matrix
compares one nonzero-Z POINT across
AC1014/AC1015/AC1018/AC1021/AC1024/AC1027, and checks the five available paired
source DXFs. The local topology control exercises nonzero-Z 3D POLYLINE,
legacy POLYLINE_MESH, and PFACE but does not qualify AutoCAD interoperability.
Modeler, surface, and other version/family fields are not compared here.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import pathlib
import subprocess
import sys
from typing import Any


class OracleError(RuntimeError):
    pass


AUTOCAD_POINT3D_PROFILES: dict[str, dict[str, str | None]] = {
    "point3d_R14.dwg": {
        "version": "AC1014",
        "sha256": "7e6b1378b053f20de2cf9d153da3485e6475d0cd5845d5a8144b09cfe17cdc46",
        "dxf_sha256": None,
    },
    "point3d_2000.dwg": {
        "version": "AC1015",
        "sha256": "af51e9d5c75a9fd2a014a80bb03633faca93de2a54278e0b403a17ee1b3d7083",
        "dxf_sha256": "3ef77eb893b58d0e96f903f3ebad93f5cf4b5d60994c00ca0d2f943a3931cc34",
    },
    "point3d_2004.dwg": {
        "version": "AC1018",
        "sha256": "ba256436ee4bb1e777296699654d9ad872c5c27fcda0e9b535868c584f4cfde5",
        "dxf_sha256": "c2f40fbb2e362c2eeb86bcc4fb9989eea445d4740334d79623bfa99cfea43085",
    },
    "point3d_2007.dwg": {
        "version": "AC1021",
        "sha256": "4d46ed43787fe97f8eef17e529396a4c33fef7c1d22c40816b2e61bfc6e89474",
        "dxf_sha256": "2afd335ab2364e16bffef8c7640c8a5399ca25926f4f0a6af27fe04e5de35eb7",
    },
    "point3d_2010.dwg": {
        "version": "AC1024",
        "sha256": "9819e04c3ae5e75fb4aa0c6265e77421faa833acec0ebbb60feb34f019259b19",
        "dxf_sha256": "1bccc1155424d48fe7d79de991db9d3dd5b9e818298e50c1e73a6579d6132cdf",
    },
    "point3d_2013.dwg": {
        "version": "AC1027",
        "sha256": "839297daa100c318d5afbfa128145d2c4782300650c86b2ab58b34cf6fe1aba4",
        "dxf_sha256": "bf1e050ecc8aa9cf09471f5bcac75f0c7844bbc895b7eb76f72489bde2567fbb",
    },
}


def is_json_word_character(value: str) -> bool:
    return value.isalnum() or value == "_"


def reject_nonstandard_json_constant(value: str) -> None:
    raise OracleError(f"non-standard JSON number {value}")


def normalize_bare_nan_tokens(value: str) -> str:
    """Replace non-standard bare NaN tokens without touching JSON strings."""
    output: list[str] = []
    in_string = False
    escaped = False
    index = 0
    while index < len(value):
        char = value[index]
        if in_string:
            output.append(char)
            if escaped:
                escaped = False
            elif char == "\\":
                escaped = True
            elif char == '"':
                in_string = False
            index += 1
            continue
        if char == '"':
            in_string = True
            output.append(char)
            index += 1
            continue

        token_start = index
        token_end = index
        if char in "+-" and value[index + 1:index + 4].lower() == "nan":
            token_end = index + 4
        elif value[index:index + 3].lower() == "nan":
            token_end = index + 3
        if token_end > token_start:
            before = value[token_start - 1] if token_start else ""
            after = value[token_end] if token_end < len(value) else ""
            if (not is_json_word_character(before)
                    and not is_json_word_character(after)):
                output.append("null")
                index = token_end
                continue

        output.append(char)
        index += 1
    return "".join(output)


def run_json(command: list[str], label: str) -> dict[str, Any]:
    result = subprocess.run(command, capture_output=True, text=True, check=False)
    if result.returncode != 0:
        detail = result.stderr.strip() or result.stdout.strip()
        raise OracleError(f"{label} failed ({result.returncode}): {detail}")
    json_text = (normalize_bare_nan_tokens(result.stdout)
                 if label == "LibreDWG dwgread" else result.stdout)
    try:
        value = json.loads(json_text,
                           parse_constant=reject_nonstandard_json_constant)
    except json.JSONDecodeError as error:
        raise OracleError(f"{label} did not return JSON: {error}") from error
    if not isinstance(value, dict):
        raise OracleError(f"{label} returned a non-object JSON root")
    return value


def finite_number(value: Any, label: str) -> float:
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        raise OracleError(f"{label} is not numeric: {value!r}")
    result = float(value)
    if not math.isfinite(result):
        raise OracleError(f"{label} is not finite")
    return result


def compare_number(actual: Any, expected: Any, label: str) -> None:
    lhs = finite_number(actual, label + " (libdxfrw)")
    rhs = finite_number(expected, label + " (LibreDWG)")
    if not math.isclose(lhs, rhs, rel_tol=1e-13, abs_tol=1e-12):
        raise OracleError(f"{label} differs: libdxfrw={lhs!r}, LibreDWG={rhs!r}")


def compare_vector(actual: Any, expected: Any, label: str) -> None:
    if not isinstance(actual, list) or not isinstance(expected, list):
        raise OracleError(f"{label} is not a vector on both readers")
    if len(actual) != len(expected):
        raise OracleError(f"{label} length differs: {len(actual)} != {len(expected)}")
    for index, (lhs, rhs) in enumerate(zip(actual, expected)):
        compare_number(lhs, rhs, f"{label}[{index}]")


def compare_point(actual: Any, expected: Any, label: str) -> None:
    if not isinstance(actual, dict) or set(actual) != {"x", "y", "z"}:
        raise OracleError(f"{label} is not a typed XYZ point in libdxfrw output")
    compare_vector([actual[axis] for axis in ("x", "y", "z")], expected, label)


def compare_point3d_witness(
        row: dict[str, Any], expected_xyz: list[float],
        expected_handle: str) -> dict[str, Any]:
    external = row["external"]
    fields = row["adapter"]["fields"]
    actual_handle = handle_key(external.get("handle"), "LibreDWG POINT")
    if actual_handle != expected_handle:
        raise OracleError(
            f"unexpected POINT handle {actual_handle!r}; "
            f"expected {expected_handle!r}")
    external_xyz = [external.get(axis) for axis in ("x", "y", "z")]
    compare_vector(external_xyz, expected_xyz, "POINT.expectedWcsXYZ")
    compare_point(fields.get("basePoint"), external_xyz, "POINT.basePoint")
    compare_point(fields.get("basePoint"), expected_xyz,
                  "POINT.expectedBasePoint")
    expected_extrusion = [0.0, 0.0, 1.0]
    compare_vector(external.get("extrusion"), expected_extrusion,
                   "POINT.expectedExtrusion (LibreDWG)")
    compare_point(fields.get("extrusion"), external.get("extrusion"),
                  "POINT.extrusion")
    return {"handle": actual_handle, "wcsXYZ": expected_xyz,
            "extrusion": expected_extrusion}


def compare_point3d_reference_dxf(
        rows: dict[str, dict[str, Any]], expected_xyz: list[float],
        expected_handle: str) -> dict[str, Any]:
    if set(rows) != {expected_handle}:
        raise OracleError(
            "paired source DXF POINT handle differs; "
            f"actual={sorted(rows)}, expected={[expected_handle]}")
    fields = rows[expected_handle]["fields"]
    compare_point(fields.get("basePoint"), expected_xyz,
                  "paired DXF POINT.expectedBasePoint")
    compare_point(fields.get("extrusion"), [0.0, 0.0, 1.0],
                  "paired DXF POINT.expectedExtrusion")
    return {"handle": expected_handle, "wcsXYZ": expected_xyz,
            "extrusion": [0.0, 0.0, 1.0]}


def handle_key(value: Any, label: str) -> str:
    if isinstance(value, list) and value and isinstance(value[-1], int):
        return format(value[-1], "x")
    raise OracleError(f"{label} has no decoded numeric handle: {value!r}")


def index_external(document: dict[str, Any], entity: str) -> dict[str, dict[str, Any]]:
    objects = document.get("OBJECTS")
    if not isinstance(objects, list):
        raise OracleError("LibreDWG JSON has no OBJECTS array")
    rows: dict[str, dict[str, Any]] = {}
    for value in objects:
        if not isinstance(value, dict) or value.get("entity") != entity:
            continue
        key = handle_key(value.get("handle"), f"LibreDWG {entity}")
        if key in rows:
            raise OracleError(f"duplicate LibreDWG {entity} handle {key}")
        rows[key] = value
    return rows


def index_adapter(document: dict[str, Any], entity: str) -> dict[str, dict[str, Any]]:
    status = document.get("status", {})
    if status.get("outcome") != "success" or status.get("operationSucceeded") is not True:
        raise OracleError(f"libdxfrw DWG read did not complete successfully: {status!r}")
    records = document.get("records")
    if not isinstance(records, list):
        raise OracleError("libdxfrw adapter JSON has no records array")
    rows: dict[str, dict[str, Any]] = {}
    for value in records:
        if (not isinstance(value, dict) or value.get("kind") != "entity"
                or value.get("entity") != entity):
            continue
        key = value.get("sourceHandle")
        if not isinstance(key, str):
            raise OracleError(f"libdxfrw {entity} has no source handle")
        key = key.lower().lstrip("0") or "0"
        if key in rows:
            raise OracleError(f"duplicate libdxfrw {entity} handle {key}")
        fields = value.get("fields")
        if not isinstance(fields, list):
            raise OracleError(f"libdxfrw {entity} {key} has no typed field list")
        rows[key] = {
            "record": value,
            "fields": {field.get("name"): field.get("value")
                       for field in fields if isinstance(field, dict)},
        }
    return rows


def compare_entity_set(external: dict[str, dict[str, Any]],
                       adapter: dict[str, dict[str, Any]], entity: str,
                       expected_count: int | None) -> list[dict[str, Any]]:
    if not external:
        raise OracleError(f"LibreDWG returned no {entity} rows")
    if set(external) != set(adapter):
        missing = sorted(set(external) - set(adapter))
        extra = sorted(set(adapter) - set(external))
        raise OracleError(
            f"{entity} handle sets differ; absent in libdxfrw={missing}, "
            f"absent in LibreDWG={extra}")
    if expected_count is not None and len(external) != expected_count:
        raise OracleError(
            f"unexpected {entity} count {len(external)}; expected {expected_count}")
    return [
        {"handle": key, "external": external[key], "adapter": adapter[key]}
        for key in sorted(external, key=lambda item: int(item, 16))
    ]


def compare_insert(row: dict[str, Any]) -> None:
    external = row["external"]
    fields = row["adapter"]["fields"]
    compare_point(fields.get("insertionPoint"), external.get("ins_pt"),
                  "INSERT.insertionPoint")
    compare_vector([fields.get("xScale"), fields.get("yScale"),
                    fields.get("zScale")], external.get("scale"), "INSERT.scale")
    compare_number(fields.get("rotationAngle"), external.get("rotation"),
                   "INSERT.rotation")
    compare_point(fields.get("extrusion"), external.get("extrusion"),
                  "INSERT.extrusion")


def compare_spline(row: dict[str, Any]) -> None:
    external = row["external"]
    fields = row["adapter"]["fields"]
    compare_number(fields.get("degree"), external.get("degree"), "SPLINE.degree")
    compare_number(fields.get("scenario"), external.get("scenario"),
                   "SPLINE.scenario")
    compare_number(fields.get("fitTolerance"), external.get("fit_tol"),
                   "SPLINE.fitTolerance")
    compare_vector(
        [fields.get("startTangent", {}).get(axis) for axis in ("x", "y", "z")],
        external.get("beg_tan_vec"), "SPLINE.startTangent")
    compare_vector(
        [fields.get("endTangent", {}).get(axis) for axis in ("x", "y", "z")],
        external.get("end_tan_vec"), "SPLINE.endTangent")
    fit_points = external.get("fit_pts")
    if not isinstance(fit_points, list):
        raise OracleError("LibreDWG SPLINE row has no fit_pts array")
    compare_number(fields.get("fitPointCount"), len(fit_points),
                   "SPLINE.fitPointCount")
    for index, point in enumerate(fit_points):
        adapter_point = fields.get(f"fitPoint.{index}.position")
        compare_point(adapter_point, point, f"SPLINE.fitPoint[{index}]")


def compare_3dface(row: dict[str, Any]) -> None:
    external = row["external"]
    fields = row["adapter"]["fields"]
    for index in range(1, 5):
        compare_point(fields.get(f"corner.{index - 1}"),
                      external.get(f"corner{index}"),
                      f"3DFACE.corner{index}")

    has_no_flags = external.get("has_no_flags")
    flags = external.get("invis_flags")
    if isinstance(has_no_flags, bool) or has_no_flags not in (0, 1):
        raise OracleError("LibreDWG 3DFACE has no valid has_no_flags indicator")
    if has_no_flags == 1:
        if flags is not None:
            raise OracleError(
                "LibreDWG 3DFACE has flags with has_no_flags=1")
        flags = 0
    elif flags is None:
        raise OracleError("LibreDWG 3DFACE omits required invisible-edge flags")
    numeric_flags = finite_number(flags, "3DFACE.invisibleEdgeFlags (LibreDWG)")
    if (not numeric_flags.is_integer() or numeric_flags < 0
            or numeric_flags > 0x0F):
        raise OracleError(f"LibreDWG 3DFACE has invalid edge flags: {flags!r}")
    compare_number(fields.get("invisibleEdgeFlags"), int(numeric_flags),
                   "3DFACE.invisibleEdgeFlags")


def compare_line(row: dict[str, Any]) -> None:
    external = row["external"]
    fields = row["adapter"]["fields"]
    compare_point(fields.get("start"), external.get("start"), "LINE.start")
    compare_point(fields.get("end"), external.get("end"), "LINE.end")
    compare_number(fields.get("thickness"), external.get("thickness"),
                   "LINE.thickness")
    compare_point(fields.get("extrusion"), external.get("extrusion"),
                  "LINE.extrusion")


def compare_ellipse(row: dict[str, Any]) -> None:
    external = row["external"]
    fields = row["adapter"]["fields"]
    compare_point(fields.get("center"), external.get("center"),
                  "ELLIPSE.center")
    compare_point(fields.get("majorAxis"), external.get("sm_axis"),
                  "ELLIPSE.majorAxis")
    compare_point(fields.get("extrusion"), external.get("extrusion"),
                  "ELLIPSE.extrusion")
    compare_number(fields.get("ratio"), external.get("axis_ratio"),
                   "ELLIPSE.ratio")
    compare_number(fields.get("startParameter"), external.get("start_angle"),
                   "ELLIPSE.startParameter")
    compare_number(fields.get("endParameter"), external.get("end_angle"),
                   "ELLIPSE.endParameter")


def compare_circle_fields(row: dict[str, Any], entity: str,
                          include_angles: bool) -> None:
    external = row["external"]
    fields = row["adapter"]["fields"]
    compare_point(fields.get("center"), external.get("center"),
                  f"{entity}.center")
    compare_number(fields.get("radius"), external.get("radius"),
                   f"{entity}.radius")
    compare_number(fields.get("thickness"), external.get("thickness"),
                   f"{entity}.thickness")
    compare_point(fields.get("extrusion"), external.get("extrusion"),
                  f"{entity}.extrusion")
    if include_angles:
        compare_number(fields.get("startAngleRadians"),
                       external.get("start_angle"), f"{entity}.startAngle")
        compare_number(fields.get("endAngleRadians"),
                       external.get("end_angle"), f"{entity}.endAngle")


def compare_arc(row: dict[str, Any]) -> None:
    compare_circle_fields(row, "ARC", include_angles=True)


def compare_circle(row: dict[str, Any]) -> None:
    compare_circle_fields(row, "CIRCLE", include_angles=False)


def compare_polyline3d(row: dict[str, Any],
                       external_vertices: dict[str, dict[str, Any]],
                       expected_vertex_count: int,
                       expected_points: list[list[float]] | None = None) -> int:
    external = row["external"]
    fields = row["adapter"]["fields"]
    parent_handle = handle_key(external.get("handle"),
                               "LibreDWG POLYLINE_3D")
    if external.get("type") != 16 or external.get("_subclass") != "AcDb3dPolyline":
        raise OracleError("sample parent is not DWG type 16 AcDb3dPolyline")
    if fields.get("flags") != 8:
        raise OracleError("libdxfrw did not preserve the sample's 3D POLYLINE flag")
    if fields.get("curveType") != external.get("curve_type"):
        raise OracleError("3D POLYLINE curve type differs")
    if fields.get("vertexCount") != expected_vertex_count:
        raise OracleError(
            f"unexpected libdxfrw 3D POLYLINE vertex count "
            f"{fields.get('vertexCount')!r}; expected {expected_vertex_count}")
    if fields.get("seqEndHandle") != handle_key(external.get("seqend"),
                                                "LibreDWG POLYLINE_3D SEQEND"):
        raise OracleError("3D POLYLINE SEQEND handle differs")

    owned_vertices = []
    for vertex_handle, vertex in external_vertices.items():
        owner_handle = handle_key(vertex.get("ownerhandle"),
                                   "LibreDWG VERTEX_3D owner")
        if owner_handle != parent_handle:
            continue
        object_index = vertex.get("index")
        if isinstance(object_index, bool) or not isinstance(object_index, int):
            raise OracleError("LibreDWG VERTEX_3D has no integer object index")
        if (vertex.get("type") != 11
                or vertex.get("_subclass") != "AcDb3dPolylineVertex"):
            raise OracleError(f"vertex {vertex_handle} is not a 3D polyline vertex")
        owned_vertices.append((object_index, vertex_handle, vertex))
    owned_vertices.sort(key=lambda item: item[0])
    if len(owned_vertices) != expected_vertex_count:
        raise OracleError(
            f"unexpected LibreDWG 3D POLYLINE child count {len(owned_vertices)}; "
            f"expected {expected_vertex_count}")

    first_handle = handle_key(external.get("first_vertex"),
                              "LibreDWG POLYLINE_3D first vertex")
    last_handle = handle_key(external.get("last_vertex"),
                             "LibreDWG POLYLINE_3D last vertex")
    if owned_vertices[0][1] != first_handle or owned_vertices[-1][1] != last_handle:
        raise OracleError("3D POLYLINE child order disagrees with first/last handles")

    nonzero_z = 0
    for index, (_, vertex_handle, vertex) in enumerate(owned_vertices):
        prefix = f"vertex.{index}"
        if fields.get(prefix + ".handle") != vertex_handle:
            raise OracleError(f"3D POLYLINE ordered vertex handle differs at {index}")
        if fields.get(prefix + ".ownerHandle") != parent_handle:
            raise OracleError(f"3D POLYLINE vertex owner differs at {index}")
        compare_point(fields.get(prefix + ".position"), vertex.get("point"),
                      f"3D POLYLINE.vertex[{index}].position")
        compare_number(fields.get(prefix + ".flags"), vertex.get("flag"),
                       f"3D POLYLINE.vertex[{index}].flags")
        point = vertex.get("point")
        if not isinstance(point, list) or len(point) != 3:
            raise OracleError(f"LibreDWG vertex {vertex_handle} has no XYZ point")
        if expected_points is not None:
            compare_vector(point, expected_points[index],
                           f"3D POLYLINE.expectedPoint[{index}]")
        if finite_number(point[2], "3D POLYLINE.vertex.z") != 0.0:
            nonzero_z += 1

    return nonzero_z


def owned_polyline_children(parent_handle: str,
                            child_groups: list[dict[str, dict[str, Any]]],
                            label: str) -> list[tuple[int, str, dict[str, Any]]]:
    owned = []
    for children in child_groups:
        for vertex_handle, vertex in children.items():
            owner_handle = handle_key(vertex.get("ownerhandle"),
                                       f"LibreDWG {label} child owner")
            if owner_handle != parent_handle:
                continue
            object_index = vertex.get("index")
            if isinstance(object_index, bool) or not isinstance(object_index, int):
                raise OracleError(f"LibreDWG {label} child has no integer object index")
            owned.append((object_index, vertex_handle, vertex))
    owned.sort(key=lambda item: item[0])
    return owned


def compare_polyline_mesh(
        row: dict[str, Any], external_vertices: dict[str, dict[str, Any]],
        expected_dimensions: tuple[int, int],
        expected_points: list[list[float]]) -> dict[str, int]:
    external = row["external"]
    fields = row["adapter"]["fields"]
    parent_handle = handle_key(external.get("handle"), "LibreDWG POLYLINE_MESH")
    if external.get("type") != 30 or external.get("_subclass") != "AcDbPolygonMesh":
        raise OracleError("sample parent is not DWG type 30 AcDbPolygonMesh")
    if fields.get("flags") != 16 or external.get("flag") != 16:
        raise OracleError("legacy POLYLINE_MESH flag differs from 16")
    if fields.get("curveType") != external.get("curve_type"):
        raise OracleError("legacy POLYLINE_MESH curve type differs")
    expected_m, expected_n = expected_dimensions
    if (fields.get("declaredVertexCount") != expected_m
            or fields.get("declaredFaceCount") != expected_n):
        raise OracleError("legacy POLYLINE_MESH M/N dimensions differ from recipe")
    if (fields.get("smoothM") != external.get("m_density")
            or fields.get("smoothN") != external.get("n_density")):
        raise OracleError("legacy POLYLINE_MESH density differs")
    if fields.get("vertexCount") != len(expected_points):
        raise OracleError("legacy POLYLINE_MESH vertex count differs from recipe")
    if fields.get("seqEndHandle") != handle_key(
            external.get("seqend"), "LibreDWG POLYLINE_MESH SEQEND"):
        raise OracleError("legacy POLYLINE_MESH SEQEND handle differs")

    owned = owned_polyline_children(parent_handle, [external_vertices],
                                    "VERTEX_MESH")
    if len(owned) != len(expected_points):
        raise OracleError(
            f"unexpected LibreDWG VERTEX_MESH count {len(owned)}; "
            f"expected {len(expected_points)}")
    linked_vertices = external.get("vertex")
    if isinstance(linked_vertices, list):
        linked_handles = [handle_key(value, "LibreDWG POLYLINE_MESH vertex link")
                          for value in linked_vertices]
        if [item[1] for item in owned] != linked_handles:
            raise OracleError(
                "legacy POLYLINE_MESH child order disagrees with vertex links")
    elif (owned[0][1] != handle_key(external.get("first_vertex"),
                                    "LibreDWG POLYLINE_MESH first vertex")
          or owned[-1][1] != handle_key(external.get("last_vertex"),
                                        "LibreDWG POLYLINE_MESH last vertex")):
        raise OracleError(
            "legacy POLYLINE_MESH child order disagrees with end handles")

    nonzero_z = 0
    for index, (_, vertex_handle, vertex) in enumerate(owned):
        if (vertex.get("type") != 12
                or vertex.get("_subclass") != "AcDbPolyFaceMeshVertex"):
            raise OracleError(f"vertex {vertex_handle} is not a DWG MESH vertex")
        prefix = f"vertex.{index}"
        if (fields.get(prefix + ".handle") != vertex_handle
                or fields.get(prefix + ".ownerHandle") != parent_handle):
            raise OracleError(f"legacy POLYLINE_MESH child link differs at {index}")
        compare_point(fields.get(prefix + ".position"), vertex.get("point"),
                      f"POLYLINE_MESH.vertex[{index}].position")
        compare_vector(vertex.get("point"), expected_points[index],
                       f"POLYLINE_MESH.expectedPoint[{index}]")
        compare_number(fields.get(prefix + ".flags"), vertex.get("flag"),
                       f"POLYLINE_MESH.vertex[{index}].flags")
        if fields.get(prefix + ".dwgSubtype") != 3:
            raise OracleError(f"libdxfrw MESH vertex subtype differs at {index}")
        if finite_number(vertex["point"][2], "POLYLINE_MESH.vertex.z") != 0.0:
            nonzero_z += 1
    return {"vertexCount": len(owned), "verticesWithNonzeroZ": nonzero_z}


def compare_polyline_pface(
        row: dict[str, Any],
        external_vertices: dict[str, dict[str, Any]],
        external_faces: dict[str, dict[str, Any]],
        expected_points: list[list[float]],
        expected_faces: list[list[int]]) -> dict[str, int]:
    external = row["external"]
    fields = row["adapter"]["fields"]
    parent_handle = handle_key(external.get("handle"), "LibreDWG POLYLINE_PFACE")
    if external.get("type") != 29 or external.get("_subclass") != "AcDbPolyFaceMesh":
        raise OracleError("sample parent is not DWG type 29 AcDbPolyFaceMesh")
    expected_child_count = len(expected_points) + len(expected_faces)
    if (external.get("numverts") != len(expected_points)
            or external.get("numfaces") != len(expected_faces)
            or fields.get("declaredVertexCount") != len(expected_points)
            or fields.get("declaredFaceCount") != len(expected_faces)):
        raise OracleError("PFACE vertex/face counts differ from recipe")
    if fields.get("flags") != 64 or fields.get("vertexCount") != expected_child_count:
        raise OracleError("PFACE flag or total child count differs")
    if fields.get("seqEndHandle") != handle_key(
            external.get("seqend"), "LibreDWG POLYLINE_PFACE SEQEND"):
        raise OracleError("PFACE SEQEND handle differs")

    owned = owned_polyline_children(parent_handle,
                                    [external_vertices, external_faces], "PFACE")
    if len(owned) != expected_child_count:
        raise OracleError(
            f"unexpected LibreDWG PFACE child count {len(owned)}; "
            f"expected {expected_child_count}")
    if (owned[0][1] != handle_key(external.get("first_vertex"),
                                  "LibreDWG POLYLINE_PFACE first vertex")
            or owned[-1][1] != handle_key(external.get("last_vertex"),
                                          "LibreDWG POLYLINE_PFACE last vertex")):
        raise OracleError("PFACE child order disagrees with first/last handles")

    nonzero_z = 0
    for index, (_, vertex_handle, vertex) in enumerate(owned):
        prefix = f"vertex.{index}"
        is_face = vertex.get("entity") == "VERTEX_PFACE_FACE"
        if is_face:
            if (vertex.get("type") != 14
                    or vertex.get("_subclass") != "AcDbFaceRecord"
                    or index < len(expected_points)):
                raise OracleError(f"child {vertex_handle} is not an ordered PFACE face")
            face_index = index - len(expected_points)
            if vertex.get("vertind") != expected_faces[face_index]:
                raise OracleError(f"LibreDWG PFACE face indices differ at {face_index}")
            if fields.get(prefix + ".dwgSubtype") != 5:
                raise OracleError(f"libdxfrw PFACE face subtype differs at {face_index}")
            for corner, expected_index in enumerate(expected_faces[face_index], start=1):
                compare_number(fields.get(f"{prefix}.faceIndex{corner}"),
                               expected_index,
                               f"PFACE.face[{face_index}].index{corner}")
        else:
            if (vertex.get("entity") != "VERTEX_PFACE"
                    or vertex.get("type") != 13
                    or vertex.get("_subclass") != "AcDbPolyFaceMeshVertex"
                    or index >= len(expected_points)):
                raise OracleError(f"child {vertex_handle} is not an ordered PFACE vertex")
            point_index = index
            compare_vector(vertex.get("point"), expected_points[point_index],
                           f"PFACE.expectedPoint[{point_index}]")
            compare_point(fields.get(prefix + ".position"), vertex.get("point"),
                          f"PFACE.vertex[{point_index}].position")
            if fields.get(prefix + ".dwgSubtype") != 4:
                raise OracleError(f"libdxfrw PFACE vertex subtype differs at {point_index}")
            if finite_number(vertex["point"][2], "PFACE.vertex.z") != 0.0:
                nonzero_z += 1
        if (fields.get(prefix + ".handle") != vertex_handle
                or fields.get(prefix + ".ownerHandle") != parent_handle):
            raise OracleError(f"PFACE child link differs at {index}")
        if not is_face:
            compare_number(fields.get(prefix + ".flags"), vertex.get("flag"),
                           f"PFACE.vertex[{index}].flags")
        # ODA §20.4.15 encodes face-record indices only. LibreDWG's `flag=128`
        # marks its internal face-record kind; libdxfrw correctly defaults the
        # public vertex flags field to zero because no DWG field carries it.
    return {"vertexCount": len(expected_points), "faceCount": len(expected_faces),
            "verticesWithNonzeroZ": nonzero_z}


def sha256_file(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--adapter", required=True,
                        help="standalone libdxfrw semantic adapter executable")
    parser.add_argument("--dwgread", default="dwgread",
                        help="LibreDWG dwgread executable (default: PATH lookup)")
    parser.add_argument("--input", required=True, type=pathlib.Path,
                        help="existing supported DWG sample/control; never modified")
    args = parser.parse_args()

    with args.input.open("rb") as source:
        version = source.read(6).decode("ascii", errors="replace")
    expected_nonzero_z_line_count = None
    expected_nonzero_z_vertex_count = None
    expected_vertex_count = None
    sample_source = None
    topology_recipe = None
    tablet_mesh_recipes: dict[str, dict[str, Any]] | None = None
    paired_reference_path: pathlib.Path | None = None
    paired_reference_digest: str | None = None
    if version == "AC1015" and args.input.name == "PolyLine3D.dwg":
        expected_digest = (
            "f51f4f65ba027bf1a001480d3c7b5bc5667960050081c67f9bb9c7bdbcfe815a"
        )
        actual_digest = sha256_file(args.input)
        if actual_digest != expected_digest:
            raise OracleError(
                "AC1015 PolyLine3D sample digest differs from the pinned "
                f"AutoCAD-authored input: {actual_digest}")
        cases = (("POLYLINE_3D", 1, compare_polyline3d),)
        expected_vertex_count = 6
        expected_nonzero_z_vertex_count = 0
        excluded = ["nonzero-Z coordinate preservation", "PFACE/POLYGON_MESH",
                    "other families and versions", "DWG writing"]
        sample_source = {
            "repository": "LibreDWG/libredwg",
            "commit": "34f02f54b9aacb5708c1d3d2070efb3e4b2d8c43",
            "path": "test/test-data/2000/PolyLine3D.dwg",
            "gitBlob": "bd1b3dde9daf91cd833c4745f8c35be119e2cf32",
            "sha256": expected_digest,
            "autocadPropertyDump": "test/test-data/2000/PolyLine3D.txt",
            "autocadDxfPair": "test/test-data/2000/PolyLine3D.dxf",
        }
    elif (version == "AC1015"
          and args.input.name == "libdxfrw_ac1015_3d_topology_control.dwg"):
        cases = (("POLYLINE_3D", 1, compare_polyline3d),
                 ("POLYLINE_MESH", 1, compare_polyline_mesh),
                 ("POLYLINE_PFACE", 1, compare_polyline_pface))
        topology_recipe = {
            "POLYLINE_3D": {
                "vertexCount": 2,
                "points": [[3.0, 1.0, 0.5], [6.0, 2.0, -0.75]],
            },
            "POLYLINE_MESH": {
                "dimensions": (3, 2),
                "points": [[0.0, 0.0, 0.0], [2.0, 0.0, 0.0],
                           [4.0, 0.0, 1.0], [0.0, 3.0, -1.0],
                           [2.0, 3.0, 2.0], [4.0, 3.0, 0.25]],
            },
            "POLYLINE_PFACE": {
                "points": [[-2.0, 0.0, 1.0], [0.0, 0.0, 1.5],
                           [1.0, 2.0, 0.0], [0.0, 3.0, -0.5],
                           [-1.0, 1.0, 2.0]],
                "faces": [[1, 2, 3, 4], [2, 3, -4, 5], [3, -4, 5, 0]],
            },
        }
        excluded = ["AutoCAD-authored interoperability", "other versions",
                    "DWG writing", "families outside this generated control"]
        sample_source = {
            "provenance": "generated locally from the checked-in .dwgadd recipe",
            "recipe": "tests/fixtures/dwg/ac1015_3d_topology_control.dwgadd",
            "writer": "LibreDWG dwgadd 0.14",
            "boundary": "cross-reader parser control only; not AutoCAD-authored",
        }
    elif (version == "AC1024"
            and args.input.name == "visualization_-_conference_room.dwg"):
        cases = (("INSERT", 6, compare_insert),
                 ("SPLINE", 2, compare_spline))
        excluded = ["PLANESURFACE", "3DSOLID", "REGION", "BODY", "MESH",
                    "other versions and families"]
    elif (version == "AC1024"
            and args.input.name == "visualization_-_condominium_with_skylight.dwg"):
        cases = (("LINE", 2, compare_line),)
        expected_nonzero_z_line_count = 2
        excluded = ["all entities other than LINE", "other versions"]
    elif version == "AC1021" and args.input.name == "tablet.dwg":
        expected_digest = (
            "7f203649dc8434ef7cf7a46f7f6def2a0192a1163ba34ebcf88ebfe69635ccd4"
        )
        actual_digest = sha256_file(args.input)
        if actual_digest != expected_digest:
            raise OracleError(
                "AC1021 tablet sample digest differs from the reviewed local "
                f"sample: {actual_digest}")
        cases = (("3DFACE", 48, compare_3dface),
                 ("ELLIPSE", 24, compare_ellipse),
                 ("ARC", 243, compare_arc),
                 ("CIRCLE", 168, compare_circle),
                 ("POLYLINE_MESH", 2, compare_polyline_mesh),
                 ("LINE", 3002, compare_line))
        expected_nonzero_z_line_count = 670
        tablet_mesh_points = [
            [5.32749676213992, 4.77769267854258, 0.0],
            [5.32007611287037, 4.85488811069972, 0.0],
            [5.27885583688404, 4.92057745252258, 0.0],
            [5.21257322296318, 4.96083681010969, 0.0],
            [5.13527791840421, 4.96713257651620, 0.0],
            [5.19517864821834, 4.77813944154093, 0.0],
            [5.18525517733353, 4.79634562383081, 0.0],
            [5.17122926102256, 4.81161697018965, 0.0],
            [5.15393093509044, 4.82304974185362, 0.0],
            [5.13438389236210, 4.82996736200440, 0.0],
        ]
        tablet_mesh_recipes = {
            "154e": {"dimensions": (2, 5), "points": tablet_mesh_points},
            "22d8": {"dimensions": (2, 5), "points": tablet_mesh_points},
        }
        excluded = ["all entities other than LINE, 3DFACE, ARC, CIRCLE, ELLIPSE, and POLYLINE_MESH",
                    "other versions", "unverified sample provenance",
                    "non-default ARC/CIRCLE extrusion/thickness",
                    "non-planar legacy mesh vertices and mesh-face semantics"]
        sample_source = {
            "provenance": ("existing user-owned local sample; original "
                           "producer/date not verified"),
            "sha256": expected_digest,
            "boundary": ("one-sample read-field comparison only; not "
                         "target-authored provenance"),
        }
    elif args.input.name in AUTOCAD_POINT3D_PROFILES:
        profile = AUTOCAD_POINT3D_PROFILES[args.input.name]
        expected_version = profile["version"]
        expected_digest = profile["sha256"]
        if version != expected_version:
            raise OracleError(
                f"{args.input.name} has signature {version}; "
                f"expected {expected_version}")
        actual_digest = sha256_file(args.input)
        if actual_digest != expected_digest:
            raise OracleError(
                f"{args.input.name} SHA-256 differs from the pinned "
                f"AutoCAD-authored input: {actual_digest}")
        cases = (("POINT", 1,
                  lambda row: compare_point3d_witness(
                      row, [50.0, 50.0, 50.0], "7c")),)
        excluded = ["other POINTs and coordinate tuples",
                    "other entities in the drawing",
                    "other versions beyond the six pinned samples",
                    "DWG writing",
                    "FreeCAD geometry on profiles not separately tested"]
        profile_label = args.input.stem.removeprefix("point3d_")
        sample_source = {
            "repository": "nextgis/dwg_samples",
            "repositoryUrl": "https://github.com/nextgis/dwg_samples",
            "corpusCommit": "ff05d5a89ce123da511ed93c5fe5e6df85e02ab8",
            "readme": "README.md",
            "authoring": "AutoCAD 2016 M.49.0.0, blank metric drawing, _POINT 50,50,50",
            "profile": profile_label,
            "sha256": expected_digest,
            "boundary": "one POINT read-field witness for this pinned DWG version only",
        }
        paired_reference_digest = profile["dxf_sha256"]
        if paired_reference_digest:
            paired_reference_path = args.input.with_suffix(".dxf")
            if not paired_reference_path.is_file():
                raise OracleError(
                    f"{args.input.name} requires its pinned paired source DXF: "
                    f"{paired_reference_path}")
            actual_reference_digest = sha256_file(paired_reference_path)
            if actual_reference_digest != paired_reference_digest:
                raise OracleError(
                    f"paired source DXF SHA-256 differs from the pinned "
                    f"target-authored file: {actual_reference_digest}")
            sample_source["pairedSourceDxf"] = {
                "path": paired_reference_path.name,
                "sha256": paired_reference_digest,
            }
        else:
            sample_source["pairedSourceDxf"] = None
    else:
        raise OracleError(
            "unsupported sample profile: expected AC1015 PolyLine3D.dwg or "
            "libdxfrw_ac1015_3d_topology_control.dwg, "
            "AC1021 tablet.dwg, or AC1024 "
            "visualization_-_conference_room.dwg / "
            "visualization_-_condominium_with_skylight.dwg, or one of the "
            "hash-pinned nextgis/dwg_samples point3d_*.dwg profiles; "
            f"got {version!r} {args.input.name!r}")

    external = run_json([args.dwgread, "-O", "minJSON", str(args.input)],
                        "LibreDWG dwgread")
    adapter = run_json([args.adapter, "--input", str(args.input),
                        "--facade", "dwgRW"], "libdxfrw semantic adapter")

    paired_reference_rows: dict[str, dict[str, Any]] | None = None
    if paired_reference_path is not None:
        reference = run_json(
            [args.adapter, "--input", str(paired_reference_path),
             "--facade", "dxfRW"],
            "libdxfrw paired source DXF adapter")
        paired_reference_rows = index_adapter(reference, "POINT")
        if len(paired_reference_rows) != 1:
            raise OracleError(
                "paired target-authored DXF must contain exactly one POINT; "
                f"found {len(paired_reference_rows)}")
        paired_point = compare_point3d_reference_dxf(
            paired_reference_rows, [50.0, 50.0, 50.0], "7c")
    else:
        paired_point = None

    results = []
    for entity, count, comparator in cases:
        adapter_entity = "POLYLINE" if entity.startswith("POLYLINE_") else entity
        adapter_rows = index_adapter(adapter, adapter_entity)
        if entity.startswith("POLYLINE_"):
            expected_flags = {"POLYLINE_3D": 8,
                              "POLYLINE_MESH": 16,
                              "POLYLINE_PFACE": 64}[entity]
            adapter_rows = {
                handle: value for handle, value in adapter_rows.items()
                if value["fields"].get("flags") == expected_flags
            }
        rows = compare_entity_set(index_external(external, entity),
                                  adapter_rows, entity, count)
        result_metrics = {}
        mesh_metrics: dict[str, dict[str, int]] = {}
        for row in rows:
            if entity == "POLYLINE_3D":
                recipe = (topology_recipe or {}).get(entity)
                vertex_count = recipe["vertexCount"] if recipe else expected_vertex_count
                nonzero_z = comparator(
                    row, index_external(external, "VERTEX_3D"), vertex_count,
                    recipe.get("points") if recipe else None)
                if (expected_nonzero_z_vertex_count is not None
                        and nonzero_z != expected_nonzero_z_vertex_count):
                    raise OracleError(
                        "unexpected nonzero-Z 3D POLYLINE vertex count "
                        f"{nonzero_z}; expected "
                        f"{expected_nonzero_z_vertex_count}")
                if recipe is not None:
                    result_metrics = {"vertexCount": vertex_count,
                                     "verticesWithNonzeroZ": nonzero_z}
            elif entity == "POLYLINE_MESH":
                if topology_recipe is not None:
                    recipe = topology_recipe[entity]
                elif tablet_mesh_recipes is not None:
                    recipe = tablet_mesh_recipes.get(row["handle"])
                    if recipe is None:
                        raise OracleError(
                            "unexpected tablet POLYLINE_MESH handle "
                            f"{row['handle']}")
                else:
                    raise OracleError(
                        "POLYLINE_MESH comparison has no expected-field recipe")
                mesh_metrics[row["handle"]] = comparator(
                    row, index_external(external, "VERTEX_MESH"),
                    recipe["dimensions"], recipe["points"])
            elif entity == "POLYLINE_PFACE":
                recipe = topology_recipe[entity]
                result_metrics = comparator(
                    row, index_external(external, "VERTEX_PFACE"),
                    index_external(external, "VERTEX_PFACE_FACE"),
                    recipe["points"], recipe["faces"])
            else:
                compared = comparator(row)
                if entity == "POINT":
                    result_metrics = compared
        if entity == "POLYLINE_MESH":
            result_metrics = {
                "meshRecords": [
                    {"handle": handle, **mesh_metrics[handle]}
                    for handle in sorted(mesh_metrics,
                                         key=lambda item: int(item, 16))
                ],
                "vertexCount": sum(
                    metrics["vertexCount"]
                    for metrics in mesh_metrics.values()),
                "verticesWithNonzeroZ": sum(
                    metrics["verticesWithNonzeroZ"]
                    for metrics in mesh_metrics.values()),
            }
        result = {"entity": entity, "count": len(rows),
                  "comparedBy": "handle", "semanticFieldsMatched": True}
        if entity == "POINT":
            result.update(result_metrics)
            if paired_point is not None:
                result["pairedSourceDxfMatched"] = paired_point
        if result_metrics:
            if entity != "POINT":
                result.update(result_metrics)
        elif entity == "POLYLINE_3D":
            result["vertexCount"] = expected_vertex_count
            result["recordsWithNonzeroVertexZ"] = nonzero_z
        elif entity == "LINE":
            nonzero_z = sum(
                any(finite_number(point[2], "LINE.endpoint.z") != 0.0
                    for point in (row["external"]["start"],
                                  row["external"]["end"]))
                for row in rows)
            if (expected_nonzero_z_line_count is not None
                    and nonzero_z != expected_nonzero_z_line_count):
                raise OracleError(
                    "unexpected nonzero-Z LINE count "
                    f"{nonzero_z}; expected {expected_nonzero_z_line_count}")
            result["recordsWithNonzeroEndpointZ"] = nonzero_z
        results.append(result)

    version_result = subprocess.run([args.dwgread, "--version"],
                                    capture_output=True, text=True, check=False)
    oracle_version = (version_result.stdout.strip()
                      if version_result.returncode == 0 else "unknown")
    layout_authority = {
        "3DFACE": "ODA v5.4.1 §20.4.32",
        "LINE": "ODA v5.4.1 §20.4.21",
        "POLYLINE_3D": "ODA v5.4.1 §§20.4.12, 20.4.17",
        "POLYLINE_MESH": "ODA v5.4.1 §§20.4.13, 20.4.34",
        "POLYLINE_PFACE": "ODA v5.4.1 §§20.4.14, 20.4.15, 20.4.33",
        "INSERT": "ODA v5.4.1 §§20.4.9-20.4.10",
        "SPLINE": "ODA v5.4.1 §20.4.40",
        "ELLIPSE": "ODA v5.4.1 §20.4.39",
        "ARC": "ODA v5.4.1 §20.4.18",
        "CIRCLE": "ODA v5.4.1 §20.4.20",
        "POINT": "ODA v5.4.1 §20.4.31",
    }
    print(json.dumps({
        "result": "matched",
        "dwgVersion": version,
        "sample": args.input.name,
        "independentReader": oracle_version,
        "rows": results,
        "layoutAuthority": {row["entity"]: layout_authority[row["entity"]]
                            for row in results},
        "sampleSource": sample_source,
        "excluded": excluded,
        "claimBoundary": (
            "read-field comparison of one pinned AutoCAD-authored POINT for "
            "this DWG revision; no DWG writer or family-wide support claim"
            if args.input.name in AUTOCAD_POINT3D_PROFILES else
            "read-field comparison on this sample only; no writer, "
            "AutoCAD interoperability, or general version support claim"),
    }, sort_keys=True, indent=2))
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, OracleError) as error:
        print(f"DWG consumer oracle: {error}", file=sys.stderr)
        raise SystemExit(1)
