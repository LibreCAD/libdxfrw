#!/usr/bin/env python3
"""Verify one AC1027 MLINESTYLE's independent peer and libdxfrw bit spans."""

from __future__ import annotations

import argparse
import copy
import hashlib
import json
import re
import sys
from pathlib import Path


PEER_SPAN_LINE = re.compile(
    r"P6H1_(STR|BODY|H)_(BEGIN|END)\s+(\S+)\s+"
    r"(?:bit=(\d+)|@(\d+)\.(\d+))"
)
PEER_STATE_LINE = re.compile(r"is_xdic_missing:\s+(\d+)")
PEER_HANDLE_LINE = re.compile(
    r"(ownerhandle|reactors\[\d+\]):\s+\((\d+)\.\d+\.([0-9A-Fa-f]+)\)"
    r"\s+abs:([0-9A-Fa-f]+)"
)
LOCAL_SPAN_LINE = re.compile(
    r"DRW_MLINESTYLE_SPAN handle=(\d+) version=(\S+) stream=(\S+) "
    r"field=(\S+) start=(\d+) end=(\d+)"
    r"(?: rawCode=(\d+) target=(\d+))?"
)
LOCAL_STATE_LINE = re.compile(
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


def extract_peer_object(trace: str, object_number: int) -> str:
    object_header = re.compile(
        rf"^Object number: {object_number}/[0-9A-Fa-f]+,.*$", re.MULTILINE
    )
    matches = list(object_header.finditer(trace))
    if len(matches) != 1:
        raise ValueError(
            f"expected one peer object {object_number}, found {len(matches)}"
        )
    match = matches[0]
    next_object = re.search(r"^< Next object:", trace[match.end() :], re.MULTILINE)
    return (
        trace[match.start() : match.end() + next_object.start()]
        if next_object
        else trace[match.start() :]
    )


def parse_peer(object_trace: str) -> tuple[
    dict[tuple[str, str], dict[str, object]], dict[str, int], dict[str, tuple[int, int]]
]:
    starts: dict[tuple[str, str], int] = {}
    spans: dict[tuple[str, str], dict[str, object]] = {}
    handles: dict[str, tuple[int, int]] = {}
    for match in PEER_SPAN_LINE.finditer(object_trace):
        stream_tag, edge, field, bit_text, byte_text, subbit_text = match.groups()
        stream = {"STR": "string", "BODY": "body", "H": "handle"}[stream_tag]
        if stream == "body" and field == "flag":
            field = "flags"
        if stream == "string":
            field = re.sub(r"^lines\[(\d+)\]\.color\.embedded$", r"element[\1].color.embedded", field)
        position = (
            int(bit_text)
            if bit_text is not None
            else int(byte_text) * 8 + int(subbit_text)
        )
        key = (stream, field)
        if edge == "BEGIN":
            if key in starts:
                raise ValueError(f"duplicate peer span start: {key}")
            starts[key] = position
        else:
            if key not in starts:
                raise ValueError(f"peer span end without start: {key}")
            item: dict[str, object] = {
                "stream": stream,
                "field": field,
                "startBit": starts.pop(key),
                "endBit": position,
            }
            if key in spans:
                raise ValueError(f"duplicate peer span: {key}")
            spans[key] = item
    if starts:
        raise ValueError(f"unterminated peer spans: {sorted(starts)}")

    state_match = PEER_STATE_LINE.search(object_trace)
    if state_match is None:
        raise ValueError("peer trace lacks the xdictionary-missing state")
    states = {"xdictionary_missing": int(state_match.group(1))}
    for match in PEER_HANDLE_LINE.finditer(object_trace):
        field, raw_code, target_hex, absolute_hex = match.groups()
        target = int(target_hex, 16)
        if target != int(absolute_hex, 16):
            raise ValueError(f"peer handle reference disagreement for {field}")
        handles[field] = (int(raw_code), target)
    return spans, states, handles


def parse_local(trace: str, expected_handle: int) -> tuple[
    dict[tuple[str, str], dict[str, object]], dict[str, int], int
]:
    spans: dict[tuple[str, str], dict[str, object]] = {}
    states: dict[str, int] = {}
    duplicates = 0
    for match in LOCAL_SPAN_LINE.finditer(trace):
        handle, version, stream, field, start, end, raw_code, target = match.groups()
        if int(handle) != expected_handle or version != "AC1027":
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
            if stream != "handle" or previous != item:
                raise ValueError(f"conflicting or unexpected repeated span: {key}")
            duplicates += 1
        else:
            spans[key] = item
    for match in LOCAL_STATE_LINE.finditer(trace):
        handle, version, field, value = match.groups()
        if int(handle) == expected_handle and version == "AC1027":
            parsed = int(value)
            if field in states and states[field] != parsed:
                raise ValueError(f"conflicting local state: {field}")
            states[field] = parsed
    return spans, states, duplicates


def compare(
    expected: dict[tuple[str, str], dict[str, object]],
    observed: dict[tuple[str, str], dict[str, object]],
) -> list[str]:
    errors: list[str] = []
    for key in sorted(expected.keys() - observed.keys()):
        errors.append(f"missing {key}")
    for key in sorted(observed.keys() - expected.keys()):
        errors.append(f"unexpected {key}")
    for key in sorted(expected.keys() & observed.keys()):
        for field in ("startBit", "endBit"):
            want = expected[key]
            got = observed[key]
            if field in want and got.get(field) != want[field]:
                errors.append(f"{key} {field}: expected {want[field]}, got {got.get(field)}")
            elif field not in want and field in got:
                errors.append(f"{key} unexpectedly has {field}={got[field]}")
    return errors


def validate_frame(trace: str, frame: dict[str, object]) -> None:
    size_start, size_end = map(int, frame["stringSizeFieldBits"])
    string_start = int(frame["hasStringsBit"])
    handle_start = int(frame["handleStartBit"])
    handle_end = int(frame["handleEndBit"])
    padding = int(frame["handlePaddingBits"])
    payload_end = int(frame["payloadEndBit"])
    if size_end - size_start != 16 or size_end != string_start:
        raise ValueError("receipt string-size and has_strings boundaries disagree")
    if string_start + 1 != handle_start or handle_end + padding != payload_end:
        raise ValueError("receipt string/handle or object-tail boundaries disagree")
    if payload_end != int(frame["sizeBytes"]) * 8:
        raise ValueError("receipt payload boundary does not match object size")

    required = (
        f"Size: {frame['sizeBytes']} [MS]",
        f"Type: {frame['fixedType']} [BOT]",
        f"Address: {frame['address']}",
        f"bitsize: {frame['preHandleBits']},",
        f"data_size: {frame['stringDataBits']}/",
        f"@{size_start // 8}.{size_start % 8} data_size: {frame['stringDataBits']}/",
        f"obj string stream +{string_start}: @{string_start // 8}.{string_start % 8} "
        f"({string_start}) has_strings: 1",
        f"padding: +{frame['handlePaddingBits']}",
        f"crc: {frame['crcHex']} [RSx]",
        f"check_CRC {frame['crcCoverageStart']}-{frame['crcCoverageEnd']} = "
        f"{frame['crcCoverageBytes']}: {frame['crcHex']} == {frame['crcHex']}",
    )
    for item in required:
        if item not in trace:
            raise ValueError(f"peer trace does not contain frame evidence: {item}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--receipt", type=Path, required=True)
    parser.add_argument("--peer-trace", type=Path, required=True)
    parser.add_argument("--peer-json", type=Path, required=True)
    parser.add_argument("--local-trace", type=Path, required=True)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--peer-binary", type=Path, required=True)
    parser.add_argument("--local-binary", type=Path, required=True)
    parser.add_argument("--peer-source-root", type=Path, required=True)
    parser.add_argument("--parser-source", type=Path, required=True)
    args = parser.parse_args()

    try:
        receipt = json.loads(args.receipt.read_text(encoding="utf-8"))
        source = args.source.resolve(strict=True)
        peer_trace = args.peer_trace.read_text(encoding="utf-8", errors="replace")
        local_trace = args.local_trace.read_text(encoding="utf-8", errors="replace")
        source_record = receipt["source"]
        peer_record = receipt["peer"]
        local_record = receipt["libdxfrwObserverRun"]
        source_hash = sha256(source)
        if source_hash != source_record["sha256"]:
            raise ValueError("source hash mismatch")
        with source.open("rb") as stream:
            magic = stream.read(6).decode("ascii", errors="replace")
        if magic != source_record["version"]:
            raise ValueError(f"source version mismatch: {magic}")
        if sha256(args.peer_binary.resolve(strict=True)) != peer_record["binarySha256"]:
            raise ValueError("peer binary hash mismatch")
        if hashlib.sha256(args.peer_trace.read_bytes()).hexdigest() != peer_record[
            "traceSha256"
        ]:
            raise ValueError("peer trace hash mismatch")
        if sha256(args.peer_json.resolve(strict=True)) != peer_record["jsonSha256"]:
            raise ValueError("peer minJSON hash mismatch")
        for relative, expected_hash in peer_record["instrumentationFiles"].items():
            if sha256(args.peer_source_root / relative) != expected_hash:
                raise ValueError(f"peer instrumentation source hash mismatch: {relative}")
        if sha256(args.local_binary.resolve(strict=True)) != local_record["binarySha256"]:
            raise ValueError("libdxfrw observer binary hash mismatch")
        if hashlib.sha256(args.local_trace.read_bytes()).hexdigest() != local_record[
            "traceSha256"
        ]:
            raise ValueError("libdxfrw observer trace hash mismatch")
        if sha256(args.parser_source.resolve(strict=True)) != local_record[
            "parserSourceSha256"
        ]:
            raise ValueError("libdxfrw parser source hash mismatch")

        expected = normalize_expected(receipt)
        target_peer_trace = extract_peer_object(
            peer_trace, int(receipt["object"]["objectNumber"])
        )
        peer_spans, peer_states, peer_handles = parse_peer(target_peer_trace)
        local_spans, local_states, duplicate_count = parse_local(
            local_trace, int(receipt["object"]["handleDecimal"])
        )
        for label, observed in (("peer", peer_spans), ("libdxfrw", local_spans)):
            errors = compare(expected, observed)
            if errors:
                raise ValueError(f"{label} span mismatch: {'; '.join(errors)}")
        peer_xdic = int(receipt["object"]["xDictionaryMissing"])
        if peer_states.get("xdictionary_missing") != peer_xdic:
            raise ValueError("peer xdictionary-missing state mismatch")
        if local_states.get("xdictionary_missing") != peer_xdic:
            raise ValueError("libdxfrw xdictionary-missing state mismatch")
        if set(peer_states) != {"xdictionary_missing"}:
            raise ValueError("unexpected peer state")
        if set(local_states) != {"xdictionary_missing"}:
            raise ValueError("unexpected libdxfrw state")
        for field, expected_handle in receipt["object"]["handleRoles"].items():
            if peer_handles.get(field) != (
                int(expected_handle["rawCode"]), int(expected_handle["target"])
            ):
                raise ValueError(f"peer handle role mismatch: {field}")
            local_handle = local_spans.get(("handle", field))
            if local_handle is None or (
                local_handle.get("rawCode"), local_handle.get("target")
            ) != (int(expected_handle["rawCode"]), int(expected_handle["target"])):
                raise ValueError(f"libdxfrw handle role mismatch: {field}")
        if set(peer_handles) != set(receipt["object"]["handleRoles"]):
            raise ValueError("peer trace has missing or unexpected handle roles")

        validate_frame(target_peer_trace, receipt["frame"])
        shifted = copy.deepcopy(local_spans)
        first_key = sorted(shifted)[0]
        shifted[first_key]["startBit"] = int(shifted[first_key]["startBit"]) + 1
        if not compare(expected, shifted):
            raise ValueError("shifted-span negative control was not rejected")
        if source_hash != sha256(source):
            raise ValueError("source changed during verification")
    except (KeyError, OSError, TypeError, ValueError, json.JSONDecodeError) as error:
        print(f"FAIL: {error}", file=sys.stderr)
        return 1

    print(
        "PASS: hash-bound AC1027 source, recorded frame bounds, "
        f"{len(expected)} exact peer/libdxfrw spans, handle roles, xdictionary state, "
        f"and shifted-span rejection ({duplicate_count} identical handle repeats)."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
