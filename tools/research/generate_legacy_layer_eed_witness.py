#!/usr/bin/env python3
"""Generate scratch R13/R14 layer-reference DWGs and test their byte order.

Requires ezdxf, LibreDWG dwgread, and an installed ODA File Converter with
access to the desktop session. Generated drawings are never copied into the
repository. The byte-swapped control is derived from the locally generated
ODA source, with the object CRC repaired, and is not a producer witness.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile

from generate_ac1012_dictionary_witness import Bits, crc16, executable_identity, model_and_frame


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(*args: str) -> None:
    result = subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False)
    if result.returncode:
        raise RuntimeError(f"{args[0]} exited {result.returncode}: {result.stderr[-1000:]!r}")


def patch_layer_bytes(source: Path, destination: Path, proof: dict,
                      appid: int, handle: int) -> None:
    data = bytearray(source.read_bytes())
    offset, size = proof["frameOffset"], proof["frameSize"]
    if data[offset-2:offset] != size.to_bytes(2, "little"):
        raise ValueError("unexpected frame envelope")
    if int.from_bytes(data[offset+size:offset+size+2], "little") != crc16(data[offset-2:offset+size]):
        raise ValueError("source CRC mismatch")
    bits = Bits(data[offset:offset+size])
    if bits.bs() != 42 or bits.handle(0, own=True)["value"] != proof["sourceHandle"]:
        raise ValueError("source dictionary identity mismatch")
    start = None
    while True:
        size_chunk = bits.bs()
        if not size_chunk:
            break
        app = bits.handle(5)["value"]
        begin = bits.position
        raw = bytes(bits.take(8) for _ in range(size_chunk))
        if app == appid:
            if start is not None or raw != b"\x03" + handle.to_bytes(8, "big"):
                raise ValueError("layer-reference source mismatch")
            start = begin + 8
    if start is None:
        raise ValueError("layer-reference chunk missing")
    for index, byte in enumerate(handle.to_bytes(8, "little")):
        for bit in range(8):
            pos = offset * 8 + start + index * 8 + bit
            mask = 0x80 >> (pos % 8)
            data[pos // 8] = ((data[pos // 8] | mask) if byte & (0x80 >> bit)
                              else (data[pos // 8] & ~mask))
    data[offset+size:offset+size+2] = crc16(data[offset-2:offset+size]).to_bytes(2, "little")
    destination.write_bytes(data)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--oda", required=True)
    parser.add_argument("--dwgread", default="dwgread")
    parser.add_argument("--output-dir", type=Path, required=True)
    args = parser.parse_args()
    import ezdxf

    args.output_dir.mkdir(parents=True, exist_ok=True)
    root = Path(tempfile.mkdtemp(prefix="legacy-layer-eed-", dir=args.output_dir))
    incoming = root / "input"
    incoming.mkdir()
    doc = ezdxf.new("R2018")
    app = doc.appids.new("LAYER_TEST")
    layer = doc.layers.new("PROBE_LAYER")
    entity = doc.modelspace().add_line((1, 2, 3), (4, 6, 9))
    dictionary = doc.rootdict.add_new_dict("TEST_DICT")
    entity.set_xdata("LAYER_TEST", [(1003, "PROBE_LAYER")])
    dictionary.set_xdata("LAYER_TEST", [(1003, "PROBE_LAYER")])
    seed = incoming / "seed.dxf"
    doc.saveas(seed)
    appid, handle = int(app.dxf.handle, 16), int(layer.dxf.handle, 16)
    if not 0 < handle <= 0xFFFFFFFF or handle.to_bytes(8, "big") == handle.to_bytes(8, "little"):
        raise ValueError("source layer handle does not distinguish byte order")
    receipt = {"ezdxfVersion": ezdxf.__version__, "oda": executable_identity(args.oda),
               "dwgread": executable_identity(args.dwgread), "seedSha256": digest(seed),
               "layerHandle": handle, "appidHandle": appid, "versions": {}}
    for version, target in (("r13", "ACAD13"), ("r14", "ACAD14")):
        source_dir, reopen, patched_dir, patched_reopen = [root / (version + suffix)
            for suffix in ("", "-reopen", "-patched", "-patched-reopen")]
        for directory in (source_dir, reopen, patched_dir, patched_reopen):
            directory.mkdir()
        run(args.oda, str(incoming), str(source_dir), target, "DWG", "0", "1", "*.dxf")
        source = source_dir / "seed.dwg"
        proof = model_and_frame(args.dwgread, source, version)
        chunks = [row for row in proof["wire"]["eed"] if row["appid"]["value"] == appid]
        expected = (b"\x03" + handle.to_bytes(8, "big")).hex()
        if len(chunks) != 1 or chunks[0]["rawHex"] != expected:
            raise ValueError(f"{version} physical layer handle mismatch")
        patched = patched_dir / "seed.dwg"
        patch_layer_bytes(source, patched, proof, appid, handle)
        for directory, out in ((source_dir, reopen), (patched_dir, patched_reopen)):
            run(args.oda, str(directory), str(out), "ACAD2018", "DXF", "0", "1", "*.dwg")
        original_dxf, patched_dxf = reopen / "seed.dxf", patched_reopen / "seed.dxf"
        original = ezdxf.readfile(original_dxf)
        mutated = ezdxf.readfile(patched_dxf)
        original_tags = [(v.code, v.value) for v in original.rootdict.get("TEST_DICT").get_xdata("LAYER_TEST")]
        patched_tags = [(v.code, v.value) for v in mutated.rootdict.get("TEST_DICT").get_xdata("LAYER_TEST")]
        if original_tags != [(1003, "PROBE_LAYER")] or patched_tags:
            raise ValueError(f"{version} reopen semantics differ from witness")
        receipt["versions"][version] = {"dwgSha256": digest(source),
            "frameSha256": proof["wire"]["frameSha256"], "layerEedHex": expected,
            "reopenedDxfSha256": digest(original_dxf), "reopenedLayer": original_tags[0][1],
            "patchedDwgSha256": digest(patched), "patchedReopenedDxfSha256": digest(patched_dxf),
            "patchedLayerTags": patched_tags, "frameCrcValid": proof["crcValid"]}
    print(json.dumps(receipt, indent=2))


if __name__ == "__main__":
    main()
