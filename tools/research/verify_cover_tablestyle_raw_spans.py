#!/usr/bin/env python3
"""Independently derive AC1027 Cover TABLESTYLE spans from its raw frame.

The cursor follows ODA's AC1027 object, R24 TABLESTYLE, cell-style,
content-format, CMC, string-stream, and handle-stream layouts. Parser spans are
only comparison targets; they are never used as cursor positions.
"""

import argparse
import hashlib
import json
import re
import struct
from pathlib import Path


SOURCE_SHA256 = "3a6c23151ebec25320722f0ecaaf20a10073c60079d3616181395f1c73deee42"
MAX_FRAME_BYTES = 1024 * 1024


def require(condition, message):
    if not condition:
        raise ValueError(message)


def sha256(path):
    digest = hashlib.sha256()
    with open(path, "rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


class Bits:
    def __init__(self, data):
        self.data = data
        self.total = len(data) * 8

    def read(self, position, count):
        require(position >= 0 and count >= 0
                and position + count <= self.total,
                "bit read escapes raw source frame")
        value = 0
        for bit in range(position, position + count):
            value = (value << 1) | (
                (self.data[bit // 8] >> (7 - bit % 8)) & 1)
        return value

    def bit_long(self, start):
        tag = self.read(start, 2)
        if tag == 0:
            return int.from_bytes(
                bytes(self.read(start + 2 + i * 8, 8) for i in range(4)),
                "little", signed=True), start + 34
        if tag == 1:
            return self.read(start + 2, 8), start + 10
        if tag == 2:
            return 0, start + 2
        raise ValueError("BL reserved selector encountered")

    def bit_short(self, start):
        tag = self.read(start, 2)
        if tag == 0:
            value = int.from_bytes(
                bytes(self.read(start + 2 + i * 8, 8) for i in range(2)),
                "little", signed=False)
            return value, start + 18
        if tag == 1:
            return self.read(start + 2, 8), start + 10
        if tag == 2:
            return 0, start + 2
        return 256, start + 2

    def bit_double(self, start):
        tag = self.read(start, 2)
        if tag == 1:
            return 1.0, start + 2
        if tag > 1:
            return 0.0, start + 2
        raw = bytes(self.read(start + 2 + index * 8, 8)
                    for index in range(8))
        return struct.unpack("<d", raw)[0], start + 66

    def variable_text(self, start):
        units, cursor = self.bit_short(start)
        require(units <= 0x7fff,
                f"TU length exceeds profile bound at bit {start}: {units}")
        require(cursor + units * 16 <= self.total,
                f"TU payload escapes frame at bit {start}: "
                f"units={units} cursor={cursor} total={self.total}")
        raw = bytes(self.read(cursor + index * 8, 8)
                    for index in range(units * 2))
        cursor += units * 16
        return raw.decode("utf-16le", errors="strict"), cursor

    def handle(self, start, owner=0, resolve_relative=False):
        code = self.read(start, 4)
        length = self.read(start + 4, 4)
        require(length <= 8, "handle payload exceeds 64-bit profile")
        raw = bytes(self.read(start + 8 + index * 8, 8)
                    for index in range(length))
        value = int.from_bytes(raw, "big") if raw else 0
        if resolve_relative and code == 0x0C:
            value = owner - value
        elif resolve_relative and code == 0x0A:
            value = owner + value
        elif resolve_relative and code == 0x08:
            value = owner - 1
        elif resolve_relative and code == 0x06:
            value = owner + 1
        return value, start + 8 + length * 8

    def raw_short16(self, start):
        bit_pos = start & 7
        byte_pos = start >> 3
        require(bit_pos != 0, "string footer RS unexpectedly byte aligned")
        current, high, low = self.data[byte_pos:byte_pos + 3]
        value = (((high << 8) | low) >> (8 - bit_pos))
        value |= current << (8 + bit_pos)
        value &= 0xFFFF
        return ((value << 8) & 0xFFFF) | (value >> 8)


def cm_color(bits, body, strings):
    index, body = bits.bit_short(body)
    rgb, body = bits.bit_long(body)
    flags = bits.read(body, 8)
    body += 8
    if flags & 1:
        _, strings = bits.variable_text(strings)
    if flags & 2:
        _, strings = bits.variable_text(strings)
    return body, strings, index, rgb, flags


def content_format(bits, body, strings, handles):
    start = body
    property_flags, body = bits.bit_long(body)
    format_flags, body = bits.bit_long(body)
    value_type, body = bits.bit_long(body)
    value_unit, body = bits.bit_long(body)
    value_format, strings = bits.variable_text(strings)
    rotation, body = bits.bit_double(body)
    block_scale, body = bits.bit_double(body)
    alignment, body = bits.bit_long(body)
    body, strings, content_color, content_rgb, content_color_flags = \
        cm_color(bits, body, strings)
    text_style_start = handles
    text_style, handles = bits.handle(handles, 106, True)
    text_height, body = bits.bit_double(body)
    return body, strings, handles, {
        "propertyFlags": property_flags,
        "formatFlags": format_flags,
        "valueType": value_type,
        "valueUnit": value_unit,
        "valueFormat": value_format,
        "rotation": rotation,
        "blockScale": block_scale,
        "alignment": alignment,
        "contentColor": content_color,
        "contentColorRgb": content_rgb,
        "contentColorFlags": content_color_flags,
        "textStyle": text_style,
        "textStyleStart": text_style_start,
        "textHeight": text_height,
        "start": start,
        "end": body,
        "stringEnd": strings,
    }


def cell_style(bits, body, strings, handles):
    start = body
    style_type, body = bits.bit_long(body)
    has_data, body = bits.bit_short(body)
    if not has_data:
        return body, strings, handles, {
            "start": start, "end": body, "type": style_type,
            "hasData": has_data, "textStyleHandleStarts": [],
            "borderHandleStarts": [],
        }
    property_flags, body = bits.bit_long(body)
    merge_flags, body = bits.bit_long(body)
    body, strings, background_color, background_rgb, background_flags = \
        cm_color(bits, body, strings)
    content_layout, body = bits.bit_long(body)
    body, strings, handles, content = content_format(
        bits, body, strings, handles)
    margin_flags, body = bits.bit_short(body)
    margins = []
    if margin_flags:
        for _ in range(6):
            value, body = bits.bit_double(body)
            margins.append(value)
    border_count, body = bits.bit_long(body)
    require(0 <= border_count <= 6, "invalid border count")
    text_handle_starts = [content["textStyleStart"]]
    border_handle_starts = []
    borders = []
    for _ in range(border_count):
        edge_flags, body = bits.bit_long(body)
        if edge_flags:
            border_flags, body = bits.bit_long(body)
            border_type, body = bits.bit_long(body)
            body, strings, border_color, border_rgb, border_color_flags = \
                cm_color(bits, body, strings)
            line_weight, body = bits.bit_long(body)
            border_handle_starts.append(handles)
            line_type_start = handles
            line_type, handles = bits.handle(handles, 106, True)
            visible, body = bits.bit_long(body)
            spacing, body = bits.bit_double(body)
            borders.append({
                "edgeFlags": edge_flags,
                "propertyFlags": border_flags,
                "type": border_type,
                "color": border_color,
                "colorRgb": border_rgb,
                "colorFlags": border_color_flags,
                "lineWeight": line_weight,
                "lineType": line_type,
                "lineTypeStart": line_type_start,
                "visible": visible,
                "spacing": spacing,
            })
    return body, strings, handles, {
        "start": start, "end": body, "type": style_type,
        "hasData": has_data, "propertyFlags": property_flags,
        "mergeFlags": merge_flags, "contentLayout": content_layout,
        "backgroundColor": background_color,
        "backgroundColorRgb": background_rgb,
        "backgroundColorFlags": background_flags,
        "content": content, "marginFlags": margin_flags,
        "margins": margins,
        "borders": borders,
        "textStyleHandleStarts": text_handle_starts,
        "borderHandleStarts": border_handle_starts,
        "stringEnd": strings,
    }


def span_rows(log):
    rows = []
    pattern = re.compile(
        r"^ typedBodySpan owner=(\d+) streamField=(\S+) startBit=(\d+) "
        r"bitSize=(\d+) valueOrIndex=(\d+) complete=(\d+)$", re.MULTILINE)
    for match in pattern.finditer(log):
        rows.append({
            "owner": int(match[1]), "field": match[2],
            "start": int(match[3]), "size": int(match[4]),
            "value": int(match[5]), "complete": match[6] == "1",
        })
    return rows


def derive_string_bounds(bits, object_end):
    require(object_end >= 17, "object is too short for string footer")
    has_strings = bits.read(object_end - 1, 1)
    require(has_strings == 1, "TABLESTYLE does not declare a string stream")
    low = bits.raw_short16(object_end - 17)
    size_end = object_end - 1
    encoded_size = low
    extended = (low & 0x8000) != 0
    if extended:
        require(object_end >= 33, "extended string footer is truncated")
        high = bits.raw_short16(object_end - 33)
        encoded_size = (low & 0x7fff) | (high << 15)
        size_end = object_end - 17
    require(encoded_size <= size_end - 16,
            "string stream size escapes raw frame")
    end = size_end - 16
    return end - encoded_size, end, encoded_size, extended


def summarize_style(style):
    content = style.get("content")
    summary = {
        "recordStartBit": style["start"],
        "recordEndBit": style["end"],
        "type": style["type"],
        "hasData": style["hasData"],
    }
    if not style["hasData"]:
        return summary
    summary.update({
        "propertyFlags": style["propertyFlags"],
        "mergeFlags": style["mergeFlags"],
        "backgroundColor": style["backgroundColor"],
        "backgroundColorRgb": style["backgroundColorRgb"],
        "backgroundColorFlags": style["backgroundColorFlags"],
        "contentLayout": style["contentLayout"],
        "content": {
            key: value for key, value in content.items()
            if key not in ("start", "end", "stringEnd", "textStyleStart")
        },
        "marginFlags": style["marginFlags"],
        "margins": style["margins"],
        "borders": [[
            border["edgeFlags"], border["propertyFlags"], border["type"],
            border["color"], border["colorRgb"], border["colorFlags"],
            border["lineWeight"], border["lineType"], border["visible"],
            border["spacing"],
        ] for border in style["borders"]],
    })
    return summary


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", required=True)
    parser.add_argument("--inventory-log", required=True)
    parser.add_argument("--peer-json", required=True)
    parser.add_argument("--inventory-binary", required=True)
    args = parser.parse_args()

    source_hash = sha256(args.source)
    require(source_hash == SOURCE_SHA256,
            f"source digest mismatch: {source_hash}")
    log = Path(args.inventory_log).read_text(encoding="utf-8")
    raw_match = re.search(
        r"^ typedBodyRawFrame owner=106 byteSize=(\d+) "
        r"handleStreamBits=(\d+) dataStreamBits=(\d+) "
        r"stringStartBit=(\d+) stringEndBit=(\d+) hex=([0-9a-f]+)$",
        log, re.MULTILINE)
    require(raw_match is not None, "raw TABLESTYLE frame not found")
    raw_size, handle_bits, data_bits, logged_string_start, logged_string_end = \
        map(int, raw_match.group(1, 2, 3, 4, 5))
    raw = bytes.fromhex(raw_match[6])
    require(0 < raw_size <= MAX_FRAME_BYTES and len(raw) == raw_size,
            "raw frame size does not match capture metadata")
    bits = Bits(raw)
    require(data_bits + handle_bits == bits.total,
            "data and handle stream sizes do not cover the captured frame")
    string_start, string_end, string_size, extended_strings = \
        derive_string_bounds(bits, data_bits)
    require((string_start, string_end)
            == (logged_string_start, logged_string_end),
            "independent string footer differs from diagnostic bounds")
    rows = span_rows(log)
    require(len(rows) >= 40 and all(row["owner"] == 106
                                    and row["complete"]
                                    and row["size"] > 0 for row in rows),
            "TABLESTYLE span inventory is incomplete")

    peer_doc = json.loads(Path(args.peer_json).read_text(encoding="utf-8"))
    peer_matches = [
        obj for obj in peer_doc["OBJECTS"]
        if isinstance(obj.get("handle"), list)
        and obj["handle"][-1] == 106
    ]
    require(len(peer_matches) == 1,
            "peer JSON does not contain one TABLESTYLE 0x6A frame")
    peer = peer_matches[0]
    peer_unknown = bytes.fromhex(peer["unknown_bits"])
    peer_unknown_bits = peer["num_unknown_bits"]
    require(peer["object"] == "TABLESTYLE"
            and peer["size"] == raw_size
            and peer_unknown_bits == bits.total - 40
            and peer_unknown[:-1] == raw[5:]
            and peer_unknown[-1] == 0,
            "LibreDWG unknown-bit carrier does not match the raw frame tail")
    peer_bits = Bits(peer_unknown[:-1])
    peer_offset = 40

    # Rebuild the AC1027 object prologue and R24 prefix from the raw bit
    # cursor. These offsets are derived here from the versioned framing and
    # field sequence; parser-reported ranges are only compared afterward.
    type_tag = bits.read(0, 2)
    require(type_tag == 1, "Cover does not use the expected AC1027 type tag")
    encoded_type = 0x1F0 + bits.read(2, 8)
    require(encoded_type == 504, "unexpected TABLESTYLE encoded type")
    own_handle, cursor = bits.handle(10, 0)
    eed_bytes, cursor = bits.bit_short(cursor)
    common_reactors, cursor = bits.bit_long(cursor)
    xdict_flag = bits.read(cursor, 1)
    has_data_storage = bits.read(cursor + 1, 1)
    cursor += 2
    require(own_handle == 106 and eed_bytes == 0
            and common_reactors == 1 and xdict_flag == 1
            and has_data_storage == 0 and cursor == peer_offset,
            "raw AC1027 object prologue did not close at peer tail offset")
    class_version = bits.read(cursor, 8)
    cursor += 8
    prefix_name_start = string_start
    prefix_name, prefix_name_end = bits.variable_text(prefix_name_start)
    string_cursor = prefix_name_end
    r24_unknown1, cursor = bits.bit_long(cursor)
    r24_unknown2, cursor = bits.bit_long(cursor)
    table_cell_start = cursor
    table_cell_type, cursor = bits.bit_long(cursor)
    table_cell_data_flags, cursor = bits.bit_short(cursor)
    require(table_cell_start == 68 and class_version == 0
            and r24_unknown1 == 8 and r24_unknown2 == 101
            and table_cell_type == 5 and table_cell_data_flags == 1,
            "ODA R24 prefix/Table cell fields disagree with raw frame")

    # Independently walk each logical stream using the ODA R24 grammar. The
    # data cursor advances through schema fields, the string cursor through TU
    # and optional CMC strings, and the handle cursor through common and typed
    # H fields. Parser ranges below are comparison targets only.
    data_cursor = table_cell_start
    # The R24 prefix stores the table entry's name in the detached string
    # stream before the cell-style strings.
    handle_cursor = data_bits
    common_owner_start = handle_cursor
    common_owner, handle_cursor = bits.handle(handle_cursor, 106, True)
    common_reactor_values = []
    for _ in range(common_reactors):
        value, handle_cursor = bits.handle(handle_cursor, 106, True)
        common_reactor_values.append(value)
    if xdict_flag != 1:
        common_xdict, handle_cursor = bits.handle(handle_cursor, 106, True)
    else:
        common_xdict = 0
    r24_owner_start = handle_cursor
    r24_owner, handle_cursor = bits.handle(handle_cursor, 106, True)
    require(common_owner == 105 and common_reactor_values == [105]
            and common_xdict == 0 and r24_owner == 0
            and prefix_name == "Standard",
            "raw TABLESTYLE prefix/common handles disagree with source profile")

    table_record_start = data_cursor
    data_cursor, string_cursor, handle_cursor, table_cell = cell_style(
        bits, data_cursor, string_cursor, handle_cursor)
    table_id_start = data_cursor
    table_id, data_cursor = bits.bit_long(data_cursor)
    table_id_end = data_cursor
    table_class_start = data_cursor
    table_class, data_cursor = bits.bit_long(data_cursor)
    table_class_end = data_cursor
    table_name_start = string_cursor
    table_name, string_cursor = bits.variable_text(string_cursor)
    table_name_end = string_cursor
    table_record_end = data_cursor
    repeated_count_start = data_cursor
    repeated_count, data_cursor = bits.bit_long(data_cursor)
    require(repeated_count == 3,
            f"expected three repeated R24 styles, found {repeated_count}")

    repeated_styles = []
    for _ in range(repeated_count):
        record_start = data_cursor
        index_start = data_cursor
        style_index, data_cursor = bits.bit_long(data_cursor)
        index_end = data_cursor
        data_cursor, string_cursor, handle_cursor, style = cell_style(
            bits, data_cursor, string_cursor, handle_cursor)
        id_start = data_cursor
        style_id, data_cursor = bits.bit_long(data_cursor)
        id_end = data_cursor
        class_start = data_cursor
        style_class, data_cursor = bits.bit_long(data_cursor)
        class_end = data_cursor
        name_start = string_cursor
        style_name, string_cursor = bits.variable_text(string_cursor)
        name_end = string_cursor
        repeated_styles.append({
            "recordStart": record_start,
            "recordEnd": data_cursor,
            "indexStart": index_start,
            "indexEnd": index_end,
            "index": style_index,
            "style": style,
            "idStart": id_start,
            "idEnd": id_end,
            "id": style_id,
            "classStart": class_start,
            "classEnd": class_end,
            "class": style_class,
            "nameStart": name_start,
            "nameEnd": name_end,
            "name": style_name,
        })

    require(data_cursor == string_start,
            f"ODA body cursor ended at {data_cursor}, string stream starts at "
            f"{string_start}")
    require(string_cursor == string_end,
            f"ODA string cursor ended at {string_cursor}, stream ends at "
            f"{string_end}")
    handle_tail_bits = bits.total - handle_cursor
    require(0 <= handle_tail_bits <= 7
            and bits.read(handle_cursor, handle_tail_bits) == 0,
            "ODA handle cursor did not close at zero padding sentinel")

    def named(field):
        return sorted((row for row in rows if row["field"] == field),
                      key=lambda row: row["start"])

    table_ids = named("table-style-body-table-cell-id")
    table_classes = named("table-style-body-table-cell-class")
    repeated_ids = named("table-style-body-cell-id")
    repeated_classes = named("table-style-body-cell-class")
    table_names = named("table-style-string-table-cell-name")
    repeated_names = named("table-style-string-cell-name")
    require(len(table_ids) == len(table_classes) == len(table_names) == 1
            and len(repeated_ids) == len(repeated_classes)
            == len(repeated_names) == 3,
            "expected one Table and three repeated identities")
    table_record_rows = named("table-style-body-table-cell-record")

    def assert_span(field, start, end):
        matches = named(field)
        require(len(matches) == 1 and matches[0]["start"] == start
                and matches[0]["start"] + matches[0]["size"] == end,
                f"independent cursor disagrees with parser span {field}: "
                f"expected [{start}, {end})")

    def assert_spans(field, expected):
        actual = named(field)
        require(len(actual) == len(expected),
                f"independent cursor count disagrees for {field}")
        for row, (start, end) in zip(actual, expected):
            require(row["start"] == start
                    and row["start"] + row["size"] == end,
                    f"independent cursor disagrees with {field} span: "
                    f"expected [{start}, {end})")

    assert_span("table-style-body-table-cell-record", table_record_start,
                table_record_end)
    assert_spans("table-style-cell-style", [
        (table_cell["start"], table_cell["end"]),
        *[(item["style"]["start"], item["style"]["end"])
          for item in repeated_styles],
    ])
    assert_spans("table-style-content-format", [
        (table_cell["content"]["start"], table_cell["content"]["end"]),
        *[(item["style"]["content"]["start"],
           item["style"]["content"]["end"])
          for item in repeated_styles],
    ])
    assert_span("table-style-body-table-cell-id", table_id_start,
                table_id_end)
    assert_span("table-style-body-table-cell-class", table_class_start,
                table_class_end)
    assert_spans("table-style-body-cell-id", [
        (item["idStart"], item["idEnd"]) for item in repeated_styles])
    assert_spans("table-style-body-cell-class", [
        (item["classStart"], item["classEnd"])
        for item in repeated_styles])
    assert_spans("table-style-string-table-cell-name", [
        (table_name_start, table_name_end)])
    assert_spans("table-style-string-cell-name", [
        (item["nameStart"], item["nameEnd"])
        for item in repeated_styles])
    assert_spans("table-style-body-cell-index", [
        (item["indexStart"], item["indexEnd"])
        for item in repeated_styles])
    assert_spans("table-style-body-repeated-cell-record", [
        (item["recordStart"], item["recordEnd"])
        for item in repeated_styles])

    typed_handle_rows = [row for row in rows
                         if row["field"].startswith("table-style-handle-")]
    typed_handle_rows.sort(key=lambda row: row["start"])
    class_specific_starts = [table_cell["textStyleHandleStarts"][0]]
    class_specific_starts.extend(table_cell["borderHandleStarts"])
    for item in repeated_styles:
        class_specific_starts.extend(item["style"]["textStyleHandleStarts"])
        class_specific_starts.extend(item["style"]["borderHandleStarts"])
    require(len(typed_handle_rows) == len(class_specific_starts),
            "independent cursor handle count differs from parser inventory")
    for row, start in zip(typed_handle_rows,
                          sorted(class_specific_starts)):
        require(row["start"] == start,
                "independent handle cursor disagrees with parser span")

    decoded_ids = []
    for row in table_ids + repeated_ids:
        value, end = bits.bit_long(row["start"])
        peer_value, peer_end = peer_bits.bit_long(row["start"] - peer_offset)
        require(end == row["start"] + row["size"],
                "BL token length differs from reported span")
        require(peer_value == value and peer_end == end - peer_offset,
                "peer unknown-bit tail disagrees at a parser-reported BL span")
        decoded_ids.append(value)
    decoded_classes = []
    for row in table_classes + repeated_classes:
        value, end = bits.bit_long(row["start"])
        peer_value, peer_end = peer_bits.bit_long(row["start"] - peer_offset)
        require(end == row["start"] + row["size"],
                "BL token length differs from reported span")
        require(peer_value == value and peer_end == end - peer_offset,
                "peer unknown-bit tail disagrees at a parser-reported BL span")
        decoded_classes.append(value)
    decoded_names = []
    for row in table_names + repeated_names:
        value, end = bits.variable_text(row["start"])
        peer_value, peer_end = peer_bits.variable_text(
            row["start"] - peer_offset)
        require(end == row["start"] + row["size"],
                "TU token length differs from reported span")
        require(peer_value == value and peer_end == end - peer_offset,
                "peer unknown-bit tail disagrees at a parser-reported TU span")
        decoded_names.append(value)

    require(decoded_ids == [4, 1, 2, 3],
            f"raw IDs disagree with parser model: {decoded_ids}")
    require(decoded_classes == [2, 1, 1, 2],
            f"raw classes disagree with parser model: {decoded_classes}")
    require(decoded_names == ["Table", "_TITLE", "_HEADER", "_DATA"],
            f"raw names disagree with parser model: {decoded_names}")

    handle_spans = sorted(
        (row for row in rows if row["field"].startswith("table-style-handle-")),
        key=lambda row: row["start"])
    require(len(handle_spans) == 28,
            f"expected 28 handle spans; found {len(handle_spans)}")
    decoded_handles = []
    decoded_text_styles = []
    decoded_border_ltypes = []
    for row in handle_spans:
        require(row["start"] >= data_bits
                and row["start"] + row["size"] <= bits.total,
                "handle span is outside the captured handle stream")
        value, end = bits.handle(row["start"], 106)
        peer_value, peer_end = peer_bits.handle(
            row["start"] - peer_offset, 106)
        require(end == row["start"] + row["size"],
                "H token length differs from reported span")
        require(peer_value == value and peer_end == end - peer_offset,
                "peer unknown-bit tail disagrees at a parser-reported H span")
        decoded_handles.append(value)
        if row["field"] == "table-style-handle-text-style":
            decoded_text_styles.append(value)
        else:
            decoded_border_ltypes.append(value)
    require(decoded_text_styles == [0, 17, 17, 17],
            f"text-style raw handles disagree: {decoded_text_styles}")
    require(decoded_border_ltypes == [0] * 6 + [20] * 18,
            "border-linetype raw handles disagree with parsed source model")

    derived_style_rows = [table_cell] + [item["style"]
                                         for item in repeated_styles]
    report = {
        "schema": "libdxfrw-cover-tablestyle-raw-span-check-v1",
        "sourceSha256": source_hash,
        "inventoryLogSha256": sha256(args.inventory_log),
        "peerJsonSha256": sha256(args.peer_json),
        "inventoryBinarySha256": sha256(args.inventory_binary),
        "verifierSha256": sha256(__file__),
        "rawFrameSha256": hashlib.sha256(raw).hexdigest(),
        "tableStyleHandle": 106,
        "independentlyDerived": {
            "encodedType": encoded_type,
            "commonPrologueEndBit": peer_offset,
            "commonReactorCount": common_reactors,
            "commonOwner": common_owner,
            "commonOwnerStartBit": common_owner_start,
            "commonReactorHandles": common_reactor_values,
            "commonXdictHandle": common_xdict,
            "r24PrefixClassVersion": class_version,
            "r24PrefixName": prefix_name,
            "r24UnknownValues": [r24_unknown1, r24_unknown2],
            "r24OwnerHandle": r24_owner,
            "r24OwnerHandleStartBit": r24_owner_start,
            "tableCellRecordStartBit": table_cell_start,
            "tableCellType": table_cell_type,
            "tableCellDataFlags": table_cell_data_flags,
            "tableCellRecordEndBit": table_record_end,
            "tableCellId": table_id,
            "tableCellClass": table_class,
            "tableCellName": table_name,
            "repeatedCountStartBit": repeated_count_start,
            "repeatedCount": repeated_count,
            "repeatedStyles": [{
                "recordStartBit": item["recordStart"],
                "recordEndBit": item["recordEnd"],
                "index": item["index"],
                "style": summarize_style(item["style"]),
                "id": item["id"],
                "class": item["class"],
                "name": item["name"],
            } for item in repeated_styles],
            "tableCellStyle": summarize_style(table_cell),
        },
        "rawFrameBytes": raw_size,
        "dataStreamBits": data_bits,
        "stringStreamStartBit": string_start,
        "stringStreamEndBit": string_end,
        "stringStreamSizeBits": string_size,
        "stringStreamExtendedSize": extended_strings,
        "handleStreamBits": handle_bits,
        "handleStreamStartBit": data_bits,
        "handleStreamStartBasis": (
            "The captured frame metadata supplies handleStreamBits; the "
            "verifier independently walks H fields and validates the terminal "
            "padding but does not independently recover the stream partition "
            "from compressed DWG pages."
        ),
        "independentBodyCursorEndBit": data_cursor,
        "independentStringCursorEndBit": string_cursor,
        "independentHandleCursorEndBit": handle_cursor,
        "handleTailZeroPaddingBits": handle_tail_bits,
        "independentTableCellStyleCount": len(derived_style_rows),
        "parserSpanRangesMatched": True,
        "decodedIds": decoded_ids,
        "decodedClasses": decoded_classes,
        "decodedNames": decoded_names,
        "decodedTextStyleHandles": decoded_text_styles,
        "decodedBorderLinetypeHandles": decoded_border_ltypes,
        "decodedHandleReferences": len(decoded_handles),
        "rawByteSpanChecks": len(decoded_ids) + len(decoded_classes)
            + len(decoded_names) + len(decoded_handles),
        "peerUnknownBits": peer_unknown_bits,
        "peerUnknownTailOffset": peer_offset,
        "peerUnknownTailMatchesRawFrame": True,
        "result": (
            "PASS independent AC1027 object, common-link, R24 prefix, body, "
            "string, and handle cursors derive the complete TABLESTYLE frame; "
            "all modeled parser field spans and peer raw bytes match"
        ),
        "limit": (
            "This is one unauthenticated AC1027 Cover source profile. It proves "
            "decompressed-frame field boundaries and byte identity only; it "
            "does not prove compressed physical-file offsets, normative "
            "repeated-order/class semantics, a second producer, dependency "
            "closure, DXF output eligibility, or native consumer meaning. "
            "ODA's repeated-order/class-meaning discrepancy remains open."
        ),
    }
    print(json.dumps(report, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
