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


MATRIX_PATH = Path(__file__).resolve().parents[1] / (
    "metadata/local-dwg-oracle-matrix-v1.json"
)
_MATRIX = json.loads(MATRIX_PATH.read_text(encoding="utf-8"))
VERSIONS = {
    entry["marker"]: entry["acadver"] for entry in _MATRIX["versions"]
}
EXPECTED_ENTITIES = tuple(_MATRIX["expectedEntities"])
ENTITY_COUNT_BOUNDS = {
    name: (bound["min"], bound["max"])
    for name, bound in _MATRIX.get("entityCountBounds", {}).items()
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
    missing_entities = [
        name for name in EXPECTED_ENTITIES
        if entity_counts.get(name, 0) < ENTITY_COUNT_BOUNDS.get(name, (1, 1))[0]
    ]
    count_violations = [
        name for name in EXPECTED_ENTITIES
        if not (
            ENTITY_COUNT_BOUNDS.get(name, (1, 1))[0]
            <= entity_counts.get(name, 0)
            <= ENTITY_COUNT_BOUNDS.get(name, (1, 1))[1]
        )
    ]
    simple_entities_ok = not missing_entities and not count_violations
    return {
        "version": expected_version,
        "oracleVersion": acadver,
        "versionMatch": acadver == expected_version,
        "lineGeometryMatch": geometry_ok,
        "simpleEntitySetMatch": simple_entities_ok,
        "missingEntities": missing_entities,
        "entityCountViolations": count_violations,
        "entityCounts": dict(sorted(entity_counts.items())),
        "qualified": acadver == expected_version and geometry_ok and simple_entities_ok,
    }


def _json_entity_names(value: object) -> list[str]:
    names: list[str] = []
    if isinstance(value, dict):
        entity = value.get("entity")
        if isinstance(entity, str):
            # LibreDWG uses a subtype-qualified name for legacy polylines;
            # normalize it to the DXF entity name used by the other oracle.
            names.append("POLYLINE" if entity == "POLYLINE_2D" else entity)
        for child in value.values():
            names.extend(_json_entity_names(child))
    elif isinstance(value, list):
        for child in value:
            names.extend(_json_entity_names(child))
    return names


def validate_json_oracle_output(path: Path, expected_version: str) -> dict[str, object]:
    document = json.loads(path.read_text(encoding="utf-8"))
    fileheader = document.get("FILEHEADER", {})
    oracle_version = fileheader.get("version", "") if isinstance(fileheader, dict) else ""
    entity_counts = Counter(_json_entity_names(document))
    missing_entities = [
        name for name in EXPECTED_ENTITIES
        if entity_counts.get(name, 0) < ENTITY_COUNT_BOUNDS.get(name, (1, 1))[0]
    ]
    count_violations = [
        name for name in EXPECTED_ENTITIES
        if not (
            ENTITY_COUNT_BOUNDS.get(name, (1, 1))[0]
            <= entity_counts.get(name, 0)
            <= ENTITY_COUNT_BOUNDS.get(name, (1, 1))[1]
        )
    ]
    return {
        "jsonOracleVersion": oracle_version,
        "jsonVersionMatch": oracle_version == expected_version,
        "jsonEntitySetMatch": not missing_entities,
        "jsonMissingEntities": missing_entities,
        "jsonEntityCountViolations": count_violations,
        "jsonEntityCounts": dict(sorted(entity_counts.items())),
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
        entity_pairs = "".join(
            "0\n%s\n" % name for name in EXPECTED_ENTITIES if name != "LINE"
        )
        path.write_text(
            "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1015\n"
            "0\nENDSEC\n0\nSECTION\n2\nENTITIES\n0\nLINE\n"
            "10\n1.0\n20\n2.0\n30\n3.0\n11\n4.0\n21\n5.0\n31\n6.0\n"
            + entity_pairs + "0\nENDSEC\n0\nEOF\n",
            encoding="utf-8",
        )
        result = validate_oracle_output(path, "AC1015")
        if not result["qualified"]:
            raise AssertionError("synthetic oracle output was not recognized")
        duplicate_path = Path(directory) / "duplicate.dxf"
        duplicate_path.write_text(
            path.read_text(encoding="utf-8").replace(
                "0\nENDSEC\n0\nEOF\n",
                "0\nPOLYLINE\n0\nPOLYLINE\n0\nENDSEC\n0\nEOF\n",
            ),
            encoding="utf-8",
        )
        duplicate_result = validate_oracle_output(duplicate_path, "AC1015")
        if "POLYLINE" not in duplicate_result["entityCountViolations"]:
            raise AssertionError("entity count bound violation was not detected")
        json_path = Path(directory) / "sample.json"
        json_path.write_text(
            json.dumps({
                "FILEHEADER": {"version": "AC1015"},
                "OBJECTS": [{"entity": name} for name in EXPECTED_ENTITIES],
            }),
            encoding="utf-8",
        )
        json_result = validate_json_oracle_output(json_path, "AC1015")
        if not json_result["jsonVersionMatch"] or not json_result["jsonEntitySetMatch"]:
            raise AssertionError("synthetic JSON oracle output was not recognized")
    print("local DWG oracle advisory self-test: PASS")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--writer")
    parser.add_argument("--oracle")
    parser.add_argument("--json-oracle")
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
                            if args.json_oracle:
                                json_output = root / (drawing.stem + ".json")
                                try:
                                    json_oracle = run(
                                        [args.json_oracle, "-O", "JSON", "-o",
                                         str(json_output), str(drawing)],
                                        args.timeout,
                                    )
                                except subprocess.TimeoutExpired:
                                    record["jsonOracleStatus"] = "timeout"
                                else:
                                    if (json_oracle.returncode != 0
                                            or not json_output.exists()):
                                        record["jsonOracleStatus"] = "failed"
                                    else:
                                        record.update(validate_json_oracle_output(
                                            json_output, expected))
                                        record["jsonOracleStatus"] = "qualified"
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
