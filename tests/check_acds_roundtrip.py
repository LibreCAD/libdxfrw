#!/usr/bin/env python3
"""Check the bounded AC1027 3DSOLID/ACDSDATA ODA round-trip contract.

This checker intentionally validates only the witnessed standalone shape used
by the external ODA acceptance gate.  It does not attempt to validate general
ACIS geometry or arbitrary ACDSDATA schemas.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
from dataclasses import dataclass
from pathlib import Path


EXPECTED_VERSION = "AC1027"
EXPECTED_SCHEMA_NAMES = (
    "AcDb_Thumbnail_Schema",
    "AcDb3DSolid_ASM_Data",
    "AcDbDs::TreatedAsObjectDataSchema",
    "AcDbDs::LegacySchema",
    "AcDbDs::IndexedPropertySchema",
    "AcDbDs::HandleAttributeSchema",
)
EXPECTED_PROXY_CLASSES = (
    (500, "ACSH_HISTORY_CLASS", "AcDbShHistory"),
    (501, "ACAD_EVALUATION_GRAPH", "AcDbEvalGraph"),
    (502, "ACSH_CONE_CLASS", "AcDbShCone"),
)
HEX_RE = re.compile(r"[0-9A-Fa-f]+")


class CheckError(ValueError):
    """A bounded round-trip contract violation."""


@dataclass(frozen=True)
class Pair:
    code: int
    value: str
    line: int


@dataclass(frozen=True)
class Record:
    kind: str
    pairs: tuple[Pair, ...]


@dataclass(frozen=True)
class Document:
    label: str
    path: Path
    sections: dict[str, tuple[tuple[Pair, ...], ...]]


@dataclass(frozen=True)
class Analysis:
    version: str
    solid_handle: str
    owner_handle: str
    history_handle: str
    evaluation_handle: str
    cone_handle: str
    material_handle: str
    sab: bytes
    sab_sha256: str


def require(condition: bool, message: str) -> None:
    if not condition:
        raise CheckError(message)


def parse_pairs(path: Path, label: str) -> list[Pair]:
    try:
        raw = path.read_bytes()
    except OSError as exc:
        raise CheckError(f"{label}: cannot read {path}: {exc}") from exc
    require(not raw.startswith(b"AutoCAD Binary DXF"),
            f"{label}: expected ASCII DXF, found binary DXF")
    try:
        lines = raw.decode("utf-8-sig").splitlines()
    except UnicodeDecodeError as exc:
        raise CheckError(f"{label}: DXF is not valid UTF-8 text: {exc}") from exc
    require(len(lines) % 2 == 0,
            f"{label}: truncated DXF code/value pair at line {len(lines)}")

    pairs: list[Pair] = []
    for index in range(0, len(lines), 2):
        code_text = lines[index].strip()
        try:
            code = int(code_text, 10)
        except ValueError as exc:
            raise CheckError(
                f"{label}: invalid group code {code_text!r} at line {index + 1}"
            ) from exc
        pairs.append(Pair(code, lines[index + 1].strip(), index + 1))
    require(pairs, f"{label}: empty DXF")
    return pairs


def parse_document(path: Path, label: str) -> Document:
    pairs = parse_pairs(path, label)
    sections: dict[str, list[tuple[Pair, ...]]] = {}
    current_name: str | None = None
    current_pairs: list[Pair] = []
    saw_eof = False
    index = 0
    while index < len(pairs):
        pair = pairs[index]
        if current_name is None:
            if pair.code == 0 and pair.value == "SECTION":
                require(index + 1 < len(pairs),
                        f"{label}: SECTION at line {pair.line} has no name")
                name_pair = pairs[index + 1]
                require(name_pair.code == 2 and bool(name_pair.value),
                        f"{label}: SECTION at line {pair.line} has an invalid name")
                current_name = name_pair.value
                current_pairs = []
                index += 2
                continue
            if pair.code == 0 and pair.value == "ENDSEC":
                raise CheckError(f"{label}: unmatched ENDSEC at line {pair.line}")
            if pair.code == 0 and pair.value == "EOF":
                require(not saw_eof, f"{label}: duplicate EOF at line {pair.line}")
                require(index == len(pairs) - 1,
                        f"{label}: data follows EOF at line {pair.line}")
                saw_eof = True
            index += 1
            continue

        if pair.code == 0 and pair.value == "ENDSEC":
            sections.setdefault(current_name, []).append(tuple(current_pairs))
            current_name = None
            current_pairs = []
        elif pair.code == 0 and pair.value == "SECTION":
            raise CheckError(f"{label}: nested SECTION at line {pair.line}")
        else:
            current_pairs.append(pair)
        index += 1

    require(current_name is None, f"{label}: unterminated {current_name} section")
    require(saw_eof, f"{label}: missing EOF marker")
    frozen = {name: tuple(values) for name, values in sections.items()}
    return Document(label, path, frozen)


def one_section(document: Document, name: str) -> tuple[Pair, ...]:
    matches = document.sections.get(name, ())
    require(len(matches) == 1,
            f"{document.label}: expected one {name} section, found {len(matches)}")
    return matches[0]


def records(section: tuple[Pair, ...]) -> list[Record]:
    result: list[Record] = []
    current: list[Pair] = []
    for pair in section:
        if pair.code == 0:
            if current:
                result.append(Record(current[0].value, tuple(current)))
            current = [pair]
        elif current:
            current.append(pair)
    if current:
        result.append(Record(current[0].value, tuple(current)))
    return result


def values(record: Record, code: int) -> list[str]:
    return [pair.value for pair in record.pairs[1:] if pair.code == code]


def one_value(record: Record, code: int, label: str) -> str:
    found = values(record, code)
    require(len(found) == 1,
            f"{label}: {record.kind} requires one group {code}, found {len(found)}")
    require(bool(found[0]), f"{label}: {record.kind} group {code} is empty")
    return found[0]


def one_int(record: Record, code: int, label: str) -> int:
    value = one_value(record, code, label)
    try:
        return int(value, 10)
    except ValueError as exc:
        raise CheckError(
            f"{label}: {record.kind} group {code} is not an integer: {value!r}"
        ) from exc


def normalize_handle(value: str, label: str) -> str:
    require(bool(HEX_RE.fullmatch(value)), f"{label}: invalid handle {value!r}")
    normalized = value.upper().lstrip("0") or "0"
    require(normalized != "0", f"{label}: handle must be nonzero")
    return normalized


def one_handle(record: Record, code: int, label: str) -> str:
    return normalize_handle(one_value(record, code, label),
                            f"{label}: {record.kind} group {code}")


def all_records(document: Document) -> list[Record]:
    result: list[Record] = []
    for section_instances in document.sections.values():
        for section in section_instances:
            result.extend(records(section))
    return result


def resolve_handle(document: Document, handle: str) -> Record:
    matches: list[Record] = []
    for record in all_records(document):
        for candidate in values(record, 5):
            if HEX_RE.fullmatch(candidate):
                normalized = candidate.upper().lstrip("0") or "0"
                if normalized == handle:
                    matches.append(record)
    require(len(matches) == 1,
            f"{document.label}: handle {handle} resolves to {len(matches)} records")
    return matches[0]


def acad_version(document: Document) -> str:
    header = one_section(document, "HEADER")
    versions: list[str] = []
    for index, pair in enumerate(header[:-1]):
        if pair.code == 9 and pair.value == "$ACADVER":
            value_pair = header[index + 1]
            require(value_pair.code == 1,
                    f"{document.label}: $ACADVER is not followed by group 1")
            versions.append(value_pair.value)
    require(len(versions) == 1,
            f"{document.label}: expected one $ACADVER, found {len(versions)}")
    return versions[0]


def schema_header(record: Record) -> Record:
    header: list[Pair] = []
    for pair in record.pairs:
        if pair.code == 101 and pair.value == "ACDSRECORD":
            break
        header.append(pair)
    return Record(record.kind, tuple(header))


def validate_schemas(document: Document, acds_records: list[Record]) -> None:
    schemas = [record for record in acds_records if record.kind == "ACDSSCHEMA"]
    require(len(schemas) == len(EXPECTED_SCHEMA_NAMES),
            f"{document.label}: expected six ACDSSCHEMA records, found {len(schemas)}")
    actual: list[str] = []
    for expected_id, schema in enumerate(schemas):
        header = schema_header(schema)
        schema_id = one_int(header, 90, document.label)
        require(schema_id == expected_id,
                f"{document.label}: ACDSSCHEMA #{expected_id} has ID {schema_id}")
        actual.append(one_value(header, 1, document.label))
    require(tuple(actual) == EXPECTED_SCHEMA_NAMES,
            f"{document.label}: ACDSSCHEMA name/order mismatch: {actual!r}")


def class_map(document: Document) -> dict[int, tuple[str, str]]:
    class_records = [record for record in records(one_section(document, "CLASSES"))
                     if record.kind == "CLASS"]
    result: dict[int, tuple[str, str]] = {}
    for class_id, record in enumerate(class_records, start=500):
        result[class_id] = (
            one_value(record, 1, document.label),
            one_value(record, 2, document.label),
        )
    return result


def validate_proxy_class(document: Document, proxy: Record,
                         expected: tuple[int, str, str]) -> None:
    class_id, record_name, cpp_name = expected
    require(one_int(proxy, 90, document.label) == 499,
            f"{document.label}: proxy {one_value(proxy, 5, document.label)} "
            "does not use proxy object class 499")
    actual_id = one_int(proxy, 91, document.label)
    require(actual_id == class_id,
            f"{document.label}: proxy class ID {actual_id} != expected {class_id}")
    require(one_int(proxy, 94, document.label) == 0,
            f"{document.label}: proxy class ID {actual_id} group 94 is not zero")
    resolved = class_map(document).get(actual_id)
    require(resolved == (record_name, cpp_name),
            f"{document.label}: class ID {actual_id} resolves to {resolved!r}, "
            f"expected {(record_name, cpp_name)!r}")


def decode_sab(document: Document, record: Record) -> bytes:
    declared = one_int(record, 94, document.label)
    require(declared > 0, f"{document.label}: ASM_Data group 94 is not positive")
    chunks = values(record, 310)
    require(bool(chunks), f"{document.label}: ASM_Data has no group 310 chunks")
    for index, chunk in enumerate(chunks, start=1):
        require(len(chunk) % 2 == 0 and bool(HEX_RE.fullmatch(chunk)),
                f"{document.label}: ASM_Data group 310 chunk #{index} is not "
                "even-length hex")
    encoded = "".join(chunks)
    sab = bytes.fromhex(encoded)
    require(len(sab) == declared,
            f"{document.label}: ASM_Data group 94 declares {declared} bytes, "
            f"but group 310 chunks assemble to {len(sab)}")
    require(sab.startswith(b"ACIS BinaryFile"),
            f"{document.label}: ASM_Data is not an ACIS SAB payload")
    return sab


def analyze(document: Document) -> Analysis:
    version = acad_version(document)
    require(version == EXPECTED_VERSION,
            f"{document.label}: expected {EXPECTED_VERSION}, found {version!r}")

    entity_records = records(one_section(document, "ENTITIES"))
    solids = [record for record in entity_records if record.kind == "3DSOLID"]
    require(len(solids) == 1,
            f"{document.label}: expected one 3DSOLID, found {len(solids)}")
    solid = solids[0]
    solid_handle = one_handle(solid, 5, document.label)
    owner_handle = one_handle(solid, 330, document.label)
    history_handle = one_handle(solid, 350, document.label)
    owner = resolve_handle(document, owner_handle)
    require(owner.kind == "BLOCK_RECORD" and "*Model_Space" in values(owner, 2),
            f"{document.label}: 3DSOLID owner {owner_handle} is not *Model_Space")

    object_records = records(one_section(document, "OBJECTS"))
    proxies = [record for record in object_records
               if record.kind == "ACAD_PROXY_OBJECT"]
    require(len(proxies) == 3,
            f"{document.label}: expected three ACAD_PROXY_OBJECT records, "
            f"found {len(proxies)}")
    history = resolve_handle(document, history_handle)
    require(history.kind == "ACAD_PROXY_OBJECT",
            f"{document.label}: history handle {history_handle} is {history.kind}")
    require(one_handle(history, 330, document.label) == solid_handle,
            f"{document.label}: history owner does not reference the 3DSOLID")
    evaluation_handle = one_handle(history, 360, document.label)
    require(not values(history, 340),
            f"{document.label}: history proxy unexpectedly has group 340")

    evaluation = resolve_handle(document, evaluation_handle)
    require(evaluation.kind == "ACAD_PROXY_OBJECT",
            f"{document.label}: evaluation handle {evaluation_handle} is "
            f"{evaluation.kind}")
    require(one_handle(evaluation, 330, document.label) == history_handle,
            f"{document.label}: evaluation owner does not reference history")
    cone_handle = one_handle(evaluation, 360, document.label)
    require(not values(evaluation, 340),
            f"{document.label}: evaluation proxy unexpectedly has group 340")

    cone = resolve_handle(document, cone_handle)
    require(cone.kind == "ACAD_PROXY_OBJECT",
            f"{document.label}: cone handle {cone_handle} is {cone.kind}")
    require(one_handle(cone, 330, document.label) == evaluation_handle,
            f"{document.label}: cone owner does not reference evaluation")
    material_handle = one_handle(cone, 340, document.label)
    require(not values(cone, 360),
            f"{document.label}: cone proxy unexpectedly has group 360")
    material = resolve_handle(document, material_handle)
    require(material.kind == "MATERIAL",
            f"{document.label}: cone group 340 handle {material_handle} resolves "
            f"to {material.kind}, not MATERIAL")
    require(len({history_handle, evaluation_handle, cone_handle}) == 3,
            f"{document.label}: proxy history chain contains a cycle")
    validate_proxy_class(document, history, EXPECTED_PROXY_CLASSES[0])
    validate_proxy_class(document, evaluation, EXPECTED_PROXY_CLASSES[1])
    validate_proxy_class(document, cone, EXPECTED_PROXY_CLASSES[2])

    acds_records = records(one_section(document, "ACDSDATA"))
    validate_schemas(document, acds_records)
    asm_records = [record for record in acds_records
                   if record.kind == "ACDSRECORD"
                   and "ASM_Data" in values(record, 2)]
    require(len(asm_records) == 1,
            f"{document.label}: expected one ASM_Data ACDSRECORD, "
            f"found {len(asm_records)}")
    asm_record = asm_records[0]
    require(one_int(asm_record, 90, document.label) == 1,
            f"{document.label}: ASM_Data record does not use schema ID 1")
    require(values(asm_record, 2) == ["AcDbDs::ID", "ASM_Data"],
            f"{document.label}: ASM_Data record key/name fields are malformed")
    asm_owner = one_handle(asm_record, 320, document.label)
    require(asm_owner == solid_handle,
            f"{document.label}: ASM_Data owner key {asm_owner} does not equal "
            f"3DSOLID handle {solid_handle}")
    sab = decode_sab(document, asm_record)
    return Analysis(
        version=version,
        solid_handle=solid_handle,
        owner_handle=owner_handle,
        history_handle=history_handle,
        evaluation_handle=evaluation_handle,
        cone_handle=cone_handle,
        material_handle=material_handle,
        sab=sab,
        sab_sha256=hashlib.sha256(sab).hexdigest(),
    )


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("libdxfrw_dxf", type=Path,
                        help="libdxfrw-produced AC1027 ASCII DXF")
    parser.add_argument("comparison_dxf", type=Path,
                        help="ODA return or re-imported AC1027 ASCII DXF")
    args = parser.parse_args(argv)
    try:
        source = analyze(parse_document(args.libdxfrw_dxf, "libdxfrw input"))
        returned = analyze(parse_document(args.comparison_dxf,
                                          "round-trip comparison"))
        require(source.sab_sha256 == returned.sab_sha256,
                "round-trip: SAB SHA-256 differs: "
                f"libdxfrw={source.sab_sha256}, ODA={returned.sab_sha256}")
        summary = {
            "documents": {
                "libdxfrw": {
                    "history": source.history_handle,
                    "solid": source.solid_handle,
                },
                "comparison": {
                    "history": returned.history_handle,
                    "solid": returned.solid_handle,
                },
            },
            "proxyClassIds": [item[0] for item in EXPECTED_PROXY_CLASSES],
            "sabBytes": len(source.sab),
            "sabSha256": source.sab_sha256,
            "schemaCount": len(EXPECTED_SCHEMA_NAMES),
            "version": EXPECTED_VERSION,
        }
        print(json.dumps(summary, sort_keys=True, separators=(",", ":")))
        return 0
    except CheckError as exc:
        print(f"check_acds_roundtrip: FAIL: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
