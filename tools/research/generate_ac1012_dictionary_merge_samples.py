#!/usr/bin/env python3
"""Create paired, scratch-only AC1012 MLINESTYLE dictionary merge samples.

The two drawings contain the canonical NOD -> ACAD_MLINESTYLE -> SAME_KEY
graph. Each SAME_KEY target is owned by ACAD_MLINESTYLE and has a different
start angle. LibreDWG is only the sample producer here; its output is not
native AutoCAD evidence. Optionally pass the focused libdxfrw test executable
to parse each extracted physical dictionary frame and exercise role/truncation
refusal and corrected retry. An optional candidate is checked for strict
refusal and explicit-partial ASCII/Binary readback.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
from pathlib import Path
import re
import struct
import subprocess
import sys
import tempfile

from generate_ac1012_dictionary_witness import (
    Bits,
    crc16,
    executable_identity,
    require,
    run,
    sha,
)


VERSION = b"AC1012"
ENTRY_KEY = "SAME_KEY"
STYLE_DICTIONARY = "ACAD_MLINESTYLE"


def read_object_frame(path: Path, dwgread: str, handle: int,
                      object_type: int) -> tuple[int, bytes]:
    """Locate one R13 frame through LibreDWG's trace and validate size/CRC."""
    data = path.read_bytes()
    require(data.startswith(VERSION), "sample is not AC1012")
    trace = run([dwgread, "-v4", str(path)])
    text = (trace.stdout + trace.stderr).decode(errors="replace")
    blocks = re.split(r"(?=Object number:)", text)
    pattern = re.compile(
        r"\bhandle: 0\.\d+\." + format(handle, "X") + r"\s+\[H 5\]",
        re.IGNORECASE,
    )
    selected = [block for block in blocks if pattern.search(block)]
    require(len(selected) == 1, "trace does not identify one selected object frame")
    match = re.search(r"Size: (\d+) \[MS\], Type: " + str(object_type)
                      + r" \[BS\], Address: (\d+)", selected[0])
    require(match is not None, "object frame size/address or type missing")
    size, address = map(int, match.groups())
    require(size < 0x8000 and address >= 2 and address + size + 2 <= len(data),
            "object frame is outside the source")
    require(data[address - 2:address] == size.to_bytes(2, "little"),
            "unexpected AC1012 frame-size prefix")
    require(int.from_bytes(data[address + size:address + size + 2], "little")
            == crc16(data[address - 2:address + size]), "object frame CRC mismatch")

    return address, data[address:address + size]


def observe_single_line_frame(frame: bytes, handle: int, layer: int) -> dict:
    """Verify the generated R13 model-space LINE's physical link roles."""
    bits = Bits(frame)
    require(bits.bs() == 19 and bits.handle(0, own=True)["value"] == handle,
            "wrong model-space LINE frame")
    require(bits.bs() == 0 and bits.take(1) == 0,
            "generated LINE has unsupported EED or proxy data")
    data_end = bits.little(4)
    require(bits.take(2) == 2 and bits.bl() == 0 and bits.take(1) == 1,
            "generated LINE has unexpected common fields")
    no_links_bit = bits.position
    require(bits.take(1) == 0, "model-space LINE must use explicit links")
    require(bits.position <= data_end <= len(frame) * 8,
            "LINE handle stream is outside its frame")
    bits.bit = data_end
    extension = bits.handle(3, handle)
    layer_ref = bits.handle(5, handle)
    previous_bit = bits.position
    previous = bits.handle(4, handle)
    next_ref = bits.handle(4, handle)
    require(extension["role"] == 3 and extension["value"] == 0 and layer_ref["role"] == 5
            and layer_ref["value"] == layer, "LINE extension or layer mismatch")
    require(previous["role"] == 4 and previous["value"] == 0
            and next_ref["role"] == 4 and next_ref["value"] == 0,
            "LINE boundary links must be typed null soft pointers")
    require(len(frame) * 8 - bits.position < 8, "unexpected LINE handle tail")
    return {"dataEndBit": data_end, "noLinksBit": no_links_bit,
            "previousHandleBit": previous_bit, "extension": extension,
            "layer": layer_ref, "previous": previous, "next": next_ref,
            "frameSha256": sha(frame)}


