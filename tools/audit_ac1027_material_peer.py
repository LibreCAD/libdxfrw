#!/usr/bin/env python3
"""Compare hash-bound AC1027 MATERIAL source/writer frames via LibreDWG 0.14."""

import argparse
import hashlib
import json
import re
import tempfile
from pathlib import Path

from bounded_process import run_bounded


NAMES = ("ByBlock", "ByLayer", "Global")
IDENTITY_FIELDS = {"index", "bitsize", "handle", "ownerhandle", "reactors", "size", "type"}


def require(value, message):
    if not value:
        raise ValueError(message)


def digest(path):
    require(path.is_file() and path.stat().st_size <= 64 * 1024 * 1024, "input absent or over budget")
    return hashlib.sha256(path.read_bytes()).hexdigest()


def decode(reader, path, target):
    result = run_bounded([str(reader), "-v9", "-O", "JSON", "-o", str(target), str(path)],
                         30, 8 * 1024 * 1024)
    require(result.returncode == 0 and target.is_file()
            and target.stat().st_size <= 128 * 1024 * 1024, "LibreDWG decode failed or exceeded budget")
    document = json.loads(target.read_text())
    trace = (result.stdout + result.stderr).decode("utf-8", errors="replace")
    parts = re.split(r"(?m)^Add object MATERIAL \[\d+\] Decode object MATERIAL\s*$", trace)
    records = {}
    for part in parts[1:]:
        name = re.search(r'(?m)^name: "([^"]+)" \[TU 1\]', part)
        boundary = re.search(r"(?m)^ bitsize: (\d+), hdl_dat:", part)
        strings = re.search(r"(?m)^\s*@\S+ data_size: (\d+)/0x[0-9a-f]+ \[RS\]", part)
        common = re.search(r"(?m)^--common_size: (\d+)", part)
        require(name is not None and boundary is not None and strings is not None and common is not None,
                "incomplete material trace")
        require(name.group(1) not in records, "duplicate material trace")
        records[name.group(1)] = tuple(int(match.group(1)) for match in (boundary, strings, common))
    require(set(records) == set(NAMES), "unexpected material trace inventory")
    return document, records


def material_graph(document):
    require(document["FILEHEADER"]["version"] == "AC1027", "unexpected DWG revision")
    classes = [row for row in document["CLASSES"] if row.get("dxfname") == "MATERIAL"]
    require(len(classes) == 1 and classes[0].get("cppname") == "AcDbMaterial"
            and classes[0].get("appname") == "ObjectDBX Classes"
            and classes[0].get("proxyflag") == 0x481 and classes[0].get("num_instances") == 3
            and classes[0].get("is_zombie") == 0 and classes[0].get("item_class_id") == 499,
            "unexpected MATERIAL class manifest")
    objects = document["OBJECTS"]
    indexed = {}
    for obj in objects:
        handle = obj.get("handle")
        if not isinstance(handle, list) or len(handle) < 3:
            continue
        require(handle[2] not in indexed, "duplicate object identity")
        indexed[handle[2]] = obj
    roots = [obj for obj in objects if obj.get("object") == "DICTIONARY"
             and "ACAD_MATERIAL" in obj.get("items", {})]
    require(len(roots) == 1, "missing or duplicated material root")
    root = roots[0]
    edge = root["items"]["ACAD_MATERIAL"]
    require(edge[0] == 2 and edge[2] in indexed, "invalid NOD material edge")
    dictionary = indexed[edge[2]]
    require(dictionary.get("object") == "DICTIONARY"
            and dictionary["ownerhandle"][0] == 4
            and dictionary["ownerhandle"][2] == root["handle"][2], "invalid material dictionary owner")
    entries = dictionary["items"]
    require(set(entries) == set(NAMES), "unexpected material dictionary entries")
    header = document["HEADER"]
    require(header["DICTIONARY_MATERIAL"][0] == 5
            and header["DICTIONARY_MATERIAL"][2] == edge[2], "invalid HEADER material dictionary")
    require(header["CMATERIAL"][0] == 5
            and header["CMATERIAL"][2] == entries["ByLayer"][2], "invalid HEADER current material")
    materials = {}
    for name in NAMES:
        ref = entries[name]
        require(ref[0] == 2 and ref[2] in indexed, "invalid dictionary child edge")
        obj = indexed[ref[2]]
        require(obj.get("object") == "MATERIAL" and obj.get("name") == name
                and obj.get("type") == classes[0]["number"], "invalid material identity")
        require(obj["ownerhandle"][0] == 4 and obj["ownerhandle"][2] == edge[2]
                and len(obj["reactors"]) == 1 and obj["reactors"][0][0] == 4
                and obj["reactors"][0][2] == edge[2], "invalid owner/reactor")
        materials[name] = obj
    layers = [obj for obj in objects if obj.get("object") == "LAYER" and obj.get("name") == "0"]
    require(len(layers) == 1 and layers[0]["material"][0] == 5
            and layers[0]["material"][2] == entries["Global"][2], "invalid layer 0 material")
    return materials


