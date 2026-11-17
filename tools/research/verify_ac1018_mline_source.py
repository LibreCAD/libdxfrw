#!/usr/bin/env python3
"""Compare one AutoCAD AC1018 MLINE source through libdxfrw and LibreDWG.

This is a scratch-source qualification aid. It never copies or adds the DWG to
the repository. The output binds local executable/source hashes and records
only the bounded MLINE/MLINESTYLE fields compared by this pass.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import re
import subprocess
import tempfile
from pathlib import Path


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def close_number(a: float, b: float, tolerance: float = 1e-10) -> bool:
    return math.isfinite(a) and math.isfinite(b) and abs(a - b) <= tolerance * max(1.0, abs(a), abs(b))


def same_vector(a: list[float], b: list[float]) -> bool:
    return len(a) == len(b) and all(close_number(float(x), float(y)) for x, y in zip(a, b))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--libdxfrw-test", required=True, type=Path)
    parser.add_argument("--libredwg-reader", required=True, type=Path)
    parser.add_argument("--source-dwg", required=True, type=Path)
    parser.add_argument("--property-dump", required=True, type=Path)
    parser.add_argument("--output-json", required=True, type=Path)
    args = parser.parse_args()

    test_exe = args.libdxfrw_test.expanduser().resolve()
    peer_exe = args.libredwg_reader.expanduser().resolve()
    dwg_path = args.source_dwg.expanduser().resolve()
    dump_path = args.property_dump.expanduser().resolve()
    for path in (test_exe, peer_exe, dwg_path, dump_path):
        if not path.is_file():
            parser.error(f"required input does not exist: {path}")

    peer_version_run = subprocess.run(
        [str(peer_exe), "--version"],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )
    peer_version = peer_version_run.stdout.decode("utf-8", "replace").strip()
    if peer_version_run.returncode or not peer_version:
        raise SystemExit("LibreDWG version query failed: " + peer_version[-1000:])

    dwg_bytes = dwg_path.read_bytes()
    dump_bytes = dump_path.read_bytes()
    dwg_sha = digest(dwg_bytes)
    dump_sha = digest(dump_bytes)
    if dwg_bytes[:6] != b"AC1018":
        parser.error("source does not have the AC1018 signature")

    local_run = subprocess.run(
        [str(test_exe), "--mline-ac1018-source-probe", str(dwg_path)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    if local_run.returncode:
        raise SystemExit("libdxfrw source probe failed: " + local_run.stderr.decode("utf-8", "replace")[-2000:])
    local = json.loads(local_run.stdout)

    with tempfile.TemporaryDirectory(prefix="libdxfrw-ac1018-mline-peer-") as temp:
        peer_json_path = Path(temp) / "peer-minjson.json"
        peer_run = subprocess.run(
            [str(peer_exe), "-O", "minJSON", str(dwg_path), "-o", str(peer_json_path)],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            check=False,
        )
        if peer_run.returncode or not peer_json_path.is_file():
            raise SystemExit("LibreDWG read failed: " + peer_run.stdout.decode("utf-8", "replace")[-2000:])
        peer_bytes = peer_json_path.read_bytes()
        peer = json.loads(peer_bytes)
        peer_log = peer_run.stdout.decode("utf-8", "replace")

    objects = peer.get("OBJECTS", [])
    peer_mlines = [row for row in objects if row.get("entity") == "MLINE"]
    peer_styles = [row for row in objects if row.get("object") == "MLINESTYLE"]
    if len(peer_mlines) != 1 or len(peer_styles) != 1:
        raise SystemExit("expected one peer MLINE and one peer MLINESTYLE")
    peer_mline, peer_style = peer_mlines[0], peer_styles[0]
    local_mline, local_style = local["mline"], local["style"]
    checks: list[dict] = []

    def check(name: str, passed: bool) -> None:
        checks.append({"name": name, "passed": bool(passed)})

    def handle(value: object) -> int:
        if isinstance(value, list):
            return int(value[-1])
        return int(value)

    check("version", local.get("version") == "AC1018" and peer.get("FILEHEADER", {}).get("version") == "AC1018")
    check("mline-count", local.get("mlineCount") == 1 and len(peer_mlines) == 1)
    check("mline-handle", int(local_mline["handle"]) == handle(peer_mline["handle"]) == 0x1BD)
    check("style-handle-reference", int(local_mline["styleHandle"]) == handle(peer_mline["mlinestyle"]) == 0x18)
    check("layer-name", local_mline.get("layer") == "0")
    for field, peer_field in (("scale", "scale"), ("justification", "justification"), ("flags", "flags")):
        check("mline-" + field, close_number(float(local_mline[field]), float(peer_mline[peer_field])))
    check("base-point", same_vector(local_mline["basePoint"], peer_mline["base_point"]))
    check("extrusion", same_vector(local_mline["extrusion"], peer_mline["extrusion"]))

    local_vertices = local_mline["vertices"]
    peer_vertices = peer_mline["verts"]
    check("vertex-count", len(local_vertices) == len(peer_vertices) == 6)
    if len(local_vertices) == len(peer_vertices):
        for index, (lhs, rhs) in enumerate(zip(local_vertices, peer_vertices)):
            check(f"vertex-{index}-position", same_vector(lhs["position"], rhs["vertex"]))
            check(f"vertex-{index}-direction", same_vector(lhs["vertexDirection"], rhs["vertex_direction"]))
            check(f"vertex-{index}-miter", same_vector(lhs["miterDirection"], rhs["miter_direction"]))
            left_lines, right_lines = lhs["lines"], rhs["lines"]
            check(f"vertex-{index}-line-count", len(left_lines) == len(right_lines) == 2)
            if len(left_lines) == len(right_lines):
                for line_index, (left_line, right_line) in enumerate(zip(left_lines, right_lines)):
                    check(f"vertex-{index}-line-{line_index}-segments",
                          len(left_line["segmentParameters"]) == len(right_line["segparms"])
                          and all(close_number(float(a), float(b)) for a, b in zip(left_line["segmentParameters"], right_line["segparms"])))
                    check(f"vertex-{index}-line-{line_index}-fills",
                          len(left_line["areaFillParameters"]) == len(right_line["areafillparms"])
                          and all(close_number(float(a), float(b)) for a, b in zip(left_line["areaFillParameters"], right_line["areafillparms"])))

    check("style-count", local_style["handle"] == handle(peer_style["handle"]) == 0x18)
    check("style-owner", int(local_style["owner"]) == handle(peer_style["ownerhandle"]) == 0x17)
    check("style-name", local_style["name"].upper() == peer_style["name"].upper() == "STANDARD")
    check("style-flags", int(local_style["flags"]) == int(peer_style["flag"]) == 0)
    check("style-start-angle-degrees", close_number(float(local_style["startAngle"]), math.degrees(float(peer_style["start_angle"]))))
    check("style-end-angle-degrees", close_number(float(local_style["endAngle"]), math.degrees(float(peer_style["end_angle"]))))
    local_elements = local_style["elements"]
    peer_elements = peer_style["lines"]
    check("style-element-count", len(local_elements) == len(peer_elements) == 2)
    if len(local_elements) == len(peer_elements):
        for index, (lhs, rhs) in enumerate(zip(local_elements, peer_elements)):
            check(f"style-element-{index}-offset", close_number(float(lhs["offset"]), float(rhs["offset"])))
            peer_linetype_index = rhs.get("lt.index", rhs.get("lt_index"))
            check(f"style-element-{index}-linetype",
                  peer_linetype_index is not None
                  and int(lhs["linetypeIndex"]) == int(peer_linetype_index) == 32767)

    property_text = "\n".join(
        line.strip() for line in dump_bytes.decode("utf-8", "replace").splitlines()
    )
    properties = {
        "handle": re.search(r'^;\s*Handle \(RO\) = "([0-9A-F]+)"$', property_text, re.MULTILINE),
        "scale": re.search(r"^;\s*MLineScale = ([0-9.eE+-]+)$", property_text, re.MULTILINE),
        "justification": re.search(r"^;\s*Justification = (-?[0-9]+)$", property_text, re.MULTILINE),
        "style": re.search(r'^;\s*StyleName \(RO\) = "([^"]+)"$', property_text, re.MULTILINE),
        "owner": re.search(r"^;\s*OwnerID \(RO\) = ([0-9]+)$", property_text, re.MULTILINE),
        "coordinates": re.search(r"^;\s*Coordinates = \(([^)]*)\)$", property_text, re.MULTILINE),
    }
    check("acad-property-handle", properties["handle"] is not None and int(properties["handle"].group(1), 16) == int(local_mline["handle"]))
    check("acad-property-scale", properties["scale"] is not None and close_number(float(properties["scale"].group(1)), float(local_mline["scale"])))
    check("acad-property-justification", properties["justification"] is not None and int(properties["justification"].group(1)) == int(local_mline["justification"]))
    check("acad-property-style", properties["style"] is not None and properties["style"].group(1).upper() == local_style["name"].upper())
    check("acad-property-owner-present", properties["owner"] is not None)
    if properties["coordinates"] is not None:
        coord_values = [float(token) for token in re.findall(r"[-+]?(?:\d+\.?\d*|\.\d+)(?:[eE][-+]?\d+)?", properties["coordinates"].group(1))]
        check("acad-property-coordinate-prefix", len(coord_values) >= 6 and
              all(close_number(coord_values[i], float(local_vertices[i // 3]["position"][i % 3]), 1e-5) for i in range(6)))
    else:
        check("acad-property-coordinate-prefix", False)

    output = {
        "schema": "libdxfrw.research-receipt.v1",
        "id": "p6-ac1018-mline-independent-source-semantics-2026-11-16",
        "status": "PASS" if all(row["passed"] for row in checks) else "FAILED",
        "supportClaimEffect": "none",
        "source": {
            "path": str(dwg_path),
            "sha256": dwg_sha,
            "signature": "AC1018",
            "producerEvidence": "LibreDWG's paired test-data/2004/Multiline.txt is an AutoCAD VLA property dump; fields and entity handle are checked against the DWG.",
            "repository": "https://github.com/LibreDWG/libredwg",
            "repositoryCommit": "5ec495e1b75f49b659b6da2f974c9388d63c409f",
            "repositoryPath": "test/test-data/2004/Multiline.dwg",
            "repositoryBlob": "81dc671a57dab755de5584fc607f445f21dfc244",
            "notCommitted": True,
        },
        "propertyDump": {"path": str(dump_path), "sha256": dump_sha, "repositoryPath": "test/test-data/2004/Multiline.txt"},
        "implementation": {"path": str(test_exe), "sha256": digest(test_exe.read_bytes()), "probe": "--mline-ac1018-source-probe"},
        "independentReader": {
            "path": str(peer_exe),
            "sha256": digest(peer_exe.read_bytes()),
            "versionCommand": [str(peer_exe), "--version"],
            "versionOutput": peer_version,
            "resultSha256": digest(peer_bytes),
            "result": "LibreDWG minJSON read returned success",
        },
        "coverage": {
            "mlineHandle": "0x1BD",
            "mlineStyleHandle": "0x18",
            "vertexCount": len(local_vertices),
            "elementCount": len(local_elements),
            "numericToleranceRelative": 1e-10,
            "checksPassed": sum(row["passed"] for row in checks),
            "checksTotal": len(checks),
            "checks": checks,
        },
        "limitations": [
            "This compares decoded source fields; it does not yet qualify ASCII/Binary writer output, graph closure, or LibreCAD appearance.",
            "The AutoCAD property dump exposes only a rounded coordinate prefix and database OwnerID, not a DWG handle mapping for the owner.",
            "The external DWG and text dump are scratch-only and are not repository fixtures.",
        ],
    }
    out_path = args.output_json.expanduser().resolve()
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(output, indent=2, sort_keys=True) + "\n")
    print(json.dumps({"status": output["status"], "checks": f"{output['coverage']['checksPassed']}/{output['coverage']['checksTotal']}", "output": str(out_path)}, sort_keys=True))
    return 0 if output["status"] == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
