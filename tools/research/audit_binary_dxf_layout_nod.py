#!/usr/bin/env python3
"""Inspect Binary DXF NOD and LAYOUT references for bounded readback evidence.

This is an audit helper, not a general DXF reader. It parses the complete
R13+ Binary DXF group/value stream using Autodesk's documented group-code type
ranges, then reports the root DICTIONARY entries, LAYOUT group 331 handles,
and the record classes owning those target handles. It does not validate all
DXF semantics or promote a format-support claim.

References:
* https://help.autodesk.com/cloudhelp/2023/ENU/AutoCAD-DXF/files/GUID-FC1C3C69-DBC2-49E4-893A-000D6538C0FE.htm
* https://help.autodesk.com/cloudhelp/2026/ENU/AutoCAD-DXF/files/GUID-2553CF98-44F6-4828-82DD-FE3BC7448113.htm
"""

from __future__ import annotations

import argparse
import hashlib
import json
import struct
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Iterable


SENTINEL = b"AutoCAD Binary DXF\r\n\x1a\x00"


@dataclass(frozen=True)
class Record:
    section: str | None
    values: tuple[tuple[int, Any], ...]

    @property
    def type(self) -> str:
        if not self.values or self.values[0][0] != 0:
            return ""
        return str(self.values[0][1])

    @property
    def handle(self) -> str | None:
        value = next((v for c, v in self.values if c == 5), None)
        normalized = normalize_handle(value)
        return None if normalized == "0" else normalized


def normalize_handle(value: Any) -> str | None:
    if value is None:
        return None
    return str(value).strip().upper().lstrip("0") or "0"


def _read_exact(data: bytes, offset: int, size: int, code: int) -> tuple[bytes, int]:
    end = offset + size
    if end > len(data):
        raise ValueError(f"truncated group {code} value at byte {offset}")
    return data[offset:end], end


def _read_cstring(data: bytes, offset: int, code: int) -> tuple[str, int]:
    end = data.find(b"\0", offset)
    if end < 0:
        raise ValueError(f"unterminated string for group {code} at byte {offset}")
    raw = data[offset:end]
    return raw.decode("utf-8"), end + 1


def read_group_value(data: bytes, offset: int, code: int) -> tuple[Any, int]:
    """Decode one R13+ Binary DXF value using Autodesk's code-range table."""
    if (
        0 <= code <= 9
        or code in (100, 101, 102, 105, 999)
        or 300 <= code <= 309
        or 320 <= code <= 369
        or 390 <= code <= 399
        or 410 <= code <= 419
        or 430 <= code <= 439
        or 470 <= code <= 481
        or 1000 <= code <= 1003
        or code == 1005
    ):
        return _read_cstring(data, offset, code)

    if 310 <= code <= 319 or code == 1004:
        raw_length, offset = _read_exact(data, offset, 1, code)
        length = raw_length[0]
        raw, offset = _read_exact(data, offset, length, code)
        return raw.hex().upper(), offset

    if (
        10 <= code <= 59
        or 110 <= code <= 149
        or 210 <= code <= 239
        or 460 <= code <= 469
        or 1010 <= code <= 1059
    ):
        raw, offset = _read_exact(data, offset, 8, code)
        return struct.unpack("<d", raw)[0], offset

    if (
        60 <= code <= 79
        or 170 <= code <= 179
        or 270 <= code <= 289
        or 370 <= code <= 409
        or 1060 <= code <= 1070
    ):
        raw, offset = _read_exact(data, offset, 2, code)
        return struct.unpack("<h", raw)[0], offset

    if 90 <= code <= 99 or 420 <= code <= 429 or 440 <= code <= 459 or code == 1071:
        raw, offset = _read_exact(data, offset, 4, code)
        return struct.unpack("<i", raw)[0], offset

    if 160 <= code <= 169:
        raw, offset = _read_exact(data, offset, 8, code)
        return struct.unpack("<q", raw)[0], offset

    if 290 <= code <= 299:
        raw, offset = _read_exact(data, offset, 1, code)
        if raw[0] not in (0, 1):
            raise ValueError(f"invalid boolean value {raw[0]} for group {code}")
        return bool(raw[0]), offset

    raise ValueError(f"unsupported group code {code} at byte {offset}")


def read_pairs(data: bytes) -> tuple[tuple[int, Any], ...]:
    if not data.startswith(SENTINEL):
        raise ValueError("missing AutoCAD Binary DXF sentinel")

    pairs: list[tuple[int, Any]] = []
    offset = len(SENTINEL)
    while offset < len(data):
        raw_code, offset = _read_exact(data, offset, 2, -1)
        code = struct.unpack("<H", raw_code)[0]
        value, offset = read_group_value(data, offset, code)
        pairs.append((code, value))
    return tuple(pairs)


def read_records(pairs: Iterable[tuple[int, Any]]) -> tuple[Record, ...]:
    section: str | None = None
    current_type: str | None = None
    current: list[tuple[int, Any]] = []
    result: list[Record] = []

    for code, value in pairs:
        if code == 0:
            if current_type not in (None, "SECTION", "ENDSEC"):
                result.append(Record(section, tuple(current)))
            if current_type == "ENDSEC":
                section = None

            current_type = str(value)
            current = [(code, value)] if current_type != "SECTION" else []
            continue

        if current_type == "SECTION" and code == 2:
            section = str(value)
            continue

        if current_type is not None:
            current.append((code, value))

    if current_type not in (None, "SECTION", "ENDSEC"):
        result.append(Record(section, tuple(current)))
    return tuple(result)