def observe_populated_style_frame(frame: bytes, handle: int, name: str,
                                  owner: int, start_angle: float) -> dict:
    """Bind a generated R13 MLINESTYLE's body and handle roles to its frame."""
    bits = Bits(frame)
    spans = {}

    def field(label, reader):
        start = bits.position
        value = reader()
        spans[label] = {"startBit": start, "endBit": bits.position, "value": value}
        return value

    def text_value():
        length = bits.bs()
        require(length <= 128, "style text exceeds the selected profile")
        raw = bytes(bits.take(8) for _ in range(length))
        require(raw.count(b"\0") <= 1 and (b"\0" not in raw or raw.endswith(b"\0")),
                "style text has an invalid terminator")
        return (raw[:-1] if raw.endswith(b"\0") else raw).decode("ascii")

    def bit_double():
        tag = bits.take(2)
        if tag == 0:
            return struct.unpack("<d", bytes(bits.take(8) for _ in range(8)))[0]
        require(tag in (1, 2), "invalid style bit-double")
        return 1.0 if tag == 1 else 0.0

    require(field("type", bits.bs) == 73
            and field("self", lambda: bits.handle(0, own=True))["value"] == handle
            and field("eedSize", bits.bs) == 0, "wrong or unsupported style frame")
    data_end = field("dataEndBit", lambda: bits.little(4))
    require(field("reactorCount", bits.bl) == 1
            and field("name", text_value) == name
            and field("description", text_value) == ""
            and field("flags", bits.bs) == 0
            and field("fillColor", bits.bs) == 256, "unexpected style prefix")
    require(math.isclose(field("startAngle", bit_double), start_angle, abs_tol=1e-10)
            and math.isclose(field("endAngle", bit_double), math.pi / 2, abs_tol=1e-10)
            and field("elementCount", lambda: bits.take(8)) == 2,
            "unexpected style angles or element count")
    for index, offset in enumerate((0.5, -0.5)):
        require(field(f"offset{index}", bit_double) == offset
                and field(f"color{index}", bits.bs) == 256
                and field(f"linetypeIndex{index}", bits.bs) == 32767,
                "unexpected style element")
    require(bits.position == data_end, "style body boundary differs")
    parent = field("owner", lambda: bits.handle(4, handle))
    reactor = field("reactor", lambda: bits.handle(4, handle))
    extension = field("extension", lambda: bits.handle(3, handle))
    require(parent["value"] == owner and reactor["value"] == owner
            and extension["role"] == 3 and extension["value"] == 0
            and len(frame) * 8 - bits.position < 8, "style handles or tail differ")
    return {"frameSha256": sha(frame), "spans": spans,
            "paddingBits": len(frame) * 8 - bits.position}


def check_single_line_graph(drawing: dict) -> tuple[dict, int, int]:
    """Bind the sole model-space LINE to its block header and layer."""
    objects = drawing["OBJECTS"]
    lines = [obj for obj in objects if obj.get("entity") == "LINE"]
    require(len(lines) == 1, "sample requires exactly one LINE")
    line = lines[0]
    handle = line.get("handle", [None])[-1]
    layer = drawing.get("HEADER", {}).get("CLAYER", [None])[-1]
    model = drawing.get("HEADER", {}).get("BLOCK_RECORD_MSPACE", [None])[-1]
    blocks = [obj for obj in objects if obj.get("object") == "BLOCK_HEADER"
              and obj.get("handle", [None])[-1] == model]
    require(isinstance(handle, int) and handle > 0 and isinstance(layer, int)
            and layer > 0 and len(blocks) == 1, "invalid LINE, layer or model handle")
    require(blocks[0].get("first_entity", [None])[-1] == handle
            and blocks[0].get("last_entity", [None])[-1] == handle
            and line.get("layer", [None])[-1] == layer,
            "model-space LINE graph is inconsistent")
    return line, handle, layer


