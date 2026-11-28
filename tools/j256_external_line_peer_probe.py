#!/usr/bin/env python3
"""Advisory peer comparison for one external, non-admitted LINE source."""
import argparse
import copy
import hashlib
import json
import math
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parent.parent
SOURCE_SHA = "85819667e8e88366ca3d8f9a0dda9770f094b6445882e08e0b2b3bf983472f7b"
PEER_SHA = "536eef295f1dfdc2e63b4f1cd2961905f98c55bbd1fe017ef3574c0f1345f1ff"
TOLERANCE = 1e-12


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def compare(native, peer):
    if native.get("status", {}).get("operationSucceeded") is not True:
        raise ValueError("native read failed")
    records = [r for r in native["records"] if r.get("recordClass") == "LINE"
               and r.get("sourceHandle") == "17e"]
    objects = [o for o in peer["OBJECTS"] if o.get("entity") == "LINE"
               and len(o.get("handle", [])) >= 3 and o["handle"][2] == 0x17e]
    if len(records) != 1 or len(objects) != 1:
        raise ValueError("LINE source identity is missing or duplicated")
    record, obj = records[0], objects[0]
    if record.get("ownerHandle") != "17c" or obj.get("ownerhandle", [])[-1:] != [0x17c]:
        raise ValueError("LINE owner mismatch")
    entries = record["fields"]
    if len({e["name"] for e in entries}) != len(entries):
        raise ValueError("duplicate native field")
    fields = {e["name"]: e["value"] for e in entries}
    result = {}
    for name in ("start", "end", "extrusion"):
        left, right = fields[name], obj[name]
        if not isinstance(left, dict) or not isinstance(right, list) or len(right) != 3:
            raise ValueError("invalid coordinate shape: " + name)
        lv = [left[k] for k in ("x", "y", "z")]
        for a, b in zip(lv, right):
            if (type(a) not in (int, float) or type(b) not in (int, float)
                    or not math.isfinite(a) or not math.isfinite(b)
                    or not math.isclose(a, b, abs_tol=TOLERANCE, rel_tol=TOLERANCE)):
                raise ValueError("coordinate mismatch: " + name)
        result[name] = {"native": lv, "independent": right}
    a, b = fields["thickness"], obj["thickness"]
    if (type(a) not in (int, float) or type(b) not in (int, float)
            or not math.isfinite(a) or not math.isfinite(b)
            or not math.isclose(a, b, abs_tol=TOLERANCE, rel_tol=TOLERANCE)):
        raise ValueError("thickness mismatch")
    result["thickness"] = {"native": a, "independent": b}
    return result


def self_test():
    point = {"x": 1.0, "y": 2.0, "z": 3.0}
    fields = [{"name": n, "value": copy.deepcopy(point)} for n in ("start", "end", "extrusion")]
    fields.append({"name": "thickness", "value": 0.0})
    native = {"status": {"operationSucceeded": True}, "records": [{
        "recordClass": "LINE", "sourceHandle": "17e", "ownerHandle": "17c", "fields": fields}]}
    obj = {"entity": "LINE", "handle": [0, 2, 0x17e], "ownerhandle": [12, 1, 2, 0x17c],
           "start": [1, 2, 3], "end": [1, 2, 3], "extrusion": [1, 2, 3], "thickness": 0}
    peer = {"OBJECTS": [obj]}
    assert len(compare(native, peer)) == 4
    wrong_owner, wrong_value, duplicate = (copy.deepcopy(peer) for _ in range(3))
    wrong_owner["OBJECTS"][0]["ownerhandle"][-1] = 1
    wrong_value["OBJECTS"][0]["start"][0] = float("nan")
    duplicate["OBJECTS"].append(copy.deepcopy(duplicate["OBJECTS"][0]))
    for invalid in (wrong_owner, wrong_value, duplicate):
        try:
            compare(native, invalid)
        except ValueError:
            continue
        raise AssertionError("invalid peer input accepted")
    print("external LINE peer probe self-test: PASS")


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--self-test", action="store_true")
    p.add_argument("--source", type=Path)
    p.add_argument("--adapter", type=Path)
    p.add_argument("--dwgread", type=Path)
    p.add_argument("--output", type=Path)
    a = p.parse_args()
    if a.self_test:
        self_test()
        return
    if not all((a.source, a.adapter, a.dwgread, a.output)):
        p.error("--source, --adapter, --dwgread and --output are required")
    source, adapter, peer = [x.resolve(strict=True) for x in (a.source, a.adapter, a.dwgread)]
    registry = json.loads((ROOT / "metadata/external-evidence-resolution-v1.json").read_text())
    reports = {r["itemId"]: r for r in registry["reports"]}
    if any(SOURCE_SHA not in reports[i]["memberSha256s"] for i in ("J256", "J260", "J293")):
        raise ValueError("registered item membership mismatch")
    if sha(source) != SOURCE_SHA or sha(peer) != PEER_SHA:
        raise ValueError("source or peer identity mismatch")
    hashes = {"adapter": sha(adapter), "dwgread": sha(peer)}
    commands = [[str(adapter), "--input", str(source), "--facade", "dwgRW", "--direction", "read", "--json"],
                [str(peer), "-O", "JSON", str(source)]]
    results = []
    for command in commands:
        runs = [subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                               timeout=10, check=True) for _ in range(2)]
        if runs[0].stdout != runs[1].stdout or runs[0].stderr != runs[1].stderr:
            raise ValueError("non-deterministic reader output")
        results.append((json.loads(runs[0].stdout), runs[0]))
    native, independent = results[0][0], results[1][0]
    if (native.get("input", {}).get("sha256") != SOURCE_SHA
            or native.get("input", {}).get("detectedVersion") != "AC1018"):
        raise ValueError("native source identity mismatch")
    fields = compare(native, independent)
    if sha(source) != SOURCE_SHA or sha(adapter) != hashes["adapter"] or sha(peer) != hashes["dwgread"]:
        raise ValueError("source or executable changed during run")
    obs = native["observations"]["inputRead"]
    report = {"schema": 1, "kind": "j256-j260-j293-external-line-peer-probe-v1",
              "advisory": True, "promotesSupportClaims": False,
              "historicalItems": ["J256", "J260", "J293"], "sourceSha256": SOURCE_SHA,
              "sourceDisposition": "external-origin-not-admitted; artifact license unconfirmed",
              "sourceVersion": "AC1018", "lineHandle": "17e", "ownerHandle": "17c",
              "comparisonTolerance": {"absolute": TOLERANCE, "relative": TOLERANCE},
              "fields": fields, "executableSha256": hashes,
              "nativeJsonSha256": hashlib.sha256(results[0][1].stdout).hexdigest(),
              "independentJsonSha256": hashlib.sha256(results[1][1].stdout).hexdigest(),
              "independentStderr": results[1][1].stderr.decode(errors="replace").strip(),
              "nativeCounters": obs["scalarCounters"], "nativeIntegrity": obs["integrity"],
              "nativeFrameCoverage": obs["frames"],
              "limitations": "One selected entity, not a complete drawing, output preservation, target parity, platform or format support claim."}
    a.output.write_text(json.dumps(report, sort_keys=True, indent=2, allow_nan=False) + "\n")


if __name__ == "__main__":
    try:
        main()
    except (ValueError, KeyError, IndexError, OSError, subprocess.SubprocessError) as error:
        print("external LINE peer probe: " + str(error), file=sys.stderr)
        sys.exit(1)
