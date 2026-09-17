#!/usr/bin/env python3
"""Check source-only DWG reader/writer parity lane contracts.

The gate is deliberately independent of DWG wire fixtures.  It verifies that
the pinned target route inventory, standalone inventory, and mapping expose
the required version/container/dispatch/graph or writer-binding contracts;
sample/spec/oracle qualification remains a later evidence lane.
"""

from __future__ import annotations

import argparse
import json
import tempfile
from pathlib import Path


READER_CATEGORIES = {
    "reader-version", "reader-pipeline", "reader-stage", "section",
    "section-declaration", "section-fallback", "named-object-class",
    "named-entity-class", "fixed-entity", "fixed-object", "raw-object-shell",
    "raw-entity-shell", "raw-custom-shell", "parser-publication", "raw-flow",
    "raw-route", "table-descriptor", "object-context-class",
}
WRITER_CATEGORIES = {
    "writer-version", "writer-pipeline", "writer-binding", "writer-entrypoint",
    "raw-flow", "raw-route", "parser-publication", "facade-method",
}
READER_PIPELINES = {"dwgRW/reader-pipeline/15", "dwgRW/reader-pipeline/18", "dwgRW/reader-pipeline/21", "dwgRW/reader-pipeline/24", "dwgRW/reader-pipeline/27", "dwgRW/reader-pipeline/32"}
WRITER_PIPELINES = {"dwgRW/writer-pipeline/15", "dwgRW/writer-pipeline/18", "dwgRW/writer-pipeline/21", "dwgRW/writer-pipeline/24", "dwgRW/writer-pipeline/27", "dwgRW/writer-pipeline/32"}
CARDINALITIES = {"1:1", "1:N", "N:1"}


class DwgLaneError(ValueError):
    pass


def read_shard(path: Path) -> dict:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as exc:
        raise DwgLaneError("cannot read %s: %s" % (path, exc)) from exc
    if not isinstance(value, dict) or not isinstance(value.get("routes"), list):
        raise DwgLaneError("%s is not a route shard" % path)
    return value


def read_mapping(path: Path) -> dict:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as exc:
        raise DwgLaneError("cannot read %s: %s" % (path, exc)) from exc
    if not isinstance(value, dict) or not isinstance(value.get("mapping"), dict):
        raise DwgLaneError("%s is not a parity mapping" % path)
    return value


def validate_mode(mode: str, mapping_path: Path, target_path: Path, standalone_path: Path) -> None:
    mapping = read_mapping(mapping_path)
    target = read_shard(target_path)
    standalone = read_shard(standalone_path)
    target_routes = [route for route in target["routes"] if route.get("facade") == "dwgRW"]
    standalone_routes = [route for route in standalone["routes"] if route.get("facade") == "dwgRW"]
    if not target_routes or not standalone_routes:
        raise DwgLaneError("target and standalone DWG route sets must be non-empty")
    categories = READER_CATEGORIES if mode == "reader" else WRITER_CATEGORIES
    target_ids = {route.get("id") for route in target_routes}
    standalone_ids = {route.get("id") for route in standalone_routes}
    observed = {route.get("category") for route in target_routes}
    missing = categories - observed
    if missing:
        raise DwgLaneError("DWG %s categories missing: %s" % (mode, ", ".join(sorted(missing))))
    required_pipelines = READER_PIPELINES if mode == "reader" else WRITER_PIPELINES
    if not required_pipelines <= target_ids:
        raise DwgLaneError("DWG %s version pipelines are incomplete" % mode)
    rows = mapping.get("mapping", {}).get("rows")
    if not isinstance(rows, list):
        raise DwgLaneError("mapping has no rows")
    lane_rows = [row for row in rows if row.get("facade") == "dwgRW" and row.get("category") in categories]
    by_target = {}
    for row in lane_rows:
        target_id = row.get("targetRouteId")
        by_target.setdefault(target_id, []).append(row)
        if target_id not in target_ids:
            raise DwgLaneError("mapping references unknown DWG target route: %s" % target_id)
        if row.get("cardinality") not in CARDINALITIES or not row.get("standaloneRouteIds"):
            raise DwgLaneError("mapping row is not closed: %s" % target_id)
        if row.get("fixtureDisposition") != "no-drawing-fixture":
            raise DwgLaneError("DWG source lane cannot admit a drawing fixture: %s" % target_id)
        if any(route_id not in standalone_ids for route_id in row["standaloneRouteIds"]):
            raise DwgLaneError("mapping references unknown standalone route: %s" % target_id)
    expected = {route["id"] for route in target_routes if route.get("category") in categories}
    if set(by_target) != expected or any(len(items) != 1 for items in by_target.values()):
        raise DwgLaneError("DWG %s target mapping is incomplete or duplicated" % mode)
    if mode == "writer":
        for route in target_routes:
            if route.get("category") != "writer-entrypoint":
                continue
            selector = route.get("selector", {})
            if selector.get("providerKind") != "dwgWriter-hierarchy" or selector.get("pipelineSelection") != "runtime-version-or-dialect-selected":
                raise DwgLaneError("writer provider/pipeline contract missing: %s" % route["id"])
            if selector.get("finalizerDisposition") not in {"output-transaction-commit", "operation-result-transaction", "delegated-provider-return"}:
                raise DwgLaneError("writer finalizer contract missing: %s" % route["id"])
    print("DWG %s parity lane: PASS (%d target routes; %d standalone routes; source-only fast gate)" % (mode, len(target_routes), len(standalone_routes)))