def read_dictionary_frame(path: Path, dwgread: str, handle: int,
                          expected_entries: dict[str, int],
                          selected_key: str = ENTRY_KEY,
                          allow_empty_name: bool = False) -> tuple[dict, bytes]:
    address, frame = read_object_frame(path, dwgread, handle, 42)
    size = len(frame)
    bits = Bits(frame)
    require(bits.bs() == 42, "selected frame is not DICTIONARY")
    own = bits.handle(0, own=True)
    require(own["value"] == handle, "dictionary self handle mismatch")
    eed = []
    while True:
        length = bits.bs()
        if length == 0:
            break
        require(length <= 1024 and len(eed) < 32, "dictionary EED exceeds the bound")
        app = bits.handle(5)
        payload = bytes(bits.take(8) for _ in range(length))
        eed.append({"appid": app, "hex": payload.hex()})

    data_end = bits.little(4)
    reactors = bits.bl()
    count = bits.bl()
    require(reactors <= 8 and 0 < count <= 64
            and count == len(expected_entries),
            "dictionary entry count differs from the bounded model")
    names = []
    for _ in range(count):
        name_length = bits.bs()
        require((name_length > 0 or allow_empty_name) and name_length <= 256,
                "dictionary TV name length is invalid")
        name_bytes = bytes(bits.take(8) for _ in range(name_length))
        require(name_bytes.count(b"\0") <= 1
                and (b"\0" not in name_bytes or name_bytes.endswith(b"\0")),
                "AC1012 TV name has an embedded or repeated terminator")
        name = name_bytes[:-1].decode("ascii") if name_bytes.endswith(b"\0") \
            else name_bytes.decode("ascii")
        names.append({"name": name,
                      "nameHasTerminator": name_bytes.endswith(b"\0")})
    require(len({item["name"] for item in names}) == len(names),
            "dictionary contains duplicate entry names")
    require(bits.position == data_end, "dictionary data span does not end after its TV entries")

    parent = bits.handle(4, handle)
    reactor_handles = [bits.handle(4, handle) for _ in range(reactors)]
    xdictionary = bits.handle(3, handle)
    entries = [dict(name=name["name"],
                    nameHasTerminator=name["nameHasTerminator"],
                    handle=bits.handle(2, handle)) for name in names]
    require({item["name"]: item["handle"]["value"] for item in entries}
            == expected_entries,
            "physical dictionary entries differ from the model")
    require(len(frame) * 8 - bits.position < 8, "unexpected data after dictionary item handle")
    selected = next((item for item in entries if item["name"] == selected_key), None)
    require(selected is not None and selected["handle"]["value"] != 0,
            "dictionary item differs from the selected key")
    proof = {
        "offset": address,
        "sizeBytes": size,
        "frameSha256": sha(frame),
        "crcValid": True,
        "self": own,
        "dataEndBit": data_end,
        "entries": entries,
        "entryName": selected["name"],
        "entryNameHasTerminator": selected["nameHasTerminator"],
        "parent": parent,
        "reactors": reactor_handles,
        "xdictionary": xdictionary,
        "itemHandle": selected["handle"],
        "cloningField": None,
        "eed": eed,
        "paddingBits": len(frame) * 8 - bits.position,
    }
    return proof, frame


def read_model(path: Path, dwgread: str,
               unique_key: str | None = None) -> dict:
    decoded = path.with_suffix(".json")
    run([dwgread, "-O", "JSON", "-o", str(decoded), str(path)])
    drawing = json.loads(decoded.read_text())
    objects = drawing["OBJECTS"]
    roots = [
        obj for obj in objects
        if obj.get("object") == "DICTIONARY"
        and STYLE_DICTIONARY in obj.get("items", {})
    ]
    require(len(roots) == 1, "ACAD_MLINESTYLE NOD membership is not unique")
    root = roots[0]
    dictionary_handle = root["items"][STYLE_DICTIONARY][-1]
    dictionaries = [
        obj for obj in objects
        if obj.get("object") == "DICTIONARY"
        and obj.get("handle", [None])[-1] == dictionary_handle
    ]
    require(len(dictionaries) == 1,
            "ACAD_MLINESTYLE child dictionary is missing or duplicated")
    dictionary = dictionaries[0]
    expected_names = {"Standard", ENTRY_KEY}
    if unique_key is not None:
        expected_names.add(unique_key)
    require(dictionary.get("numitems") == len(expected_names),
            "ACAD_MLINESTYLE item count differs from the selected profile")
    raw_entries = dictionary.get("items", {})
    dictionary_entries = {
        name: value[-1] for name, value in raw_entries.items()
        if isinstance(value, list) and len(value) >= 3
    }
    require(len(dictionary_entries) == dictionary.get("numitems")
            and set(dictionary_entries) == expected_names,
            "ACAD_MLINESTYLE entries are incomplete")
    entry = raw_entries.get(ENTRY_KEY)
    require(isinstance(entry, list) and len(entry) >= 3,
            "SAME_KEY does not resolve to one object handle")
    target_handle = entry[-1]
    target = [
        obj for obj in objects
        if obj.get("object") == "MLINESTYLE"
        and obj.get("handle", [None])[-1] == target_handle
    ]
    require(len(target) == 1, "SAME_KEY target is not one MLINESTYLE")
    require(target[0].get("name") == ENTRY_KEY,
            "SAME_KEY target style name does not match its dictionary key")
    target_owner = target[0].get("ownerhandle", [None])[-1]
    require(target_owner == dictionary_handle,
            "SAME_KEY target is not owned by ACAD_MLINESTYLE")
    dimstyles = [
        obj for obj in objects
        if obj.get("object") == "DIMSTYLE" and obj.get("name") == "Standard"
    ]
    require(len(dimstyles) == 1, "Standard DIMSTYLE record is not unique")
    current_dimstyle = drawing.get("HEADER", {}).get("DIMSTYLE")
    require(isinstance(current_dimstyle, list) and len(current_dimstyle) >= 4
            and current_dimstyle[-1] == dimstyles[0].get("handle", [None])[-1],
            "header current DIMSTYLE handle does not resolve to Standard")
    styles = {}
    for name in expected_names - {"Standard"}:
        style_handle = dictionary_entries[name]
        style = [
            obj for obj in objects
            if obj.get("object") == "MLINESTYLE"
            and obj.get("handle", [None])[-1] == style_handle
        ]
        require(len(style) == 1 and style[0].get("name") == name
                and style[0].get("ownerhandle", [None])[-1] == dictionary_handle,
                f"{name} does not resolve to an owned MLINESTYLE")
        styles[name] = {
            "handle": style_handle,
            "ownerHandle": dictionary_handle,
            "startAngle": style[0].get("start_angle"),
        }
    return {
        "sha256": sha(path.read_bytes()),
        "dictionaryHandle": dictionary_handle,
        "rootHandle": root["handle"][-1],
        "dictionaryOwner": dictionary.get("ownerhandle", [None])[-1],
        "dictionaryEntries": dictionary_entries,
        "headerCurrentDimStyleReference": current_dimstyle,
        "entry": {"name": ENTRY_KEY, "raw": entry, "targetHandle": target_handle},
        "targetStyle": {
            "handle": target_handle,
            "name": target[0].get("name"),
            "ownerHandle": target_owner,
            "startAngle": target[0].get("start_angle"),
        },
        "namedStyles": styles,
        "standardDimStyle": {
            "handle": dimstyles[0].get("handle", [None])[-1],
            "name": dimstyles[0].get("name"),
        },
    }


