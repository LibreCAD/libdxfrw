#!/usr/bin/env python3
"""Compare eight selected MPOLYGON fields; advisory, not qualification."""
import argparse
import copy
import hashlib
import json
import math
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parent.parent
FIXTURE = ROOT / "tests/fixtures/dwg/mpolygon_solid.dwg"
FIXTURE_SHA = "66a171c55bc90fdd0764ad3556ec786503666a30a4473a637d168a01979ffc29"
PEER_SHA = "536eef295f1dfdc2e63b4f1cd2961905f98c55bbd1fe017ef3574c0f1345f1ff"
MAPPINGS = {
    "solid": ("is_solid_fill", True), "associative": ("is_associative", False),
    "name": ("name", "SOLID"), "hpattern": ("pattern_type", 0),
    "basePoint.z": ("elevation", 0.0), "extPoint.x": ("extrusion.0", 0.0),
    "extPoint.y": ("extrusion.1", 0.0), "extPoint.z": ("extrusion.2", 1.0),
}


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def selected(native, peer):
    if native.get("status", {}).get("operationSucceeded") is not True:
        raise ValueError("native read failed")
    records = [r for r in native["records"] if r.get("recordClass") == "MPOLYGON"]
    objects = [r for r in peer["OBJECTS"] if r.get("entity") == "MPOLYGON"]
    if len(records) != 1 or len(objects) != 1 or records[0].get("sourceHandle") != "2f":
        raise ValueError("MPOLYGON selection is not unique")
    fields = records[0]["fields"]
    if len({f["name"] for f in fields}) != len(fields):
        raise ValueError("duplicate native field")
    fields = {f["name"]: f["value"] for f in fields}
    obj = objects[0]
    if len(obj.get("handle", [])) < 3 or obj["handle"][2] != 47:
        raise ValueError("independent handle mismatch")
    observed = {}
    for field, (peer_field, expected) in MAPPINGS.items():
        value = fields[field]
        if peer_field.startswith("extrusion."):
            other = obj["extrusion"][int(peer_field[-1])]
        else:
            other = obj[peer_field]
        if isinstance(expected, bool):
            if type(value) is not bool or type(other) is not int or other not in (0, 1):
                raise ValueError("invalid boolean fields: " + field)
            match = value == expected and bool(other) == expected
        elif isinstance(expected, float):
            match = (type(value) in (int, float) and type(other) in (int, float)
                     and math.isfinite(value) and math.isfinite(other)
                     and math.isclose(value, expected, abs_tol=1e-12, rel_tol=1e-12)
                     and math.isclose(other, expected, abs_tol=1e-12, rel_tol=1e-12)
                     and math.isclose(value, other, abs_tol=1e-12, rel_tol=1e-12))
        else:
            match = type(value) is type(expected) and type(other) is type(expected) and value == other == expected
        if not match:
            raise ValueError("mismatch: " + field)
        observed[field] = {"native": value, "independent": other, "independentField": peer_field}
    return observed


def self_test():
    native = {"status": {"operationSucceeded": True}, "records": [{
        "recordClass": "MPOLYGON", "sourceHandle": "2f",
        "fields": [{"name": name, "value": value}
                   for name, (_, value) in MAPPINGS.items()]}]}
    peer = {"OBJECTS": [{"entity": "MPOLYGON", "handle": [0, 1, 47],
                         "is_solid_fill": 1, "is_associative": 0,
                         "name": "SOLID", "pattern_type": 0,
                         "elevation": 0.0, "extrusion": [0.0, 0.0, 1.0]}]}
    assert len(selected(native, peer)) == 8
    bad_peer = copy.deepcopy(peer)
    bad_peer["OBJECTS"][0]["is_solid_fill"] = 0
    duplicate = copy.deepcopy(native)
    duplicate["records"].append(copy.deepcopy(duplicate["records"][0]))
    bad_coordinate = copy.deepcopy(peer)
    bad_coordinate["OBJECTS"][0]["extrusion"][2] = float("nan")
    for a, b in ((native, bad_peer), (duplicate, peer), (native, bad_coordinate)):
        try:
            selected(a, b)
        except ValueError:
            continue
        raise AssertionError("invalid comparison was accepted")
    print("j284 mpolygon probe self-test: PASS")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--adapter", type=Path)
    parser.add_argument("--dwgread", type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    if args.self_test:
        self_test()
        return
    if not args.adapter or not args.dwgread or not args.output:
        parser.error("--adapter, --dwgread and --output are required")
    adapter, peer = args.adapter.resolve(strict=True), args.dwgread.resolve(strict=True)
    if digest(FIXTURE) != FIXTURE_SHA or digest(peer) != PEER_SHA:
        raise ValueError("fixture or independent reader hash mismatch")
    binary_hashes = {"adapter": digest(adapter), "dwgread": digest(peer)}
    commands = [[str(adapter), "--input", str(FIXTURE), "--facade", "dwgRW", "--direction", "read", "--json"],
                [str(peer), "-O", "JSON", str(FIXTURE)]]
    data = []
    for command in commands:
        runs = [subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                               timeout=10, check=True) for _ in range(2)]
        if runs[0].stdout != runs[1].stdout or runs[0].stderr != runs[1].stderr:
            raise ValueError("non-deterministic reader output")
        data.append((json.loads(runs[0].stdout), runs[0]))
    native, peer_result = data[0][0], data[1][0]
    if (native.get("input", {}).get("sha256") != FIXTURE_SHA
            or native.get("input", {}).get("detectedVersion") != "AC1032"):
        raise ValueError("native input identity/version mismatch")
    fields = selected(native, peer_result)
    if digest(FIXTURE) != FIXTURE_SHA or any(digest(p) != binary_hashes[n] for n, p in (("adapter", adapter), ("dwgread", peer))):
        raise ValueError("input or executable changed during run")
    obs = native["observations"]["inputRead"]
    report = {"schema": 1, "kind": "j284-mpolygon-peer-probe-v1", "advisory": True,
              "promotesSupportClaims": False, "fixtureSha256": FIXTURE_SHA,
              "independentJsonSha256": hashlib.sha256(data[1][1].stdout).hexdigest(),
              "nativeJsonSha256": hashlib.sha256(data[0][1].stdout).hexdigest(),
              "independentStderr": data[1][1].stderr.decode("utf-8", errors="replace").strip(),
              "executableSha256": binary_hashes, "fields": fields,
              "nativeCounters": obs["scalarCounters"], "nativeIntegrity": obs["integrity"],
              "nativeFrameCoverage": obs["frames"],
              "limitations": "Standalone and peer only; no pinned target comparison, preservation, native platform matrix, broad-format or color claim."}
    args.output.write_text(json.dumps(report, sort_keys=True, indent=2, allow_nan=False) + "\n")


if __name__ == "__main__":
    try:
        main()
    except (ValueError, KeyError, IndexError, OSError, subprocess.SubprocessError) as error:
        print("j284 mpolygon probe: " + str(error), file=sys.stderr)
        sys.exit(1)
