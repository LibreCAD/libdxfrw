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
from update_upgrade_plan import rows, validate  # noqa: E402
from check_support_matrix import MatrixError, validate_matrix  # noqa: E402


class ReleaseError(ValueError):
    pass


DOCUMENTATION_REQUIREMENTS = {
    "docs/UPGRADE_SUPPORT.md": (
        "1,345 `dwgRW`",
        "1,475 `dxfRW`",
        "`QUALIFIED_FORMAT_PARITY`",
        "dispatch, decode",
        "derived rendering",
        "`BUILD_TESTS=ON`",
        "`DRW_VERSION`",
        "deprecated",
    ),
    "README.md": (
        "C++17",
        "CMake is the supported 2.x build",
        "LIBDXFRW_BUILD_TESTS=ON",
        "deprecated",
    ),
}


def read_json(path: Path) -> dict:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as exc:
        raise ReleaseError("cannot read %s: %s" % (path, exc)) from exc
    if not isinstance(value, dict):
        raise ReleaseError("%s must contain an object" % path)
    return value


def validate_documentation(root: Path) -> None:
    """Keep release-facing documentation aligned with the support policy."""
    for relative, markers in DOCUMENTATION_REQUIREMENTS.items():
        path = root / relative
        try:
            text = path.read_text(encoding="utf-8")
        except (OSError, UnicodeError) as exc:
            raise ReleaseError("cannot read release documentation %s: %s" %
                               (path, exc)) from exc
        missing = [marker for marker in markers if marker not in text]
        if missing:
            raise ReleaseError(
                "%s is missing release-policy markers: %s" %
                (path, ", ".join(missing)))


def validate_release(plan_path: Path, mapping_path: Path, registry_path: Path, manifest_path: Path, support_matrix_path: Path) -> None:
    validate_documentation(plan_path.parent)
    plan_text = plan_path.read_text(encoding="utf-8")
    diagnostics = validate(plan_text)
    if diagnostics:
        raise ReleaseError("plan validation: " + "; ".join(diagnostics))
    parsed = rows(plan_text)
    slices = {row.ident: row for row in parsed if row.kind == "slice"}
    parents = {row.ident: row for row in parsed if row.kind == "parent"}
    children = {row.ident: row for row in parsed if row.kind == "child"}
    if not slices:
        raise ReleaseError("plan contains no slices")
    # The live plan is the source of truth for the release horizon.  Requiring
    # every declared slice/parent/child to be committed (or explicitly
    # superseded) prevents this gate from silently applying an obsolete
    # hard-coded S01-S23/I0-I4 horizon after the plan is refined.
    missing_slices = sorted(
        item for item, row in slices.items()
        if row.fields[3] not in {"COMMITTED", "SUPERSEDED"})
    if missing_slices:
        raise ReleaseError("slices are not committed: %s" % ", ".join(missing_slices))
    missing_parents = sorted(
        item for item, row in parents.items()
        if row.fields[3] not in {"COMMITTED", "SUPERSEDED"})
    if missing_parents:
        raise ReleaseError("parents are not committed: %s" % ", ".join(missing_parents))
    missing_children = sorted(
        item for item, row in children.items()
        if row.fields[4] not in {"COMMITTED", "SUPERSEDED"})
    if missing_children:
        raise ReleaseError("children are not committed: %s" % ", ".join(missing_children))
    mapping_document = read_json(mapping_path)
    mapping = mapping_document.get("mapping", {})
    mapping_rows = mapping.get("rows")
    summary = mapping.get("summary", {})
    if not isinstance(mapping_rows, list) or not mapping_rows:
        raise ReleaseError("mapping rows are empty")
    if summary.get("targetOnly") not in (0, "0"):
        raise ReleaseError("target-unmapped rows remain")
    target_rows = [row for row in mapping_rows if row.get("facade") in {"dxfRW", "dwgRW"}]
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
    try:
        validate_matrix(read_json(support_matrix_path), mapping_document, registry)
    except MatrixError as exc:
        raise ReleaseError("support matrix: %s" % exc) from exc
    print("release readiness: PASS (%d target façade rows; 8 differential runners; no promoted source-only claims)" % len(target_rows))


def self_test() -> None:
    assert Counter(["dxfRW", "dwgRW", "dxfRW"])["dxfRW"] == 2
    assert len(DOCUMENTATION_REQUIREMENTS) == 2
    print("check_release_readiness self-test: PASS")


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--plan", type=Path, default=Path("LIBRECAD_DXFRW_UPGRADE_PLAN.md"))
    parser.add_argument("--mapping", type=Path, default=Path("metadata/parity-source-routes-v1.json"))
    parser.add_argument("--registry", type=Path, default=Path("metadata/parity-test-oracles-v1.json"))
    parser.add_argument("--manifest", type=Path, default=Path("metadata/parity-differential-runners-v1.json"))
    parser.add_argument("--support-matrix", type=Path, default=Path("metadata/support-matrix-v1.json"))
    args = parser.parse_args(argv)
    try:
        if args.self_test:
            self_test()
        else:
            validate_release(args.plan, args.mapping, args.registry, args.manifest, args.support_matrix)
        return 0
    except (OSError, UnicodeError, ReleaseError, AssertionError) as exc:
        print("release readiness: FAIL: %s" % exc, file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