def verify_pair(left_doc, left_trace, right_doc, right_trace):
    left, right = material_graph(left_doc), material_graph(right_doc)
    observations = []
    for name in NAMES:
        a, b = left[name], right[name]
        expected_boundary = 727 if name == "Global" else 743
        expected_strings = 120 if name == "Global" else 136
        left_boundary, left_strings, left_common = left_trace[name]
        right_boundary, right_strings, right_common = right_trace[name]
        self_delta = 8 * (b["handle"][1] - a["handle"][1])
        require(a["handle"][0] == b["handle"][0] == 0 and self_delta in (0, 8, 16, 24),
                "unexpected self-handle width")
        require(left_boundary == expected_boundary and left_strings == expected_strings
                and right_boundary == left_boundary + self_delta and right_strings == left_strings
                and right_common == left_common + self_delta, "unexpected declared boundary or string span")
        for obj, boundary in ((a, left_boundary), (b, right_boundary)):
            tail_bits = 8 * sum(1 + ref[1] for ref in (obj["ownerhandle"], *obj["reactors"]))
            padding = (8 - ((boundary + tail_bits) % 8)) % 8
            require(obj["size"] * 8 == boundary + tail_bits + padding,
                    "unexpected handle tail or frame padding")
        fields_a = {k: v for k, v in a.items() if k not in IDENTITY_FIELDS}
        fields_b = {k: v for k, v in b.items() if k not in IDENTITY_FIELDS}
        require(fields_a == fields_b, "independent material decoded fields differ")
        observations.append((name, len(fields_a), left_boundary, right_boundary, left_strings))
    return observations


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dwgread", type=Path, required=True)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--generated", type=Path, required=True)
    parser.add_argument("--source-sha256", required=True)
    parser.add_argument("--generated-sha256", required=True)
    args = parser.parse_args()
    reader = args.dwgread.resolve(strict=True)
    version = run_bounded([str(reader), "--version"], 5, 4096)
    require(version.returncode == 0 and version.stdout.strip() == b"dwgread 0.14", "requires LibreDWG 0.14")
    source, generated = args.source.resolve(strict=True), args.generated.resolve(strict=True)
    require(digest(source) == args.source_sha256 and digest(generated) == args.generated_sha256,
            "DWG hash mismatch")
    with tempfile.TemporaryDirectory(prefix="ac1021-material-peer-") as tmp:
        directory = Path(tmp)
        left_doc, left_trace = decode(reader, source, directory / "source.json")
        right_doc, right_trace = decode(reader, generated, directory / "generated.json")
    for name, count, left_boundary, right_boundary, left_strings in verify_pair(
            left_doc, left_trace, right_doc, right_trace):
        print(f"{name}: {count} peer-decoded fields; boundaries {left_boundary}/{right_boundary}; "
              f"strings {left_strings} bits; owner/reactor role 4; padding inferred from frame length")
    require(digest(source) == args.source_sha256 and digest(generated) == args.generated_sha256,
            "DWG changed during decoding")
    print("AC1027 source/writer material graph and frame audit: PASS")


if __name__ == "__main__":
    main()
