#!/usr/bin/env python3
"""Recover one encrypted SAT v1 payload from an ASCII DXF modeler entity.

This is a research aid for comparing DXF carrier bytes with independent ACIS
readers. It does not validate SAT semantics or promote libdxfrw support.

The decoder mirrors LibreDWG's SAT1 transform (0x9f - byte for bytes above
space) and its DXF caret escape. Group 1 starts a SAT text line; a following
group 3 continues that line. A later group 1 starts the next SAT line.
"""

from __future__ import annotations

import argparse
from pathlib import Path
import sys


class DecodeError(ValueError):
    """The selected DXF entity is not a bounded SAT v1 text carrier."""


def _ascii(value: bytes, label: str) -> str:
    try:
        return value.decode("ascii")
    except UnicodeDecodeError as exc:
        raise DecodeError(f"{label} is not ASCII") from exc


def _pairs(data: bytes):
    lines = data.splitlines()
    if len(lines) % 2:
        raise DecodeError("DXF ends with an incomplete group-code/value pair")
    for offset in range(0, len(lines), 2):
        code_text = lines[offset].strip()
        try:
            code = int(code_text)
        except ValueError as exc:
            raise DecodeError(
                f"invalid group code on DXF line {offset + 1}: {code_text!r}"
            ) from exc
        yield code, lines[offset + 1]


def _entities(data: bytes):
    current_type = None
    current = []
    for code, value in _pairs(data):
        if code == 0:
            if current_type is not None:
                yield current_type, current
            current_type = _ascii(value.strip(), "DXF group 0 value")
            current = []
        elif current_type is not None:
            current.append((code, value))
    if current_type is not None:
        yield current_type, current


def _decode_sat1_line(encrypted: bytes) -> bytes:
    # LibreDWG's DXF writer emits "^ " for an encrypted caret so the SAT
    # record lexer cannot confuse that byte with a SAT control marker.
    escaped = encrypted.replace(b"^ ", b"^")
    decoded = bytearray(len(escaped))
    for index, value in enumerate(escaped):
        if value > 0x7E:
            raise DecodeError("SAT1 DXF carrier contains a non-ASCII payload byte")
        decoded[index] = 0x9F - value if value > 0x20 else value
    return bytes(decoded)


def extract_sat1_payload(dxf: bytes, handle: str) -> bytes:
    """Return decoded SAT v1 text for an exact 3DSOLID/REGION/BODY handle."""
    wanted = handle.strip()
    if wanted[:2].lower() == "0x":
        wanted = wanted[2:]
    wanted = wanted.upper()
    if not wanted or any(c not in "0123456789ABCDEF" for c in wanted):
        raise DecodeError(f"invalid hexadecimal handle: {handle!r}")

    matches = []
    for entity_type, fields in _entities(dxf):
        if entity_type not in {"3DSOLID", "REGION", "BODY"}:
            continue
        handles = [
            _ascii(value.strip(), "entity handle").upper()
            for code, value in fields
            if code == 5
        ]
        if handles == [wanted]:
            matches.append((entity_type, fields))
    if len(matches) != 1:
        raise DecodeError(
            f"expected one modeler entity at handle {wanted}, found {len(matches)}"
        )

    entity_type, fields = matches[0]
    versions = [
        _ascii(value.strip(), "modeler format version")
        for code, value in fields
        if code == 70
    ]
    if versions != ["1"]:
        raise DecodeError(
            f"{entity_type} {wanted} is not an exact SAT v1 carrier (group 70={versions})"
        )

    lines: list[bytes] = []
    for code, value in fields:
        if code == 1:
            lines.append(value)
        elif code == 3:
            if not lines:
                raise DecodeError("group 3 appears before any group 1 SAT line")
            lines[-1] += value
    if not lines:
        raise DecodeError(f"{entity_type} {wanted} has no group 1/3 SAT payload")

    sat_lines = [_decode_sat1_line(line) for line in lines]
    payload = b"\n".join(sat_lines) + b"\n"
    header = payload.splitlines()[0].split()
    if len(header) != 4:
        raise DecodeError("decoded payload does not have a four-field SAT header")
    try:
        int(header[0])
        int(header[1])
        int(header[2])
        int(header[3])
    except ValueError as exc:
        raise DecodeError("decoded payload has a malformed SAT header") from exc
    end_line = payload.splitlines()[-1].rstrip(b" \t")
    # SAT/ASM end markers are ordinary unprefixed tokens in the pinned
    # InventorLoader and ezdxf readers. Also retain the prefixed spellings
    # accepted by this helper before it encountered an unprefixed source.
    if end_line not in {
        b"End-of-ACIS-data", b"End-of-ASM-data",
        b"#End-of-ACIS-data", b"#End-of-ASM-data",
    }:
        raise DecodeError("decoded payload does not end with an ACIS/ASM end marker")
    return payload


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("dxf", type=Path, help="ASCII DXF input")
    parser.add_argument("handle", help="hex handle of 3DSOLID, REGION, or BODY")
    parser.add_argument("output", type=Path, help="output SAT path")
    parser.add_argument("--force", action="store_true", help="replace output if it exists")
    args = parser.parse_args()

    if args.output.exists() and not args.force:
        parser.error(f"output already exists: {args.output} (use --force to replace)")
    try:
        payload = extract_sat1_payload(args.dxf.read_bytes(), args.handle)
        args.output.write_bytes(payload)
    except (OSError, DecodeError) as exc:
        print(f"decode_dxf_sat1: {exc}", file=sys.stderr)
        return 1
    print(f"wrote {len(payload)} SAT bytes to {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
