#!/usr/bin/env python3
"""Check the source-only DXF parity lane before expensive runtime evidence.

This gate validates route ownership, mapping closure, transport anchors, raw
eligibility, and writer contracts.  It deliberately does not promote format
support and never reads or creates a drawing fixture.
"""

from __future__ import annotations

import argparse
import json
import tempfile
from pathlib import Path


REQUIRED_CATEGORIES = {
    "dxf-transport-node",
    "group-code-reader-range",
    "group-code-raw-rule",
    "group-code-raw-classifier",
    "dxf-entity",
    "dxf-object",
    "dxf-class",
    "dxf-table",
    "dxf-section",
    "parser-publication",
    "raw-flow",
    "raw-route",
    "writer-entrypoint",
    "writer-version",
}
TRANSPORT_ANCHORS = {
    "dxfRW/dxf-transport-node/writer-ascii",
    "dxfRW/dxf-transport-node/writer-binary",
    "dxfRW/dxf-transport-node/writer-binary-r12",
    "dxfRW/dxf-transport-node/reader-ascii",
    "dxfRW/dxf-transport-node/reader-binary",
    "dxfRW/dxf-transport-node/reader-binary-r12",
}
CARDINALITIES = {"1:1", "1:N", "N:1"}


class DxfLaneError(ValueError):
    pass


def read(path: Path) -> dict:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as exc:
        raise DxfLaneError("cannot read %s: %s" % (path, exc)) from exc
    if not isinstance(value, dict) or not isinstance(value.get("routes"), list):
        raise DxfLaneError("%s is not a route shard" % path)
    return value


def read_mapping(path: Path) -> dict:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as exc:
        raise DxfLaneError("cannot read %s: %s" % (path, exc)) from exc
    if not isinstance(value, dict) or not isinstance(value.get("mapping"), dict):
        raise DxfLaneError("%s is not a parity mapping" % path)
    return value


def validate(mapping_path: Path, target_path: Path, standalone_path: Path) -> None:
    mapping = read_mapping(mapping_path)
    target = read(target_path)
    standalone = read(standalone_path)
    target_routes = [route for route in target["routes"] if route.get("facade") == "dxfRW"]
    standalone_routes = [route for route in standalone["routes"] if route.get("facade") == "dxfRW"]
    if not target_routes or not standalone_routes:
        raise DxfLaneError("target and standalone DXF route sets must be non-empty")
    target_ids = [route.get("id") for route in target_routes]
    standalone_ids = [route.get("id") for route in standalone_routes]
    if any(not isinstance(route_id, str) or not route_id.startswith("dxfRW/") for route_id in target_ids + standalone_ids):
        raise DxfLaneError("DXF route IDs must be façade-owned")
    if len(target_ids) != len(set(target_ids)) or len(standalone_ids) != len(set(standalone_ids)):
        raise DxfLaneError("DXF route IDs must be unique")
    target_categories = {route.get("category") for route in target_routes}
    missing = REQUIRED_CATEGORIES - target_categories
    if missing:
        raise DxfLaneError("DXF route categories missing: %s" % ", ".join(sorted(missing)))
    if not TRANSPORT_ANCHORS <= set(target_ids):
        raise DxfLaneError("DXF transport anchors are incomplete")
    target_by_id = {route["id"]: route for route in target_routes}
    standalone_by_id = {route["id"]: route for route in standalone_routes}
    rows = mapping.get("mapping", {}).get("rows")
    if not isinstance(rows, list):
        raise DxfLaneError("mapping has no rows")
    dxf_rows = [row for row in rows if row.get("facade") == "dxfRW"]
    by_target = {}
    for row in dxf_rows:
        target_id = row.get("targetRouteId")
        by_target.setdefault(target_id, []).append(row)
        if target_id not in target_by_id:
            raise DxfLaneError("mapping references unknown DXF target route: %s" % target_id)
        if row.get("cardinality") not in CARDINALITIES or not row.get("standaloneRouteIds"):
            raise DxfLaneError("mapping row is not a closed DXF route: %s" % target_id)
        if row.get("fixtureDisposition") != "no-drawing-fixture":
            raise DxfLaneError("DXF source lane cannot admit a drawing fixture: %s" % target_id)
        if any(route_id not in standalone_by_id for route_id in row["standaloneRouteIds"]):
            raise DxfLaneError("mapping references unknown standalone route: %s" % target_id)
    if set(by_target) != set(target_by_id) or any(len(rows_for_id) != 1 for rows_for_id in by_target.values()):
        raise DxfLaneError("DXF target route mapping is incomplete or duplicated")
    for route in target_routes:
        if route.get("category") != "writer-entrypoint":
            continue
        selector = route.get("selector", {})
        if selector.get("providerKind") != "dxfWriter-hierarchy":
            raise DxfLaneError("writer provider ownership missing: %s" % route["id"])
        if selector.get("pipelineSelection") != "runtime-version-or-dialect-selected":
            raise DxfLaneError("writer pipeline selection missing: %s" % route["id"])
        if selector.get("finalizerDisposition") not in {"output-transaction-commit", "operation-result-transaction", "delegated-provider-return"}:
            raise DxfLaneError("writer finalizer disposition missing: %s" % route["id"])
    print("DXF parity lane: PASS (%d target routes; %d standalone routes; source-only fast gate)" % (len(target_routes), len(standalone_routes)))


def self_test() -> None:
    route = lambda route_id, category: {"id": route_id, "facade": "dxfRW", "category": category, "selector": {}}
    categories = sorted(REQUIRED_CATEGORIES)
    anchors = sorted(TRANSPORT_ANCHORS)
    routes = [route("dxfRW/%s/%03d" % (category, i), category) for i, category in enumerate(categories)]
    routes.extend(route(anchor, "dxf-transport-node") for anchor in anchors if anchor not in {item["id"] for item in routes})
    for item in routes:
        if item["category"] == "writer-entrypoint":
            item["selector"] = {"providerKind": "dxfWriter-hierarchy", "pipelineSelection": "runtime-version-or-dialect-selected", "finalizerDisposition": "delegated-provider-return"}
    mapping = {"mapping": {"rows": [{"facade": "dxfRW", "targetRouteId": item["id"], "standaloneRouteIds": [item["id"]], "cardinality": "1:1", "fixtureDisposition": "no-drawing-fixture"} for item in routes]}}
    with tempfile.TemporaryDirectory(prefix="libdxfrw-dxf-lane-") as directory:
        root = Path(directory)
        (root / "mapping.json").write_text(json.dumps(mapping), encoding="utf-8")
        shard = {"routes": routes}
        (root / "target.json").write_text(json.dumps(shard), encoding="utf-8")
        (root / "standalone.json").write_text(json.dumps(shard), encoding="utf-8")
        validate(root / "mapping.json", root / "target.json", root / "standalone.json")
    print("check_dxf_lane self-test: PASS")


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--mapping", type=Path, default=Path("metadata/parity-source-routes-v1.json"))
    parser.add_argument("--target", type=Path, default=Path("metadata/parity-source-routes-v1/target-dxfRW.json"))
    parser.add_argument("--standalone", type=Path, default=Path("metadata/parity-source-routes-v1/standalone-dxfRW.json"))
    args = parser.parse_args(argv)
    try:
        if args.self_test:
            self_test()
        else:
            validate(args.mapping, args.target, args.standalone)
        return 0
    except (OSError, UnicodeError, DxfLaneError, AssertionError) as exc:
        print("DXF parity lane: FAIL: %s" % exc)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
