#!/usr/bin/env python3
"""Check reviewed target/package differential deltas.

The differential runner remains fail-closed for every mismatch.  This helper
is a separate release-review gate: it accepts only deltas whose source,
target/standalone output hashes, and normalized-summary hashes match a
metadata-only reviewed-debt registry.  It never opens drawing payloads.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import tempfile
from pathlib import Path
from typing import Any


SCHEMA = 1
KIND = "libdxfrw-differential-debt"
REPORT_KIND = "libdxfrw-json-target-package-differential"


class DebtError(ValueError):
    """Raised when the reviewed-debt contract is not satisfied."""


def load_json(path: Path) -> object:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as exc:
        raise DebtError(f"cannot read {path}: {exc}") from exc


def canonical_digest(value: object) -> str:
    try:
        encoded = json.dumps(
            value, sort_keys=True, separators=(",", ":"),
            ensure_ascii=False,
        ).encode("utf-8")
    except (TypeError, UnicodeError) as exc:
        raise DebtError(f"cannot canonicalize summary: {exc}") from exc
    return hashlib.sha256(encoded).hexdigest()


def _hash(value: object, label: str) -> str:
    if not isinstance(value, str) or len(value) != 64:
        raise DebtError(f"{label} must be a 64-character SHA-256 hex string")
    try:
        int(value, 16)
    except ValueError as exc:
        raise DebtError(f"{label} is not hexadecimal") from exc
    return value


def load_registry(path: Path) -> dict[str, Any]:
    value = load_json(path)
    if not isinstance(value, dict):
        raise DebtError("debt registry root must be an object")
    if value.get("schema") != SCHEMA or value.get("kind") != KIND:
        raise DebtError("unexpected debt registry schema/kind")
    target_commit = value.get("targetCommit")
    if not isinstance(target_commit, str) or not target_commit:
        raise DebtError("debt registry needs targetCommit")
    entries = value.get("entries")
    if not isinstance(entries, list) or not entries:
        raise DebtError("debt registry needs non-empty entries")
    by_source: dict[str, dict[str, Any]] = {}
    ids: set[str] = set()
    for index, entry in enumerate(entries, 1):
        if not isinstance(entry, dict):
            raise DebtError(f"debt entry {index} is not an object")
        entry_id = entry.get("id")
        if not isinstance(entry_id, str) or not entry_id or entry_id in ids:
            raise DebtError(f"debt entry {index} has a duplicate/invalid id")
        ids.add(entry_id)
        source = _hash(entry.get("sourceSha256"), f"debt {entry_id} sourceSha256")
        version = entry.get("inputVersion")
        if not isinstance(version, str) or not version:
            raise DebtError(f"debt {entry_id} needs inputVersion")
        if entry.get("relation") != "delta":
            raise DebtError(f"debt {entry_id} must describe a delta")
        reason = entry.get("reason")
        if not isinstance(reason, str) or not reason.strip():
            raise DebtError(f"debt {entry_id} needs a review reason")
        for side in ("target", "standalone"):
            side_value = entry.get(side)
            if not isinstance(side_value, dict):
                raise DebtError(f"debt {entry_id} needs {side} hashes")
            _hash(side_value.get("outputSha256"),
                  f"debt {entry_id} {side}.outputSha256")
            _hash(side_value.get("summarySha256"),
                  f"debt {entry_id} {side}.summarySha256")
        if source in by_source:
            raise DebtError(f"duplicate debt source hash: {source}")
        by_source[source] = entry
    return {"targetCommit": target_commit, "entries": by_source}


def check(report_path: Path, registry_path: Path) -> dict[str, int]:
    registry = load_registry(registry_path)
    report = load_json(report_path)
    if not isinstance(report, dict) or report.get("kind") != REPORT_KIND:
        raise DebtError("report is not a JSON target/package differential")
    if not isinstance(report.get("targetCommit"), str):
        raise DebtError("differential report has no targetCommit")
    if report["targetCommit"] != registry["targetCommit"]:
        raise DebtError("differential target commit does not match debt registry")
    rows = report.get("rows")
    if not isinstance(rows, list):
        raise DebtError("differential report has no rows")
    seen: set[str] = set()
    reviewed = 0
    unreviewed = 0
    for index, row in enumerate(rows, 1):
        if not isinstance(row, dict):
            raise DebtError(f"differential row {index} is not an object")
        relation = row.get("relation")
        if relation == "equal":
            continue
        source = row.get("sourceSha256")
        entry = registry["entries"].get(source)
        if entry is None:
            unreviewed += 1
            continue
        if source in seen:
            raise DebtError(f"reviewed debt appears more than once: {source}")
        seen.add(source)
        if row.get("inputVersion") != entry["inputVersion"]:
            raise DebtError(f"reviewed debt version changed: {source}")
        for side in ("target", "standalone"):
            actual = row.get(side)
            expected = entry[side]
            if not isinstance(actual, dict):
                raise DebtError(f"reviewed debt row has no {side} result: {source}")
            if actual.get("outputSha256") != expected["outputSha256"]:
                raise DebtError(f"reviewed debt {side} output hash changed: {source}")
            summary = actual.get("summary")
            if not isinstance(summary, dict):
                raise DebtError(f"reviewed debt row has no {side} summary: {source}")
            if canonical_digest(summary) != expected["summarySha256"]:
                raise DebtError(f"reviewed debt {side} summary changed: {source}")
        if relation != "delta":
            raise DebtError(f"non-delta mismatch is never reviewed debt: {source}")
        reviewed += 1
    stale = len(registry["entries"]) - len(seen)
    if stale:
        raise DebtError(f"reviewed debt registry has {stale} stale entries")
    if unreviewed:
        raise DebtError(f"differential report has {unreviewed} unreviewed mismatches")
    return {"reviewed": reviewed, "unreviewed": unreviewed}


def self_test() -> None:
    with tempfile.TemporaryDirectory(prefix="libdxfrw-debt-check-") as directory:
        root = Path(directory)
        summary = {
            "sourceFormat": "dwg", "version": "AC1021",
            "diagnostics": {"entityParseFailures": 0},
            "entityCount": 0, "objectCount": 0,
            "entityTypes": [], "objectTypes": [], "status": {},
        }
        source = "a" * 64
        target_output = "b" * 64
        standalone_output = "c" * 64
        entry = {
            "id": "debt-1", "sourceSha256": source,
            "inputVersion": "AC1021", "relation": "delta",
            "reason": "pinned target omits the legacy page-map entities",
            "target": {"outputSha256": target_output,
                        "summarySha256": canonical_digest(summary)},
            "standalone": {"outputSha256": standalone_output,
                            "summarySha256": canonical_digest(summary)},
        }
        registry = {"schema": SCHEMA, "kind": KIND,
                    "targetCommit": "3c7785e", "entries": [entry]}
        report = {"schema": 2, "kind": REPORT_KIND,
                  "targetCommit": "3c7785e", "rows": [{
                      "sourceSha256": source, "inputVersion": "AC1021",
                      "relation": "delta",
                      "target": {"outputSha256": target_output,
                                  "summary": summary},
                      "standalone": {"outputSha256": standalone_output,
                                      "summary": summary},
                  }]}
        registry_path = root / "registry.json"
        report_path = root / "report.json"
        registry_path.write_text(json.dumps(registry), encoding="utf-8")
        report_path.write_text(json.dumps(report), encoding="utf-8")
        assert check(report_path, registry_path) == {"reviewed": 1, "unreviewed": 0}
        report["rows"][0]["target"]["outputSha256"] = "d" * 64
        report_path.write_text(json.dumps(report), encoding="utf-8")
        try:
            check(report_path, registry_path)
        except DebtError as exc:
            assert "output hash changed" in str(exc)
        else:
            raise AssertionError("changed reviewed debt was accepted")
    print("check_differential_debt self-test: PASS")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--report", type=Path)
    parser.add_argument("--registry", type=Path)
    args = parser.parse_args(argv)
    try:
        if args.self_test:
            self_test()
            return 0
        if args.report is None or args.registry is None:
            parser.error("--report and --registry are required unless --self-test is used")
        result = check(args.report.resolve(), args.registry.resolve())
        print("differential debt: PASS (%d reviewed mismatches; %d unreviewed)" %
              (result["reviewed"], result["unreviewed"]))
        return 0
    except (OSError, UnicodeError, DebtError) as exc:
        print(f"differential debt: FAIL: {exc}")
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
