#!/usr/bin/env python3
"""Check LibreDWG semantic readback of the selected AC1027 MLINESTYLE DXFs.

LibreDWG's current dxf2dwg writer only emits through R2004, so this verifier
checks an AC1027 DXF input after LibreDWG parses it and writes an AC1018 DWG,
then dwgread emits minJSON. It verifies the bounded style and dictionary graph;
it does not claim source-wide or native-application fidelity.
"""

from __future__ import annotations

import argparse
import copy
import hashlib
import json
import math
from pathlib import Path
from typing import Any


EXPECTED_SOURCE_SHA256 = (
    "3a6c23151ebec25320722f0ecaaf20a10073c60079d3616181395f1c73deee42"
)


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def object_handle(value: dict[str, Any]) -> int:
    raw = value.get("handle")
    require(isinstance(raw, list) and bool(raw), "missing object handle")
    return int(raw[-1])


def reference_handle(value: Any) -> int:
    require(isinstance(value, list) and bool(value), "missing handle reference")
    return int(value[-1])


def one(items: list[dict[str, Any]], predicate, label: str) -> dict[str, Any]:
    matches = [item for item in items if predicate(item)]
    require(len(matches) == 1, f"expected one {label}, found {len(matches)}")
    return matches[0]


def mutable_style(data: dict[str, Any]) -> dict[str, Any]:
    return one(data["OBJECTS"], lambda value: value.get("object") == "MLINESTYLE",
               "self-test MLINESTYLE")


def mutable_dictionary(data: dict[str, Any], handle: int) -> dict[str, Any]:
    return one([value for value in data["OBJECTS"]
                if value.get("object") == "DICTIONARY"],
               lambda value: object_handle(value) == handle,
               f"self-test dictionary {handle:#x}")


def verify_peer_json(data: dict[str, Any], label: str) -> None:
    require(data.get("FILEHEADER", {}).get("version") == "AC1018",
            f"{label}: peer did not produce the bounded AC1018 reread")
    objects = data.get("OBJECTS")
    require(isinstance(objects, list), f"{label}: missing OBJECTS")
    style = one(objects, lambda value: value.get("object") == "MLINESTYLE",
                f"{label} MLINESTYLE")
    require(object_handle(style) == 0x18, f"{label}: style handle changed")
    require(style.get("type") == 73 and style.get("name") == "Standard",
            f"{label}: style type or name changed")
    require(style.get("flag") == 0 and style.get("description") == "",
            f"{label}: style flags or description changed")
    for key in ("start_angle", "end_angle"):
        require(math.isclose(float(style.get(key, math.nan)), math.pi / 2,
                             rel_tol=0.0, abs_tol=1e-12),
                f"{label}: {key} changed")
    require(reference_handle(style.get("ownerhandle")) == 0x17,
            f"{label}: style owner target changed")
    reactors = style.get("reactors")
    require(isinstance(reactors, list) and len(reactors) == 1
            and reference_handle(reactors[0]) == 0x17,
            f"{label}: style reactor role changed")

    # LibreDWG normalizes DXF group 62 value 256 through its CMC parser to the
    # method-tagged value below; the source differential independently binds
    # this profile's fill and line colors to BYLAYER.
    bylayer = {"rgb": "c2000000"}
    require(style.get("fill_color") == bylayer,
            f"{label}: fill color did not retain the BYLAYER projection")
    lines = style.get("lines")
    require(isinstance(lines, list) and len(lines) == 2,
            f"{label}: line element count changed")
    for index, expected_offset in enumerate((0.5, -0.5)):
        element = lines[index]
        require(math.isclose(float(element.get("offset", math.nan)),
                             expected_offset, rel_tol=0.0, abs_tol=1e-12),
                f"{label}: line {index} offset/order changed")
        require(element.get("color") == bylayer,
                f"{label}: line {index} color changed")
        require(element.get("lt.index") == 32767,
                f"{label}: line {index} BYLAYER linetype changed")

    dictionaries = [value for value in objects
                    if value.get("object") == "DICTIONARY"]
    named_dictionary = one(
        dictionaries,
        lambda value: object_handle(value) == 0x17,
        f"{label} MLINESTYLE owner dictionary")
    require(reference_handle(named_dictionary.get("ownerhandle")) == 0x0C,
            f"{label}: owner dictionary parent changed")
    require(named_dictionary.get("items", {}).get("Standard")
            and reference_handle(named_dictionary["items"]["Standard"]) == 0x18,
            f"{label}: Standard dictionary membership changed")
    owner_reactors = named_dictionary.get("reactors")
    require(isinstance(owner_reactors, list) and len(owner_reactors) == 1
            and reference_handle(owner_reactors[0]) == 0x0C,
            f"{label}: owner dictionary reactor role changed")

    root = one(dictionaries, lambda value: object_handle(value) == 0x0C,
               f"{label} named object dictionary")
    root_edges = [value for key, value in root.get("items", {}).items()
                  if key == "ACAD_MLINESTYLE"]
    require(len(root_edges) == 1 and reference_handle(root_edges[0]) == 0x17,
            f"{label}: root ACAD_MLINESTYLE membership changed")
    header = data.get("HEADER", {})
    require(reference_handle(header.get("DICTIONARY_ACAD_MLINESTYLE")) == 0x17,
            f"{label}: header ACAD_MLINESTYLE handle changed")
    require(reference_handle(header.get("CMLSTYLE")) == 0x18,
            f"{label}: current MLINESTYLE identity changed")


