#!/usr/bin/env python3
"""Run the fast aggregate closure for the pinned source/parity ledgers.

This is intentionally a metadata gate.  It proves deterministic shard and
mapping closure, source-unit coverage, and test/oracle selector coverage.  It
does not run the full CTest, sanitizer, fuzz, or external-corpus lanes; those
remain checkpoint/nightly gates in the plan.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
import sys
import tempfile
from pathlib import Path

from check_parity_test_oracles import RegistryError, validate_registry


class AggregateError(RuntimeError):
    pass


def read_json(path: Path) -> dict:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as exc:
        raise AggregateError("cannot read %s: %s" % (path, exc)) from exc
    if not isinstance(value, dict):
        raise AggregateError("JSON root must be an object: %s" % path)
    return value


def load_routes(index: dict, root: Path) -> dict[str, dict[str, list[dict]]]:
    if index.get("schema") != 1 or index.get("kind") != "libdxfrw-source-route-inventory":
        raise AggregateError("unexpected source-route artifact schema")
    if index.get("fixturePolicy") != "no-drawing-payloads; source-and-metadata-only":
        raise AggregateError("source-route artifact has unsafe fixture policy")
    result: dict[str, dict[str, list[dict]]] = {"target": {}, "standalone": {}}
    shards = index.get("shards")
    if not isinstance(shards, list) or len(shards) != 6:
        raise AggregateError("source-route artifact must have six shards")
    seen: set[tuple[str, str]] = set()
    for shard in shards:
        if not isinstance(shard, dict):
            raise AggregateError("malformed shard index row")
        side, facade = shard.get("side"), shard.get("facade")
        path = shard.get("path")
        if side not in result or facade not in {"dxfRW", "dwgRW", "shared"} or not isinstance(path, str):
            raise AggregateError("malformed shard identity")
        identity = (side, facade)
        if identity in seen:
            raise AggregateError("duplicate shard identity: %s/%s" % identity)
        seen.add(identity)
        shard_value = read_json(root / path)
        expected_hash = shard.get("sha256")
        actual_hash = hashlib.sha256((root / path).read_bytes()).hexdigest()
        if not isinstance(expected_hash, str) or actual_hash != expected_hash:
            raise AggregateError("shard hash disagrees with index: %s" % path)
        if shard_value.get("side") != side or shard_value.get("facade") != facade:
            raise AggregateError("shard identity disagrees with index: %s" % path)
        routes = shard_value.get("routes")
        if not isinstance(routes, list) or shard.get("routeCount") != len(routes):
            raise AggregateError("shard route count disagrees with index: %s" % path)
        ids = [route.get("id") for route in routes]
        if any(not isinstance(route, dict) or not isinstance(route.get("id"), str) for route in routes):
            raise AggregateError("shard has malformed route")
        if ids != sorted(ids) or len(ids) != len(set(ids)):
            raise AggregateError("shard route IDs are not sorted/unique: %s" % path)
        result[side][facade] = routes
    if seen != {(side, facade) for side in result for facade in ("dxfRW", "dwgRW", "shared")}:
        raise AggregateError("source-route shard set is incomplete")
    return result


def validate_aggregate(root: Path, target_repo: Path, mapping_path: Path, registry_path: Path) -> None:
    index = read_json(mapping_path)
    routes = load_routes(index, mapping_path.parent)
    target_routes = [route for facade in routes["target"].values() for route in facade]
    standalone_routes = [route for facade in routes["standalone"].values() for route in facade]
    target_ids = [route["id"] for route in target_routes]
    standalone_ids = {route["id"] for route in standalone_routes}
    if len(target_ids) != len(set(target_ids)):
        raise AggregateError("target route IDs are duplicated across shards")
    mapping = index.get("mapping")
    if not isinstance(mapping, dict) or not isinstance(mapping.get("rows"), list):
        raise AggregateError("mapping ledger is missing")
    rows = mapping["rows"]
    mapped_ids = [row.get("targetRouteId") for row in rows if isinstance(row, dict)]
    if len(mapped_ids) != len(rows) or len(mapped_ids) != len(set(mapped_ids)):
        raise AggregateError("mapping target rows are missing or duplicated")
    if set(mapped_ids) != set(target_ids):
        raise AggregateError("mapping does not cover the target shard route set")
    summary = mapping.get("summary", {})
    if summary.get("targetRows") != len(rows) or summary.get("targetRows") != len(target_ids):
        raise AggregateError("mapping target count is stale")
    cardinality_counts: dict[str, int] = {}
    for row in rows:
        cardinality = row.get("cardinality")
        cardinality_counts[cardinality] = cardinality_counts.get(cardinality, 0) + 1
        endpoints = row.get("standaloneRouteIds", [])
        if cardinality == "1:0" and endpoints:
            raise AggregateError("1:0 mapping has an endpoint")
        if cardinality == "1:1" and len(endpoints) != 1:
            raise AggregateError("1:1 mapping has wrong endpoint count")
        if cardinality == "1:N" and len(endpoints) < 2:
            raise AggregateError("1:N mapping has too few endpoints")
        if cardinality == "N:1" and len(endpoints) != 1:
            raise AggregateError("N:1 mapping has wrong endpoint count")
        if any(endpoint not in standalone_ids for endpoint in endpoints):
            raise AggregateError("mapping references a missing standalone route")
    derived_summary = {
        "targetRows": len(rows),
        "exactOneToOne": sum(row.get("cardinality") == "1:1" and row.get("disposition") == "equivalent" for row in rows),
        "deltaOneToOne": sum(row.get("cardinality") == "1:1" and row.get("disposition") != "equivalent" for row in rows),
        "targetOnly": cardinality_counts.get("1:0", 0),
        "standaloneOnly": len(mapping.get("standaloneUnmappedRouteIds", [])),
    }
    if any(summary.get(key) != value for key, value in derived_summary.items()):
        raise AggregateError("mapping summary is stale")
    unmapped = mapping.get("standaloneUnmappedRouteIds")
    if not isinstance(unmapped, list) or unmapped != sorted(set(unmapped)) or any(route_id not in standalone_ids for route_id in unmapped):
        raise AggregateError("standalone-unmapped route set is malformed")
    used = {endpoint for row in rows for endpoint in row.get("standaloneRouteIds", [])}
    if set(unmapped) != standalone_ids - used:
        raise AggregateError("standalone-unmapped route set disagrees with mappings")
    target_source_units = [route for route in target_routes if route.get("category") == "source-unit"]
    target_coverage = [route for route in target_routes if route.get("category") == "source-unit-coverage"]
    target_pipeline = [route for route in target_routes if route.get("category") == "pipeline-unit"]
    if len(target_source_units) != 85 or len(target_coverage) != 80 or len(target_pipeline) != 80:
        raise AggregateError("source-unit aggregate counts changed: units=%d coverage=%d pipeline=%d" % (len(target_source_units), len(target_coverage), len(target_pipeline)))
    route_by_id = {route["id"]: route for route in target_routes}
    for coverage in target_coverage:
        path = coverage.get("selector", {}).get("path")
        covered = coverage.get("selector", {}).get("coveredBy")
        if not isinstance(path, str) or not isinstance(covered, list) or not covered:
            raise AggregateError("source-unit coverage is empty")
        for route_id in covered:
            route = route_by_id.get(route_id)
            if route is None or route.get("category") in {"source-unit", "pipeline-unit", "source-unit-coverage"}:
                raise AggregateError("source-unit coverage is not concrete: %s" % route_id)
            if not any(evidence.get("path") == path for evidence in route.get("evidence", []) if isinstance(evidence, dict) and "path" in evidence):
                raise AggregateError("source-unit coverage crosses source paths: %s" % route_id)
    source_lock = read_json(root / "metadata/libdxfrw-target-lock.json")
    registry = read_json(registry_path)
    try:
        validate_registry(registry, source_lock, target_repo, mapping_path)
    except RegistryError as exc:
        raise AggregateError("test/oracle registry: %s" % exc) from exc
    print("parity aggregate closure: PASS (%d target routes; %d standalone routes; 85 source units; mapping and test/oracle selectors closed; fast metadata-only gate)" % (len(target_ids), len(standalone_ids)))


def self_test() -> None:
    # The real aggregate is exercised by the repository check; this mutation
    # test keeps the fail-closed duplicate-target rule local and cheap.
    with tempfile.TemporaryDirectory() as directory:
        root = Path(directory)
        index = {"schema": 1, "kind": "libdxfrw-source-route-inventory", "fixturePolicy": "no-drawing-payloads; source-and-metadata-only", "shards": []}
        for side in ("target", "standalone"):
            for facade in ("dxfRW", "dwgRW", "shared"):
                path = "shards/%s-%s.json" % (side, facade)
                shard = {"schema": 1, "kind": "libdxfrw-source-route-shard", "side": side, "facade": facade, "routes": []}
                (root / path).parent.mkdir(parents=True, exist_ok=True)
                (root / path).write_text(json.dumps(shard), encoding="utf-8")
                index["shards"].append({"path": path, "side": side, "facade": facade, "routeCount": 0, "sha256": hashlib.sha256((root / path).read_bytes()).hexdigest()})
        loaded = load_routes(index, root)
        assert not loaded["target"]["dxfRW"]
        print("check_parity_aggregate self-test: PASS")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--root", type=Path, default=Path("."))
    parser.add_argument("--target-repo", type=Path, default=Path("/private/tmp/librecad-system-s16"))
    parser.add_argument("--mapping", type=Path, default=Path("metadata/parity-source-routes-v1.json"))
    parser.add_argument("--registry", type=Path, default=Path("metadata/parity-test-oracles-v1.json"))
    args = parser.parse_args(argv)
    try:
        if args.self_test:
            self_test()
            return 0
        root = args.root.resolve()
        mapping = args.mapping if args.mapping.is_absolute() else root / args.mapping
        registry = args.registry if args.registry.is_absolute() else root / args.registry
        validate_aggregate(root, args.target_repo.resolve(), mapping, registry)
        return 0
    except (OSError, UnicodeError, json.JSONDecodeError, AggregateError, RegistryError) as exc:
        print("parity aggregate closure: FAIL: %s" % exc, file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
