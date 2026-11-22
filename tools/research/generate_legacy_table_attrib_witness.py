#!/usr/bin/env python3
"""Create a scratch R14 TABLE/ATTRIB wire control from a hash-bound source.

This deliberately edits an external source's raw unsupported TABLE payload;
it is not a native producer witness and does not qualify complete output.
No DWG, DXF, or derivative should be committed to this repository.
"""
import argparse
import hashlib
import json
import re
import shutil
import struct
import subprocess
import tempfile
from pathlib import Path

from audit_ac1024_sortents_boundary import Bits, crc16, require

SOURCE_HASH = "d1ec4519be14ac49ecab5ec024ad951271e00261a1851f623a784a97ba6ede73"
BODY_START, BODY_SIZE = 73315, 4062
UNKNOWN_START, ATTR_BIT, DATA_END, INSERT_BIT = 26041, 26189, 32206, 32310
INSERTED = bytes((0x42, 0x07, 0x05, 0x42, 0x07, 0x05, 0x32, 0x07, 0x06))


def digest(data):
    return hashlib.sha256(data).hexdigest()


def run(command, *, failure=False):
    result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                            check=False, timeout=120)
    if not failure:
        require(result.returncode == 0,
                f"{command[0]} failed: {result.stderr.decode(errors='replace')[-1000:]}")
    return result


def executable(name):
    path = shutil.which(name)
    require(path is not None, f"executable not found: {name}")
    resolved = Path(path).resolve()
    return {"path": str(resolved), "sha256": digest(resolved.read_bytes())}


def bitstring(data):
    return "".join(f"{value:08b}" for value in data)


def frame(data, start, expected_size):
    end = start + 2 + expected_size
    require(start >= 0 and end + 2 <= len(data), "truncated frame")
    require(struct.unpack_from("<H", data, start)[0] == expected_size,
            "unexpected frame size")
    require(struct.unpack_from("<H", data, end)[0] == crc16(data[start:end]),
            "frame CRC mismatch")
    return data[start + 2:end]


def table_handles(body, *, with_attributes):
    bits = Bits(body)
    require(bits.bs() == 523 and bits.handle() == (0, 2, 0x528), "TABLE identity mismatch")
    bits.bit = ATTR_BIT
    require(bits.take(1) == int(with_attributes), "TABLE attribute flag mismatch")
    bits.bit = DATA_END
    expected = [(3, 2, 0x10FC), (5, 2, 0x275), (8, 0, 0), (10, 2, 0x16C),
                (5, 2, 0x570)]
    if with_attributes:
        expected += [(4, 2, 0x705), (4, 2, 0x705), (3, 2, 0x706)]
    expected.append((5, 1, 0x87))
    for expected_handle in expected:
        require(bits.handle() == expected_handle, "TABLE handle role or target mismatch")