def verify_source_receipts(source_path: Path,
                           differential_path: Path,
                           span_path: Path) -> None:
    digest = hashlib.sha256(source_path.read_bytes()).hexdigest()
    require(digest == EXPECTED_SOURCE_SHA256,
            f"source hash mismatch: {digest}")
    differential = json.loads(differential_path.read_text())
    spans = json.loads(span_path.read_text())
    require(differential.get("source", {}).get("sha256") == digest
            and spans.get("source", {}).get("sha256") == digest,
            "source receipts do not bind the selected Cover.dwg hash")
    source_object = differential["source"]["object"]
    require(source_object.get("handle") == "0x18"
            and source_object.get("ownerHandle") == "0x17"
            and source_object.get("dictionary") == "ACAD_MLINESTYLE"
            and source_object.get("dictionaryKey") == "Standard",
            "source MLINESTYLE identity/role receipt changed")
    fields = differential["comparison"]["fields"]
    require(fields["name"]["libdxfrw"] == "Standard"
            and fields["flags"]["libdxfrw"] == 0
            and fields["fillColor"]["libdxfrw"] == 256
            and fields["startAngle"]["libdxfrwDegrees"] == 90.0
            and fields["endAngle"]["libdxfrwDegrees"] == 90.0
            and [row["offset"] for row in fields["elements"]]
                == [0.5, -0.5],
            "source semantic differential no longer matches the selected profile")
    require(spans.get("object", {}).get("type") == 73
            and spans.get("object", {}).get("handleHex") == "0x18"
            and spans.get("object", {}).get("handleRoles", {}).get(
                "ownerhandle", {}).get("target") == 23
            and spans.get("object", {}).get("handleRoles", {}).get(
                "reactors[0]", {}).get("target") == 23,
            "source physical-span receipt no longer binds owner/reactor roles")


def run_self_tests(template: dict[str, Any]) -> int:
    mutations = (
        ("offset", lambda data: mutable_style(data)["lines"][0].__setitem__(
            "offset", 0.75)),
        ("style owner", lambda data: mutable_style(data)["ownerhandle"].__setitem__(
            -1, 0x19)),
        ("dictionary target", lambda data: mutable_dictionary(data, 0x17)[
            "items"]["Standard"].__setitem__(-1, 0x19)),
        ("fill color", lambda data: mutable_style(data)["fill_color"].__setitem__(
            "rgb", "c3000000")),
    )
    for label, mutate in mutations:
        altered = copy.deepcopy(template)
        mutate(altered)
        try:
            verify_peer_json(altered, f"self-test {label}")
        except (TypeError, ValueError, KeyError, IndexError):
            continue
        raise ValueError(f"self-test mutation was accepted: {label}")
    return len(mutations)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--differential-receipt", type=Path, required=True)
    parser.add_argument("--span-receipt", type=Path, required=True)
    parser.add_argument("--ascii-json", type=Path, required=True)
    parser.add_argument("--binary-json", type=Path, required=True)
    args = parser.parse_args()

    verify_source_receipts(args.source, args.differential_receipt,
                           args.span_receipt)
    ascii_data = json.loads(args.ascii_json.read_text())
    binary_data = json.loads(args.binary_json.read_text())
    verify_peer_json(ascii_data, "ASCII")
    verify_peer_json(binary_data, "Binary")
    require(ascii_data == binary_data,
            "ASCII and Binary outputs differ after independent peer readback")
    mutation_count = run_self_tests(ascii_data)
    print("PASS: physical source receipts, owner/reactor/dictionary graph, "
          "style fields, ASCII/Binary peer equivalence, and "
          f"{mutation_count} negative controls")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, TypeError, ValueError, KeyError, IndexError) as error:
        raise SystemExit(f"FAIL: {error}")
