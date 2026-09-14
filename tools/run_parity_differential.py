#!/usr/bin/env python3
"""Run the narrow target/standalone parity comparison contract.

The harness is intentionally format-neutral.  It validates runner metadata,
normalizes two JSON result envelopes, and reports semantic/callback/carrier/
graph/error/exact-byte deltas without importing drawing fixtures.  Real target
or standalone adapters are supplied by a later lane; the self-test uses only
an in-memory JSON object and a temporary local-from-scratch input.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import subprocess
import sys
import tempfile
from pathlib import Path

from normalize_drawing_result import NormalizationError, normalize


SCHEMA = 1
KIND = "libdxfrw-parity-differential"
FIXTURE_POLICY = "no-drawing-payloads; source-and-metadata-only"
SIDES = {"target", "standalone"}
FACADES = {"dxfRW", "dwgRW"}
DIRECTIONS = {"read", "write"}
REQUIRED_RESULT_FIELDS = {
    "format",
    "version",
    "toolVersion",
    "exitStatus",
    "diagnostics",
    "entities",
    "objects",
    "relationships",
    "opaquePayloadHashes",
}
MISMATCH_KINDS = {
    "semantic",
    "callback-order",
    "carrier",
    "graph",
    "error-stage",
    "exact-byte",
    "tool-version",
}


class DifferentialError(ValueError):
    pass


def read_json(path: Path) -> object:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as exc:
        raise DifferentialError("cannot read JSON %s: %s" % (path, exc)) from exc


def canonical(value: object) -> object:
    if isinstance(value, dict):
        return {key: canonical(value[key]) for key in sorted(value)}
    if isinstance(value, list):
        return [canonical(item) for item in value]
    return value


def result_digest(value: dict) -> str:
    encoded = json.dumps(canonical(value), sort_keys=True, separators=(",", ":")).encode("utf-8")
    return hashlib.sha256(encoded).hexdigest()


def validate_result(value: object, label: str) -> dict:
    if not isinstance(value, dict):
        raise DifferentialError("%s result must be a JSON object" % label)
    missing = REQUIRED_RESULT_FIELDS - set(value)
    if missing:
        raise DifferentialError("%s result is missing: %s" % (label, ", ".join(sorted(missing))))
    try:
        normalized = normalize(value)
    except NormalizationError as exc:
        raise DifferentialError("%s result normalization failed: %s" % (label, exc)) from exc
    if not isinstance(normalized["diagnostics"], list):
        raise DifferentialError("%s diagnostics must be a list" % label)
    if not isinstance(normalized["entities"], list) or not isinstance(normalized["objects"], list):
        raise DifferentialError("%s entities and objects must be lists" % label)
    if not isinstance(normalized["relationships"], list):
        raise DifferentialError("%s relationships must be a list" % label)
    return normalized


def compare_results(target: object, standalone: object, require_exact_bytes: bool = False) -> dict:
    left = validate_result(target, "target")
    right = validate_result(standalone, "standalone")
    mismatches = []

    def add(kind: str, path: str, target_value: object, standalone_value: object) -> None:
        if target_value != standalone_value:
            mismatches.append(
                {
                    "kind": kind,
                    "path": path,
                    "target": canonical(target_value),
                    "standalone": canonical(standalone_value),
                }
            )

    add("semantic", "format", left["format"], right["format"])
    add("semantic", "version", left["version"], right["version"])
    add("semantic", "entities", left["entities"], right["entities"])
    add("semantic", "objects", left["objects"], right["objects"])
    add("callback-order", "callbacks", left.get("callbacks", []), right.get("callbacks", []))
    add("graph", "relationships", left["relationships"], right["relationships"])
    add("carrier", "opaquePayloadHashes", left["opaquePayloadHashes"], right["opaquePayloadHashes"])
    add("error-stage", "exitStatus", left["exitStatus"], right["exitStatus"])
    add("error-stage", "diagnostics", left["diagnostics"], right["diagnostics"])
    add("tool-version", "toolVersion", left["toolVersion"], right["toolVersion"])

    target_bytes = left.get("exactReplayBytes")
    standalone_bytes = right.get("exactReplayBytes")
    if require_exact_bytes or target_bytes is not None or standalone_bytes is not None:
        add("exact-byte", "exactReplayBytes", target_bytes, standalone_bytes)

    mismatches.sort(key=lambda item: (item["kind"], item["path"]))
    kinds = sorted({item["kind"] for item in mismatches})
    return {
        "schema": SCHEMA,
        "status": "match" if not mismatches else "mismatch",
        "mismatchCount": len(mismatches),
        "mismatchKinds": kinds,
        "mismatches": mismatches,
        "target": {"normalizedSha256": result_digest(left)},
        "standalone": {"normalizedSha256": result_digest(right)},
        "comparisonPolicy": {
            "semantic": "exact-after-normalization",
            "callbacks": "ordered-event-list",
            "relationships": "sorted-from-kind-to",
            "opaquePayloads": "sha256-only",
            "exactReplayBytes": "required-only-when-requested-or-present",
        },
    }


def validate_runner(runner: object, index: int) -> dict:
    if not isinstance(runner, dict):
        raise DifferentialError("runner %d must be an object" % index)
    required = {"id", "side", "facade", "direction", "command", "inputMode", "outputMode", "options", "promotesSupport"}
    missing = required - set(runner)
    if missing:
        raise DifferentialError("runner %d is missing: %s" % (index, ", ".join(sorted(missing))))
    if not isinstance(runner["id"], str) or not runner["id"]:
        raise DifferentialError("runner %d has an invalid id" % index)
    if runner["side"] not in SIDES or runner["facade"] not in FACADES or runner["direction"] not in DIRECTIONS:
        raise DifferentialError("runner %d has an invalid side/facade/direction" % index)
    if not isinstance(runner["command"], list) or not runner["command"] or any(not isinstance(x, str) or not x for x in runner["command"]):
        raise DifferentialError("runner %d has an invalid command" % index)
    if runner["command"].count("{input}") != 1:
        raise DifferentialError("runner %d command must contain exactly one {input}" % index)
    if runner["inputMode"] not in {"local-from-scratch-json", "runtime-external-path"}:
        raise DifferentialError("runner %d has an invalid input mode" % index)
    if runner["outputMode"] != "json-stdout":
        raise DifferentialError("runner %d must emit json-stdout" % index)
    if not isinstance(runner["options"], list) or any(not isinstance(x, str) or not x for x in runner["options"]):
        raise DifferentialError("runner %d has an invalid options list" % index)
    if runner["promotesSupport"] is not False:
        raise DifferentialError("runner %d cannot promote support" % index)
    return runner


def validate_manifest(manifest: object) -> list[dict]:
    if not isinstance(manifest, dict) or manifest.get("schema") != SCHEMA or manifest.get("kind") != KIND:
        raise DifferentialError("unexpected differential manifest schema or kind")
    if manifest.get("fixturePolicy") != FIXTURE_POLICY:
        raise DifferentialError("differential manifest has unsafe fixture policy")
    runners_value = manifest.get("runners")
    if not isinstance(runners_value, list) or not runners_value:
        raise DifferentialError("differential manifest needs runners")
    runners = [validate_runner(runner, i + 1) for i, runner in enumerate(runners_value)]
    ids = [runner["id"] for runner in runners]
    if ids != sorted(ids) or len(ids) != len(set(ids)):
        raise DifferentialError("runner IDs must be sorted and unique")
    keys = [(runner["side"], runner["facade"], runner["direction"]) for runner in runners]
    if len(keys) != len(set(keys)):
        raise DifferentialError("runner side/facade/direction keys must be unique")
    for side in sorted(SIDES):
        for facade in sorted(FACADES):
            for direction in sorted(DIRECTIONS):
                if (side, facade, direction) not in keys:
                    raise DifferentialError("missing runner for %s/%s/%s" % (side, facade, direction))
    grouped = {}
    for runner in runners:
        grouped.setdefault((runner["facade"], runner["direction"]), []).append(runner)
    for key, pair in grouped.items():
        if len(pair) != 2 or {runner["side"] for runner in pair} != SIDES:
            raise DifferentialError("runner pair is incomplete: %s" % (key,))
        if pair[0]["options"] != pair[1]["options"]:
            raise DifferentialError("runner pair options differ: %s" % (key,))
    return runners


def run_runner(runner: dict, input_value: object) -> dict:
    if runner["inputMode"] != "local-from-scratch-json":
        raise DifferentialError("external-path runners require an explicit runtime invocation")
    with tempfile.TemporaryDirectory(prefix="libdxfrw-parity-") as directory:
        input_path = Path(directory) / "input.json"
        input_path.write_text(json.dumps(input_value, sort_keys=True), encoding="utf-8")
        command = [argument.replace("{input}", str(input_path)) for argument in runner["command"]]
        environment = os.environ.copy()
        environment["LIBDXFRW_PARITY_OPTIONS"] = json.dumps(runner["options"], separators=(",", ":"))
        try:
            completed = subprocess.run(command, check=True, capture_output=True, text=True, env=environment)
        except (OSError, subprocess.CalledProcessError) as exc:
            raise DifferentialError("runner %s failed: %s" % (runner["id"], exc)) from exc
        try:
            value = json.loads(completed.stdout)
        except json.JSONDecodeError as exc:
            raise DifferentialError("runner %s did not emit JSON" % runner["id"]) from exc
        return validate_result(value, runner["id"])


def self_test() -> None:
    base = {
        "format": "DXF",
        "version": "AC1027",
        "toolVersion": "test",
        "exitStatus": 0,
        "diagnostics": [],
        "entities": [{"handle": "1", "type": "LINE"}],
        "objects": [],
        "relationships": [],
        "opaquePayloadHashes": [],
        "callbacks": [{"kind": "entity", "handle": "1"}],
    }
    assert compare_results(base, dict(base))["status"] == "match"
    changed = dict(base)
    changed["entities"] = [{"handle": "1", "type": "CIRCLE"}]
    report = compare_results(base, changed)
    assert report["status"] == "mismatch" and "semantic" in report["mismatchKinds"]
    changed = dict(base)
    changed["callbacks"] = list(reversed(base["callbacks"])) + [{"kind": "end"}]
    assert "callback-order" in compare_results(base, changed)["mismatchKinds"]
    python = sys.executable
    echo = [python, "-c", "import json,sys; print(json.dumps(json.load(open(sys.argv[1]))))", "{input}"]
    runners = []
    for side in sorted(SIDES):
        runners.append({"id": "%s-dxf-read" % side, "side": side, "facade": "dxfRW", "direction": "read", "command": echo, "inputMode": "local-from-scratch-json", "outputMode": "json-stdout", "options": ["deterministic", "AC1027"], "promotesSupport": False})
        runners.append({"id": "%s-dxf-write" % side, "side": side, "facade": "dxfRW", "direction": "write", "command": echo, "inputMode": "local-from-scratch-json", "outputMode": "json-stdout", "options": ["deterministic", "AC1027"], "promotesSupport": False})
        runners.append({"id": "%s-dwg-read" % side, "side": side, "facade": "dwgRW", "direction": "read", "command": echo, "inputMode": "local-from-scratch-json", "outputMode": "json-stdout", "options": ["deterministic", "AC1027"], "promotesSupport": False})
        runners.append({"id": "%s-dwg-write" % side, "side": side, "facade": "dwgRW", "direction": "write", "command": echo, "inputMode": "local-from-scratch-json", "outputMode": "json-stdout", "options": ["deterministic", "AC1027"], "promotesSupport": False})
    runners.sort(key=lambda runner: runner["id"])
    manifest = {"schema": SCHEMA, "kind": KIND, "fixturePolicy": FIXTURE_POLICY, "runners": runners}
    validate_manifest(manifest)
    output = run_runner(runners[0], base)
    assert output["entities"] == base["entities"]
    print("run_parity_differential self-test: PASS")


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--manifest", type=Path)
    parser.add_argument("--target", type=Path)
    parser.add_argument("--standalone", type=Path)
    parser.add_argument("--require-exact-bytes", action="store_true")
    args = parser.parse_args(argv)
    try:
        if args.self_test:
            self_test()
            return 0
        if args.manifest is not None:
            validate_manifest(read_json(args.manifest))
            print("differential manifest: PASS")
        if args.target is None and args.standalone is None:
            if args.manifest is None:
                parser.error("--manifest or both --target/--standalone are required")
            return 0
        if args.target is None or args.standalone is None:
            parser.error("--target and --standalone must be supplied together")
        report = compare_results(read_json(args.target), read_json(args.standalone), args.require_exact_bytes)
        print(json.dumps(report, sort_keys=True, indent=2))
        return 0 if report["status"] == "match" else 1
    except (OSError, UnicodeError, DifferentialError, AssertionError) as exc:
        print("differential: FAIL: %s" % exc, file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