def run_candidate(candidate: str, source: Path, root: Path, label: str,
                  partial: bool, binary: bool = False,
                  expected_angle: float | None = None,
                  expected_styles: dict[str, float] | None = None,
                  expected_line_count: int | None = None) -> dict:
    candidate_path = Path(candidate).resolve()
    mode = "explicit-partial" if partial else "strict"
    encoding = "binary" if binary else "ascii"
    candidate_output = root / f"{label}-candidate-{mode}-{encoding}.dxf"
    sentinel = b"preserve prior DXF destination on refusal\n"
    candidate_output.write_bytes(sentinel)
    command = [str(candidate_path), str(source), "-o", str(candidate_output),
               "-v2010", "-y"]
    if binary:
        command.append("-b")
    command.append("-d")
    if partial:
        command.append("--allow-partial-dwg-read")
    result = subprocess.run(
        command,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        timeout=120,
        check=False,
    )
    log = result.stdout.decode(errors="replace")
    preserved = candidate_output.read_bytes() == sentinel
    require(result.returncode == 0 or preserved,
            f"candidate replaced its destination after {mode} refusal")
    if partial:
        require(result.returncode == 0 and not preserved,
                f"candidate did not emit the bounded {encoding} partial projection")
    else:
        require(result.returncode != 0 and preserved,
                "candidate unexpectedly accepted strict output for the unqualified source")
        require("AC1012 ACAD_MLINESTYLE dictionary duplicate-merge semantics lack native qualification"
                in log,
                "strict refusal did not identify the unresolved AC1012 merge semantics")
    audit = None
    if result.returncode == 0:
        audit_script = Path(__file__).with_name("audit_dxf_dictionary_cloning.py")
        audited = subprocess.run(
            [sys.executable, str(audit_script), "--details", str(candidate_output)],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            timeout=30,
            check=False,
        )
        require(audited.returncode == 0,
                f"independent {encoding} DXF dictionary/style audit failed")
        audit = json.loads(audited.stdout)
        roots = audit.get("mlinestyle_roots", [])
        require(len(roots) == 1, "output ACAD_MLINESTYLE root is not unique")
        style_dictionary = roots[0]["target"]
        require(style_dictionary.get("owner") == roots[0].get("owner"),
                "ACAD_MLINESTYLE dictionary owner does not resolve to the NOD")
        selected = [style for style in roots[0].get("styles", [])
                    if style.get("key") == ENTRY_KEY]
        require(style_dictionary.get("cloning") == 0 and len(selected) == 1,
                "output selected dictionary field/entry is missing")
        style = selected[0]
        require(style.get("name") == ENTRY_KEY
                and style.get("owner") == style_dictionary.get("handle")
                and style.get("start_angle") is not None,
                "output MLINESTYLE identity or owner differs from its dictionary")
        if expected_angle is not None:
            require(abs(style["start_angle"] - expected_angle) < 1e-8,
                    "output MLINESTYLE start angle differs from the selected source")
        if expected_line_count is not None:
            require(style.get("line_count") == expected_line_count
                    and style.get("end_angle") is not None
                    and abs(style["end_angle"] - 90.0) < 1e-8,
                    "output MLINESTYLE elements or end angle differ")
        if expected_styles is not None:
            for name, angle in expected_styles.items():
                matches = [item for item in roots[0].get("styles", [])
                           if item.get("key") == name and item.get("name") == name]
                require(len(matches) == 1
                        and matches[0].get("owner") == roots[0]["target"].get("handle")
                        and abs(matches[0]["start_angle"] - angle) < 1e-8
                        and (expected_line_count is None or
                             (matches[0].get("line_count") == expected_line_count
                              and matches[0].get("end_angle") is not None
                              and abs(matches[0]["end_angle"] - 90.0) < 1e-8)),
                        f"output MLINESTYLE {name} differs from the selected source")
    lines = [line.strip() for line in log.splitlines()
             if ("DWG read observations:" in line
                 or "frameCoverageStatus=" in line
                 or "Opaque callback dispositions:" in line
                 or "integrity row:" in line
                 or "DXF export preflight failed:" in line
                 or "DXF export failed:" in line)]
    return {
        "mode": mode,
        "encoding": encoding,
        "executable": executable_identity(str(candidate_path)),
        "command": command,
        "exitCode": result.returncode,
        "destinationPreservedOnRefusal": preserved if result.returncode else None,
        "outputSha256": sha(candidate_output.read_bytes()) if result.returncode == 0 else None,
        "independentDxfAudit": audit,
        "diagnostics": lines[-12:],
    }


