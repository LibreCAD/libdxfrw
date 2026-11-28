#!/usr/bin/env python3
"""Hash-bound AC1021 geometry matrix check using LibreDWG and optional ODA DXF."""

import argparse
import hashlib
import json
import math
import tempfile
from pathlib import Path

from audit_ac1021_material_peer import material_graph
from bounded_process import run_bounded


def require(value, message):
    if not value:
        raise ValueError(message)


def digest(path):
    require(path.is_file() and path.stat().st_size <= 64 * 1024 * 1024, "input absent or over budget")
    return hashlib.sha256(path.read_bytes()).hexdigest()


def close(actual, expected, label):
    if isinstance(expected, tuple):
        require(len(actual) >= len(expected), label + ": insufficient coordinates")
        for value, target in zip(actual, expected):
            close(value, target, label)
    else:
        require(math.isclose(actual, expected, rel_tol=1e-9, abs_tol=1e-8), label + ": value mismatch")


STANDARD = (0., 0., 1.)
TILT = (0., 1., 0.)
NEGATIVE = (0., 0., -1.)
LINE = ("LINE", STANDARD, 0.)
POINT = ("POINT", STANDARD, 0.)
CIRCLE = ("CIRCLE", STANDARD, 0.)
ARC = ("ARC", STANDARD, 0.)
MODES = {
    "line": [LINE], "point": [POINT], "circle": [CIRCLE],
    "circle_tilt": [("CIRCLE", TILT, 1.25)],
    "circle_negative": [("CIRCLE", NEGATIVE, 1.25)],
    "arc": [ARC], "arc_tilt": [("ARC", TILT, .5)],
    "arc_negative": [("ARC", NEGATIVE, .5)],
    "mixed": [LINE, CIRCLE, ARC],
    "source": [LINE, POINT, CIRCLE, ("CIRCLE", TILT, 1.25),
               ("CIRCLE", NEGATIVE, 1.25), ARC, ("ARC", TILT, .5),
               ("ARC", NEGATIVE, .5)],
}


def wcs(point, normal):
    # Explicit arbitrary-axis bases for these three tested unit normals.
    xaxis, yaxis = {
        STANDARD: ((1., 0., 0.), (0., 1., 0.)),
        TILT: ((-1., 0., 0.), (0., 0., 1.)),
        NEGATIVE: ((-1., 0., 0.), (0., 1., 0.)),
    }[normal]
    return tuple(point[0] * xaxis[i] + point[1] * yaxis[i] + point[2] * normal[i]
                 for i in range(3))


def check_fields(kind, normal, thickness, center, radius=None, start=None, end=None):
    if kind == "LINE":
        close(center, (1., 2., 3.), "LINE start")
        close(end, (4., 5., 6.), "LINE end")
    elif kind == "POINT":
        close(center, (3., 4., 5.), "POINT location")
    else:
        close(center, (7., 8., 2.) if kind == "CIRCLE" else (10., 11., 3.), kind + " center")
        close(radius, 2.5 if kind == "CIRCLE" else 4., kind + " radius")
        if kind == "ARC":
            close(start, math.radians(20.), "ARC start angle")
            close(end, math.radians(115.), "ARC end angle")


def check_graph(document):
    material_graph(document)
    objects = document["OBJECTS"]
    by_handle = {obj["handle"][2]: obj for obj in objects if isinstance(obj.get("handle"), list)}
    roots = [obj for obj in objects if obj.get("object") == "DICTIONARY"
             and "ACAD_PLOTSTYLENAME" in obj.get("items", {})]
    require(len(roots) == 1, "missing or duplicated plotstyle root")
    root = roots[0]
    edge = root["items"]["ACAD_PLOTSTYLENAME"]
    require(edge[0] == 2 and edge[2] in by_handle, "invalid plotstyle root edge")
    dictionary = by_handle[edge[2]]
    require(dictionary.get("object") == "DICTIONARYWDFLT"
            and dictionary["ownerhandle"][0] == 4
            and dictionary["ownerhandle"][2] == root["handle"][2], "invalid plotstyle dictionary owner")
    normal = dictionary["items"].get("Normal")
    require(normal and normal[0] == 2 and dictionary["defaultid"][0] == 5
            and dictionary["defaultid"][2] == normal[2] and normal[2] in by_handle,
            "invalid default plotstyle reference")
    placeholder = by_handle[normal[2]]
    require(placeholder.get("object") == "PLACEHOLDER" and placeholder["ownerhandle"][0] == 4
            and placeholder["ownerhandle"][2] == edge[2], "invalid plotstyle placeholder")
    return by_handle


