#!/usr/bin/env python3
"""Author a DXF witness for external AC1021 BLOCK_CONTROL conversion.

The source is the repository's locally authored ellipse control. Output is
scratch evidence; the converted DWG is not a test fixture.
"""
import argparse
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument("output", type=Path)
args = parser.parse_args()
source = Path(__file__).resolve().parents[1] / "tests/fixtures/dxf/ac1015_ellipse_freecad_control.dxf"
lines = source.read_text(encoding="ascii").splitlines()
if len(lines) % 2:
    raise SystemExit("unexpected DXF pair framing")
pairs = list(zip(lines[::2], lines[1::2]))
if any(code.strip() in {"5", "105"} and value.strip().upper() == "F0" for code, value in pairs):
    raise SystemExit("chosen table handle F0 already exists")
indices = [i for i in range(len(pairs) - 1)
           if pairs[i][0].strip() == "0" and pairs[i][1].strip() == "TABLE"
           and pairs[i + 1][0].strip() == "2" and pairs[i + 1][1].strip() == "BLOCK_RECORD"]
if len(indices) != 1:
    raise SystemExit("expected exactly one BLOCK_RECORD table")
if not any(code.strip() == "5" and value.strip().upper() == "33" for code, value in pairs):
    raise SystemExit("expected existing NOD handle 33")
at = indices[0] + 2
pairs[at:at] = [("5", "F0"), ("102", "{ACAD_REACTORS"), ("330", "33"), ("102", "}")]
args.output.parent.mkdir(parents=True, exist_ok=True)
args.output.write_text("".join(f"{code.strip()}\n{value}\n" for code, value in pairs), encoding="ascii")