def write_recipe(path: Path, angle: int, unique_key: str | None = None,
                 unique_angle: int | None = None) -> None:
    recipe = (
        "line (0 0 0) (1 1 0)\n"
        'dimstyle "Standard"\n'
        f'mlinestyle "{ENTRY_KEY}"\n'
        f"mlinestyle.start_angle = {angle}\n"
    )
    if unique_key is not None:
        require(unique_angle is not None, "unique-key control needs a unique angle")
        recipe += (
            f'mlinestyle "{unique_key}"\n'
            f"mlinestyle.start_angle = {unique_angle}\n"
        )
    path.write_text(recipe)


def materialize_r13_source(initial: Path, output: Path, dwgread: str,
                           dwgwrite: str, root: Path, label: str) -> dict:
    decoded = root / f"{label}-preheader.json"
    run([dwgread, "-O", "JSON", "-o", str(decoded), str(initial)])
    drawing = json.loads(decoded.read_text())
    dimstyles = [obj for obj in drawing["OBJECTS"]
                 if obj.get("object") == "DIMSTYLE"
                 and obj.get("name") == "Standard"]
    require(len(dimstyles) == 1,
            "producer did not create one Standard DIMSTYLE record")
    dimstyle_handle = dimstyles[0].get("handle", [None])[-1]
    require(isinstance(dimstyle_handle, int) and dimstyle_handle != 0,
            "Standard DIMSTYLE has no nonzero handle")
    header = drawing.get("HEADER", {})
    layer_reference = header.get("CLAYER")
    require(isinstance(layer_reference, list) and len(layer_reference) >= 4
            and layer_reference[0] == 5 and layer_reference[1] > 0,
            "producer did not provide a versioned hard-pointer header reference")
    original_reference = header.get("DIMSTYLE")
    header["DIMSTYLE"] = [layer_reference[0], layer_reference[1],
                          dimstyle_handle, dimstyle_handle]
    line, line_handle, layer_handle = check_single_line_graph(drawing)
    original_no_links = line.get("nolinks")
    line["nolinks"] = 0
    line["prev_entity"] = [4, 0, 0, 0]
    line["next_entity"] = [4, 0, 0, 0]
    styles = [obj for obj in drawing["OBJECTS"] if obj.get("object") == "MLINESTYLE"]
    standard = [obj for obj in styles if obj.get("name") == "Standard"]
    require(len(standard) == 1 and len(standard[0].get("lines", [])) == 2,
            "producer did not provide a two-element Standard MLINESTYLE")
    standard_lines = standard[0]["lines"]
    require([row.get("offset") for row in standard_lines] == [0.5, -0.5]
            and all(row.get("color") == 256 and row.get("lt_index") == 32767
                    for row in standard_lines)
            and math.isclose(standard[0].get("end_angle", 0), math.pi / 2, abs_tol=1e-10),
            "Standard MLINESTYLE has an unexpected element profile")
    for style in styles:
        if style.get("name") != "Standard":
            require(style.get("name") in {ENTRY_KEY, "SOURCE_ONLY", "DEST_ONLY"}
                    and not style.get("lines"), "unexpected generated style payload")
            style["lines"] = [dict(row) for row in standard_lines]
            style["end_angle"] = standard[0]["end_angle"]
    patched = root / f"{label}-header-bound.json"
    patched.write_text(json.dumps(drawing, separators=(",", ":")) + "\n")
    run([dwgwrite, "-I", "JSON", "--as", "r13", "-o", str(output),
         str(patched)])
    require(output.read_bytes().startswith(VERSION),
            "versioned DWG writer did not produce AC1012")
    readback = root / f"{label}-header-readback.json"
    run([dwgread, "-O", "JSON", "-o", str(readback), str(output)])
    decoded_output = json.loads(readback.read_text())
    normalized = decoded_output["HEADER"].get("DIMSTYLE")
    require(isinstance(normalized, list) and len(normalized) >= 4
            and normalized[0] == 5 and normalized[1] > 0
            and normalized[-1] == dimstyle_handle,
            "encoded current DIMSTYLE hard pointer does not read back")
    decoded_line, decoded_handle, decoded_layer = check_single_line_graph(decoded_output)
    require(decoded_handle == line_handle and decoded_layer == layer_handle
            and decoded_line.get("nolinks") == 0
            and decoded_line.get("prev_entity") == [4, 0, 0, 0]
            and decoded_line.get("next_entity") == [4, 0, 0, 0],
            "explicit model-space LINE links did not read back")
    decoded_styles = [obj for obj in decoded_output["OBJECTS"]
                      if obj.get("object") == "MLINESTYLE"]
    require(len(decoded_styles) == len(styles) and all(
        obj.get("lines") == standard_lines and math.isclose(
            obj.get("end_angle", 0), math.pi / 2, abs_tol=1e-10)
        for obj in decoded_styles), "populated MLINESTYLE data did not read back")
    style_wire = {}
    for style in decoded_styles:
        name = style["name"]
        handle = style["handle"][-1]
        owner = style["ownerhandle"][-1]
        offset, style_frame = read_object_frame(output, dwgread, handle, 73)
        proof = observe_populated_style_frame(style_frame, handle, name, owner,
                                               style["start_angle"])
        bad_bit = proof["spans"]["extension"]["startBit"] + 3
        damaged = bytearray(style_frame)
        damaged[bad_bit // 8] ^= 1 << (7 - bad_bit % 8)
        try:
            observe_populated_style_frame(bytes(damaged), handle, name, owner,
                                          style["start_angle"])
        except ValueError:
            pass
        else:
            raise ValueError("style extension role mutation unexpectedly accepted")
        require(observe_populated_style_frame(style_frame, handle, name, owner,
                                             style["start_angle"])["frameSha256"] == proof["frameSha256"],
                "style frame retry changed")
        style_wire[name] = {"handle": handle, "offset": offset, "wire": proof,
                            "wrongRoleRejectedAndRetryPassed": True}
    offset, frame = read_object_frame(output, dwgread, line_handle, 19)
    line_wire = observe_single_line_frame(frame, line_handle, layer_handle)
    for bit in (line_wire["noLinksBit"], line_wire["previousHandleBit"] + 3):
        damaged = bytearray(frame)
        damaged[bit // 8] ^= 1 << (7 - bit % 8)
        try:
            observe_single_line_frame(bytes(damaged), line_handle, layer_handle)
        except ValueError:
            continue
        raise ValueError("LINE wire mutation unexpectedly accepted")
    return {
        "inputSha256": sha(initial.read_bytes()),
        "inputJsonSha256": sha(decoded.read_bytes()),
        "patchedJsonSha256": sha(patched.read_bytes()),
        "outputSha256": sha(output.read_bytes()),
        "headerDimstyleBefore": original_reference,
        "headerDimstyleAfter": normalized,
        "targetDimstyle": {"name": "Standard", "handle": dimstyle_handle},
        "styleElements": {"count": 2, "lines": standard_lines,
                          "endAngleRadians": standard[0]["end_angle"]},
        "styleFrames": style_wire,
        "modelLine": {"handle": line_handle, "originalNoLinks": original_no_links,
                      "frameOffset": offset, "wire": line_wire,
                      "noLinksAndRoleMutationsRejected": True},
        "normalization": (
            "Set the AC1012 HEADER DIMSTYLE H field to the unique Standard "
            "DIMSTYLE handle and write explicit typed-null previous/next links "
            "for the sole model-space LINE, and populate each MLINESTYLE with "
            "the two Standard elements, per ODA v5.4.1 sections 9 and 20.4."
        ),
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-dir", type=Path, required=True,
                        help="scratch parent; each run creates a new child directory")
    parser.add_argument("--dwgadd", default="dwgadd")
    parser.add_argument("--dwgread", default="dwgread")
    parser.add_argument("--dwgwrite", default="dwgwrite")
    parser.add_argument("--candidate", help="optional libdxfrw dwg2dxf executable")
    parser.add_argument("--dictionary-source-test",
                        help="optional libdxfrw AC1012 physical-frame test executable")
    parser.add_argument(
        "--unique-entry-control", action="store_true",
        help=("add DEST_ONLY to the destination and SOURCE_ONLY to the source, "
              "alongside the SAME_KEY collision"))
    args = parser.parse_args()
    require(b"0.14" in run([args.dwgadd, "--version"]).stdout,
            "LibreDWG 0.14 dwgadd is required")
    require(b"0.14" in run([args.dwgread, "--version"]).stdout,
            "LibreDWG 0.14 dwgread is required")
    require(b"0.14" in run([args.dwgwrite, "--version"]).stdout,
            "LibreDWG 0.14 dwgwrite is required")
    args.output_dir.mkdir(parents=True, exist_ok=True)
    root = Path(tempfile.mkdtemp(prefix="ac1012-dictionary-merge-", dir=args.output_dir))

    results = {
        "schema": "libdxfrw.research-receipt.v1",
        "id": ("p6-ac1012-collision-and-unique-entry-control-2026-11-24"
               if args.unique_entry_control else
               "p6-ac1012-populated-dictionary-merge-samples-2026-11-22"),
        "status": ("PAIRED_SPEC_BOUND_SOURCE_AND_PARTIAL_ASCII_BINARY_PROOF_PASS_NATIVE_SEMANTICS_OPEN"
                   if args.candidate else "PAIRED_SPEC_BOUND_SOURCE_NATIVE_SEMANTICS_OPEN"),
        "supportClaimEffect": "none",
        "authority": {
            "document": "ODA Open Design Specification v5.4.1",
            "sections": [
                "9 HEADER VARIABLES: DIMSTYLE is an H hard pointer",
                "20.4.1-20.4.2: entity link flag and common handle roles",
                "20.4.44 DICTIONARY",
                "20.4.73 MLINESTYLE",
            ],
            "source": "https://www.opendesign.com/files/guestdownloads/OpenDesign_Specification_for_.dwg_files.pdf",
            "layoutApplied": "AC1012 DICTIONARY has counted entry-name streams and soft-owner entry handles; R2000+ cloning and hard-owner fields are absent. Each MLINESTYLE is owned by the canonical ACAD_MLINESTYLE dictionary, the current DIMSTYLE header reference is a hard pointer, and the sole model LINE uses explicit null soft-pointer links.",
        },
        "producer": "GNU LibreDWG dwgadd/dwgread/dwgwrite 0.14; generated header DIMSTYLE and model LINE links normalized by a specification-directed JSON rewrite",
        "generatorSha256": sha(Path(__file__).read_bytes()),
        "executables": {
            "dwgadd": executable_identity(args.dwgadd),
            "dwgread": executable_identity(args.dwgread),
            "dwgwrite": executable_identity(args.dwgwrite),
        },
        "scratchDirectory": str(root),
        "samples": {},
        "limits": [
            "The paired files are locally generated and specification-directed controls, not AutoCAD-authored samples; native normal-open and duplicate-merge semantics remain unqualified.",
            "When tested, strict conversion still refuses unqualified opaque callbacks. Explicit partial ASCII/Binary output is source-profile evidence only and is not a completeness/support claim.",
            "The matching key and distinct MLINESTYLE start angles are discriminating sample state only; this does not establish native duplicate-merge behavior.",
            "No DWG or DXF fixture is added to the repository.",
        ],
    }

    for label, angle in (
        ("destination", 15),
        ("source", 37),
    ):
        unique_key = None
        unique_angle = None
        if args.unique_entry_control:
            unique_key = "DEST_ONLY" if label == "destination" else "SOURCE_ONLY"
            unique_angle = 22 if label == "destination" else 51
        recipe = root / f"{label}.dwgadd"
        write_recipe(recipe, angle, unique_key, unique_angle)
        initial = root / f"{label}-dwgadd-ac1012.dwg"
        run([args.dwgadd, "--as", "r13", "-o", str(initial), str(recipe)])
        path = root / f"{label}-ac1012.dwg"
        construction = materialize_r13_source(
            initial, path, args.dwgread, args.dwgwrite, root, label)
        model = read_model(path, args.dwgread, unique_key)
        require(abs(model["targetStyle"]["startAngle"] - math.radians(angle)) < 1e-10,
                "source MLINESTYLE start angle differs from the recipe")
        if unique_key is not None:
            require(abs(model["namedStyles"][unique_key]["startAngle"]
                        - math.radians(unique_angle)) < 1e-10,
                    "source non-conflicting MLINESTYLE differs from the recipe")
        frame, frame_bytes = read_dictionary_frame(
            path, args.dwgread, model["dictionaryHandle"],
            model["dictionaryEntries"])
        require(model["dictionaryOwner"] == model["rootHandle"],
                "ACAD_MLINESTYLE dictionary owner does not point to the NOD")
        require(frame["itemHandle"]["value"] == model["entry"]["targetHandle"],
                "physical dictionary item handle differs from the peer model")
        require(frame["parent"]["value"] == model["rootHandle"],
                "physical dictionary owner handle differs from the peer model")
        require(len(frame["reactors"]) == 1
                and frame["reactors"][0]["value"] == model["rootHandle"],
                "physical dictionary reactor does not resolve to the NOD")
        require(frame["xdictionary"]["value"] == 0,
                "unexpected extension dictionary on this bounded profile")
        require(model["targetStyle"]["name"] == ENTRY_KEY,
                "dictionary target does not identify the expected style")
        require(model["targetStyle"]["ownerHandle"] == model["dictionaryHandle"],
                "MLINESTYLE target owner does not point to ACAD_MLINESTYLE")
        require(frame["cloningField"] is None,
                "AC1012 DICTIONARY unexpectedly reports a cloning field")
        candidate = None
        candidate_strict_binary = None
        candidate_partial = None
        if args.candidate:
            candidate = run_candidate(args.candidate, path, root, label, False)
            candidate_strict_binary = run_candidate(
                args.candidate, path, root, label, False, binary=True)
            expected_output_styles = (
                {unique_key: float(unique_angle)}
                if unique_key is not None else None
            )
            candidate_partial = {
                "ascii": run_candidate(
                    args.candidate, path, root, label, True,
                    expected_angle=float(angle),
                    expected_styles=expected_output_styles, expected_line_count=2),
                "binary": run_candidate(
                    args.candidate, path, root, label, True, binary=True,
                    expected_angle=float(angle),
                    expected_styles=expected_output_styles, expected_line_count=2),
            }
        dictionary_frame_test = None
        if args.dictionary_source_test:
            test_path = Path(args.dictionary_source_test).resolve()
            frame_path = root / f"{label}-dictionary-frame.bin"
            frame_path.write_bytes(frame_bytes)
            result = subprocess.run(
                [str(test_path), "--source-dictionary-frame", str(frame_path),
                 str(model["dictionaryHandle"]), str(model["rootHandle"]),
                 str(model["entry"]["targetHandle"])],
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                timeout=30,
                check=False,
            )
            dictionary_frame_test = {
                "executable": executable_identity(str(test_path)),
                "exitCode": result.returncode,
                "output": result.stdout.decode(errors="replace").strip(),
            }
            require(result.returncode == 0,
                    "libdxfrw physical AC1012 dictionary frame test failed")
        results["samples"][label] = {
            "file": path.name,
            "recipe": recipe.name,
            "construction": construction,
            "source": model,
            "wire": frame,
            "candidateStrict": candidate,
            "candidateStrictBinary": candidate_strict_binary,
            "candidatePartial": candidate_partial,
            "libdxfrwPhysicalFrameTest": dictionary_frame_test,
        }

    require(results["samples"]["destination"]["source"]["entry"]["name"]
            == results["samples"]["source"]["source"]["entry"]["name"],
            "paired dictionaries do not share the duplicate key")
    require(results["samples"]["destination"]["source"]["targetStyle"]["startAngle"]
            != results["samples"]["source"]["source"]["targetStyle"]["startAngle"],
            "paired MLINESTYLE targets do not have distinct values")
    if args.unique_entry_control:
        require("DEST_ONLY" in results["samples"]["destination"]["source"]["dictionaryEntries"]
                and "SOURCE_ONLY" not in results["samples"]["destination"]["source"]["dictionaryEntries"]
                and "SOURCE_ONLY" in results["samples"]["source"]["source"]["dictionaryEntries"]
                and "DEST_ONLY" not in results["samples"]["source"]["source"]["dictionaryEntries"],
                "non-conflicting dictionary keys are not source/destination-distinct")
        results["discriminator"] = (
            "Both dictionaries have SAME_KEY with distinct 15/37-degree targets; "
            "the destination additionally has DEST_ONLY (22 degrees), and the "
            "source additionally has SOURCE_ONLY (51 degrees). A native merge "
            "can report both duplicate-key selection and retention of non-conflicting entries."
        )
    receipt = root / "receipt.json"
    receipt.write_text(json.dumps(results, indent=2) + "\n")
    print(root)
    print(json.dumps({
        label: {"sha256": item["source"]["sha256"],
                "entryTarget": item["source"]["targetStyle"]}
        for label, item in results["samples"].items()
    }, indent=2))


if __name__ == "__main__":
    main()
