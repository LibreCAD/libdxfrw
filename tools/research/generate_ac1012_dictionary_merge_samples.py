#!/usr/bin/env python3
"""Create paired, scratch-only AC1012 DICTIONARY merge observation samples.

The two drawings contain the same NOD -> TEST_DICT -> SAME_KEY graph. The
entry targets distinct, named MLINESTYLE objects so an ObjectARX observer can
identify which value survives a duplicate-key clone. LibreDWG is only the
sample producer here; its output is not native AutoCAD evidence. Optionally
pass the focused libdxfrw test executable to parse each extracted physical
dictionary frame and exercise role/truncation refusal and corrected retry.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
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
TEST_DICTIONARY = "TEST_DICT"


def read_dictionary_frame(path: Path, dwgread: str,
                          handle: int) -> tuple[dict, bytes]:
    data = path.read_bytes()
    require(data.startswith(VERSION), "sample is not AC1012")
    trace = run([dwgread, "-v4", str(path)])
    text = (trace.stdout + trace.stderr).decode(errors="replace")
    blocks = re.split(r"(?=Object number:)", text)
    pattern = re.compile(
        r"\bhandle: 0\.1\." + format(handle, "X") + r"\s+\[H 5\]",
        re.IGNORECASE,
    )
    selected = [block for block in blocks if pattern.search(block)]
    require(len(selected) == 1, "trace does not identify one selected dictionary frame")
    match = re.search(
        r"Size: (\d+) \[MS\], Type: 42 \[BS\], Address: (\d+)", selected[0]
    )
    require(match is not None, "dictionary frame size/address missing")
    size, address = map(int, match.groups())
    require(size < 0x8000 and address >= 2 and address + size + 2 <= len(data),
            "dictionary frame is outside the source")
    require(data[address - 2:address] == size.to_bytes(2, "little"),
            "unexpected AC1012 frame-size prefix")
    require(int.from_bytes(data[address + size:address + size + 2], "little")
            == crc16(data[address - 2:address + size]), "dictionary frame CRC mismatch")

    frame = data[address:address + size]
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
    require(reactors <= 8 and count == 1, "expected one bounded dictionary item")
    name_length = bits.bs()
    require(0 < name_length <= 256, "dictionary TV name length is invalid")
    name_bytes = bytes(bits.take(8) for _ in range(name_length))
    require(name_bytes.count(b"\0") <= 1
            and (b"\0" not in name_bytes or name_bytes.endswith(b"\0")),
            "AC1012 TV name has an embedded or repeated terminator")
    name = name_bytes[:-1].decode("ascii") if name_bytes.endswith(b"\0") \
        else name_bytes.decode("ascii")
    require(bits.position == data_end, "dictionary data span does not end after its TV entry")

    parent = bits.handle(4, handle)
    reactor_handles = [bits.handle(4, handle) for _ in range(reactors)]
    xdictionary = bits.handle(3, handle)
    item = bits.handle(2, handle)
    require(len(frame) * 8 - bits.position < 8, "unexpected data after dictionary item handle")
    require(name == ENTRY_KEY and item["value"] != 0, "dictionary item differs from the selected key")
    proof = {
        "offset": address,
        "sizeBytes": size,
        "frameSha256": sha(frame),
        "crcValid": True,
        "self": own,
        "dataEndBit": data_end,
        "entryName": name,
        "entryNameHasTerminator": name_bytes.endswith(b"\0"),
        "parent": parent,
        "reactors": reactor_handles,
        "xdictionary": xdictionary,
        "itemHandle": item,
        "cloningField": None,
        "eed": eed,
        "paddingBits": len(frame) * 8 - bits.position,
    }
    return proof, frame


def read_model(path: Path, dwgread: str) -> dict:
    decoded = path.with_suffix(".json")
    run([dwgread, "-O", "JSON", "-o", str(decoded), str(path)])
    objects = json.loads(decoded.read_text())["OBJECTS"]
    roots = [
        obj for obj in objects
        if obj.get("object") == "DICTIONARY"
        and TEST_DICTIONARY in obj.get("items", {})
    ]
    require(len(roots) == 1, "TEST_DICT NOD membership is not unique")
    root = roots[0]
    dictionary_handle = root["items"][TEST_DICTIONARY][-1]
    dictionaries = [
        obj for obj in objects
        if obj.get("object") == "DICTIONARY"
        and obj.get("handle", [None])[-1] == dictionary_handle
    ]
    require(len(dictionaries) == 1, "TEST_DICT child dictionary is missing or duplicated")
    dictionary = dictionaries[0]
    require(dictionary.get("numitems") == 1, "TEST_DICT must contain one entry")
    entry = dictionary.get("items", {}).get(ENTRY_KEY)
    require(isinstance(entry, list) and len(entry) >= 3,
            "SAME_KEY does not resolve to one object handle")
    target_handle = entry[-1]
    target = [
        obj for obj in objects
        if obj.get("object") == "MLINESTYLE"
        and obj.get("handle", [None])[-1] == target_handle
    ]
    require(len(target) == 1, "SAME_KEY target is not one MLINESTYLE")
    return {
        "sha256": sha(path.read_bytes()),
        "dictionaryHandle": dictionary_handle,
        "rootHandle": root["handle"][-1],
        "dictionaryOwner": dictionary.get("ownerhandle", [None])[-1],
        "entry": {"name": ENTRY_KEY, "raw": entry, "targetHandle": target_handle},
        "targetStyle": {
            "handle": target_handle,
            "name": target[0].get("name"),
            "startAngle": target[0].get("start_angle"),
        },
    }


def write_recipe(path: Path, angle: int, target_handle: int) -> None:
    path.write_text(
        "line (0 0 0) (1 1 0)\n"
        'mlinestyle "ProbeStyle"\n'
        f"mlinestyle.start_angle = {angle}\n"
        f'dictionary "{TEST_DICTIONARY}" "{ENTRY_KEY}" {target_handle}\n'
    )


def style_handles(path: Path, dwgread: str) -> dict[str, int]:
    decoded = path.with_suffix(".json")
    run([dwgread, "-O", "JSON", "-o", str(decoded), str(path)])
    objects = json.loads(decoded.read_text())["OBJECTS"]
    styles = {
        obj.get("name"): obj.get("handle", [None])[-1]
        for obj in objects if obj.get("object") == "MLINESTYLE"
    }
    require(isinstance(styles.get("Standard"), int)
            and isinstance(styles.get("ProbeStyle"), int),
            "DWG producer did not provide both selected MLINESTYLE targets")
    return styles


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-dir", type=Path, required=True,
                        help="scratch parent; each run creates a new child directory")
    parser.add_argument("--dwgadd", default="dwgadd")
    parser.add_argument("--dwgread", default="dwgread")
    parser.add_argument("--candidate", help="optional libdxfrw dwg2dxf executable")
    parser.add_argument("--dictionary-source-test",
                        help="optional libdxfrw AC1012 physical-frame test executable")
    args = parser.parse_args()
    require(b"0.14" in run([args.dwgadd, "--version"]).stdout,
            "LibreDWG 0.14 dwgadd is required")
    require(b"0.14" in run([args.dwgread, "--version"]).stdout,
            "LibreDWG 0.14 dwgread is required")
    args.output_dir.mkdir(parents=True, exist_ok=True)
    root = Path(tempfile.mkdtemp(prefix="ac1012-dictionary-merge-", dir=args.output_dir))

    probe_recipe = root / "probe.dwgadd"
    probe_recipe.write_text('line (0 0 0) (1 1 0)\nmlinestyle "ProbeStyle"\n')
    probe = root / "probe.dwg"
    run([args.dwgadd, "--as", "r13", "-o", str(probe), str(probe_recipe)])
    handles = style_handles(probe, args.dwgread)

    results = {
        "schema": "libdxfrw.research-receipt.v1",
        "id": "p6-ac1012-populated-dictionary-merge-samples-2026-11-22",
        "status": "PAIRED_SPEC_BOUND_SAMPLE_GENERATION_PASS_NATIVE_SEMANTICS_OPEN",
        "supportClaimEffect": "none",
        "authority": {
            "document": "ODA Open Design Specification v5.4.1",
            "section": "20.4.44 DICTIONARY",
            "source": "https://www.opendesign.com/files/guestdownloads/OpenDesign_Specification_for_.dwg_files.pdf",
            "layoutApplied": "AC1012 DICTIONARY has a counted entry-name stream and soft-owner entry handles; the R2000+ cloning and hard-owner fields are absent.",
        },
        "producer": "GNU LibreDWG dwgadd 0.14",
        "generatorSha256": sha(Path(__file__).read_bytes()),
        "executables": {
            "dwgadd": executable_identity(args.dwgadd),
            "dwgread": executable_identity(args.dwgread),
        },
        "scratchDirectory": str(root),
        "styleHandles": handles,
        "samples": {},
        "limits": [
            "The paired files are generated controls, not AutoCAD-authored samples.",
            "LibreDWG output must pass native normal-open without recovery before any ObjectARX observation counts.",
            "The exact entry target and MLINESTYLE values are source evidence only; this does not establish native duplicate-merge behavior or output qualification.",
            "No DWG or DXF fixture is added to the repository.",
        ],
    }

    for label, target_name, angle in (
        ("destination", "Standard", 15),
        ("source", "ProbeStyle", 37),
    ):
        target_handle = handles[target_name]
        recipe = root / f"{label}.dwgadd"
        write_recipe(recipe, angle, target_handle)
        path = root / f"{label}-ac1012.dwg"
        run([args.dwgadd, "--as", "r13", "-o", str(path), str(recipe)])
        model = read_model(path, args.dwgread)
        frame, frame_bytes = read_dictionary_frame(
            path, args.dwgread, model["dictionaryHandle"])
        require(model["dictionaryOwner"] == model["rootHandle"],
                "TEST_DICT owner does not point to the NOD")
        require(frame["itemHandle"]["value"] == model["entry"]["targetHandle"],
                "physical dictionary item handle differs from the peer model")
        require(frame["parent"]["value"] == model["rootHandle"],
                "physical dictionary owner handle differs from the peer model")
        require(len(frame["reactors"]) == 1
                and frame["reactors"][0]["value"] == model["rootHandle"],
                "physical dictionary reactor does not resolve to the NOD")
        require(frame["xdictionary"]["value"] == 0,
                "unexpected extension dictionary on this bounded profile")
        require(model["targetStyle"]["name"] == target_name,
                "dictionary target does not identify the expected style")
        require(frame["cloningField"] is None,
                "AC1012 plain DICTIONARY unexpectedly reports a cloning field")
        candidate = None
        if args.candidate:
            candidate_path = Path(args.candidate).resolve()
            candidate_output = root / f"{label}-candidate-strict.dxf"
            sentinel = b"preserve prior DXF destination on strict refusal\n"
            candidate_output.write_bytes(sentinel)
            result = subprocess.run(
                [str(candidate_path), str(path), "-o", str(candidate_output), "-y", "-d"],
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                timeout=120,
                check=False,
            )
            log = result.stdout.decode(errors="replace")
            preserved = candidate_output.read_bytes() == sentinel
            require(result.returncode == 0 or preserved,
                    "candidate replaced its destination after refusing strict output")
            lines = [line.strip() for line in log.splitlines()
                     if ("DWG read observations:" in line
                         or "frameCoverageStatus=" in line
                         or "Opaque callback dispositions:" in line
                         or "DXF export preflight failed:" in line
                         or "DXF export failed:" in line)]
            candidate = {
                "executable": executable_identity(str(candidate_path)),
                "exitCode": result.returncode,
                "destinationPreservedOnRefusal": preserved if result.returncode else None,
                "outputSha256": sha(candidate_output.read_bytes()) if result.returncode == 0 else None,
                "diagnostics": lines[-8:],
            }
        dictionary_frame_test = None
        if args.dictionary_source_test:
            test_path = Path(args.dictionary_source_test).resolve()
            frame_path = root / f"{label}-dictionary-frame.bin"
            frame_path.write_bytes(frame_bytes)
            result = subprocess.run(
                [str(test_path), "--source-dictionary-frame", str(frame_path),
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
            "source": model,
            "wire": frame,
            "candidateStrict": candidate,
            "libdxfrwPhysicalFrameTest": dictionary_frame_test,
        }

    require(results["samples"]["destination"]["source"]["entry"]["name"]
            == results["samples"]["source"]["source"]["entry"]["name"],
            "paired dictionaries do not share the duplicate key")
    require(results["samples"]["destination"]["source"]["entry"]["targetHandle"]
            != results["samples"]["source"]["source"]["entry"]["targetHandle"],
            "paired dictionary entries do not have distinct targets")
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
