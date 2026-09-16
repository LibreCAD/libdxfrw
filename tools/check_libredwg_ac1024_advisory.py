#!/usr/bin/env python3
"""Validate the hash-only LibreDWG AC1024 advisory report.

The report records external input/output hashes and bounded status metadata.
It is deliberately non-promoting: the external corpus never becomes a
fixture or a format-support claim.
"""

from __future__ import annotations

import argparse
import copy
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_REPORT = ROOT / "metadata/libredwg-ac1024-advisory-v1.json"
HEX64 = 64


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


def _hex(value: object, label: str) -> None:
    if not isinstance(value, str) or len(value) != HEX64:
        raise ReportError(f"{label} must be a {HEX64}-character hexadecimal value")
    try:
        int(value, 16)
    except ValueError as exc:
        raise ReportError(f"{label} is not hexadecimal") from exc


def _size(value: object, label: str) -> None:
    if not isinstance(value, int) or isinstance(value, bool) or value <= 0:
        raise ReportError(f"{label} must be a positive integer")


def validate(report: dict[str, object]) -> None:
    if report.get("schema") != 1 or report.get("advisory") is not True:
        raise ReportError("report must be schema 1 and explicitly advisory")
    if report.get("fixturePolicy") != "external-only; never committed":
        raise ReportError("fixture policy changed")
    if report.get("oracle") != "LibreDWG dwgread 0.14":
        raise ReportError("oracle identity changed")

    runner = report.get("runner")
    if not isinstance(runner, dict):
        raise ReportError("runner provenance is missing")
    if runner != {
        "format": "minJSON",
        "name": "dwgread",
        "timeoutSeconds": 2,
        "version": "0.14",
    }:
        raise ReportError("runner provenance changed")

    source = report.get("sourceCorpus")
    if not isinstance(source, dict) or source != {
        "note": "source files remain outside repository; report retains hashes/statuses only",
        "selection": "available external AC1024 DWG corpus",
    }:
        raise ReportError("source corpus provenance changed")

    rows = report.get("rows")
    if not isinstance(rows, list) or len(rows) != 9:
        raise ReportError("report must contain exactly nine rows")
    if report.get("inputCount") != len(rows):
        raise ReportError("inputCount does not match rows")

    seen: set[str] = set()
    for index, row in enumerate(rows):
        label = f"rows[{index}]"
        if not isinstance(row, dict):
            raise ReportError(f"{label} must be an object")
        source_hash = row.get("sourceSha256")
        if source_hash in seen:
            raise ReportError(f"{label}.sourceSha256 is duplicated")
        if not isinstance(source_hash, str):
            raise ReportError(f"{label}.sourceSha256 is missing")
        seen.add(source_hash)
        _hex(source_hash, f"{label}.sourceSha256")
        _hex(row.get("outputSha256"), f"{label}.outputSha256")
        _size(row.get("sourceSize"), f"{label}.sourceSize")
        _size(row.get("outputSize"), f"{label}.outputSize")
        if row.get("inputVersion") != "AC1024":
            raise ReportError(f"{label}.inputVersion is not AC1024")
        if row.get("status") != "converted" or row.get("exitStatus") != 0:
            raise ReportError(f"{label} is not a successful conversion")

    if report.get("statusCounts") != {"converted": len(rows)}:
        raise ReportError("statusCounts are not row-derived")


def self_test() -> None:
    report = load(DEFAULT_REPORT)
    validate(report)

    invalid = copy.deepcopy(report)
    invalid["statusCounts"] = {"converted": 8, "failed": 1}
    try:
        validate(invalid)
    except ReportError:
        pass
    else:
        raise AssertionError("tampered counters were accepted")

    invalid = copy.deepcopy(report)
    invalid["rows"][0]["inputVersion"] = "AC1021"
    try:
        validate(invalid)
    except ReportError:
        pass
    else:
        raise AssertionError("tampered version was accepted")
    print("check_libredwg_ac1024_advisory self-test: PASS")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--report", type=Path, default=DEFAULT_REPORT)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args(argv)
    try:
        if args.self_test:
            self_test()
        else:
            validate(load(args.report))
            print("LibreDWG AC1024 advisory: PASS (9/9 converted; hashes only)")
        return 0
    except (ReportError, AssertionError) as exc:
        print(f"LibreDWG AC1024 advisory: FAIL: {exc}")
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
