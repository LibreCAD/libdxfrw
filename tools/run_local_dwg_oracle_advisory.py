#!/usr/bin/env python3
"""Run the local DWG writer probe through an independent DXF oracle.

The writer probe creates all DWG inputs locally from scratch.  This helper
keeps both the DWG and oracle DXF output in a temporary directory and emits
only version/status/hash metadata, so it is safe for advisory qualification
without adding drawing payloads to the repository.
"""

from __future__ import annotations

import argparse
from collections import Counter
import hashlib
import json
import subprocess
import tempfile
from pathlib import Path


VERSIONS = {
    "13": "AC1015",
    "14": "AC1018",
    "15": "AC1021",
    "16": "AC1024",
    "17": "AC1027",
    "18": "AC1032",
}


def digest(path: Path) -> dict[str, object]:
    hasher = hashlib.sha256()
    size = 0
    with path.open("rb") as stream:
        while True:
            chunk = stream.read(1024 * 1024)
            if not chunk:
                break
            size += len(chunk)
            hasher.update(chunk)
    return {"size": size, "sha256": hasher.hexdigest()}


def parse_pairs(text: str) -> list[tuple[str, str]]:
    lines = text.splitlines()
    if len(lines) % 2:
        raise ValueError("DXF oracle output has an odd number of lines")
    pairs = []
    for index in range(0, len(lines), 2):
        code = lines[index].strip()
        value = lines[index + 1].strip()
        if not code:
            raise ValueError("DXF oracle output contains an empty group code")
        pairs.append((code, value))
    return pairs


def validate_oracle_output(path: Path, expected_version: str) -> dict[str, object]:
    pairs = parse_pairs(path.read_text(encoding="utf-8"))
    acadver = next(
        (value for code, value in pairs if code == "1" and value.startswith("AC")),
        "",
    )
    line_start = None
    line_end = None
    for index, (code, value) in enumerate(pairs):
        if code == "0" and value == "LINE":
            line_start = index
            break
    if line_start is not None:
        tail = []
        for code, value in pairs[line_start + 1 :]:
            if code == "0":
                break
            tail.append((code, value))
        values = {code: value for code, value in tail}
        line_end = values
    expected = {"10": "1.0", "20": "2.0", "30": "3.0",
                "11": "4.0", "21": "5.0", "31": "6.0"}
    geometry_ok = line_end is not None and all(
        line_end.get(code) == value for code, value in expected.items()
    )
    entity_counts = Counter(value for code, value in pairs if code == "0")
    simple_entities_ok = all(
        entity_counts.get(name, 0) == 1
        for name in ("LINE", "POINT", "CIRCLE", "ARC", "LWPOLYLINE")
    )
    return {
        "version": expected_version,
        "oracleVersion": acadver,
        "versionMatch": acadver == expected_version,
        "lineGeometryMatch": geometry_ok,
        "simpleEntitySetMatch": simple_entities_ok,
        "qualified": acadver == expected_version and geometry_ok and simple_entities_ok,
    }


def run(command: list[str], timeout: float) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        command,
        check=False,
        capture_output=True,
        text=True,
        timeout=timeout,
    )


def self_test() -> None:
    with tempfile.TemporaryDirectory(prefix="libdxfrw-oracle-selftest-") as directory:
        path = Path(directory) / "sample.dxf"
        path.write_text(
            "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1015\n"
            "0\nENDSEC\n0\nSECTION\n2\nENTITIES\n0\nLINE\n"
            "10\n1.0\n20\n2.0\n30\n3.0\n11\n4.0\n21\n5.0\n31\n6.0\n"
            "0\nPOINT\n0\nCIRCLE\n0\nARC\n0\nLWPOLYLINE\n"
            "0\nENDSEC\n0\nEOF\n",
            encoding="utf-8",
        )
        result = validate_oracle_output(path, "AC1015")
        if not result["qualified"]:
            raise AssertionError("synthetic oracle output was not recognized")
    print("local DWG oracle advisory self-test: PASS")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--writer")
    parser.add_argument("--oracle")
    parser.add_argument("--timeout", type=float, default=20.0)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        self_test()
        return 0
    if not args.writer or not args.oracle:
        parser.error("--writer and --oracle are required unless --self-test is used")

    records: list[dict[str, object]] = []
    with tempfile.TemporaryDirectory(prefix="libdxfrw-local-oracle-") as directory:
        root = Path(directory)
        try:
            writer = run([args.writer, "--keep-dir", str(root)], args.timeout)
        except subprocess.TimeoutExpired:
            payload = {"status": "writer-timeout", "records": []}
        else:
            if writer.returncode != 0:
                payload = {
                    "status": "writer-failed",
                    "writerExit": writer.returncode,
                    "records": [],
                }
            else:
                for drawing in sorted(root.glob("*.dwg")):
                    marker = drawing.stem.rsplit("-", 1)[-1]
                    expected = VERSIONS.get(marker)
                    if expected is None:
                        records.append({"input": drawing.name, "status": "unknown-version"})
                        continue
                    output = root / (drawing.stem + ".dxf")
                    record: dict[str, object] = {
                        "input": drawing.name,
                        "version": expected,
                        "inputDigest": digest(drawing),
                    }
                    try:
                        oracle = run(
                            [args.oracle, "-m", "-y", "-o", str(output), str(drawing)],
                            args.timeout,
                        )
                    except subprocess.TimeoutExpired:
                        record["status"] = "timeout"
                    else:
                        record["oracleExit"] = oracle.returncode
                        if oracle.returncode != 0 or not output.exists():
                            record["status"] = "failed"
                        else:
                            record.update(validate_oracle_output(output, expected))
                            record["outputDigest"] = digest(output)
                            record["status"] = (
                                "qualified" if record["qualified"] else "mismatch"
                            )
                    records.append(record)
                payload = {
                    "status": "complete",
                    "writer": args.writer,
                    "oracle": args.oracle,
                    "records": records,
                }
    if args.output:
        args.output.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n",
                               encoding="utf-8")
    print(json.dumps(payload, indent=2, sort_keys=True))
    return 0 if payload["status"] == "complete" and len(records) == len(VERSIONS) and all(
        record.get("status") == "qualified" for record in records
    ) else 1


if __name__ == "__main__":
    raise SystemExit(main())
