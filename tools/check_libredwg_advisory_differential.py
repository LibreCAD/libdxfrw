#!/usr/bin/env python3
"""Validate the hash-only independent LibreDWG advisory differential."""

from __future__ import annotations

import copy
import argparse
import json
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_REPORT = ROOT / "metadata/libredwg-advisory-differential-v1.json"
GIT_HEX = 40
SHA256_HEX = 64


class ReportError(RuntimeError):
    pass


def load(path: Path) -> dict[str, object]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as exc:
        raise ReportError(f"could not read {path}: {exc}") from exc
    if not isinstance(value, dict):
        raise ReportError("report must be a JSON object")
    return value


def _hex(value: object, length: int, label: str) -> None:
    if not isinstance(value, str) or len(value) != length:
        raise ReportError(f"{label} must be a {length}-character hexadecimal value")
    try:
        int(value, 16)
    except ValueError as exc:
        raise ReportError(f"{label} is not hexadecimal") from exc


def _size(value: object, label: str) -> None:
    if not isinstance(value, int) or isinstance(value, bool) or value < 0:
        raise ReportError(f"{label} must be a non-negative integer")


def validate(report: dict[str, object]) -> None:
    if report.get("schema") != 1 or report.get("advisory") is not True:
        raise ReportError("report must be schema 1 and explicitly advisory")
    if report.get("fixturePolicy") != "external-only; never committed":
        raise ReportError("fixture policy changed")
    if report.get("oracle") != "LibreDWG dwg2dxf 0.14":
        raise ReportError("independent oracle identity changed")

    source = report.get("sourceCorpus")
    if not isinstance(source, dict):
        raise ReportError("sourceCorpus provenance is missing")
    if source.get("repository") != "LibreCAD/LibreCAD":
        raise ReportError("source repository changed")
    if source.get("path") != "librecad/src/lib/filters/tests/testdata":
        raise ReportError("source path changed")
    if source.get("selection") != "top-level *.dwg only":
        raise ReportError("source selection changed")
    if source.get("note") != (
        "temporary external checkout; source blobs are not copied into this repository"
    ):
        raise ReportError("source retention note changed")
    _hex(source.get("commit"), GIT_HEX, "sourceCorpus.commit")

    runner = report.get("runner")
    if not isinstance(runner, dict) or runner.get("name") != "dwg2dxf":
        raise ReportError("runner identity is missing")
    if runner.get("version") != "0.14" or runner.get("timeoutSeconds") != 10:
        raise ReportError("runner version or timeout changed")

    rows = report.get("rows")
    if not isinstance(rows, list) or len(rows) != 21:
        raise ReportError("report must contain exactly 21 rows")
    if report.get("inputCount") != len(rows):
        raise ReportError("inputCount does not match rows")
    seen: set[str] = set()
    converted = 0
    for index, row in enumerate(rows):
        label = f"rows[{index}]"
        if not isinstance(row, dict):
            raise ReportError(f"{label} must be an object")
        path = row.get("path")
        if not isinstance(path, str) or not path.endswith(".dwg"):
            raise ReportError(f"{label}.path must name a DWG")
        if "/" in path or path in seen:
            raise ReportError(f"{label}.path must be a unique top-level name")
        seen.add(path)
        _hex(row.get("sourceSha256"), SHA256_HEX, f"{label}.sourceSha256")
        _size(row.get("sourceSize"), f"{label}.sourceSize")
        if row.get("status") != "converted" or row.get("exitStatus") != 0:
            raise ReportError(f"{label} is not a successful independent conversion")
        if row.get("diagnosticCode") != "success":
            raise ReportError(f"{label}.diagnosticCode is not success")
        _hex(row.get("outputSha256"), SHA256_HEX, f"{label}.outputSha256")
        _size(row.get("outputSize"), f"{label}.outputSize")
        if row["outputSize"] == 0:
            raise ReportError(f"{label}.outputSize must be nonzero")
        converted += 1

    if report.get("statusCounts") != {"converted": converted}:
        raise ReportError("statusCounts are not row-derived")


def self_test() -> None:
    report = load(DEFAULT_REPORT)
    validate(report)
    with tempfile.TemporaryDirectory(prefix="libdxfrw-libredwg-advisory-"):
        invalid = copy.deepcopy(report)
        invalid["statusCounts"] = {"converted": 20, "failed": 1}
        try:
            validate(invalid)
        except ReportError:
            pass
        else:
            raise AssertionError("tampered status counters were accepted")
        invalid = copy.deepcopy(report)
        invalid["rows"][0]["status"] = "failed"
        try:
            validate(invalid)
        except ReportError:
            pass
        else:
            raise AssertionError("failed oracle row was accepted")
    print("check_libredwg_advisory_differential self-test: PASS")


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
            print("LibreDWG advisory differential: PASS (21/21 converted; hashes only)")
        return 0
    except ReportError as exc:
        print(f"LibreDWG advisory differential: FAIL: {exc}")
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
