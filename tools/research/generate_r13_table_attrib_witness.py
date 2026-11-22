#!/usr/bin/env python3
"""Create a scratch AC1012 TABLE/ATTRIB control from a hash-bound source.

The derivative is not native producer evidence and must not be committed.
"""
import argparse
import json
import re
import struct
import tempfile
from pathlib import Path

from audit_ac1024_sortents_boundary import Bits, crc16, require
from generate_legacy_table_attrib_witness import bitstring, digest, executable, frame, run

SOURCE_HASH = "53558a8ae4bf666f906c9c2e4d3c3ee7ef61690e3b0f4d4a858e4e1bf555fc89"
BODY_START, BODY_SIZE = 142858, 4475
UNKNOWN_START, ATTR_BIT, INSERT_BIT = 23833, 23981, 34332


def objects(path, insert_handle, attrib_handle, seqend_handle):
    document = json.loads(path.read_text())
    result = {o["handle"][-1]: o for o in document["OBJECTS"] if o.get("handle")}
    for handle, kind in ((0x4F2, "TABLE"), (insert_handle, "INSERT"),
                         (attrib_handle, "ATTRIB"), (seqend_handle, "SEQEND")):
        require(handle in result and result[handle].get("entity") == kind,
                f"missing {kind} {handle:X}")
    return document, result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("--output-dir", required=True, type=Path)
    parser.add_argument("--dwgread", default="dwgread")
    parser.add_argument("--dwgwrite", default="dwgwrite")
    parser.add_argument("--reader-probe")
    ordering = parser.add_mutually_exclusive_group()
    ordering.add_argument("--earlier-children", action="store_true")
    ordering.add_argument("--mixed-children", action="store_true")
    ordering.add_argument("--multiple-children", action="store_true")
    args = parser.parse_args()
    insert_handle, attrib_handle, seqend_handle = (
        (0x190, 0x192, 0x191) if args.earlier_children or args.multiple_children else
        (0x756, 0x757, 0x191) if args.mixed_children else (0x756, 0x757, 0x759))
    last_handle = 0x757 if args.multiple_children else attrib_handle
    inserted = bytes((0x42, attrib_handle >> 8, attrib_handle & 255,
                      0x42, last_handle >> 8, last_handle & 255,
                      0x32, seqend_handle >> 8, seqend_handle & 255))
    source = args.source.read_bytes()
    require(source.startswith(b"AC1012") and digest(source) == SOURCE_HASH,
            "source hash or version mismatch")
    parent = args.output_dir.resolve()
    repository = Path(__file__).resolve().parents[2]
    require(parent != repository and repository not in parent.parents,
            "output must be outside the repository")
    for tool in (args.dwgread, args.dwgwrite):
        require(b"0.14" in run([tool, "--version"]).stdout, "LibreDWG 0.14 required")
    parent.mkdir(parents=True, exist_ok=True)
    root = Path(tempfile.mkdtemp(prefix="r13-table-attrib-", dir=parent))
    original_json = root / "source.json"
    run([args.dwgread, "-O", "JSON", "-o", str(original_json), str(args.source)])
    document, source_objects = objects(original_json, insert_handle, attrib_handle, seqend_handle)
    table, insert, attribute, seqend = (source_objects[h] for h in
                                        (0x4F2, insert_handle, attrib_handle, seqend_handle))
    require(table.get("has_attribs") == 0 and insert.get("has_attribs") == 1,
            "unexpected source attribute states")
    for key, target in (("first_attrib", attrib_handle), ("last_attrib", attrib_handle)):
        require(insert.get(key, [None])[-1] == target, f"unexpected source {key}")
    sequence_insert = source_objects[0x190] if args.mixed_children else insert
    require(sequence_insert.get("seqend", [None])[-1] == seqend_handle,
            "unexpected source seqend")
    require(attribute.get("ownerhandle", [None])[-1] == insert_handle and
            seqend.get("ownerhandle", [None])[-1] ==
            (0x190 if args.mixed_children else insert_handle), "source owner mismatch")
    original = frame(source, BODY_START - 2, BODY_SIZE)
    original_bits = bitstring(original)
    unknown = original_bits[UNKNOWN_START:]
    require(len(unknown) == table.get("num_unknown_bits") == 11967,
            "unexpected raw payload size")
    encoded = bitstring(bytes.fromhex(table["unknown_bits"]))
    require(encoded[:len(unknown)] == unknown, "source raw payload mismatch")
    require(original_bits[ATTR_BIT] == "0", "source flag is not clear")
    offset = ATTR_BIT - UNKNOWN_START
    unknown = unknown[:offset] + "1" + unknown[offset + 1:]
    offset = INSERT_BIT - UNKNOWN_START
    unknown = unknown[:offset] + bitstring(inserted) + unknown[offset:]
    full_bytes = len(unknown) // 8
    raw = bytearray(int(unknown[i:i + 8], 2) for i in range(0, full_bytes * 8, 8))
    remainder = unknown[full_bytes * 8:]
    if remainder:
        raw.append(sum((bit == "1") << i for i, bit in enumerate(remainder)))
    table["unknown_bits"], table["num_unknown_bits"] = raw.hex().upper(), len(unknown)
    table["size"] += len(inserted)
    table["has_attribs"] = 1
    for key in ("first_attrib", "last_attrib"):
        table[key] = insert.pop(key)
    table["seqend"] = sequence_insert["seqend"]
    insert.pop("seqend")
    insert["has_attribs"] = 0
    if args.mixed_children:
        for key in ("first_attrib", "last_attrib", "seqend"):
            sequence_insert.pop(key)
        sequence_insert["has_attribs"] = 0
    attribute["ownerhandle"] = [4, 2, 0x4F2, 0x4F2]
    seqend["ownerhandle"] = [4, 2, 0x4F2, 0x4F2]
    if args.multiple_children:
        other_insert, other_attribute = source_objects[0x756], source_objects[0x757]
        require(other_insert.get("first_attrib", [None])[-1] == 0x757 and
                other_attribute.get("ownerhandle", [None])[-1] == 0x756,
                "unexpected second source attribute")
        table["last_attrib"] = [4, 2, 0x757, 0x757]
        attribute["next_entity"] = [4, 2, 0x757, 0x757]
        other_attribute["prev_entity"] = [4, 2, 0x192, 0x192]
        other_attribute["ownerhandle"] = [4, 2, 0x4F2, 0x4F2]
        for key in ("first_attrib", "last_attrib", "seqend"):
            other_insert.pop(key)
        other_insert["has_attribs"] = 0
    edit = root / "edit.json"
    edit.write_text(json.dumps(document, separators=(",", ":")))
    output = root / "table-attrib.dwg"
    write = run([args.dwgwrite, "-I", "JSON", "--as", "r13", "-y", "-o",
                 str(output), str(edit)])
    require(b"TABLE unsupported" in write.stdout + write.stderr, "expected raw writer path")
    reopened = root / "reopened.json"
    run([args.dwgread, "-O", "JSON", "-o", str(reopened), str(output)])
    _, observed = objects(reopened, insert_handle, attrib_handle, seqend_handle)
    require(observed[0x4F2].get("has_attribs") == 1 and
            observed[insert_handle].get("has_attribs") == 0, "peer attribute flag mismatch")
    for key, target in (("first_attrib", attrib_handle), ("last_attrib", last_handle),
                        ("seqend", seqend_handle)):
        require(observed[0x4F2].get(key, [None])[-1] == target, "peer handle mismatch")
    require(all(observed[h].get("ownerhandle", [None])[-1] == 0x4F2
                for h in (attrib_handle, seqend_handle)), "peer ownership mismatch")
    if args.multiple_children:
        require(observed[0x757].get("ownerhandle", [None])[-1] == 0x4F2 and
                observed[attrib_handle].get("next_entity", [None])[-1] == 0x757,
                "peer multiple-attribute chain mismatch")
    trace = run([args.dwgread, "-v4", str(output)])
    blocks = re.split(r"(?=Object number:)", (trace.stdout + trace.stderr).decode(errors="replace"))
    spans = {}
    for handle, expected_size in ((0x4F2, BODY_SIZE + len(inserted)),
                                  (attrib_handle, 104 if args.multiple_children else
                                   102 if args.earlier_children else 96),
                                  (seqend_handle, 19)):
        selected = [b for b in blocks if re.search(rf"\bhandle: 0\.2\.{handle:X}\s", b)]
        require(len(selected) == 1, "peer frame identity mismatch")
        match = re.search(r"Size: (\d+) \[MS\], Type: \d+ \[BS\], Address: (\d+)", selected[0])
        require(match is not None and int(match.group(1)) == expected_size,
                "peer frame span mismatch")
        spans[handle] = (int(match.group(2)), expected_size)
    address, size = spans[0x4F2]
    emitted = frame(output.read_bytes(), address - 2, size)
    expected = (original_bits[:ATTR_BIT] + "1" + original_bits[ATTR_BIT + 1:INSERT_BIT]
                + bitstring(inserted) + original_bits[INSERT_BIT:])
    require(bitstring(emitted) == expected, "TABLE body changed outside intended edits")
    probe = None
    if args.reader_probe:
        probe = executable(args.reader_probe)
        for name, handle, bit in (("wrong-role", 0x4F2, INSERT_BIT + 3),
                                  ("wrong-attrib-owner", attrib_handle,
                                   (735 if args.earlier_children or args.multiple_children else 687) + 23),
                                  ("wrong-seqend-owner", seqend_handle, 81 + 23)):
            body_address, body_size = spans[handle]
            body = frame(output.read_bytes(), body_address - 2, body_size)
            if handle != 0x4F2:
                stream = Bits(body)
                stream.bit = bit - 23
                require(stream.handle() == (4, 2, 0x4F2), "child owner span mismatch")
            bad = bytearray(output.read_bytes())
            bad[body_address + bit // 8] ^= 0x80 >> (bit % 8)
            struct.pack_into("<H", bad, body_address + body_size,
                             crc16(bad[body_address - 2:body_address + body_size]))
            bad_path = root / f"{name}.dwg"
            bad_path.write_bytes(bad)
            frame(bad, body_address - 2, body_size)
            profile = ("r13-multi-attrs" if args.multiple_children else
                       "r13-early-attrs" if args.earlier_children else
                       "r13-mixed-attrs" if args.mixed_children else "r13-attrs")
            run([args.reader_probe, str(bad_path), profile, str(output)])
    receipt = {"schema": 1, "scope": "controlled AC1012 TABLE compound; not native semantics",
               "earlierChildren": args.earlier_children,
               "mixedChildren": args.mixed_children,
               "multipleChildren": args.multiple_children,
               "sourceSha256": SOURCE_HASH, "outputSha256": digest(output.read_bytes()),
               "frameSha256": digest(emitted), "generatorSha256": digest(Path(__file__).read_bytes()),
               "dwgread": executable(args.dwgread), "dwgwrite": executable(args.dwgwrite),
               "readerProbe": probe,
               "limits": ["No derivative is committed.", "Other rewritten objects are not qualified.",
                          "No native TABLE semantics or strict output claim is established."]}
    (root / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
    print(root)
    print(json.dumps(receipt, indent=2))


if __name__ == "__main__":
    main()
