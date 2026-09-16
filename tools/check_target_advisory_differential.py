#!/usr/bin/env python3
"""Validate the hash-only target/package DWG advisory differential.

The report is deliberately non-promoting: it records source, output, and
semantic hashes for an external LibreCAD testdata corpus, never drawing
payloads.  Keeping a strict checker beside the report makes the evidence
reproducible and fail closed if a row, counter, or provenance field drifts.
"""

from __future__ import annotations

import argparse
import copy
import hashlib
import json
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_REPORT = ROOT / "metadata/target-advisory-differential-v1.json"
EXPECTED_KIND = "libdxfrw-target-advisory-differential"
EXPECTED_POLICY = "lockedRepositoryBlob; hashes-and-summaries-only; never committed drawing bytes"
SHA256_HEX = 64
GIT_HEX = 40


class ReportError(RuntimeError):
    """Raised when the advisory report is malformed or inconsistent."""


def load(path: Path) -> dict[str, object]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as exc:
        raise ReportError(f"could not read {path}: {exc}") from exc
    if not isinstance(value, dict):
        raise ReportError("advisory report must be a JSON object")
    return value


def _sha(value: object, label: str) -> None:
    if not isinstance(value, str) or len(value) != SHA256_HEX:
        raise ReportError(f"{label} must be a SHA-256 hexadecimal string")
    try:
        int(value, 16)
    except ValueError as exc:
        raise ReportError(f"{label} is not hexadecimal") from exc


def _git_commit(value: object, label: str) -> None:
    if not isinstance(value, str) or len(value) != GIT_HEX:
        raise ReportError(f"{label} must be a full Git commit id")
    try:
        int(value, 16)
    except ValueError as exc:
        raise ReportError(f"{label} is not hexadecimal") from exc


def _size(value: object, label: str) -> None:
    if not isinstance(value, int) or isinstance(value, bool) or value < 0:
        raise ReportError(f"{label} must be a non-negative integer")


def _counter(value: object, label: str) -> dict[str, int]:
    if not isinstance(value, dict):
        raise ReportError(f"{label} must be an object")
    result: dict[str, int] = {}
    for key, count in value.items():
        if not isinstance(key, str):
            raise ReportError(f"{label} has a non-string key")
        if not isinstance(count, int) or isinstance(count, bool) or count < 0:
            raise ReportError(f"{label}.{key} must be a non-negative integer")
        result[key] = count
    return result


def _output(value: object, label: str) -> None:
    if not isinstance(value, dict):
        raise ReportError(f"{label} must be an object")
    _sha(value.get("outputSha256"), f"{label}.outputSha256")
    _sha(value.get("summarySha256"), f"{label}.summarySha256")
    _size(value.get("outputSize"), f"{label}.outputSize")


