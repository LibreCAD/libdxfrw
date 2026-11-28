#!/usr/bin/env python3
"""Compare one libdxfrw MLINESTYLE observer trace with an independent receipt."""

from __future__ import annotations

import argparse
import copy
import hashlib
import json
import re
import sys
from pathlib import Path


SPAN_LINE = re.compile(
    r"DRW_MLINESTYLE_SPAN handle=(\d+) version=(\S+) stream=(\S+) "
    r"field=(\S+) start=(\d+) end=(\d+)"
    r"(?: rawCode=(\d+) target=(\d+))?"
)
STATE_LINE = re.compile(
    r"DRW_MLINESTYLE_STATE handle=(\d+) version=(\S+) "
    r"field=(\S+) value=(\d+)"
)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def span_key(span: dict[str, object]) -> tuple[str, str]:
    return str(span["stream"]), str(span["field"])


def normalize_expected(receipt: dict[str, object]) -> dict[tuple[str, str], dict[str, object]]:
    raw_spans = receipt.get("spans")
    if not isinstance(raw_spans, list):
        raise ValueError("receipt must contain a spans array")
    expected: dict[tuple[str, str], dict[str, object]] = {}
    for raw in raw_spans:
        if not isinstance(raw, dict):
            raise ValueError("receipt span must be an object")
        item = dict(raw)
        key = span_key(item)
        if key in expected:
            raise ValueError(f"duplicate span in receipt: {key}")
        if int(item["endBit"]) < int(item["startBit"]):
            raise ValueError(f"negative span in receipt: {key}")
        expected[key] = item
    return expected


def parse_observed(trace: str, expected_handle: int) -> tuple[
    dict[tuple[str, str], dict[str, object]], dict[str, int], int
]:
    spans: dict[tuple[str, str], dict[str, object]] = {}
    states: dict[str, int] = {}
    duplicate_count = 0
    for match in SPAN_LINE.finditer(trace):
        handle, version, stream, field, start, end, raw_code, target = match.groups()
        if int(handle) != expected_handle or version != "AC1021":
            continue
        item: dict[str, object] = {
            "stream": stream,
            "field": field,
            "startBit": int(start),
            "endBit": int(end),
        }
        if raw_code is not None:
            item["rawCode"] = int(raw_code)
            item["target"] = int(target)
        key = span_key(item)
        previous = spans.get(key)
        if previous is not None:
            if stream != "handle":
                raise ValueError(f"unexpected repeated parser field span: {key}")
            if previous != item:
                raise ValueError(f"conflicting repeated observer span: {key}")
            duplicate_count += 1
        else:
            spans[key] = item
    for match in STATE_LINE.finditer(trace):
        handle, version, field, value = match.groups()
        if int(handle) == expected_handle and version == "AC1021":
            parsed_value = int(value)
            if field in states and states[field] != parsed_value:
                raise ValueError(f"conflicting repeated observer state: {field}")
            states[field] = parsed_value
    return spans, states, duplicate_count


def compare(
    expected: dict[tuple[str, str], dict[str, object]],
    observed: dict[tuple[str, str], dict[str, object]],
) -> list[str]:
    errors: list[str] = []
    for key in sorted(expected.keys() - observed.keys()):
        errors.append(f"missing observer span {key}")
    for key in sorted(observed.keys() - expected.keys()):
        errors.append(f"unexpected observer span {key}")
    for key in sorted(expected.keys() & observed.keys()):
        want = expected[key]
        got = observed[key]
        for field in ("startBit", "endBit", "rawCode", "target"):
            if field in want and got.get(field) != want[field]:
                errors.append(
                    f"{key} {field}: expected {want[field]!r}, observed {got.get(field)!r}"
                )
            elif field not in want and field in got:
                errors.append(f"{key} unexpectedly reports {field}={got[field]!r}")
    return errors


def verify_shifted_span_control(
    expected: dict[tuple[str, str], dict[str, object]],
    observed: dict[tuple[str, str], dict[str, object]],
) -> None:
    shifted = copy.deepcopy(observed)
    key = sorted(shifted)[0]
    shifted[key]["startBit"] = int(shifted[key]["startBit"]) + 1
    if not compare(expected, shifted):
        raise ValueError("shifted-span negative control was not rejected")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--receipt", type=Path, required=True)
    parser.add_argument("--trace", type=Path, required=True)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--parser-source", type=Path, required=True)
    args = parser.parse_args()

    try:
        receipt = json.loads(args.receipt.read_text(encoding="utf-8"))
        source = args.source.resolve(strict=True)
        trace = args.trace.read_text(encoding="utf-8", errors="replace")
        source_record = receipt["source"]
        observer_run = receipt["libdxfrwObserverRun"]
        source_hash_before = sha256(source)
        if source_hash_before != source_record["sha256"]:
            raise ValueError("source hash does not match the peer-span receipt")
        if sha256(args.parser_source.resolve(strict=True)) != observer_run[
            "parserSourceSha256"
        ]:
            raise ValueError("libdxfrw parser source hash does not match the receipt")
        with source.open("rb") as source_stream:
            source_version = source_stream.read(6).decode("ascii", errors="replace")
        if source_version != source_record["version"]:
            raise ValueError(
                f"source version mismatch: expected {source_record['version']}, "
                f"observed {source_version}"
            )
        binary_hash = sha256(args.binary.resolve(strict=True))
        if binary_hash != observer_run["binarySha256"]:
            raise ValueError("observer binary hash does not match the receipt")
        trace_hash = hashlib.sha256(args.trace.read_bytes()).hexdigest()
        if trace_hash != observer_run["traceSha256"]:
            raise ValueError("observer trace hash does not match the receipt")
        expected = normalize_expected(receipt)
        observed, states, duplicate_count = parse_observed(
            trace, int(receipt["object"]["handleDecimal"])
        )
        errors = compare(expected, observed)
        if errors:
            raise ValueError("; ".join(errors))
        expected_state = 1 if receipt["object"]["xDictionaryMissing"] else 0
        if states.get("xdictionary_missing") != expected_state:
            raise ValueError(
                "xdictionary-missing state mismatch: "
                f"expected {expected_state}, observed {states.get('xdictionary_missing')}"
            )
        if set(states) != {"xdictionary_missing"}:
            raise ValueError(f"unexpected MLINESTYLE observer states: {sorted(states)}")
        if source_hash_before != sha256(source):
            raise ValueError("source changed during verification")
        verify_shifted_span_control(expected, observed)
    except (KeyError, OSError, ValueError, json.JSONDecodeError) as error:
        print(f"FAIL: {error}", file=sys.stderr)
        return 1

    print(
        "PASS: source hash, "
        f"{len(observed)} unique MLINESTYLE spans, xdictionary state, "
        f"and shifted-span rejection verified ({duplicate_count} identical repeated spans)."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
