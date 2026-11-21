#!/usr/bin/env python3
"""Generate scratch DWGs and verify EED-layer wire order with ODA reopen.

Requires the built dwg_eed_layer_write_tests executable, LibreDWG dwgread,
ezdxf, and an ODA File Converter with access to the desktop session. Drawings
and peer output stay in a new scratch directory, never in the repository.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import tempfile

from generate_ac1012_dictionary_witness import Bits, crc16, executable_identity


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(*args: str) -> subprocess.CompletedProcess:
    result = subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False)
    if result.returncode:
        raise RuntimeError(f"{args[0]} failed: {result.stderr[-1200:]!r}")
    return result


def record_frame(dwgread: str, path: Path, handle: int, object_type: int) -> dict:
    trace = run(dwgread, "-v4", str(path))
    blocks = re.split(r"(?=Object number:)", (trace.stdout + trace.stderr).decode(errors="replace"))
    pattern = r"\bhandle: 0\.\d+\." + format(handle, "X") + r"\s"
    candidates = [block for block in blocks if re.search(pattern, block)]
    if len(candidates) != 1:
        raise ValueError("record source frame is missing or ambiguous")
    match = re.search(r"Size: (\d+) \[MS\], Type: " + str(object_type)
                      + r" \[BS\], Address: (\d+)", candidates[0])
    if not match:
        raise ValueError("record source frame location missing")
    size, address = map(int, match.groups())
    source = path.read_bytes()
    if address < 2 or address + size + 2 > len(source) or source[address-2:address] != size.to_bytes(2, "little"):
        raise ValueError("record frame envelope invalid")
    if int.from_bytes(source[address+size:address+size+2], "little") != crc16(source[address-2:address+size]):
        raise ValueError("record frame CRC mismatch")
    frame = source[address:address+size]
    bits = Bits(frame)
    if bits.bs() != object_type:
        raise ValueError("record frame type mismatch")
    body_end = bits.little(4)
    if bits.handle(0, own=True)["value"] != handle:
        raise ValueError("record frame self handle mismatch")
    size_eed = bits.bs()
    if size_eed == 0:
        payload = b""
    else:
        appid = bits.handle(5)["value"]
        payload = bytes(bits.take(8) for _ in range(size_eed))
        if appid != 0x14 or bits.bs() != 0:
            raise ValueError("record EED APPID or terminator mismatch")
    return {"frameSha256": hashlib.sha256(frame).hexdigest(), "frameOffset": address,
            "frameSize": size, "bodyEndBit": body_end, "frameCrcValid": True,
            "eedHex": payload.hex()}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--candidate-test", required=True)
    parser.add_argument("--oda", required=True)
    parser.add_argument("--dwgread", default="dwgread")
    parser.add_argument("--output-dir", type=Path, required=True)
    args = parser.parse_args()
    import ezdxf

    args.output_dir.mkdir(parents=True, exist_ok=True)
    root = Path(tempfile.mkdtemp(prefix="dwg-eed-layer-writer-", dir=args.output_dir))
    receipt = {"oda": executable_identity(args.oda), "dwgread": executable_identity(args.dwgread),
               "ezdxfVersion": ezdxf.__version__, "profiles": {}}
    expected = {"big": "030000000000000012", "little": "031200000000000000",
                "raw": "030807060504030201", "none": ""}
    for mode in ("big", "little", "none", "raw", "named"):
        source_dir, reopened = root / (mode + "-source"), root / (mode + "-reopened")
        source_dir.mkdir(); reopened.mkdir()
        drawing = source_dir / "seed.dwg"
        run(args.candidate_test, "--emit", str(drawing), mode)
        model_path = source_dir / "peer.json"
        run(args.dwgread, "-O", "JSON", "-o", str(model_path), str(drawing))
        model = json.loads(model_path.read_text())
        tables = [record for record in model["OBJECTS"]
                  if record.get("object") == "LAYER" and record.get("name") == "EED_TABLE"]
        if len(tables) != 1:
            raise ValueError(f"{mode} table identity missing or ambiguous")
        table_handle = tables[0]["handle"][-1]
        frame = record_frame(args.dwgread, drawing, 0x130, 19)
        table_frame = record_frame(args.dwgread, drawing, table_handle, 51)
        if mode == "named":
            if table_handle <= 0xff:
                raise ValueError("named target is not a multibyte handle")
            expected[mode] = (bytes([3]) + table_handle.to_bytes(8, "big")).hex()
        table_expected = expected["big"] if mode == "raw" else expected[mode]
        if frame["eedHex"] != expected[mode] or table_frame["eedHex"] != table_expected:
            raise ValueError(f"{mode} EED bytes mismatch: {frame['eedHex']}, {table_frame['eedHex']}")
        item = {"dwgSha256": sha(drawing), "lineFrame": frame,
                "tableHandle": table_handle, "tableFrame": table_frame}
        if mode != "raw":
            run(args.oda, str(source_dir), str(reopened), "ACAD2013", "DXF", "0", "1", "*.dwg")
            dxf = reopened / "seed.dxf"
            error = reopened / "seed.dxf.err"
            if not dxf.is_file() or error.exists():
                raise ValueError(f"{mode} ODA reopen failed: {error.read_text() if error.exists() else 'no DXF'}")
            document = ezdxf.readfile(dxf)
            lines = list(document.modelspace().query("LINE"))
            if len(lines) != 1 or tuple(lines[0].dxf.start) != (1.0, 2.0, 3.0) \
                    or tuple(lines[0].dxf.end) != (4.0, 5.0, 6.0):
                raise ValueError(f"{mode} LINE geometry mismatch")
            values = ([(item.code, item.value) for item in lines[0].get_xdata("ACAD")]
                      if lines[0].has_xdata("ACAD") else [])
            expected_values = ([(1003, "EED_TABLE")] if mode == "named"
                               else [(1003, "0")] if mode == "big" else [])
            if values != expected_values:
                raise ValueError(f"{mode} ODA LINE EED mismatch: {values!r}")
            layer = document.layers.get("EED_TABLE")
            table_values = ([(tag.code, tag.value) for tag in layer.get_xdata("ACAD")]
                            if layer.has_xdata("ACAD") else [])
            if table_values != expected_values:
                raise ValueError(f"{mode} ODA LAYER EED mismatch: {table_values!r}")
            item.update({"dxfSha256": sha(dxf), "lineXdata": values, "tableXdata": table_values,
                         "lineGeometryMatched": True})
        receipt["profiles"][mode] = item
    print(json.dumps(receipt, indent=2))


if __name__ == "__main__":
    main()
