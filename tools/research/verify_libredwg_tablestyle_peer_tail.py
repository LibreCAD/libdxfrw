#!/usr/bin/env python3
"""Walk an R2018 TABLESTYLE from LibreDWG's retained raw unknown-bit tail.

This control uses the ODA R2018 fixture and the LibreDWG 0.14 minJSON raw
tail, not libdxfrw parser spans. Five synthetic leading bytes preserve the
absolute bit positions after LibreDWG's 40-bit common-object prefix; those
bytes are never treated as source evidence.
"""

import argparse
import hashlib
import importlib.util
import json
import subprocess
from datetime import date
from pathlib import Path


DWG_SHA256 = "1b719034b3d65a66830db64ae65662f0c9249f8f05ce906d212f7ffd4b34473c"
DXF_SHA256 = "442520445458a4d2d5ad71c17ed68ccf447459922ea1213de61c9601cc5eae51"
TABLESTYLE_HANDLE = 107


def sha256(path):
    digest = hashlib.sha256()
    with open(path, "rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def require(condition, message):
    if not condition:
        raise ValueError(message)


def load_cursor_helpers():
    path = Path(__file__).with_name("verify_cover_tablestyle_raw_spans.py")
    spec = importlib.util.spec_from_file_location("libdxfrw_raw_cursor", path)
    require(spec is not None and spec.loader is not None,
            "cannot load shared raw DWG bit cursor")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module, path


def find_source_dxf_styles(path):
    lines = Path(path).read_text(encoding="utf-8").splitlines()
    require(len(lines) % 2 == 0, "DXF source has an incomplete group pair")
    pairs = [(int(lines[index].strip()), lines[index + 1].strip())
             for index in range(0, len(lines), 2)]
    matches = []
    index = 0
    while index < len(pairs):
        if pairs[index][0] != 0:
            index += 1
            continue
        end = index + 1
        while end < len(pairs) and pairs[end][0] != 0:
            end += 1
        if pairs[index] == (0, "CELLSTYLEMAP"):
            entity = pairs[index + 1:end]
            cell = None
            for code, value in entity:
                if code == 1 and value == "CELLSTYLE_BEGIN":
                    require(cell is None, "nested DXF CELLSTYLE_BEGIN")
                    cell = {}
                elif cell is not None:
                    if code == 309 and value == "CELLSTYLE_END":
                        require(all(field in cell for field in (90, 91, 300)),
                                "DXF CELLSTYLE lacks ID/class/name")
                        matches.append({
                            "id": int(cell[90]),
                            "class": int(cell[91]),
                            "name": cell[300],
                        })
                        cell = None
                    elif code in (90, 91, 300):
                        cell[code] = value
            require(cell is None, "unterminated DXF CELLSTYLE")
            return matches
        index = end
    raise ValueError("source DXF TABLESTYLE handle 0x6B was not found")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--dwg", required=True)
    parser.add_argument("--source-dxf", required=True)
    parser.add_argument("--minjson", required=True)
    parser.add_argument("--dwgread-bin", required=True)
    parser.add_argument("--fixture-repo-revision", required=True)
    args = parser.parse_args()

    dwg_hash = sha256(args.dwg)
    dxf_hash = sha256(args.source_dxf)
    require(dwg_hash == DWG_SHA256,
            f"DWG digest mismatch: {dwg_hash}")
    require(dxf_hash == DXF_SHA256,
            f"source DXF digest mismatch: {dxf_hash}")
    version_run = subprocess.run(
        [args.dwgread_bin, "--version"], check=True,
        stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    libredwg_version = version_run.stdout.strip()
    require(libredwg_version == "dwgread 0.14",
            f"unexpected LibreDWG version: {libredwg_version}")
    dwgread_hash = sha256(args.dwgread_bin)

    cursor, cursor_path = load_cursor_helpers()
    minjson = json.loads(Path(args.minjson).read_text(encoding="utf-8"))
    peer_rows = [obj for obj in minjson["OBJECTS"]
                 if obj.get("object") == "TABLESTYLE"
                 and isinstance(obj.get("handle"), list)
                 and obj["handle"][-1] == TABLESTYLE_HANDLE]
    require(len(peer_rows) == 1, "expected one peer TABLESTYLE handle 0x6B")
    peer = peer_rows[0]
    tail = bytes.fromhex(peer["unknown_bits"])
    source_frame_bits = peer["size"] * 8
    require(peer["type"] == 505 and peer["size"] == 926
            and peer["num_unknown_bits"] == source_frame_bits - 40
            and len(tail) * 8 - peer["num_unknown_bits"] == 8
            and tail[-1] == 0,
            "LibreDWG raw tail size/padding does not match its frame metadata")

    # LibreDWG's carrier starts at source bit 40. Its final byte is alignment
    # padding; prepend a coordinate-only prefix and retain the full tail data.
    raw = bytes(5) + tail[:-1]
    bits = cursor.Bits(raw)
    data_bits = peer["bitsize"]
    require(bits.total == source_frame_bits and 40 + len(tail[:-1]) * 8
            == bits.total,
            "coordinate reconstruction does not match the peer frame size")
    string_start, string_end, string_size, extended = \
        cursor.derive_string_bounds(bits, data_bits)

    body = 40
    class_version = bits.read(body, 8)
    body += 8
    prefix_name, strings = bits.variable_text(string_start)
    unknown1, body = bits.bit_long(body)
    unknown2, body = bits.bit_long(body)
    table_record_start = body

    handle = data_bits
    object_handle = peer["handle"][-1]
    common_owner, handle = bits.handle(handle, object_handle, True)
    reactors = []
    for _ in peer["reactors"]:
        value, handle = bits.handle(handle, object_handle, True)
        reactors.append(value)
    if peer["is_xdic_missing"] != 1:
        xdict, handle = bits.handle(handle, object_handle, True)
    else:
        xdict = 0
    r24_owner, handle = bits.handle(handle, object_handle, True)
    require(prefix_name == peer["name"] == "Standard"
            and class_version == peer["unknown_rc"] == 0
            and [unknown1, unknown2]
                == [peer["unknown_bl1"], peer["unknown_bl2"]]
            and common_owner == peer["ownerhandle"][-1]
            and reactors == [item[-1] for item in peer["reactors"]]
            and xdict == 0 and r24_owner == 0,
            "raw prefix/common handles disagree with LibreDWG's typed fields")

    cell_type, probe = bits.bit_long(body)
    data_flags, probe = bits.bit_short(probe)
    require(cell_type == peer["sty.cellstyle.type"] == 5
            and data_flags == peer["sty.cellstyle.data_flags"] == 1,
            "raw Table cell prologue disagrees with LibreDWG fields")
    body, strings, handle, table_style = cursor.cell_style(
        bits, body, strings, handle)
    table_id, body = bits.bit_long(body)
    table_class, body = bits.bit_long(body)
    table_record_end = body
    table_name, strings = bits.variable_text(strings)
    count, body = bits.bit_long(body)

    require((table_id, table_class, table_name, count)
            == (peer["sty.id"], peer["sty.type"], peer["sty.name"],
                peer["numoverrides"])
            == (4, 2, "Table", 3),
            "raw Table identity/count disagrees with peer typed fields")

    repeated = []
    for _ in range(count):
        record_start = body
        style_index, body = bits.bit_long(body)
        body, strings, handle, style = cursor.cell_style(
            bits, body, strings, handle)
        style_id, body = bits.bit_long(body)
        style_class, body = bits.bit_long(body)
        record_end = body
        name, strings = bits.variable_text(strings)
        repeated.append({
            "index": style_index,
            "recordStartBit": record_start,
            "recordEndBit": record_end,
            "id": style_id,
            "class": style_class,
            "name": name,
            "textStyleHandle": style["content"]["textStyle"],
            "borderLineTypeHandles": [border["lineType"]
                                       for border in style["borders"]],
        })

    require(body == string_start and strings == string_end,
            "R24 peer-tail body/string cursors did not close at sentinels")
    handle_padding_bits = bits.total - handle
    require(0 <= handle_padding_bits <= 7
            and bits.read(handle, handle_padding_bits) == 0,
            "peer-tail handle cursor did not close at zero padding")
    source_styles = find_source_dxf_styles(args.source_dxf)
    derived_styles = [{"id": item["id"], "class": item["class"],
                       "name": item["name"]} for item in repeated]
    require(repeated[0]["id"] == peer["ovr.id"]
            and repeated[0]["class"] == peer["ovr.type"]
            and repeated[0]["name"] == peer["ovr.name"],
            "first raw named style disagrees with LibreDWG typed fields")

    report = {
        "schema": "libdxfrw-research-receipt-v1",
        "title": "AC1032 ODA TABLESTYLE peer-tail raw cursor",
        "date": date.today().isoformat(),
        "supportClaimEffect": "none",
        "source": {
            "dwg": {"path": str(Path(args.dwg)), "sha256": dwg_hash,
                    "version": "AC1032"},
            "sourceDxf": {"path": str(Path(args.source_dxf)),
                          "sha256": dxf_hash,
                          "parsedCellStyles": source_styles},
            "fixtureProvenance": (
                "The tracked dwg-parser PROVENANCE.md identifies the DWG as "
                "an ODA R2018 export of an ACAD_TABLE scaffold from the "
                "MIT-licensed ezdxf fixture plus locally authored CJK cell "
                "text. The tracked source DXF retains the TABLESTYLE cell "
                "records; exact generation-input identity is not claimed."
            ),
        },
        "independentPeer": {
            "tool": libredwg_version,
            "binaryPath": str(Path(args.dwgread_bin)),
            "binarySha256": dwgread_hash,
            "minJsonSha256": sha256(args.minjson),
            "fixtureRepositoryRevision": args.fixture_repo_revision,
            "objectHandle": peer["handle"][-1],
            "frameBytes": peer["size"],
            "tailBits": peer["num_unknown_bits"],
            "tailStartBit": 40,
        },
        "cursor": {
            "helperSha256": sha256(cursor_path),
            "dataStreamEndBit": data_bits,
            "stringStartBit": string_start,
            "stringEndBit": string_end,
            "stringSizeBits": string_size,
            "extendedStringSize": extended,
            "tableRecordStartBit": table_record_start,
            "tableRecordEndBit": table_record_end,
            "tableId": table_id,
            "tableClass": table_class,
            "tableName": table_name,
            "repeatedCount": count,
            "repeatedStyles": repeated,
            "bodyEndBit": body,
            "stringEndCursorBit": strings,
            "handleStreamStartBit": data_bits,
            "handleFieldsEndBit": handle,
            "handlePaddingBits": handle_padding_bits,
            "allStreamSentinelsMatch": True,
        },
        "comparison": {
            "peerRecognizedTableCellAndFirstNamedStyleMatch": True,
            "sourceDxfCellStyles": source_styles,
            "sourceDxfStylesMatchDecodedRepeatedStyles":
                source_styles[-3:] == derived_styles,
            "independentlyDecodedOrder": [item["name"]
                                           for item in repeated],
            "independentlyDecodedIds": [item["id"] for item in repeated],
            "independentlyDecodedClasses": [item["class"]
                                             for item in repeated],
        },
        "interpretation": {
            "confirms": (
                "The ODA R2018 AC1032 file's LibreDWG unknown-bit carrier "
                "independently decodes to title/header/data order, IDs 1/2/3 "
                "and classes 1/1/2. Its TABLESTYLE extension repeats the "
                "tracked source DXF cell-style values. This is a second "
                "cross-version ODA-produced physical witness and not a "
                "libdxfrw parser-span input."
            ),
            "doesNotConfirm": (
                "Both physical witnesses are ODA-produced; this does not "
                "settle the conflict with ODA v5.4.1's stated "
                "data/title/header order and class 1=data/class 2=label, "
                "establish native application meaning, qualify source "
                "producer breadth, or promote support. The output cursor "
                "uses LibreDWG's retained unknown_bits payload plus frame "
                "metadata, not an independently recovered compressed-page "
                "frame."
            ),
        },
    }
    print(json.dumps(report, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
