#!/usr/bin/env python3
"""Check the diagnostic RTEXT successor; this is not a qualification gate."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "metadata/ac1024-candidate-fields-v3-experiment.json"


def require(condition, message):
    if not condition:
        raise ValueError(message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def selected(document, expected_handle, expected):
    records = [value for value in document["OBJECTS"]
               if value.get("entity") == "RTEXT"]
    require(len(records) == 1, "expected one independent RTEXT record")
    entity = records[0]
    require(entity.get("handle", [])[-1:] == [expected_handle], "RTEXT handle mismatch")
    require(entity.get("text_value") == expected["text"], "RTEXT text mismatch")
    require(entity.get("pt") == expected["insertion"], "RTEXT insertion mismatch")
    rotation = entity.get("rotation")
    require(isinstance(rotation, (float, int)) and math.isfinite(rotation), "invalid rotation")
    delta = abs(math.degrees(rotation) - expected["rotationDegrees"])
    require(delta <= expected["rotationToleranceDegrees"], "rotation outside tolerance")
    seed = document["HEADER"]["HANDSEED"][-1]
    require(seed == expected_handle + 1, "unexpected HEADER HANDSEED")
    return {"handle": expected_handle, "insertion": entity["pt"],
            "rotationRadians": rotation, "rotationErrorDegrees": delta, "handseed": seed}


def self_test():
    expected = {"insertion": [90., 91., 0.], "rotationDegrees": 15.,
                "rotationToleranceDegrees": 1e-12, "text": "LOCAL_RTEXT"}
    record = {"entity": "RTEXT", "handle": [0, 1, 48], "pt": [90., 91., 0.],
              "rotation": math.pi / 12, "text_value": "LOCAL_RTEXT"}
    good = {"OBJECTS": [record], "HEADER": {"HANDSEED": [4, 1, 49, 49]}}
    selected(good, 48, expected)
    for field, bad in (("pt", [90., 91., 1.]), ("rotation", 0.), ("rotation", float("nan"))):
        changed = dict(record)
        changed[field] = bad
        try:
            selected({"OBJECTS": [changed], "HEADER": good["HEADER"]}, 48, expected)
        except ValueError:
            continue
        raise ValueError(f"negative {field} passed")
    print("RTEXT successor verifier self-test: PASS")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--work-dir", type=Path)
    parser.add_argument("--writer", type=Path)
    parser.add_argument("--oracle", type=Path)
    parser.add_argument("--oracle-library", type=Path)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        self_test()
        return
    require(all((args.work_dir, args.writer, args.oracle, args.oracle_library)),
            "work directory, writer, oracle and oracle library are required")
    manifest = json.loads(MANIFEST.read_bytes())
    require(manifest["status"] == "DIAGNOSTIC_NOT_QUALIFIED" and not manifest["promotesSupport"],
            "unexpected qualification status")
    author = manifest["localAuthor"]
    for path, digest in ((ROOT / author["source"], author["sourceSha256"]),
                         (ROOT / author["adapterSource"], author["adapterSourceSha256"]),
                         (args.writer, author["observedExecutableSha256"]),
                         (args.oracle, manifest["oracle"]["executableSha256"]),
                         (args.oracle_library, manifest["oracle"]["librarySha256"])):
        require(sha(path.read_bytes()) == digest, f"source or artifact changed: {path}")
    outcomes = []
    for control in manifest["controls"]:
        stem = args.work_dir / ("p9-rtext-v3-" + control["name"])
        drawing = stem.with_suffix(".dwg").read_bytes()
        output = stem.with_suffix(".json").read_bytes()
        log = stem.with_suffix(".log").read_text(errors="replace")
        require(sha(drawing) == control["dwgSha256"] and sha(output) == control["oracleJsonSha256"],
                "drawing or independent JSON changed")
        warnings = [line for line in log.splitlines() if line.startswith("Warning:")]
        errors = [line for line in log.splitlines() if line.startswith("ERROR:")]
        require(len(warnings) == control["warningCount"] and len(errors) == control["errorCount"],
                "diagnostic count changed")
        value = selected(json.loads(output), control["entityHandle"], manifest["selectedFields"])
        value.update({"name": control["name"], "warnings": warnings, "errors": len(errors)})
        outcomes.append(value)
    print(json.dumps({"status": manifest["status"], "promotesSupport": False,
                      "observations": outcomes}, sort_keys=True))


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, KeyError, IndexError, json.JSONDecodeError) as exc:
        print(f"RTEXT successor verifier failed: {exc}", file=sys.stderr)
        sys.exit(1)
