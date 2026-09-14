#!/usr/bin/env python3
"""Validate the local-from-scratch DWG oracle contract.

The contract is metadata only: generated DWGs and oracle outputs stay in a
temporary directory.  Keeping the six version routes and expected entity set
in one checked-in document prevents the fast oracle runner and the plan from
silently drifting apart.
"""

from __future__ import annotations

import argparse
import json
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_MANIFEST = ROOT / "metadata/local-dwg-oracle-matrix-v1.json"
EXPECTED_SCHEMA = "local-dwg-oracle-matrix-v1"
EXPECTED_VERSIONS = [
    ("13", "AC1015"),
    ("14", "AC1018"),
    ("15", "AC1021"),
    ("16", "AC1024"),
    ("17", "AC1027"),
    ("18", "AC1032"),
]
EXPECTED_ENTITIES = [
    "LINE", "POINT", "CIRCLE", "ARC", "LWPOLYLINE", "TEXT", "MTEXT",
    "ELLIPSE", "TRACE", "SOLID", "3DFACE", "RAY", "XLINE", "3DLINE",
    "POLYLINE", "SPLINE", "HATCH", "LEADER",
]


def load(path: Path) -> dict[str, object]:
    document = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(document, dict):
        raise ValueError("oracle matrix must be a JSON object")
    return document


def validate(document: dict[str, object]) -> None:
    if document.get("schema") != EXPECTED_SCHEMA:
        raise ValueError("unexpected oracle matrix schema")
    if document.get("fixturePolicy") != "localFromScratchRuntimeOnly":
        raise ValueError("oracle matrix must remain runtime-only")
    oracle = document.get("oracle")
    if not isinstance(oracle, dict) or oracle.get("qualification") != "advisory":
        raise ValueError("oracle matrix must be advisory and independent")

    versions = document.get("versions")
    if not isinstance(versions, list) or len(versions) != len(EXPECTED_VERSIONS):
        raise ValueError("oracle matrix must contain exactly six versions")
    actual_versions = []
    for entry in versions:
        if not isinstance(entry, dict):
            raise ValueError("version entry must be an object")
        marker = entry.get("marker")
        acadver = entry.get("acadver")
        if not isinstance(marker, str) or not isinstance(acadver, str):
            raise ValueError("version entry lacks marker/acadver")
        actual_versions.append((marker, acadver))
        if entry.get("dxfStatus") not in {"qualified", "mismatch"}:
            raise ValueError("version entry has invalid DXF status")
        if entry.get("jsonStatus") != "qualified":
            raise ValueError("JSON reader status must remain qualified evidence")
        missing = entry.get("dxfMissingEntities")
        if not isinstance(missing, list) or any(not isinstance(name, str) for name in missing):
            raise ValueError("DXF missing-entity evidence must be a string list")
        if entry.get("dxfStatus") == "qualified" and missing:
            raise ValueError("qualified DXF route cannot list missing entities")
    if actual_versions != EXPECTED_VERSIONS:
        raise ValueError("version order or mapping drifted")
    if actual_versions[0] == EXPECTED_VERSIONS[0]:
        first = versions[0]
        assert isinstance(first, dict)
        if first.get("dxfStatus") != "mismatch" or first.get("dxfMissingEntities") != ["HATCH", "LEADER", "SPLINE"]:
            raise ValueError("AC1015 discrepancy disposition changed unexpectedly")
    if any(entry.get("dxfStatus") == "mismatch" for entry in versions[1:]):
        raise ValueError("only the observed AC1015 exporter discrepancy is allowed")

    entities = document.get("expectedEntities")
    if entities != EXPECTED_ENTITIES:
        raise ValueError("expected entity set drifted")
    if len(set(entities)) != len(entities):
        raise ValueError("expected entity set contains duplicates")
    bounds = document.get("entityCountBounds")
    if not isinstance(bounds, dict):
        raise ValueError("entity count bounds are missing")
    if set(bounds) - set(EXPECTED_ENTITIES):
        raise ValueError("entity count bounds contain an unknown entity")
    for name, bound in bounds.items():
        if (not isinstance(bound, dict)
                or not isinstance(bound.get("min"), int)
                or not isinstance(bound.get("max"), int)
                or bound["min"] < 1
                or bound["min"] > bound["max"]):
            raise ValueError("invalid entity count bound for %s" % name)


def self_test() -> None:
    document = load(DEFAULT_MANIFEST)
    validate(document)
    with tempfile.TemporaryDirectory(prefix="libdxfrw-oracle-matrix-") as directory:
        path = Path(directory) / "invalid.json"
        invalid = dict(document)
        invalid["expectedEntities"] = list(EXPECTED_ENTITIES) + ["LINE"]
        path.write_text(json.dumps(invalid), encoding="utf-8")
        try:
            validate(load(path))
        except ValueError:
            pass
        else:
            raise AssertionError("duplicate entity was accepted")
    print("local DWG oracle matrix: PASS")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        self_test()
        return 0
    validate(load(args.manifest))
    print("local DWG oracle matrix: PASS (%s)" % args.manifest)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