def self_test() -> None:
    categories = sorted(READER_CATEGORIES | WRITER_CATEGORIES)
    routes = [{"id": "dwgRW/%s/%03d" % (category, index), "facade": "dwgRW", "category": category, "selector": {}} for index, category in enumerate(categories)]
    routes.extend({"id": pipeline, "facade": "dwgRW", "category": pipeline.split("/")[1], "selector": {}} for pipeline in sorted(READER_PIPELINES | WRITER_PIPELINES))
    for route in routes:
        if route["category"] == "writer-entrypoint":
            route["selector"] = {"providerKind": "dwgWriter-hierarchy", "pipelineSelection": "runtime-version-or-dialect-selected", "finalizerDisposition": "delegated-provider-return"}
    rows = [{"facade": "dwgRW", "category": route["category"], "targetRouteId": route["id"], "standaloneRouteIds": [route["id"]], "cardinality": "1:1", "fixtureDisposition": "no-drawing-fixture"} for route in routes]
    value = {"mapping": {"rows": rows}}
    with tempfile.TemporaryDirectory(prefix="libdxfrw-dwg-lane-") as directory:
        root = Path(directory)
        (root / "mapping.json").write_text(json.dumps(value), encoding="utf-8")
        shard = {"routes": routes}
        (root / "target.json").write_text(json.dumps(shard), encoding="utf-8")
        (root / "standalone.json").write_text(json.dumps(shard), encoding="utf-8")
        validate_mode("reader", root / "mapping.json", root / "target.json", root / "standalone.json")
        validate_mode("writer", root / "mapping.json", root / "target.json", root / "standalone.json")
    print("check_dwg_lane self-test: PASS")


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--mode", choices=("reader", "writer"), default="reader")
    parser.add_argument("--mapping", type=Path, default=Path("metadata/parity-source-routes-v1.json"))
    parser.add_argument("--target", type=Path, default=Path("metadata/parity-source-routes-v1/target-dwgRW.json"))
    parser.add_argument("--standalone", type=Path, default=Path("metadata/parity-source-routes-v1/standalone-dwgRW.json"))
    args = parser.parse_args(argv)
    try:
        if args.self_test:
            self_test()
        else:
            validate_mode(args.mode, args.mapping, args.target, args.standalone)
        return 0
    except (OSError, UnicodeError, DwgLaneError, AssertionError) as exc:
        print("DWG parity lane: FAIL: %s" % exc)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