def model_objects(path):
    data = json.loads(path.read_text())
    objects = {item["handle"][-1]: item for item in data["OBJECTS"] if item.get("handle")}
    for handle, kind in ((0x528, "TABLE"), (0x704, "INSERT"),
                         (0x705, "ATTRIB"), (0x706, "SEQEND")):
        require(handle in objects and objects[handle].get("entity") == kind,
                f"missing {kind} {handle:X}")
    return data, objects


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--dwgread", default="dwgread")
    parser.add_argument("--dwgwrite", default="dwgwrite")
    parser.add_argument("--parser-probe", help="optional table legacy prefix test executable")
    parser.add_argument("--reader-probe", help="optional table legacy external test executable")
    args = parser.parse_args()
    source = args.source.read_bytes()
    require(source.startswith(b"AC1014") and digest(source) == SOURCE_HASH,
            "source hash or version mismatch")
    parent = args.output_dir.resolve()
    repository = Path(__file__).resolve().parents[2]
    require(parent != repository and repository not in parent.parents,
            "output must be outside the repository")
    for tool in (args.dwgread, args.dwgwrite):
        require(b"0.14" in run([tool, "--version"]).stdout, "LibreDWG 0.14 required")
    parent.mkdir(parents=True, exist_ok=True)
    root = Path(tempfile.mkdtemp(prefix="r14-table-attrib-", dir=parent))

    source_json = root / "source.json"
    run([args.dwgread, "-O", "JSON", "-o", str(source_json), str(args.source)])
    document, objects = model_objects(source_json)
    table, insert, attribute, seqend = (objects[h] for h in (0x528, 0x704, 0x705, 0x706))
    require(table.get("has_attribs") == 0 and insert.get("has_attribs") == 1,
            "unexpected source attribute states")
    for key, target in (("first_attrib", 0x705), ("last_attrib", 0x705), ("seqend", 0x706)):
        require(insert.get(key, [None])[-1] == target, f"unexpected source {key}")
    require(attribute.get("ownerhandle", [None])[-1] == 0x704
            and seqend.get("ownerhandle", [None])[-1] == 0x704,
            "unexpected source compound ownership")
    original = frame(source, BODY_START - 2, BODY_SIZE)
    table_handles(original, with_attributes=False)
    original_bits = bitstring(original)
    unknown = original_bits[UNKNOWN_START:]
    require(len(unknown) == table.get("num_unknown_bits") == 6455,
            "unexpected unsupported TABLE payload size")
    json_bytes = bytes.fromhex(table["unknown_bits"])
    require(bitstring(json_bytes) == unknown[:len(json_bytes) * 8],
            "JSON raw payload differs from physical source")
    offset = ATTR_BIT - UNKNOWN_START
    require(unknown[offset] == "0", "source attribute bit was not clear")
    unknown = unknown[:offset] + "1" + unknown[offset + 1:]
    insert_offset = INSERT_BIT - UNKNOWN_START
    unknown = unknown[:insert_offset] + bitstring(INSERTED) + unknown[insert_offset:]
    # LibreDWG 0.14 writes a partial final raw byte least-significant bit first.
    full_bytes = len(unknown) // 8
    raw = bytearray(int(unknown[i:i + 8], 2) for i in range(0, full_bytes * 8, 8))
    remainder = unknown[full_bytes * 8:]
    if remainder:
        raw.append(sum((bit == "1") << index for index, bit in enumerate(remainder)))
    table["unknown_bits"] = raw.hex().upper()
    table["num_unknown_bits"] = len(unknown)
    table["size"] += len(INSERTED)
    table["has_attribs"] = 1
    for key in ("first_attrib", "last_attrib", "seqend"):
        table[key] = insert.pop(key)
    insert["has_attribs"] = 0
    attribute["ownerhandle"] = [4, 2, 0x528, 0x528]
    seqend["ownerhandle"] = [4, 2, 0x528, 0x528]
    edited_json = root / "edited.json"
    edited_json.write_text(json.dumps(document, separators=(",", ":")))
    output = root / "table-attrib.dwg"
    write = run([args.dwgwrite, "-I", "JSON", "--as", "r14", "-y", "-o",
                 str(output), str(edited_json)])
    require(b"TABLE unsupported" in write.stdout + write.stderr,
            "expected raw TABLE writer path was not observed")

    reopened_json = root / "reopened.json"
    run([args.dwgread, "-O", "JSON", "-o", str(reopened_json), str(output)])
    _, reopened = model_objects(reopened_json)
    actual = reopened[0x528]
    require(actual.get("has_attribs") == 1 and reopened[0x704].get("has_attribs") == 0,
            "peer did not reopen the modified attribute flag")
    for key, target in (("first_attrib", 0x705), ("last_attrib", 0x705), ("seqend", 0x706)):
        require(actual.get(key, [None])[-1] == target, f"peer {key} mismatch")
    require(reopened[0x705].get("ownerhandle", [None])[-1] == 0x528
            and reopened[0x706].get("ownerhandle", [None])[-1] == 0x528,
            "peer compound ownership mismatch")
    trace = run([args.dwgread, "-v4", str(output)])
    blocks = re.split(r"(?=Object number:)", (trace.stdout + trace.stderr).decode(errors="replace"))
    selected = [block for block in blocks if re.search(r"\bhandle: 0\.2\.528\s", block)]
    require(len(selected) == 1, "peer trace does not identify one TABLE frame")
    match = re.search(r"Size: (\d+) \[MS\], Type: 523 \[BS\], Address: (\d+)", selected[0])
    require(match is not None, "peer frame location missing")
    size, address = map(int, match.groups())
    require(size == BODY_SIZE + len(INSERTED), "unexpected rewritten TABLE size")
    emitted = frame(output.read_bytes(), address - 2, size)
    table_handles(emitted, with_attributes=True)
    expected_bits = (original_bits[:ATTR_BIT] + "1" + original_bits[ATTR_BIT + 1:INSERT_BIT]
                     + bitstring(INSERTED) + original_bits[INSERT_BIT:])
    require(bitstring(emitted) == expected_bits, "TABLE body changed outside the intended edits")
    frame_path = root / "table-frame.bin"
    frame_path.write_bytes(emitted)
    probes = {}
    if args.parser_probe:
        probes["parser"] = executable(args.parser_probe)
        run([args.parser_probe, "--source-frame", str(frame_path)])
        # A role mutation is checked against the same frame by a fresh process.
        wrong = bytearray(emitted)
        wrong[(INSERT_BIT + 3) // 8] ^= 0x80 >> ((INSERT_BIT + 3) % 8)
        wrong_path = root / "wrong-role-frame.bin"
        wrong_path.write_bytes(wrong)
        require(run([args.parser_probe, "--source-frame", str(wrong_path)],
                    failure=True).returncode != 0, "parser accepted wrong attribute role")
        run([args.parser_probe, "--source-frame", str(frame_path)])
    if args.reader_probe:
        probes["reader"] = executable(args.reader_probe)
        def reject_then_retry(name, body_address, body_size, bit):
            bad = bytearray(output.read_bytes())
            bad[body_address + bit // 8] ^= 0x80 >> (bit % 8)
            struct.pack_into("<H", bad, body_address + body_size,
                             crc16(bad[body_address - 2:body_address + body_size]))
            bad_path = root / f"{name}.dwg"
            bad_path.write_bytes(bad)
            frame(bad, body_address - 2, body_size)
            run([args.reader_probe, str(bad_path), "r14-attrs", str(output)])

        reject_then_retry("wrong-role", address, size, INSERT_BIT + 3)
        # In this hash-bound rewrite the ATTRIB/SEQEND common handle tails
        # start at these bit offsets. Mutate their owner targets (not roles).
        for name, body_address, body_size, owner_bit in (
                ("wrong-attrib-owner", 99873, 89, 625),
                ("wrong-seqend-owner", 99966, 20, 81)):
            body = frame(output.read_bytes(), body_address - 2, body_size)
            bits = Bits(body)
            bits.bit = owner_bit
            require(bits.handle() == (4, 2, 0x528), "child owner span changed")
            reject_then_retry(name, body_address, body_size, owner_bit + 23)
    receipt = {
        "schema": 1, "scope": "controlled R14 TABLE compound and malformed-role retry; not native semantics",
        "sourceSha256": SOURCE_HASH, "outputSha256": digest(output.read_bytes()),
        "frameSha256": digest(emitted), "frameAddress": address, "frameBytes": size,
        "toolSha256": digest(Path(__file__).read_bytes()),
        "executables": {"dwgread": executable(args.dwgread), "dwgwrite": executable(args.dwgwrite),
                        **probes},
        "limits": ["The source and derivative remain outside the repository.",
                   "Other unsupported source objects are not qualified by LibreDWG's rewrite.",
                   "This is not native TABLE attribute, cloning, display, or output evidence."],
    }
    (root / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
    print(root)
    print(json.dumps(receipt, indent=2))


if __name__ == "__main__":
    main()