def dictionary_entries(record: Record) -> list[dict[str, Any]]:
    entries: list[dict[str, Any]] = []
    values = record.values
    for i, (code, name) in enumerate(values[:-1]):
        if code == 3 and values[i + 1][0] in (350, 360):
            entries.append({
                "name": str(name),
                "handle": normalize_handle(values[i + 1][1]),
                "entryCode": values[i + 1][0],
            })
    return entries


def _first(record: Record, code: int, *, after: int = 0) -> Any:
    return next((v for c, v in record.values[after:] if c == code), None)


def layout_report(record: Record) -> dict[str, Any]:
    layout_marker = next(
        (i for i, pair in enumerate(record.values) if pair == (100, "AcDbLayout")),
        None,
    )
    if layout_marker is None:
        raise ValueError(f"LAYOUT record {record.handle} lacks AcDbLayout marker")

    body = record.values[layout_marker + 1:]
    return {
        "handle": record.handle,
        "name": next((str(v) for c, v in body if c == 1), None),
        "ownerDictionary": next(
            (normalize_handle(v) for c, v in record.values if c == 330), None
        ),
        "blockRecord": next((normalize_handle(v) for c, v in body if c == 330), None),
        "lastActiveViewport": next(
            (normalize_handle(v) for c, v in body if c == 331), None
        ),
    }


def audit(path: Path) -> dict[str, Any]:
    data = path.read_bytes()
    pairs = read_pairs(data)
    records = read_records(pairs)
    if not records or records[-1].type != "EOF" or records[-1].section is not None:
        raise ValueError("Binary DXF stream does not end with an EOF record")

    record_by_handle: dict[str, Record] = {}
    duplicate_handles: list[str] = []
    for record in records:
        if not record.handle:
            continue
        if record.handle in record_by_handle:
            duplicate_handles.append(record.handle)
        else:
            record_by_handle[record.handle] = record

    dictionaries = [
        record for record in records
        if record.section == "OBJECTS"
        and record.type in ("DICTIONARY", "DICTIONARYWDFLT")
    ]
    root = next(
        (
            record for record in dictionaries
            if any(entry["name"] == "ACAD_LAYOUT" for entry in dictionary_entries(record))
            and normalize_handle(_first(record, 330)) == "0"
        ),
        None,
    )
    if root is None:
        raise ValueError("root NOD DICTIONARY with ACAD_LAYOUT entry was not found")

    entries = dictionary_entries(root)
    if len({entry["name"].casefold() for entry in entries}) != len(entries):
        raise ValueError("duplicate root dictionary names")

    layouts = [
        layout_report(record) for record in records
        if record.section == "OBJECTS" and record.type == "LAYOUT"
    ]
    targets: dict[str, Any] = {}
    for layout in layouts:
        target_handle = layout["lastActiveViewport"]
        if target_handle is None:
            targets[layout["name"] or "<unnamed>"] = None
            continue
        target = record_by_handle.get(target_handle)
        targets[layout["name"] or "<unnamed>"] = None if target is None else {
            "handle": target.handle,
            "section": target.section,
            "type": target.type,
            "name": _first(target, 2),
            "owner": normalize_handle(_first(target, 330)),
        }

    root_targets: dict[str, Any] = {}
    for entry in entries:
        target = record_by_handle.get(entry["handle"] or "")
        root_targets[entry["name"]] = None if target is None else {
            "handle": target.handle,
            "section": target.section,
            "type": target.type,
        }

    layout_dictionary_handle = next(
        entry["handle"] for entry in entries if entry["name"] == "ACAD_LAYOUT"
    )
    layout_dictionary = record_by_handle.get(layout_dictionary_handle or "")

    issues: list[str] = []
    if duplicate_handles:
        issues.append("duplicate non-null handles")
    if any(value is None for value in root_targets.values()):
        issues.append("one or more NOD entries reference missing records")
    if layout_dictionary is None or layout_dictionary.type not in ("DICTIONARY", "DICTIONARYWDFLT"):
        issues.append("ACAD_LAYOUT entry does not resolve to a dictionary record")
    if any(layout["lastActiveViewport"] and targets[layout["name"] or "<unnamed>"] is None
           for layout in layouts):
        issues.append("one or more group 331 handles do not resolve")

    return {
        "schema": "libdxfrw-binary-dxf-layout-nod-audit-v1",
        "file": str(path),
        "bytes": len(data),
        "sha256": hashlib.sha256(data).hexdigest(),
        "decodedPairs": len(pairs),
        "parsedRecords": len(records),
        "duplicateHandles": sorted(set(duplicate_handles)),
        "rootDictionary": {
            "type": root.type,
            "handle": root.handle,
            "entryCount": len(entries),
            "entries": entries,
            "resolvedEntryTargetCount": sum(value is not None for value in root_targets.values()),
            "unresolvedEntries": sorted(name for name, value in root_targets.items() if value is None),
            "entryTargets": root_targets,
        },
        "acadLayoutDictionary": None if layout_dictionary is None else {
            "handle": layout_dictionary.handle,
            "type": layout_dictionary.type,
            "section": layout_dictionary.section,
        },
        "layouts": layouts,
        "group331Targets": targets,
        "structuralStatus": "PASS" if not issues else "INCOMPLETE",
        "issues": issues,
        "scope": "Structural Binary DXF readback only; this tool does not establish native application acceptance or general DXF support.",
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("binary_dxf", type=Path)
    args = parser.parse_args()
    try:
        report = audit(args.binary_dxf)
    except (OSError, ValueError, struct.error) as error:
        parser.error(str(error))
    print(json.dumps(report, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
