#!/usr/bin/env python3
"""Non-promoting J268 successor: compare three already-admitted DWG streams.

This bounded experiment is not the missing historical 28-path report. Both
converters and their source/build provenance must be recorded separately.
Generated DXFs exist only in a temporary directory and are never retained.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent.parent
FIXTURES = {
    "large_radial.dwg": "d3f7c22db4b6c3953e2d716da719d7e7e5b285cf73bd4e317f0a171d3859f98a",
    "mpolygon_solid.dwg": "66a171c55bc90fdd0764ad3556ec786503666a30a4473a637d168a01979ffc29",
    "rtext_arctext.dwg": "db5663cdc5c1759fdc1f68f9122e596307de06317e766cd5e0aab55912e5d511",
}


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(converter, source, target):
    try:
        result = subprocess.run([str(converter), str(source), "-y", "-v2010", str(target)],
                                stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                                timeout=4, check=False)
    except subprocess.TimeoutExpired:
        return {"status": "timeout", "exitStatus": None}
    exists = target.is_file() and target.stat().st_size > 0
    observation = {"exitStatus": result.returncode,
                   "status": "converted" if result.returncode == 0 and exists else "failed",
                   "nonemptyOutput": exists}
    if exists:
        observation["outputSha256"] = sha(target)
        observation["outputSize"] = target.stat().st_size
    return observation


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--baseline", type=Path, required=True)
    parser.add_argument("--current", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    executables = {"baseline": args.baseline.resolve(strict=True),
                   "current": args.current.resolve(strict=True)}
    executable_hashes = {name: sha(path) for name, path in executables.items()}
    rows = []
    with tempfile.TemporaryDirectory(prefix="j268-successor-") as temporary:
        for name, expected in sorted(FIXTURES.items()):
            source = ROOT / "tests" / "fixtures" / "dwg" / name
            if sha(source) != expected or not source.read_bytes().startswith(b"AC10"):
                raise ValueError("fixture identity/magic mismatch: " + name)
            row = {"source": "tests/fixtures/dwg/" + name, "sourceSha256": expected,
                   "inputVersion": source.read_bytes()[:6].decode("ascii")}
            for side, executable in executables.items():
                row[side] = run(executable, source, Path(temporary) / (side + "-" + name + ".dxf"))
            if sha(source) != expected:
                raise ValueError("fixture changed during execution: " + name)
            rows.append(row)
    if any(sha(path) != executable_hashes[name] for name, path in executables.items()):
        raise ValueError("converter changed during execution")
    hashes = sorted(FIXTURES.values())
    report = {"schema": 1, "kind": "j268-three-admitted-fixture-successor-v1",
              "advisory": True, "promotesSupportClaims": False,
              "historicalJ268MembershipRecovered": False,
              "scope": "Three selected already-admitted J284 fixtures; no assertion about membership in J268.",
              "execution": {"timeoutSeconds": 4, "targetDxfVersion": "AC1024",
                            "converterSha256": executable_hashes},
              "setDigestDefinition": "SHA-256 over LF-terminated sorted unique source hashes",
              "setDigestSha256": hashlib.sha256(("\n".join(hashes) + "\n").encode("ascii")).hexdigest(),
              "rows": rows}
    args.output.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