def match_expected(entities, expected, get_type, get_normal):
    remaining = list(entities)
    matches = []
    for kind, normal, thickness in expected:
        found = [entity for entity in remaining if get_type(entity) == kind
                 and len(get_normal(entity)) == 3
                 and all(math.isclose(a, b, abs_tol=1e-9) for a, b in zip(get_normal(entity), normal))]
        require(len(found) == 1, f"missing or ambiguous {kind} normal {normal}")
        entity = found[0]
        remaining.remove(entity)
        matches.append((entity, kind, normal, thickness))
    require(not remaining, "unexpected additional model-space entities")
    return matches


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mode", choices=MODES, required=True)
    parser.add_argument("--dwgread", type=Path, required=True)
    parser.add_argument("--dwg", type=Path, required=True)
    parser.add_argument("--dwg-sha256", required=True)
    parser.add_argument("--dxf", type=Path)
    parser.add_argument("--dxf-sha256")
    args = parser.parse_args()
    require((args.dxf is None) == (args.dxf_sha256 is None), "DXF and its SHA-256 must be supplied together")
    reader, dwg = args.dwgread.resolve(strict=True), args.dwg.resolve(strict=True)
    version = run_bounded([str(reader), "--version"], 5, 4096)
    require(version.returncode == 0 and version.stdout.strip() == b"dwgread 0.14", "requires LibreDWG 0.14")
    require(digest(dwg) == args.dwg_sha256, "DWG SHA-256 mismatch")
    with tempfile.TemporaryDirectory(prefix="ac1021-geometry-peer-") as tmp:
        target = Path(tmp) / "decoded.json"
        result = run_bounded([str(reader), "-O", "JSON", "-o", str(target), str(dwg)], 30, 1024 * 1024)
        require(result.returncode == 0 and target.is_file()
                and target.stat().st_size <= 128 * 1024 * 1024, "LibreDWG decode failed or exceeded budget")
        document = json.loads(target.read_text())
    indexed = check_graph(document)
    entities = [obj for obj in document["OBJECTS"] if obj.get("entity")
                and obj.get("entmode") == 2 and obj["entity"] not in ("BLOCK", "ENDBLK")]
    for obj, kind, normal, thickness in match_expected(entities, MODES[args.mode],
            lambda obj: obj["entity"], lambda obj: obj.get("extrusion", STANDARD)):
        require(obj.get("entmode") == 2 and obj["layer"][0] == 5 and obj["layer"][2] in indexed
                and indexed[obj["layer"][2]].get("name") == "0", kind + " model-space/layer reference")
        close(obj.get("extrusion", STANDARD), normal, kind + " normal")
        close(obj.get("thickness", 0), thickness, kind + " thickness")
        center = ((obj["x"], obj["y"], obj["z"]) if kind == "POINT"
                  else obj["start"] if kind == "LINE" else obj["center"])
        check_fields(kind, normal, thickness, center, obj.get("radius"),
                     obj.get("start_angle"), obj.get("end_angle") if kind == "ARC" else obj.get("end"))
    if args.dxf is not None:
        import ezdxf
        require(ezdxf.__version__ == "1.4.5", "requires ezdxf 1.4.5")
        dxf = args.dxf.resolve(strict=True)
        require(digest(dxf) == args.dxf_sha256, "DXF SHA-256 mismatch")
        doc = ezdxf.readfile(dxf)
        audit = doc.audit()
        require(doc.dxfversion == "AC1021" and not audit.errors and not audit.fixes,
                "invalid DXF revision or audit reported errors or repairs")
        for entity, kind, normal, thickness in match_expected(list(doc.modelspace()), MODES[args.mode],
                lambda obj: obj.dxftype(), lambda obj: obj.dxf.get("extrusion", STANDARD)):
            require(entity.dxf.layer == "0", kind + " DXF layer")
            close(entity.dxf.get("extrusion", STANDARD), normal, kind + " DXF normal")
            close(entity.dxf.get("thickness", 0.), thickness, kind + " DXF thickness")
            center = (entity.dxf.start if kind == "LINE" else entity.dxf.location if kind == "POINT"
                      else entity.dxf.center)
            start = math.radians(entity.dxf.start_angle) if kind == "ARC" else None
            end = math.radians(entity.dxf.end_angle) if kind == "ARC" else entity.dxf.end if kind == "LINE" else None
            radius = entity.dxf.radius if kind in ("ARC", "CIRCLE") else None
            check_fields(kind, normal, thickness, center, radius, start, end)
            if kind == "ARC":
                for angle, actual in ((start, entity.start_point), (end, entity.end_point)):
                    expected = (10. + 4. * math.cos(angle), 11. + 4. * math.sin(angle), 3.)
                    close(actual, wcs(expected, normal), "ARC WCS endpoint")
            if kind in ("ARC", "CIRCLE"):
                close(entity.ocs().to_wcs(center), wcs(center, normal), kind + " WCS center")
        require(digest(dxf) == args.dxf_sha256, "DXF changed during decode")
    require(digest(dwg) == args.dwg_sha256, "DWG changed during decode")
    print(f"AC1021 {args.mode}: peer geometry/graph" + (" and DXF geometry" if args.dxf else "") + " PASS")


if __name__ == "__main__":
    main()
