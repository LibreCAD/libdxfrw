#!/usr/bin/env python3
"""Validate the metadata-only queue of runtime evidence still needing proof.

The queue deliberately carries paths, classifications, and hash/status
metadata only.  It never opens or copies a DWG/DXF payload, so it is safe to
run in the fast inner loop while fixture/oracle evidence is unavailable.
"""

from __future__ import annotations

import argparse
import json
import tempfile
from pathlib import Path


class QueueError(RuntimeError):
    pass


def load_json(path: Path):
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as exc:
        raise QueueError(f"could not read {path}: {exc}") from exc


def build_queue(registry, advisory):
    if not isinstance(registry, dict) or registry.get("schema") != 1:
        raise QueueError("registry schema must be 1")
    entries = registry.get("entries")
    routes = registry.get("routes")
    if not isinstance(entries, list) or not isinstance(routes, list):
        raise QueueError("registry entries/routes must be arrays")

    entry_by_id = {}
    for entry in entries:
        if not isinstance(entry, dict) or not isinstance(entry.get("id"), str):
            raise QueueError("every registry entry needs a string id")
        entry_id = entry["id"]
        if entry_id in entry_by_id:
            raise QueueError(f"duplicate registry entry: {entry_id}")
        entry_by_id[entry_id] = entry

    queue = []
    route_ids = set()
    for route in routes:
        if not isinstance(route, dict) or not isinstance(route.get("id"), str):
            raise QueueError("every oracle route needs a string id")
        route_id = route["id"]
        if route_id in route_ids:
            raise QueueError(f"duplicate oracle route: {route_id}")
        route_ids.add(route_id)
        if route.get("promotesSupport") is not False:
            raise QueueError(f"runtime queue route must be non-promoting: {route_id}")
        entry_ids = route.get("entryIds")
        if not isinstance(entry_ids, list):
            raise QueueError(f"route {route_id} entryIds must be an array")
        for entry_id in entry_ids:
            if entry_id not in entry_by_id:
                raise QueueError(f"route {route_id} names unknown entry {entry_id}")
            entry = entry_by_id[entry_id]
            classification = entry.get("classification")
            if classification in {"fixture-blocked", "external-advisory"}:
                queue.append({
                    "route": route_id,
                    "entry": entry_id,
                    "classification": classification,
                    "path": entry.get("path"),
                })

    if not isinstance(advisory, dict):
        raise QueueError("advisory report must be an object")
    # The original queue contract used a row-oriented schema with an
    # ``advisory: true`` marker.  The convergence plan now records the same
    # non-promoting boundary in a compact, metadata-only S07 summary.  Accept
    # both forms so the fast CTest gate cannot regress when the report is
    # refreshed without reintroducing drawing bytes.
    if advisory.get("schema") == "libdxfrw-external-advisory-report-v1":
        run = advisory.get("run")
        deferred = advisory.get("deferredExternal")
        if not isinstance(run, dict) or run.get("ordinaryCiEligible") is not False:
            raise QueueError("advisory report must be explicitly non-promoting")
        if run.get("promotesSupportClaims") is not False:
            raise QueueError("advisory report must not promote support claims")
        if not isinstance(deferred, list):
            raise QueueError("advisory report deferredExternal must be an array")
        for row in deferred:
            if not isinstance(row, dict):
                raise QueueError("deferred advisory row must be an object")
            if row.get("status") != "DEFERRED_EXTERNAL":
                raise QueueError("deferred advisory row has an unknown status")
            if row.get("promotesSupportClaims") is not False:
                raise QueueError("deferred advisory row must be non-promoting")
        return queue, deferred
    if advisory.get("advisory") is not True:
        raise QueueError("advisory report must be explicitly non-promoting")
    rows = advisory.get("rows")
    if not isinstance(rows, list):
        raise QueueError("advisory report rows must be an array")
    for row in rows:
        if not isinstance(row, dict):
            raise QueueError("advisory report row must be an object")
        if row.get("status") not in {"converted", "failed", "timeout"}:
            raise QueueError("advisory row has an unknown status")
        if "sourceSha256" not in row or "sourceSize" not in row:
            raise QueueError("advisory row must carry source hash and size")
    return queue, rows


def validate(registry_path: Path, advisory_path: Path):
    queue, rows = build_queue(load_json(registry_path), load_json(advisory_path))
    print(
        "runtime evidence queue: PASS (%d fixture/oracle-blocked entries, "
        "%d advisory rows; payload bytes not read)" % (len(queue), len(rows))
    )


def self_test():
    registry = {
        "schema": 1,
        "entries": [
            {"id": "e1", "classification": "fixture-blocked", "path": "tests/a"},
            {"id": "e2", "classification": "portable", "path": "tests/b"},
        ],
        "routes": [
            {"id": "r1", "entryIds": ["e1", "e2"], "promotesSupport": False}
        ],
    }
    advisory = {
        "schema": 1,
        "advisory": True,
        "rows": [
            {"status": "timeout", "sourceSha256": "abc", "sourceSize": 3}
        ],
    }
    with tempfile.TemporaryDirectory() as directory:
        root = Path(directory)
        registry_path = root / "registry.json"
        advisory_path = root / "advisory.json"
        registry_path.write_text(json.dumps(registry), encoding="utf-8")
        advisory_path.write_text(json.dumps(advisory), encoding="utf-8")
        queue, rows = build_queue(load_json(registry_path), load_json(advisory_path))
        assert len(queue) == 1 and queue[0]["entry"] == "e1"
        assert len(rows) == 1 and rows[0]["status"] == "timeout"
        summary_path = root / "summary.json"
        summary_path.write_text(json.dumps({
            "schema": "libdxfrw-external-advisory-report-v1",
            "run": {"ordinaryCiEligible": False,
                    "promotesSupportClaims": False},
            "deferredExternal": [
                {"id": "J256", "status": "DEFERRED_EXTERNAL",
                 "promotesSupportClaims": False}]}), encoding="utf-8")
        _, summary_rows = build_queue(load_json(registry_path),
                                      load_json(summary_path))
        assert len(summary_rows) == 1
    print("check_runtime_evidence_queue self-test: PASS")


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--registry", type=Path)
    parser.add_argument("--advisory", type=Path)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args(argv)
    try:
        if args.self_test:
            self_test()
            return 0
        if args.registry is None or args.advisory is None:
            parser.error("--registry and --advisory are required unless --self-test is used")
        validate(args.registry, args.advisory)
        return 0
    except QueueError as exc:
        print(f"runtime evidence queue: FAIL: {exc}")
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
