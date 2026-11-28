#!/usr/bin/env python3
"""Generate scratch R13 ASCII/binary LTYPE inputs for LibreCAD's optional test.

Requires ezdxf and an installed ODA File Converter. The output directory must
not already exist. Generated drawings stay outside the repository.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
from pathlib import Path


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True,
                        help="new scratch directory for generated drawings")
    parser.add_argument("--odafc", type=Path, required=True,
                        help="path to the installed ODA File Converter executable")
    args = parser.parse_args()

    import ezdxf  # optional research dependency; never vendored

    converter = args.odafc.resolve(strict=True)
    if not converter.is_file():
        parser.error("--odafc must name a converter executable")
    root = args.output.resolve()
    repository = Path(__file__).resolve().parents[2]
    if root == repository or repository in root.parents:
        parser.error("--output must be outside the repository")
    root.mkdir(parents=True, exist_ok=False)
    input_dir, ascii_dir, binary_dir = (root / name for name in ("input", "ascii", "binary"))
    for directory in (input_dir, ascii_dir, binary_dir):
        directory.mkdir()

    doc = ezdxf.new("R2010")
    doc.linetypes.new("VENDOR_BINARY", dxfattribs={
        "description": "Binary marks", "pattern": [15.0, 10.0, -5.0]})
    doc.modelspace().add_line((0, 0), (20, 0),
                              dxfattribs={"linetype": "VENDOR_BINARY"})
    source = input_dir / "source.dxf"
    doc.saveas(source)
    for target, kind in ((ascii_dir, "DXF"), (binary_dir, "DXB")):
        subprocess.run([str(converter), str(input_dir), str(target),
                        "ACAD13", kind, "0", "1"], check=True)

    ascii_file, binary_file = ascii_dir / "source.dxf", binary_dir / "source.dxf"
    ascii_bytes, binary_bytes = ascii_file.read_bytes(), binary_file.read_bytes()
    if ascii_bytes.startswith(b"AutoCAD Binary DXF"):
        raise ValueError("ASCII conversion unexpectedly produced binary DXF")
    groups = [line.strip() for line in ascii_bytes.splitlines()]
    version_pairs = list(zip(groups[::2], groups[1::2]))
    if not any(version_pairs[i] == (b"9", b"$ACADVER") and
               version_pairs[i + 1] == (b"1", b"AC1012")
               for i in range(len(version_pairs) - 1)):
        raise ValueError("ASCII conversion is not AC1012")
    if not binary_bytes.startswith(b"AutoCAD Binary DXF\r\n\x1a\0") or \
       b"\x09\0$ACADVER\0\x01\0AC1012\0" not in binary_bytes:
        raise ValueError("binary conversion is not AC1012 binary DXF")

    manifest = {"ezdxfVersion": ezdxf.__version__, "converter": str(converter),
                "files": {name: {"path": str(path), "sha256": sha256(path)}
                          for name, path in (("source", source), ("ascii", ascii_file),
                                             ("binary", binary_file))}}
    (root / "receipt.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(json.dumps(manifest, indent=2))
    print("Set LIBRECAD_R13_LTYPE_ASCII and LIBRECAD_R13_LTYPE_BINARY to the generated paths.")


if __name__ == "__main__":
    main()
