#!/usr/bin/env python3
"""Independently verify physical TABLEGEOMETRY spans against typed and peer data."""

import argparse
import json
import math
import pathlib
import struct


class BitReader:
    """Small independent MSB-first reader for the DWG primitive types in this profile."""

    def __init__(self, data: bytes, position: int = 0):
        self.data = data
        self.position = position

    def bit(self) -> int:
        if self.position >= len(self.data) * 8:
            raise ValueError("truncated bit stream")
        value = (self.data[self.position // 8] >> (7 - self.position % 8)) & 1
        self.position += 1
        return value

    def bits(self, count: int) -> int:
        value = 0
        for _ in range(count):
            value = (value << 1) | self.bit()
        return value

    def raw8(self) -> int:
        return self.bits(8)

    def raw16(self) -> int:
        return self.raw8() | (self.raw8() << 8)

    def raw32(self) -> int:
        return self.raw16() | (self.raw16() << 16)

    def bit_short(self) -> int:
        selector = self.bits(2)
        if selector == 0:
            value = self.raw16()
            return value - (1 << 16) if value & (1 << 15) else value
        if selector == 1:
            return self.raw8()
        if selector == 2:
            return 0
        return 256

    def bit_long(self) -> int:
        selector = self.bits(2)
        if selector == 0:
            value = self.raw32()
            return value - (1 << 32) if value & (1 << 31) else value
        if selector == 1:
            return self.raw8()
        if selector == 2:
            return 0
        raise ValueError("invalid BL selector 3")

    def bit_double(self) -> float:
        selector = self.bits(2)
        if selector == 0:
            raw = bytes(self.raw8() for _ in range(8))
            return struct.unpack("<d", raw)[0]
        if selector == 1:
            return 1.0
        if selector == 2:
            return 0.0
        raise ValueError("invalid BD selector")

    def handle(self) -> tuple[int, int, int, int, int]:
        start = self.position
        lead = self.raw8()
        code = lead >> 4
        size = lead & 0x0F
        value = 0
        for _ in range(size):
            value = (value << 8) | self.raw8()
        return start, self.position, code, size, value


def scan_object(path: pathlib.Path, handle_bits: int) -> dict:
    data = path.read_bytes()
    reader = BitReader(data)
    fields = []

    start = reader.position
    type_selector = reader.bits(2)
    if type_selector == 0:
        object_type = reader.raw8()
    elif type_selector == 1:
        object_type = reader.raw8() + 0x1F0
    else:
        object_type = reader.raw16()
    fields.append(("objectType", start, reader.position, object_type))
    if object_type != 525:
        raise ValueError(f"expected TABLEGEOMETRY type 525, got {object_type}")

    own_handle = reader.handle()
    fields.append(("objectHandle", own_handle[0], own_handle[1], own_handle[4]))

    def read_field(name: str, decoder):
        begin = reader.position
        value = decoder()
        fields.append((name, begin, reader.position, value))
        return value

    eed_size = read_field("eedSize", reader.bit_short)
    if eed_size != 0:
        raise ValueError(f"profile expects no common EED, got {eed_size} bytes")
    reactors = read_field("reactorCount", reader.bit_long)
    xdict_missing = read_field("xdictMissing", reader.bit)
    has_data_storage = read_field("hasDataStorage", reader.bit)

    body_start = reader.position
    object_end = len(data) * 8 - handle_bits
    if body_start != 40 or object_end <= body_start:
        raise ValueError(f"unexpected body boundary: start={body_start}, end={object_end}")

    def body_field(name: str, decoder):
        begin = reader.position
        value = decoder()
        fields.append((name, begin, reader.position, value))
        if reader.position > object_end:
            raise ValueError(f"{name} crosses object data end")
        return value

    row_count = body_field("rows", reader.bit_long)
    column_count = body_field("columns", reader.bit_long)
    cell_count = body_field("cells", reader.bit_long)
    if row_count * column_count != cell_count or not 0 <= cell_count <= 10000:
        raise ValueError("invalid TABLEGEOMETRY dimensions")

    cells = []
    for cell_index in range(cell_count):
        prefix = f"cell[{cell_index}]"
        flags = body_field(f"{prefix}.flags", reader.bit_long)
        width = body_field(f"{prefix}.widthWithGap", reader.bit_double)
        height = body_field(f"{prefix}.heightWithGap", reader.bit_double)
        content_count = body_field(f"{prefix}.contentCount", reader.bit_long)
        if not 0 <= content_count <= 10000:
            raise ValueError(f"invalid content count in cell {cell_index}")

        contents = []
        for content_index in range(content_count):
            content_prefix = f"{prefix}.content[{content_index}]"
            points = []
            for point_name in ("topLeft", "center"):
                point = [
                    body_field(f"{content_prefix}.{point_name}.{axis}", reader.bit_double)
                    for axis in "xyz"
                ]
                points.append(point)
            scalars = [
                body_field(f"{content_prefix}.{name}", reader.bit_double)
                for name in (
                    "contentWidth",
                    "contentHeight",
                    "width",
                    "height",
                    "unknownBD",
                )
            ]
            contents.append(
                {"topLeft": points[0], "center": points[1], "scalars": scalars}
            )
        cells.append(
            {
                "flags": flags,
                "widthWithGap": width,
                "heightWithGap": height,
                "contents": contents,
            }
        )

    geometry_end = reader.position
    if object_end - geometry_end != 1:
        raise ValueError(
            f"expected one final string-presence bit, got {object_end - geometry_end} bits"
        )
    reader.position = geometry_end
    string_stream_present = reader.bit()
    if string_stream_present != 0:
        raise ValueError("sample unexpectedly advertises a string stream")

    reader.position = object_end
    handles = []

    def read_handle(role: str, index: int) -> dict:
        begin, end, code, size, raw_value = reader.handle()
        resolved = raw_value
        if code == 0x06:
            resolved = own_handle[4] + 1
        elif code == 0x08:
            resolved = own_handle[4] - 1
        elif code == 0x0A:
            resolved = own_handle[4] + raw_value
        elif code == 0x0C:
            resolved = own_handle[4] - raw_value
        result = {
            "role": role,
            "index": index,
            "begin": begin,
            "end": end,
            "code": code,
            "size": size,
            "raw": raw_value,
            "resolved": resolved,
        }
        handles.append(result)
        return result

    owner = read_handle("ownerSoftPointer", -1)
    for index in range(reactors):
        read_handle("reactorSoftPointer", index)
    if xdict_missing == 0:
        read_handle("extensionDictionaryHardOwner", -1)
    for index in range(cell_count):
        read_handle("cellSoftPointer", index)

    alignment_bits = [reader.bit() for _ in range(len(data) * 8 - reader.position)]
    if len(alignment_bits) >= 8:
        raise ValueError("handle alignment tail is not shorter than one byte")

    return {
        "data": data,
        "handle": own_handle[4],
        "type": object_type,
        "rows": row_count,
        "columns": column_count,
        "cellCount": cell_count,
        "objectBits": object_end,
        "handleBits": handle_bits,
        "bodyStart": body_start,
        "geometryEnd": geometry_end,
        "fields": fields,
        "cells": cells,
        "handles": handles,
        "owner": owner,
        "handleStart": object_end,
        "handleEnd": reader.position - len(alignment_bits),
        "alignmentBits": alignment_bits,
        "stringStreamPresent": string_stream_present,
    }


def near(left: float, right: float) -> bool:
    return math.isclose(left, right, rel_tol=1e-12, abs_tol=1e-12)


def find_fields(fields: list, name: str) -> list[tuple[int, int, object]]:
    return [(begin, end, value) for field_name, begin, end, value in fields if field_name == name]


def compare_object(
    version: str,
    handle: int,
    handle_bits: int,
    peer_path: pathlib.Path,
    raw_path: pathlib.Path,
    adapter_path: pathlib.Path,
    span_path: pathlib.Path,
) -> dict:
    peer_json = json.loads(peer_path.read_text())
    peer_objects = [
        value
        for records in peer_json.values()
        if isinstance(records, list)
        for value in records
        if isinstance(value, dict)
        and value.get("object") == "TABLEGEOMETRY"
        and value["handle"][-1] == handle
    ]
    if len(peer_objects) != 1:
        raise ValueError(f"expected one peer record for handle {handle:#x}")
    peer = peer_objects[0]
    scanned = scan_object(raw_path, handle_bits)

    # Parse only the relevant lines from the current libdxfrw capture probe.
    typed = {}
    for line in adapter_path.read_text().splitlines():
        parts = line.split()
        if not parts:
            continue
        if parts[0] == "TYPED":
            typed[int(parts[1])] = {
                "parent": int(parts[2]),
                "rows": int(parts[3]),
                "columns": int(parts[4]),
                "cellCount": int(parts[5]),
                "cells": {},
            }
        elif parts[0] == "CELL":
            typed[int(parts[1])]["cells"][int(parts[2])] = {
                "flags": int(parts[3]),
                "widthWithGap": float(parts[4]),
                "heightWithGap": float(parts[5]),
                "reference": int(parts[6]),
                "contents": [],
            }
        elif parts[0] == "CONTENT":
            typed[int(parts[1])]["cells"][int(parts[2])]["contents"].append(
                [float(value) for value in parts[4:]]
            )
    model = typed[handle]

    counts = (scanned["rows"], scanned["columns"], scanned["cellCount"])
    peer_counts = (peer["numrows"], peer["numcols"], len(peer["cells"]))
    typed_counts = (model["rows"], model["columns"], model["cellCount"])
    if counts != peer_counts or counts != typed_counts:
        raise ValueError(f"count mismatch for {handle:#x}: {counts}, {peer_counts}, {typed_counts}")
    if model["parent"] != scanned["owner"]["resolved"]:
        raise ValueError("typed and independently decoded owner handles differ")
    if model["parent"] != peer["ownerhandle"][-1]:
        raise ValueError("typed and peer owner handles differ")

    if find_fields(scanned["fields"], "reactorCount")[0][2] != 0:
        raise ValueError("this evidence profile expects zero reactors")
    if find_fields(scanned["fields"], "xdictMissing")[0][2] != 1:
        raise ValueError("this evidence profile expects no extension dictionary")

    for index, (decoded_cell, peer_cell) in enumerate(zip(scanned["cells"], peer["cells"])):
        typed_cell = model["cells"][index]
        if decoded_cell["flags"] != peer_cell["geom_data_flag"] or typed_cell["flags"] != peer_cell["geom_data_flag"]:
            raise ValueError(f"cell flags differ at {handle:#x}[{index}]")
        for key, peer_key in (("widthWithGap", "width_w_gap"), ("heightWithGap", "height_w_gap")):
            value = decoded_cell[key]
            if not near(value, peer_cell[peer_key]) or not near(typed_cell[key], peer_cell[peer_key]):
                raise ValueError(f"{key} differs at {handle:#x}[{index}]")
        if typed_cell["reference"] != peer_cell["tablegeometry"][-1] or typed_cell["reference"] != 0:
            raise ValueError(f"cell reference differs at {handle:#x}[{index}]")
        peer_contents = peer_cell.get("geometry", [])
        if len(decoded_cell["contents"]) != len(peer_contents) or len(typed_cell["contents"]) != len(peer_contents):
            raise ValueError(f"content count differs at {handle:#x}[{index}]")

        for decoded, typed_content, peer_content in zip(
            decoded_cell["contents"], typed_cell["contents"], peer_contents
        ):
            expected = (
                peer_content["dist_top_left"]
                + peer_content["dist_center"]
                + [
                    peer_content[key]
                    for key in ("content_width", "content_height", "width", "height", "unknown")
                ]
            )
            actual = decoded["topLeft"] + decoded["center"] + decoded["scalars"]
            if not all(near(left, right) for left, right in zip(actual, expected)):
                raise ValueError(f"peer geometry differs at {handle:#x}")
            if len(typed_content) != 12 or typed_content[-1] != 1:
                raise ValueError("typed content does not retain the DWG unknown-value marker")
            if not all(near(left, right) for left, right in zip(typed_content[:11], expected)):
                raise ValueError(f"typed geometry differs at {handle:#x}")

    # Compare every independent Python span with the separate C++ cursor walk.
    cpp_fields = {}
    cpp_handles = {}
    for line in span_path.read_text().splitlines():
        parts = line.split()
        if parts[0] == "FIELD":
            cpp_fields[parts[1]] = (int(parts[2]), int(parts[3]))
        elif parts[0] == "HANDLE":
            cpp_handles[(parts[1], int(parts[2]))] = tuple(map(int, parts[3:9]))

    def check_span(cpp_name: str, python_name: str) -> None:
        observed = find_fields(scanned["fields"], python_name)
        if len(observed) != 1 or cpp_fields.get(cpp_name) != observed[0][:2]:
            raise ValueError(f"span mismatch for {cpp_name}")

    check_span("objectType", "objectType")
    check_span("rows", "rows")
    check_span("columns", "columns")
    check_span("cellCount", "cells")
    if cpp_fields["rows"][0] != scanned["bodyStart"]:
        raise ValueError("unexpected body start")

    for index, cell in enumerate(scanned["cells"]):
        for suffix, cpp_suffix in (
            ("flags", "cellFlags"),
            ("widthWithGap", "cellWidthWithGap"),
            ("heightWithGap", "cellHeightWithGap"),
            ("contentCount", "contentCount"),
        ):
            check_span(f"{cpp_suffix}[{index}]", f"cell[{index}].{suffix}")
        for content_index, _ in enumerate(cell["contents"]):
            for field_name in ("topLeft", "center"):
                first = find_fields(
                    scanned["fields"],
                    f"cell[{index}].content[{content_index}].{field_name}.x",
                )[0][0]
                last = find_fields(
                    scanned["fields"],
                    f"cell[{index}].content[{content_index}].{field_name}.z",
                )[0][1]
                if cpp_fields[f"{field_name}[{index}][{content_index}]"] != (first, last):
                    raise ValueError(f"3BD span mismatch for {field_name} in cell {index}")
            for python_name, cpp_name in (
                ("contentWidth", "contentWidth"),
                ("contentHeight", "contentHeight"),
                ("width", "contentW"),
                ("height", "contentH"),
                ("unknownBD", "unknownBD"),
            ):
                name = f"cell[{index}].content[{content_index}].{python_name}"
                check_span(f"{cpp_name}[{index}][{content_index}]", name)

    raw_tail = bytes.fromhex(peer["unknown_bits"])
    if len(scanned["data"]) - 5 != len(raw_tail) or scanned["data"][5:] != raw_tail:
        raise ValueError("captured source bytes differ from LibreDWG unknown_bits")

    for handle in scanned["handles"]:
        key = (handle["role"], handle["index"])
        expected = (
            handle["begin"],
            handle["end"],
            handle["code"],
            handle["size"],
            handle["raw"],
            handle["resolved"],
        )
        if cpp_handles.get(key) != expected:
            raise ValueError(f"handle span or value mismatch for {key}")

    owner = scanned["owner"]
    if owner["code"] != 0x0A or owner["resolved"] != peer["ownerhandle"][-1]:
        raise ValueError("owner handle is not the evidenced relative soft pointer")
    cell_handles = [item for item in scanned["handles"] if item["role"] == "cellSoftPointer"]
    if len(cell_handles) != scanned["cellCount"]:
        raise ValueError("cell handle count mismatch")
    if not all(
        (item["code"], item["size"], item["raw"], item["resolved"], item["end"] - item["begin"])
        == (4, 0, 0, 0, 8)
        for item in cell_handles
    ):
        raise ValueError("cell reference is not an absolute null soft pointer")

    unknown_fields = [field for field in scanned["fields"] if field[0].endswith(".unknownBD")]
    expected_unknown_count = sum(len(cell["contents"]) for cell in scanned["cells"])
    if len(unknown_fields) != expected_unknown_count or not all(
        value == 0.0 and end - begin == 2
        for _, begin, end, value in unknown_fields
    ):
        raise ValueError("unknown BD values are not the observed zero selector profile")
    if scanned["geometryEnd"] != scanned["objectBits"] - 1:
        raise ValueError("geometry does not stop immediately before the final presence bit")
    if scanned["stringStreamPresent"] != 0:
        raise ValueError("string stream presence bit is not zero")
    if len(scanned["alignmentBits"]) >= 8:
        raise ValueError("invalid handle stream alignment length")

    return scanned, peer


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Independently walk TABLEGEOMETRY DWG source spans and compare typed/peer values."
    )
    parser.add_argument("--version", choices=("AC1027", "AC1032"), required=True)
    parser.add_argument("--peer-json", type=pathlib.Path, required=True)
    parser.add_argument("--adapter-log", type=pathlib.Path, required=True)
    parser.add_argument("--raw-prefix", required=True, help="Prefix before <handle>.bin")
    parser.add_argument("--span-prefix", required=True, help="Prefix before <handle>-spans.txt")
    parser.add_argument(
        "--record",
        action="append",
        required=True,
        metavar="HANDLE:HANDLE_BITS",
        help="Repeat once for each TABLEGEOMETRY source object",
    )
    args = parser.parse_args()

    for specification in args.record:
        handle_text, bits_text = specification.split(":", 1)
        handle = int(handle_text, 0)
        handle_bits = int(bits_text, 0)
        raw_path = pathlib.Path(f"{args.raw_prefix}{handle}.bin")
        span_path = pathlib.Path(f"{args.span_prefix}{handle}-spans.txt")
        scanned, peer = compare_object(
            args.version,
            handle,
            handle_bits,
            args.peer_json,
            raw_path,
            args.adapter_log,
            span_path,
        )
        contents = sum(len(cell["contents"]) for cell in scanned["cells"])
        decoded_values = 3 + 4 * scanned["cellCount"] + 11 * contents
        field_spans = 3 + 4 * scanned["cellCount"] + 7 * contents
        print(
            args.version,
            hex(handle),
            f"cells={scanned['cellCount']}",
            f"contents={contents}",
            f"decoded-values={decoded_values}",
            f"physical-field-spans={field_spans}",
            f"body={scanned['bodyStart']}..{scanned['geometryEnd']}",
            f"handle-stream={scanned['handleStart']}..{scanned['handleEnd']}",
            f"owner={peer['ownerhandle'][-1]:#x}",
            f"null-cell-soft-pointers={scanned['cellCount']}",
            f"string-stream-present={scanned['stringStreamPresent']}",
            f"alignment-bits={''.join(map(str, scanned['alignmentBits']))}",
            "PASS",
        )


if __name__ == "__main__":
    main()
