#!/usr/bin/env python3
"""Validate the deterministic, non-promoting DWG/DXF support matrix.

The matrix is derived from the pinned source-route mapping and the
metadata-only oracle registry.  It deliberately reports source/dispatch
coverage separately from format qualification: no source-only row can become
an advertised support claim.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import tempfile
from collections import Counter
from pathlib import Path


class MatrixError(ValueError):
    pass


FACADES = ("dxfRW", "dwgRW")
SOURCE_ONLY_POLICY = "no-drawing-payloads; source-and-metadata-only"


def read_json(path: Path) -> dict:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as exc:
        raise MatrixError("cannot read %s: %s" % (path, exc)) from exc
    if not isinstance(value, dict):
        raise MatrixError("%s must contain an object" % path)
    return value


def route_digest(route_ids: list[str]) -> str:
    payload = "\n".join(sorted(route_ids)).encode("utf-8")
    return hashlib.sha256(payload).hexdigest()


def _rows(mapping: dict) -> list[dict]:
    rows = mapping.get("mapping", {}).get("rows")
    if not isinstance(rows, list) or not rows:
        raise MatrixError("source-route mapping rows are empty")
    if any(not isinstance(row, dict) for row in rows):
        raise MatrixError("source-route mapping contains a malformed row")
    return rows


def _claim_status(row: dict) -> str:
    # An exact source selector is useful evidence, but it is not wire-format
    # proof.  Every reviewed delta/adaptation remains experimental until an
    # independent runtime/oracle lane promotes it explicitly.
    if row.get("disposition") == "equivalent" and row.get("implementationState") == "same-selector":
        return "SOURCE_PARITY"
    return "EXPERIMENTAL"


def build_matrix(mapping: dict, registry: dict) -> dict:
    if mapping.get("schema") != 1 or mapping.get("kind") != "libdxfrw-source-route-inventory":
        raise MatrixError("unexpected source-route mapping schema")
    if mapping.get("fixturePolicy") != SOURCE_ONLY_POLICY:
        raise MatrixError("source-route mapping has an unsafe fixture policy")
    source_mapping = mapping.get("mapping", {})
    if source_mapping.get("summary", {}).get("targetOnly") != 0:
        raise MatrixError("target-unmapped routes remain")

    if registry.get("schema") != 1 or registry.get("kind") != "librecad-parity-test-oracle-registry":
        raise MatrixError("unexpected oracle registry schema")
    if registry.get("fixturePolicy") != SOURCE_ONLY_POLICY:
        raise MatrixError("oracle registry has an unsafe fixture policy")
    routes = registry.get("routes")
    links = registry.get("featureLinks")
    if not isinstance(routes, list) or not isinstance(links, list):
        raise MatrixError("oracle registry routes/featureLinks are missing")
    if any(not isinstance(route, dict) for route in routes) or any(not isinstance(link, dict) for link in links):
        raise MatrixError("oracle registry routes/featureLinks contain a malformed row")
    if any(route.get("promotesSupport") is not False for route in routes):
        raise MatrixError("an oracle route promotes support")
    if any(link.get("promotesSupport") is not False for link in links):
        raise MatrixError("an oracle feature link promotes support")

    rows = _rows(mapping)
    if any(row.get("fixtureDisposition") != "no-drawing-fixture" for row in rows):
        raise MatrixError("mapping contains a non-source-only fixture disposition")
    by_facade = {}
    for facade in FACADES:
        selected = [row for row in rows if row.get("facade") == facade]
        if not selected:
            raise MatrixError("mapping has no %s rows" % facade)
        route_ids = [row.get("targetRouteId") for row in selected]
        if any(not isinstance(route_id, str) or not route_id for route_id in route_ids):
            raise MatrixError("mapping has a malformed %s target route ID" % facade)
        if len(route_ids) != len(set(route_ids)):
            raise MatrixError("mapping duplicates a %s target route ID" % facade)
        statuses = Counter(_claim_status(row) for row in selected)
        by_facade[facade] = {
            "routeCount": len(selected),
            "routeIdsSha256": route_digest(route_ids),
            "statusCounts": dict(sorted(statuses.items())),
            "dispositionCounts": dict(sorted(Counter(row.get("disposition") for row in selected).items())),
            "implementationStateCounts": dict(sorted(Counter(row.get("implementationState") for row in selected).items())),
            "categoryCounts": dict(sorted(Counter(row.get("category") for row in selected).items())),
            "advertisedRows": 0,
            "qualifiedFormatParityRows": 0,
        }

    target_rows = sum(value["routeCount"] for value in by_facade.values())
    target_summary = source_mapping.get("summary", {})
    standalone_only = source_mapping.get("standaloneUnmappedRouteIds", [])
    if not isinstance(standalone_only, list) or any(not isinstance(value, str) for value in standalone_only):
        raise MatrixError("standalone-unmapped route set is malformed")
    if standalone_only != sorted(set(standalone_only)):
        raise MatrixError("standalone-unmapped route set is not sorted/unique")
    target = mapping.get("provenance", {}).get("target", {})
    if not isinstance(target, dict) or not isinstance(target.get("commit"), str):
        raise MatrixError("target provenance is missing")

    return {
        "schema": 1,
        "kind": "libdxfrw-support-matrix",
        "target": {
            "repository": target.get("repository"),
            "commit": target["commit"],
        },
        "fixturePolicy": SOURCE_ONLY_POLICY,
        "claimPolicy": {
            "sourceOnlyStatuses": ["SOURCE_PARITY", "DISPATCH_PARITY", "EXPERIMENTAL"],
            "promotableStatus": "QUALIFIED_FORMAT_PARITY",
            "advertisedRows": 0,
            "qualifiedFormatParityRows": 0,
            "promotionRequires": [
                "target-versus-standalone differential",
                "independent reader or auditor",
                "fixture-policy-eligible positive evidence",
                "preservation and unsupported-disposition review",
            ],
        },
        "facades": by_facade,
        "combined": {
            "targetRows": target_rows,
            "dxfRWRows": by_facade["dxfRW"]["routeCount"],
            "dwgRWRows": by_facade["dwgRW"]["routeCount"],
            "targetUnmappedRows": int(target_summary.get("targetOnly", -1)),
            "standaloneOnlyRows": len(standalone_only),
            "zeroTargetUnmapped": target_summary.get("targetOnly") == 0,
            "sharedRouteRows": sum(1 for row in rows if row.get("facade") == "shared"),
            "oracleRoutes": len(routes),
            "oracleFeatureLinks": len(links),
        },
        "standaloneOnlyRouteIds": sorted(set(standalone_only)),
    }


def validate_matrix(value: dict, mapping: dict, registry: dict) -> None:
    expected = build_matrix(mapping, registry)
    if value != expected:
        raise MatrixError("support matrix is stale or differs from deterministic source inputs")
    if value["claimPolicy"]["advertisedRows"] != 0 or value["claimPolicy"]["qualifiedFormatParityRows"] != 0:
        raise MatrixError("source-only matrix cannot advertise or qualify format support")
    if not value["combined"]["zeroTargetUnmapped"] or value["combined"]["targetUnmappedRows"] != 0:
        raise MatrixError("combined target-unmapped result is not zero")


def write_matrix(path: Path, mapping: dict, registry: dict) -> None:
    value = build_matrix(mapping, registry)
    path.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def self_test() -> None:
    mapping = {
        "schema": 1,
        "kind": "libdxfrw-source-route-inventory",
        "fixturePolicy": SOURCE_ONLY_POLICY,
        "provenance": {"target": {"repository": "example", "commit": "abc"}},
        "mapping": {
            "rows": [
                {"facade": "dxfRW", "targetRouteId": "dxfRW/a", "fixtureDisposition": "no-drawing-fixture", "disposition": "equivalent", "implementationState": "same-selector", "category": "parser"},
                {"facade": "dwgRW", "targetRouteId": "dwgRW/a", "fixtureDisposition": "no-drawing-fixture", "disposition": "delta-review", "implementationState": "selector-delta", "category": "reader"},
                {"facade": "shared", "targetRouteId": "shared/a", "fixtureDisposition": "no-drawing-fixture", "disposition": "equivalent", "implementationState": "same-selector", "category": "shared"},
            ],
            "summary": {"targetOnly": 0},
            "standaloneUnmappedRouteIds": [],
        },
    }
    registry = {
        "schema": 1,
        "kind": "librecad-parity-test-oracle-registry",
        "fixturePolicy": SOURCE_ONLY_POLICY,
        "routes": [{"promotesSupport": False}],
        "featureLinks": [{"promotesSupport": False}],
    }
    value = build_matrix(mapping, registry)
    validate_matrix(value, mapping, registry)
    with tempfile.TemporaryDirectory() as directory:
        path = Path(directory) / "matrix.json"
        write_matrix(path, mapping, registry)
        assert json.loads(path.read_text(encoding="utf-8")) == value
    tampered = json.loads(json.dumps(value))
    tampered["claimPolicy"]["advertisedRows"] = 1
    try:
        validate_matrix(tampered, mapping, registry)
    except MatrixError:
        pass
    else:
        raise AssertionError("tampered support claim was accepted")
    print("check_support_matrix self-test: PASS")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--mapping", type=Path, default=Path("metadata/parity-source-routes-v1.json"))
    parser.add_argument("--registry", type=Path, default=Path("metadata/parity-test-oracles-v1.json"))
    parser.add_argument("--matrix", type=Path, default=Path("metadata/support-matrix-v1.json"))
    parser.add_argument("--write", action="store_true", help="write the deterministic matrix")
    args = parser.parse_args(argv)
    try:
        if args.self_test:
            self_test()
            return 0
        mapping = read_json(args.mapping)
        registry = read_json(args.registry)
        if args.write:
            write_matrix(args.matrix, mapping, registry)
            print("support matrix: WROTE %s" % args.matrix)
        else:
            validate_matrix(read_json(args.matrix), mapping, registry)
            print("support matrix: PASS (%s)" % args.matrix)
        return 0
    except (OSError, UnicodeError, json.JSONDecodeError, MatrixError, AssertionError) as exc:
        print("support matrix: FAIL: %s" % exc)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
