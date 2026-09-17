#!/usr/bin/env python3
"""Compare pinned-target and installed-package JSON dumpers.

This is an evidence/advisory lane.  Both dumpers receive the same already
admitted fixture path and options.  The report retains source/output hashes,
bounded semantic summaries, exit categories, and mismatch relations only; it
never copies DWG/DXF bytes into the repository or into the report.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import subprocess
import sys
import tempfile
from collections import Counter
from pathlib import Path
from typing import Any


SCHEMA = 2
KIND = "libdxfrw-json-target-package-differential"
ALLOWED_ORIGINS = {"lockedRepositoryBlob", "localFromScratch"}
FIXTURE_POLICY = "lockedRepositoryBlob/localFromScratch; hashes-and-summaries-only"
MAX_OUTPUT_BYTES = 16 * 1024 * 1024
MAX_RECORD_HASHES = 256


class DifferentialError(ValueError):
    """Raised when the evidence contract cannot be satisfied."""


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    try:
        with path.open("rb") as stream:
            for chunk in iter(lambda: stream.read(1024 * 1024), b""):
                digest.update(chunk)
    except OSError as exc:
        raise DifferentialError(f"cannot hash {path}: {exc}") from exc
    return digest.hexdigest()


def input_version(path: Path) -> str:
    try:
        prefix = path.read_bytes()[:6].decode("ascii", "replace")
    except OSError as exc:
        raise DifferentialError(f"cannot read fixture header {path}: {exc}") from exc
    return prefix if re.fullmatch(r"AC\d{4}", prefix) else "UNKNOWN"


def _status_summary(document: dict[str, Any]) -> dict[str, Any]:
    """Extract the optional coarse reader/writer status contract.

    The pinned dumpers currently expose diagnostics inside the semantic
    envelope, but a future target or standalone adapter may also report the
    legacy boolean/error/stage result.  Keep only known scalar fields so the
    differential report cannot retain arbitrary runner output.
    """
    status: dict[str, Any] = {}
    nested = document.get("status")
    if isinstance(nested, dict):
        for key in ("ok", "error", "stage", "errorCode", "errorStage"):
            if key not in nested:
                continue
            value = nested.get(key)
            if isinstance(value, (bool, int, str)) or value is None:
                status[key] = value
    for key in ("ok", "error", "stage", "errorCode", "errorStage"):
        if key in document:
            value = document[key]
            if isinstance(value, (bool, int, str)) or value is None:
                status[key] = value
    return status


def _record_hashes(records: list[object]) -> tuple[list[str], str]:
    """Hash canonical records while retaining only bounded evidence."""
    hashes: list[str] = []
    sequence = hashlib.sha256()
    for record in records:
        if not isinstance(record, dict):
            canonical = json.dumps(record, sort_keys=True, separators=(",", ":"))
        else:
            canonical = json.dumps(
                record, sort_keys=True, separators=(",", ":"),
                ensure_ascii=False,
            )
        digest = hashlib.sha256(canonical.encode("utf-8")).hexdigest()
        sequence.update(digest.encode("ascii"))
        if len(hashes) < MAX_RECORD_HASHES:
            hashes.append(digest)
    return hashes, sequence.hexdigest()


def summary(document: object) -> dict[str, Any]:
    if not isinstance(document, dict):
        raise DifferentialError("dumper output root is not a JSON object")
    entities = document.get("entities")
    objects = document.get("objects")
    diagnostics = document.get("diagnostics")
    if not isinstance(entities, list) or not isinstance(objects, list):
        raise DifferentialError("dumper output has no entity/object arrays")
    if not isinstance(diagnostics, dict):
        raise DifferentialError("dumper output has no diagnostics object")
    entity_hashes, entity_sequence_hash = _record_hashes(entities)
    object_hashes, object_sequence_hash = _record_hashes(objects)
    entity_types = sorted(
        str(item.get("type", ""))
        for item in entities
        if isinstance(item, dict)
    )
    object_types = sorted(
        str(item.get("type", ""))
        for item in objects
        if isinstance(item, dict)
    )
    return {
        "sourceFormat": document.get("sourceFormat"),
        "version": document.get("version"),
        "diagnostics": {
            str(key): value for key, value in sorted(diagnostics.items())
        },
        "entityCount": len(entities),
        "objectCount": len(objects),
        "entityTypes": entity_types,
        "objectTypes": object_types,
        "entityRecordSha256": entity_hashes,
        "entityRecordSequenceSha256": entity_sequence_hash,
        "objectRecordSha256": object_hashes,
        "objectRecordSequenceSha256": object_sequence_hash,
        "status": _status_summary(document),
    }


def _value_relation(left: object, right: object,
                    *, missing: str = "unavailable") -> str:
    if left is None or right is None:
        return missing
    return "equal" if left == right else "delta"


def _summary_relation(left: dict[str, Any], right: dict[str, Any]) -> str:
    return "equal" if left == right else "delta"


def _status_relation(left: dict[str, Any], right: dict[str, Any]) -> str:
    if not left or not right:
        return "not-reported"
    return _summary_relation(left, right)


def run_dumper(executable: Path, source: Path, output: Path,
               timeout_seconds: float) -> dict[str, Any]:
    result: dict[str, Any] = {}
    command = ([sys.executable, str(executable), str(source), "-o", str(output)]
               if executable.suffix.lower() == ".py"
               else [str(executable), str(source), "-o", str(output)])
    try:
        completed = subprocess.run(
            command,
            capture_output=True,
            text=True,
            check=False,
            timeout=timeout_seconds if timeout_seconds > 0 else None,
        )
    except subprocess.TimeoutExpired:
        return {"status": "timeout", "diagnosticCode": "timeout"}
    except OSError as exc:
        return {"status": "runner-error", "diagnosticCode": "runner-error",
                "message": str(exc)}

    result["exitStatus"] = completed.returncode
    if completed.returncode != 0:
        result["status"] = "failed"
        result["diagnosticCode"] = "nonzero-exit"
    elif not output.is_file():
        result["status"] = "failed"
        result["diagnosticCode"] = "missing-output"
    else:
        result["status"] = "completed"
        result["diagnosticCode"] = "success"

    if output.is_file():
        try:
            if output.stat().st_size > MAX_OUTPUT_BYTES:
                raise DifferentialError("dumper output exceeds bounded size")
            encoded = output.read_bytes()
            result["outputSha256"] = hashlib.sha256(encoded).hexdigest()
            result["outputSize"] = len(encoded)
            document = json.loads(encoded.decode("utf-8"))
            result["summary"] = summary(document)
        except (OSError, UnicodeError, json.JSONDecodeError,
                DifferentialError) as exc:
            result["status"] = "invalid-output"
            result["diagnosticCode"] = "invalid-json"
            result["message"] = str(exc)

    # Keep diagnostics bounded and avoid retaining arbitrary runner output.
    if completed.stderr.strip():
        result["stderrSha256"] = hashlib.sha256(
            completed.stderr.encode("utf-8", "replace")
        ).hexdigest()
    return result


def load_fixtures(path: Path, root: Path) -> list[dict[str, Any]]:
    try:
        manifest = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as exc:
        raise DifferentialError(f"cannot read fixture registry {path}: {exc}") from exc
    if not isinstance(manifest, dict) or not isinstance(manifest.get("fixtures"), list):
        raise DifferentialError("fixture registry has no fixtures list")
    fixtures: list[dict[str, Any]] = []
    for index, fixture in enumerate(manifest["fixtures"], 1):
        if not isinstance(fixture, dict):
            raise DifferentialError(f"fixture {index} is not an object")
        rel = fixture.get("path")
        origin = fixture.get("originKind")
        if not isinstance(rel, str) or not rel:
            raise DifferentialError(f"fixture {index} has no path")
        if origin not in ALLOWED_ORIGINS:
            raise DifferentialError(
                f"fixture {rel} has ineligible originKind {origin!r}"
            )
        source = (root / rel).resolve()
        if not source.is_file():
            raise DifferentialError(f"admitted fixture is absent: {rel}")
        row = dict(fixture)
        row["path"] = rel
        row["_source"] = source
        fixtures.append(row)
    return fixtures


def compare(root: Path, registry: Path, target: Path, standalone: Path,
            limit: int, timeout_seconds: float,
            target_commit: str | None = None) -> dict[str, Any]:
    if not target.is_file() or not standalone.is_file():
        raise DifferentialError("both dumper paths must be files")
    fixtures = load_fixtures(registry, root)
    if limit >= 0:
        fixtures = fixtures[:limit]
    rows: list[dict[str, Any]] = []
    with tempfile.TemporaryDirectory(prefix="libdxfrw-json-differential-") as temp:
        output_root = Path(temp)
        for index, fixture in enumerate(fixtures):
            source = fixture["_source"]
            target_result = run_dumper(
                target, source, output_root / f"{index}-target.json", timeout_seconds
            )
            standalone_result = run_dumper(
                standalone, source, output_root / f"{index}-standalone.json",
                timeout_seconds,
            )
            if (target_result.get("status") == "completed"
                    and standalone_result.get("status") == "completed"):
                relation = (
                    "equal"
                    if target_result.get("outputSha256")
                    == standalone_result.get("outputSha256")
                    else "delta"
                )
            elif target_result.get("status") == standalone_result.get("status"):
                relation = "both-failed"
            else:
                relation = "one-sided-failure"
            rows.append({
                "path": fixture["path"],
                "originKind": fixture["originKind"],
                "sourceSha256": sha256(source),
                "sourceSize": source.stat().st_size,
                "inputVersion": input_version(source),
                "target": target_result,
                "standalone": standalone_result,
                "summaryEqual": target_result.get("summary")
                == standalone_result.get("summary"),
                "byteRelation": _value_relation(
                    target_result.get("outputSha256"),
                    standalone_result.get("outputSha256"),
                ),
                "semanticRelation": (
                    _summary_relation(target_result["summary"],
                                      standalone_result["summary"])
                    if isinstance(target_result.get("summary"), dict)
                    and isinstance(standalone_result.get("summary"), dict)
                    else "unavailable"
                ),
                "statusRelation": (
                    _status_relation(target_result["summary"]["status"],
                                     standalone_result["summary"]["status"])
                    if isinstance(target_result.get("summary"), dict)
                    and isinstance(standalone_result.get("summary"), dict)
                    else "unavailable"
                ),
                "relation": relation,
            })
    rows.sort(key=lambda row: (row["inputVersion"], row["sourceSha256"]))
    counts = Counter(row["relation"] for row in rows)
    byte_counts = Counter(row["byteRelation"] for row in rows)
    semantic_counts = Counter(row["semanticRelation"] for row in rows)
    status_counts = Counter(row["statusRelation"] for row in rows)
    report: dict[str, Any] = {
        "schema": SCHEMA,
        "kind": KIND,
        "advisory": True,
        "fixturePolicy": FIXTURE_POLICY,
        "inputCount": len(rows),
        "relationCounts": dict(sorted(counts.items())),
        "byteRelationCounts": dict(sorted(byte_counts.items())),
        "semanticRelationCounts": dict(sorted(semantic_counts.items())),
        "statusRelationCounts": dict(sorted(status_counts.items())),
        "rows": rows,
    }
    if target_commit:
        report["targetCommit"] = target_commit
    return report


def self_test() -> None:
    with tempfile.TemporaryDirectory(prefix="libdxfrw-json-differential-test-") as temp:
        root = Path(temp)
        source = root / "sample.dxf"
        source.write_text("0\nEOF\n", encoding="utf-8")
        payload = {
            "sourceFormat": "dxf",
            "version": "AC1009",
            "diagnostics": {"entityParseFailures": 0, "objectParseFailures": 0},
            "entities": [{"type": "LINE"}],
            "objects": [],
        }
        runner = root / "runner.py"
        runner.write_text(
            "#!/usr/bin/env python3\n"
            "import json, pathlib, sys\n"
            f"payload = {json.dumps(payload)!r}\n"
            "pathlib.Path(sys.argv[sys.argv.index('-o') + 1]).write_text(payload)\n",
            encoding="utf-8",
        )
        runner.chmod(0o755)
        registry = root / "registry.json"
        registry.write_text(json.dumps({"fixtures": [{
            "path": "sample.dxf", "originKind": "localFromScratch"
        }]}), encoding="utf-8")
        report = compare(root, registry, runner, runner, -1, 2.0)
        assert report["relationCounts"] == {"equal": 1}
        assert report["byteRelationCounts"] == {"equal": 1}
        assert report["semanticRelationCounts"] == {"equal": 1}
        assert report["statusRelationCounts"] == {"not-reported": 1}
        bad = root / "bad.py"
        bad.write_text(
            "#!/usr/bin/env python3\n"
            "import json, pathlib, sys\n"
            f"payload = {json.dumps({**payload, 'entities': [{'type': 'LINE', 'x': 99}]})!r}\n"
            "pathlib.Path(sys.argv[sys.argv.index('-o') + 1]).write_text(payload)\n",
            encoding="utf-8",
        )
        bad.chmod(0o755)
        mismatch = compare(root, registry, runner, bad, -1, 2.0)
        assert mismatch["relationCounts"] == {"delta": 1}
        assert mismatch["byteRelationCounts"] == {"delta": 1}
        assert mismatch["semanticRelationCounts"] == {"delta": 1}
        assert mismatch["rows"][0]["byteRelation"] == "delta"
        assert mismatch["rows"][0]["semanticRelation"] == "delta"
        pretty = root / "pretty.py"
        pretty.write_text(
            "#!/usr/bin/env python3\n"
            "import json, pathlib, sys\n"
            f"payload = {json.dumps(payload)!r}\n"
            "pathlib.Path(sys.argv[sys.argv.index('-o') + 1]).write_text("
            "json.dumps(json.loads(payload), indent=2) + '\\n')\n",
            encoding="utf-8",
        )
        pretty.chmod(0o755)
        formatting = compare(root, registry, runner, pretty, -1, 2.0)
        assert formatting["relationCounts"] == {"delta": 1}
        assert formatting["rows"][0]["byteRelation"] == "delta"
        assert formatting["rows"][0]["semanticRelation"] == "equal"
        assert formatting["rows"][0]["statusRelation"] == "not-reported"
        status_payload = {
            **payload,
            "status": {"ok": True, "error": 0, "stage": "complete"},
        }
        status_runner = root / "status.py"
        status_runner.write_text(
            "#!/usr/bin/env python3\n"
            "import json, pathlib, sys\n"
            f"payload = {json.dumps(status_payload)!r}\n"
            "pathlib.Path(sys.argv[sys.argv.index('-o') + 1]).write_text(payload)\n",
            encoding="utf-8",
        )
        status_runner.chmod(0o755)
        status_report = compare(root, registry, status_runner, status_runner,
                                -1, 2.0)
        assert status_report["statusRelationCounts"] == {"equal": 1}
        status_bad = root / "status_bad.py"
        status_bad.write_text(
            "#!/usr/bin/env python3\n"
            "import json, pathlib, sys\n"
            f"payload = {json.dumps({**status_payload, 'status': {'ok': False, 'error': 5, 'stage': 'header'}})!r}\n"
            "pathlib.Path(sys.argv[sys.argv.index('-o') + 1]).write_text(payload)\n",
            encoding="utf-8",
        )
        status_bad.chmod(0o755)
        status_mismatch = compare(root, registry, status_runner, status_bad,
                                  -1, 2.0)
        assert status_mismatch["statusRelationCounts"] == {"delta": 1}
        ineligible = root / "ineligible.json"
        ineligible.write_text(json.dumps({"fixtures": [{
            "path": "sample.dxf", "originKind": "externalCorpus"
        }]}), encoding="utf-8")
        try:
            compare(root, ineligible, runner, runner, -1, 2.0)
        except DifferentialError as exc:
            assert "ineligible" in str(exc)
        else:
            raise AssertionError("ineligible fixture was accepted")
    print("run_json_target_package_differential self-test: PASS")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--root", type=Path)
    parser.add_argument("--registry", type=Path)
    parser.add_argument("--target", type=Path)
    parser.add_argument("--standalone", type=Path)
    parser.add_argument("--limit", type=int, default=-1)
    parser.add_argument("--timeout", type=float, default=30.0)
    parser.add_argument("--target-commit")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args(argv)
    try:
        if args.self_test:
            self_test()
            return 0
        if args.root is None or args.registry is None:
            parser.error("--root and --registry are required unless --self-test is used")
        if args.target is None or args.standalone is None:
            parser.error("--target and --standalone are required unless --self-test is used")
        if args.limit < -1 or args.timeout < 0:
            parser.error("--limit must be >= -1 and --timeout must be non-negative")
        report = compare(
            args.root.resolve(), args.registry.resolve(), args.target.resolve(),
            args.standalone.resolve(), args.limit, args.timeout, args.target_commit
        )
        encoded = json.dumps(report, sort_keys=True, indent=2) + "\n"
        if args.output:
            args.output.write_text(encoded, encoding="utf-8")
        else:
            sys.stdout.write(encoded)
        return 0 if report["relationCounts"].get("delta", 0) == 0 \
            and report["relationCounts"].get("one-sided-failure", 0) == 0 \
            and report["relationCounts"].get("both-failed", 0) == 0 else 1
    except (OSError, UnicodeError, DifferentialError, subprocess.SubprocessError) as exc:
        print(f"json target/package differential: FAIL: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
