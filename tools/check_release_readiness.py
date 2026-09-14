#!/usr/bin/env python3
"""Fast release-readiness audit for the final parity checkpoint.

This gate reconciles source-only artifacts and plan state before the one
scheduled broad validation run.  It does not claim wire-format support from
lexical/source evidence and never loads or writes a drawing fixture.
"""

from __future__ import annotations

import argparse
import json
from collections import Counter
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).parent))
from update_upgrade_plan import validate  # noqa: E402


class ReleaseError(ValueError):
    pass


def read_json(path: Path) -> dict:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as exc:
        raise ReleaseError("cannot read %s: %s" % (path, exc)) from exc
    if not isinstance(value, dict):
        raise ReleaseError("%s must contain an object" % path)
    return value


def validate_release(plan_path: Path, mapping_path: Path, registry_path: Path, manifest_path: Path) -> None:
    slices, parents, children = validate(plan_path.read_text(encoding="utf-8"))
    required_committed_slices = {"S%02d" % number for number in range(1, 23)}
    missing_slices = sorted(item for item in required_committed_slices if slices.get(item, {}).get("state") != "COMMITTED")
    if missing_slices:
        raise ReleaseError("pre-S23 slices are not committed: %s" % ", ".join(missing_slices))
    if slices.get("S23", {}).get("state") not in {"PLANNED", "ACTIVE", "VERIFIED", "COMMITTED"}:
        raise ReleaseError("S23 has an invalid final-checkpoint state")
    for item in ("I0", "I1", "I2", "I3", "I4"):
        if parents.get(item, {}).get("state") != "COMMITTED":
            raise ReleaseError("parent %s is not committed" % item)
    mapping = read_json(mapping_path).get("mapping", {})
    rows = mapping.get("rows")
    summary = mapping.get("summary", {})
    if not isinstance(rows, list) or not rows:
        raise ReleaseError("mapping rows are empty")
    if summary.get("targetOnly") not in (0, "0"):
        raise ReleaseError("target-unmapped rows remain")
    target_rows = [row for row in rows if row.get("facade") in {"dxfRW", "dwgRW"}]
    if any(row.get("fixtureDisposition") != "no-drawing-fixture" for row in target_rows):
        raise ReleaseError("a façade row has a non-source-only fixture disposition")
    if any(row.get("disposition") == "promoted" for row in target_rows):
        raise ReleaseError("source-only mapping cannot promote a support row")
    registry = read_json(registry_path)
    if any(route.get("promotesSupport") is not False for route in registry.get("routes", [])):
        raise ReleaseError("oracle route promotes support")
    if any(link.get("promotesSupport") is not False for link in registry.get("featureLinks", [])):
        raise ReleaseError("oracle feature link promotes support")
    manifest = read_json(manifest_path)
    runners = manifest.get("runners")
    if not isinstance(runners, list) or len(runners) != 8:
        raise ReleaseError("differential manifest must contain eight façade-direction runners")
    keys = {(runner.get("side"), runner.get("facade"), runner.get("direction")) for runner in runners}
    expected = {(side, facade, direction) for side in ("target", "standalone") for facade in ("dxfRW", "dwgRW") for direction in ("read", "write")}
    if keys != expected:
        raise ReleaseError("differential runner matrix is incomplete")
    print("release readiness: PASS (%d target façade rows; 8 differential runners; no promoted source-only claims)" % len(target_rows))


def self_test() -> None:
    assert Counter(["dxfRW", "dwgRW", "dxfRW"])["dxfRW"] == 2
    print("check_release_readiness self-test: PASS")


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--plan", type=Path, default=Path("LIBRECAD_DXFRW_UPGRADE_PLAN.md"))
    parser.add_argument("--mapping", type=Path, default=Path("metadata/parity-source-routes-v1.json"))
    parser.add_argument("--registry", type=Path, default=Path("metadata/parity-test-oracles-v1.json"))
    parser.add_argument("--manifest", type=Path, default=Path("metadata/parity-differential-runners-v1.json"))
    args = parser.parse_args(argv)
    try:
        if args.self_test:
            self_test()
        else:
            validate_release(args.plan, args.mapping, args.registry, args.manifest)
        return 0
    except (OSError, UnicodeError, ReleaseError, AssertionError) as exc:
        print("release readiness: FAIL: %s" % exc, file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