def validate(report: dict[str, object]) -> None:
    if report.get("schema") != 1:
        raise ReportError("advisory report schema must be 1")
    if report.get("kind") != EXPECTED_KIND:
        raise ReportError("unexpected advisory report kind")
    if report.get("advisory") is not True:
        raise ReportError("advisory report must remain non-promoting")
    if report.get("fixturePolicy") != EXPECTED_POLICY:
        raise ReportError("drawing payload policy changed")

    source = report.get("sourceCorpus")
    if not isinstance(source, dict):
        raise ReportError("sourceCorpus provenance is missing")
    if source.get("repository") != "LibreCAD/LibreCAD":
        raise ReportError("source corpus repository drifted")
    if source.get("path") != "librecad/src/lib/filters/tests/testdata":
        raise ReportError("source corpus path drifted")
    _git_commit(source.get("commit"), "sourceCorpus.commit")
    if source.get("note") != (
        "temporary external checkout; source blobs are not copied into this repository"
    ):
        raise ReportError("source corpus retention note drifted")

    target = report.get("target")
    if not isinstance(target, dict):
        raise ReportError("target provenance is missing")
    if target.get("repository") != "LibreCAD/LibreCAD":
        raise ReportError("target repository drifted")
    _git_commit(target.get("commit"), "target.commit")

    runner = report.get("runner")
    if not isinstance(runner, dict):
        raise ReportError("runner provenance is missing")
    if runner.get("target") != "libdxfrw_json_dump" or runner.get(
        "standalone"
    ) != "libdxfrw_json_dump":
        raise ReportError("target/standalone runner names drifted")
    if runner.get("timeoutSeconds") != 10:
        raise ReportError("per-input timeout bound drifted")

    rows = report.get("rows")
    if not isinstance(rows, list) or not rows:
        raise ReportError("advisory report rows must be a non-empty array")
    if report.get("inputCount") != len(rows):
        raise ReportError("inputCount does not match row count")
    paths: set[str] = set()
    relation_counts: dict[str, int] = {}
    byte_counts: dict[str, int] = {}
    semantic_counts: dict[str, int] = {}
    for index, row in enumerate(rows):
        label = f"rows[{index}]"
        if not isinstance(row, dict):
            raise ReportError(f"{label} must be an object")
        path = row.get("path")
        if not isinstance(path, str) or not path.endswith(".dwg"):
            raise ReportError(f"{label}.path must name a DWG")
        if path in paths:
            raise ReportError(f"duplicate advisory path: {path}")
        paths.add(path)
        version = row.get("inputVersion")
        if not isinstance(version, str) or not version:
            raise ReportError(f"{label}.inputVersion is missing")
        _sha(row.get("sourceSha256"), f"{label}.sourceSha256")
        _size(row.get("sourceSize"), f"{label}.sourceSize")
        for key, counts in (("relation", relation_counts),
                            ("byteRelation", byte_counts),
                            ("semanticRelation", semantic_counts)):
            relation = row.get(key)
            if relation not in {"equal", "delta"}:
                raise ReportError(f"{label}.{key} has an invalid value")
            counts[relation] = counts.get(relation, 0) + 1
        _output(row.get("target"), f"{label}.target")
        _output(row.get("standalone"), f"{label}.standalone")
        if row["relation"] == "equal" and row["target"] != row["standalone"]:
            raise ReportError(f"{label} equal relation has different outputs")
        if row["byteRelation"] == "equal":
            if row["target"]["outputSha256"] != row["standalone"]["outputSha256"]:
                raise ReportError(f"{label} equal byte relation has different hashes")
        if row["semanticRelation"] == "equal":
            if row["target"]["summarySha256"] != row["standalone"]["summarySha256"]:
                raise ReportError(f"{label} equal semantic relation has different hashes")

    if _counter(report.get("relationCounts"), "relationCounts") != relation_counts:
        raise ReportError("relationCounts are not row-derived")
    if _counter(report.get("byteRelationCounts"), "byteRelationCounts") != byte_counts:
        raise ReportError("byteRelationCounts are not row-derived")
    if _counter(report.get("semanticRelationCounts"), "semanticRelationCounts") != semantic_counts:
        raise ReportError("semanticRelationCounts are not row-derived")
    if len(rows) != 21 or relation_counts != {"equal": 21}:
        raise ReportError("the locked expanded corpus must remain 21 equal relations")
    if byte_counts != {"equal": 21} or semantic_counts != {"equal": 21}:
        raise ReportError("the locked expanded corpus must remain byte/semantic equal")


def self_test() -> None:
    report = load(DEFAULT_REPORT)
    validate(report)
    with tempfile.TemporaryDirectory(prefix="libdxfrw-target-advisory-") as directory:
        invalid = copy.deepcopy(report)
        invalid["relationCounts"] = {"equal": 20, "delta": 1}
        try:
            validate(invalid)
        except ReportError:
            pass
        else:
            raise AssertionError("tampered row counters were accepted")
        invalid = copy.deepcopy(report)
        invalid["rows"][0]["target"]["outputSha256"] = hashlib.sha256(
            b"tampered"
        ).hexdigest()
        try:
            validate(invalid)
        except ReportError:
            pass
        else:
            raise AssertionError("tampered equal output was accepted")
    print("check_target_advisory_differential self-test: PASS")


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--report", type=Path, default=DEFAULT_REPORT)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args(argv)
    try:
        if args.self_test:
            self_test()
        else:
            validate(load(args.report))
            print("target advisory differential: PASS (21 equal; hashes/summaries only)")
        return 0
    except ReportError as exc:
        print(f"target advisory differential: FAIL: {exc}")
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
