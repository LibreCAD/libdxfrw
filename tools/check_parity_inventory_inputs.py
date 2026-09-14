#!/usr/bin/env python3
"""Verify the pinned LibreCAD inputs used to build the parity inventory.

The eventual parity ledger is derived directly from pinned target and
standalone source. This check pins the target's inventory grammar and its
advisory roadmap material without treating a generated report, a curated seed,
or a prose document as proof of a route mapping. It admits only UTF-8 source
and metadata text and never reads or admits a DWG/DXF fixture payload.
"""

from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
import tempfile
from pathlib import Path


SCHEMA = 1
SOURCE_LOCK_SCHEMA = 1
BLOB_RE = re.compile(r"^[0-9a-f]{40}$")
MODE_RE = re.compile(r"^100(?:644|755)$")
FACT_RE = re.compile(r"^[a-z0-9][a-z0-9-]*$")
LOCKABLE_PATH_RE = re.compile(
    r"^(?:scripts/[A-Za-z0-9._-]+\.py|"
    r"libraries/libdxfrw/(?:[A-Za-z0-9._-]+\.(?:json|md)|\.snapshot-revision))$"
)
DRAWING_SUFFIXES = {".dwg", ".dxf"}
VALID_KINDS = {
    "generator",
    "generator-test",
    "seed",
    "generated-report",
    "roadmap-aggregator",
    "provenance",
    "prose-context",
}
VALID_AUTHORITIES = {"canonical", "advisory", "context"}
ALLOWED_AUTHORITIES = {
    "generator": {"canonical", "advisory"},
    "generator-test": {"advisory"},
    "seed": {"advisory"},
    "generated-report": {"advisory"},
    "roadmap-aggregator": {"advisory"},
    "provenance": {"canonical"},
    "prose-context": {"context"},
}
FIXTURE_POLICY = "no-drawing-payloads; source-and-metadata-only"


class InputError(RuntimeError):
    """Raised for an invalid input lock or mismatched target tree."""


def read_json(path: Path) -> dict:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as exc:
        raise InputError("cannot read JSON %s: %s" % (path, exc)) from exc
    if not isinstance(value, dict):
        raise InputError("JSON root must be an object: %s" % path)
    return value


def run_git_bytes(repo: Path, *args: str) -> bytes:
    try:
        return subprocess.check_output(
            ["git", "-C", str(repo), *args], stderr=subprocess.STDOUT
        )
    except (OSError, subprocess.CalledProcessError) as exc:
        raise InputError("git command failed: %s" % exc) from exc


def run_git(repo: Path, *args: str) -> str:
    try:
        return run_git_bytes(repo, *args).decode("utf-8")
    except UnicodeDecodeError as exc:
        raise InputError("git text command returned non-UTF-8 output") from exc


def validate_source_lock(source_lock: dict) -> dict:
    if source_lock.get("schema") != SOURCE_LOCK_SCHEMA:
        raise InputError("source-lock schema must be %d" % SOURCE_LOCK_SCHEMA)
    target = source_lock.get("libreCAD")
    if not isinstance(target, dict):
        raise InputError("source lock has no LibreCAD target")
    for field in ("commit", "snapshotRevision"):
        if not isinstance(target.get(field), str) or not BLOB_RE.fullmatch(target[field]):
            raise InputError("source lock has invalid LibreCAD %s" % field)
    if not isinstance(target.get("repository"), str) or not target["repository"]:
        raise InputError("source lock has invalid LibreCAD repository")
    if target.get("sourceRoot") != "libraries/libdxfrw/src":
        raise InputError("source lock has unexpected LibreCAD source root")
    return target


def validate_entry(entry: object, index: int) -> dict:
    if not isinstance(entry, dict):
        raise InputError("entry %d is not an object" % index)
    required = {"path", "mode", "blob", "kind", "authority", "allowedFacts"}
    missing = required - set(entry)
    if missing:
        raise InputError("entry %d is missing %s" % (index, sorted(missing)))
    path = entry["path"]
    mode = entry["mode"]
    blob = entry["blob"]
    kind = entry["kind"]
    authority = entry["authority"]
    facts = entry["allowedFacts"]
    if not isinstance(path, str) or not LOCKABLE_PATH_RE.fullmatch(path):
        raise InputError("entry %d has unsafe or unsupported path: %r" % (index, path))
    if Path(path).suffix.lower() in DRAWING_SUFFIXES or "/testdata/" in "/" + path:
        raise InputError("entry %d attempts to lock a drawing fixture: %s" % (index, path))
    if not isinstance(mode, str) or not MODE_RE.fullmatch(mode):
        raise InputError("entry %d has invalid mode: %r" % (index, mode))
    if not isinstance(blob, str) or not BLOB_RE.fullmatch(blob):
        raise InputError("entry %d has invalid Git blob: %r" % (index, blob))
    if kind not in VALID_KINDS:
        raise InputError("entry %d has invalid kind: %r" % (index, kind))
    if authority not in VALID_AUTHORITIES or authority not in ALLOWED_AUTHORITIES[kind]:
        raise InputError("entry %d has wrong authority for %s: %r" % (index, kind, authority))
    if not isinstance(facts, list) or any(
        not isinstance(fact, str) or not FACT_RE.fullmatch(fact) for fact in facts
    ):
        raise InputError("entry %d has invalid allowedFacts" % index)
    if facts != sorted(set(facts)):
        raise InputError("entry %d allowedFacts must be sorted and unique" % index)
    if kind == "prose-context" and facts:
        raise InputError("prose-context entry %d must not claim any facts" % index)
    if kind != "prose-context" and not facts:
        raise InputError("entry %d needs at least one allowed fact" % index)
    return entry


def validate_inputs(inputs: dict, source_lock: dict) -> list[dict]:
    target_lock = validate_source_lock(source_lock)
    if inputs.get("schema") != SCHEMA:
        raise InputError("input-lock schema must be %d" % SCHEMA)
    target = inputs.get("target")
    if not isinstance(target, dict):
        raise InputError("input lock has no target object")
    if target.get("commit") != target_lock["commit"]:
        raise InputError("input-lock target commit differs from source lock")
    if target.get("repository") != target_lock["repository"]:
        raise InputError("input-lock target repository differs from source lock")
    if inputs.get("fixturePolicy") != FIXTURE_POLICY:
        raise InputError("input lock has an unsafe fixture policy")
    entries_value = inputs.get("entries")
    if not isinstance(entries_value, list) or not entries_value:
        raise InputError("input lock needs a non-empty entries array")
    entries = [validate_entry(entry, index + 1) for index, entry in enumerate(entries_value)]
    paths = [entry["path"] for entry in entries]
    if paths != sorted(paths) or len(paths) != len(set(paths)):
        raise InputError("entry paths must be sorted and unique")
    if not any(entry["kind"] == "generator" and entry["authority"] == "canonical" for entry in entries):
        raise InputError("input lock needs at least one canonical generator")
    if not any(entry["kind"] == "seed" for entry in entries):
        raise InputError("input lock needs at least one reviewed seed")
    return entries


def target_entries(repo: Path, commit: str, paths: list[str]) -> dict[str, tuple[str, str]]:
    run_git(repo, "cat-file", "-e", commit + "^{commit}")
    output = run_git(
        repo,
        "ls-tree",
        "-r",
        "--format=%(objectmode)|%(objectname)|%(path)",
        commit,
        "--",
        *paths,
    )
    result: dict[str, tuple[str, str]] = {}
    for line in output.splitlines():
        fields = line.split("|", 2)
        if len(fields) != 3:
            raise InputError("malformed Git tree output")
        mode, blob, path = fields
        result[path] = (mode, blob)
    return result


def decode_target_text(data: bytes, path: str) -> str:
    if b"\0" in data:
        raise InputError("target input is not text (NUL byte): %s" % path)
    if data.startswith(b"AC10") or data.startswith(b"AutoCAD Binary DXF"):
        raise InputError("target input resembles a drawing payload: %s" % path)
    try:
        text = data.decode("utf-8")
    except UnicodeDecodeError as exc:
        raise InputError("target input is not UTF-8 text: %s" % path) from exc
    first = text.lstrip("\ufeff\r\n\t ")
    if first.startswith("0\nSECTION") or first.startswith("0\r\nSECTION"):
        raise InputError("target input resembles an ASCII DXF payload: %s" % path)
    return text


def target_text(repo: Path, commit: str, path: str) -> str:
    return decode_target_text(run_git_bytes(repo, "show", "%s:%s" % (commit, path)), path)


def validate_target_text(repo: Path, target_lock: dict, entries: list[dict]) -> None:
    for entry in entries:
        text = target_text(repo, target_lock["commit"], entry["path"])
        if entry["kind"] == "provenance" and entry["path"].endswith("/.snapshot-revision"):
            if text.strip() != target_lock["snapshotRevision"]:
                raise InputError("target snapshot revision differs from source lock")


def check(inputs_path: Path, source_lock_path: Path, target_repo: Path) -> None:
    source_lock = read_json(source_lock_path)
    target_lock = validate_source_lock(source_lock)
    inputs = read_json(inputs_path)
    entries = validate_inputs(inputs, source_lock)
    expected = {entry["path"]: (entry["mode"], entry["blob"]) for entry in entries}
    actual = target_entries(target_repo, target_lock["commit"], list(expected))
    if actual != expected:
        missing = sorted(set(expected) - set(actual))
        extra = sorted(set(actual) - set(expected))
        changed = sorted(path for path in set(expected) & set(actual) if expected[path] != actual[path])
        raise InputError(
            "target inventory inputs differ: missing=%s extra=%s changed=%s"
            % (missing, extra, changed)
        )
    validate_target_text(target_repo, target_lock, entries)
    canonical_generators = sum(
        entry["kind"] == "generator" and entry["authority"] == "canonical"
        for entry in entries
    )
    advisory = sum(entry["authority"] == "advisory" for entry in entries)
    print(
        "parity inventory input check: PASS (%d entries; %d canonical generators; %d advisory inputs; no drawing payloads)"
        % (len(entries), canonical_generators, advisory)
    )


def expect_input_error(callback, message: str) -> None:
    try:
        callback()
    except InputError:
        return
    raise AssertionError(message)


def git_call(repo: Path, *args: str) -> None:
    subprocess.check_call(
        ["git", "-C", str(repo), *args],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )


def self_test() -> None:
    with tempfile.TemporaryDirectory() as directory:
        root = Path(directory)
        repo = root / "target"
        repo.mkdir()
        git_call(repo, "init", "-q")
        git_call(repo, "config", "user.email", "parity-test@example.invalid")
        git_call(repo, "config", "user.name", "Parity Test")
        snapshot_revision = "c" * 40
        files = {
            "libraries/libdxfrw/.snapshot-revision": snapshot_revision + "\n",
            "libraries/libdxfrw/cross_read_parity_seed.json": "{}\n",
            "scripts/inventory.py": "#!/usr/bin/env python3\nprint('routes')\n",
        }
        for relative, content in files.items():
            path = repo / relative
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(content, encoding="utf-8")
        (repo / "scripts/inventory.py").chmod(0o755)
        git_call(repo, "add", ".")
        git_call(repo, "commit", "-qm", "test inputs")
        commit = run_git(repo, "rev-parse", "HEAD").strip()
        tree = target_entries(repo, commit, sorted(files))
        entries = [
            {
                "path": path,
                "mode": tree[path][0],
                "blob": tree[path][1],
                "kind": kind,
                "authority": authority,
                "allowedFacts": facts,
            }
            for path, kind, authority, facts in (
                (
                    "libraries/libdxfrw/.snapshot-revision",
                    "provenance",
                    "canonical",
                    ["bundled-snapshot-revision"],
                ),
                (
                    "libraries/libdxfrw/cross_read_parity_seed.json",
                    "seed",
                    "advisory",
                    ["reviewed-dispositions-reference"],
                ),
                ("scripts/inventory.py", "generator", "canonical", ["routes"]),
            )
        ]
        source_lock = {
            "schema": SOURCE_LOCK_SCHEMA,
            "libreCAD": {
                "commit": commit,
                "repository": "example:LibreCAD.git",
                "snapshotRevision": snapshot_revision,
                "sourceRoot": "libraries/libdxfrw/src",
            },
        }
        inputs = {
            "schema": SCHEMA,
            "target": {"commit": commit, "repository": "example:LibreCAD.git"},
            "fixturePolicy": FIXTURE_POLICY,
            "entries": entries,
        }
        source_lock_path = root / "source-lock.json"
        inputs_path = root / "inputs.json"
        source_lock_path.write_text(json.dumps(source_lock), encoding="utf-8")
        inputs_path.write_text(json.dumps(inputs), encoding="utf-8")
        check(inputs_path, source_lock_path, repo)

        bad_inputs = json.loads(json.dumps(inputs))
        bad_inputs["entries"][2]["blob"] = "d" * 40
        inputs_path.write_text(json.dumps(bad_inputs), encoding="utf-8")
        expect_input_error(lambda: check(inputs_path, source_lock_path, repo), "blob mismatch was accepted")

        bad_inputs = json.loads(json.dumps(inputs))
        bad_inputs["entries"][2]["mode"] = "100644"
        inputs_path.write_text(json.dumps(bad_inputs), encoding="utf-8")
        expect_input_error(lambda: check(inputs_path, source_lock_path, repo), "mode mismatch was accepted")

        bad_inputs = json.loads(json.dumps(inputs))
        bad_inputs["entries"][2]["path"] = "fixtures/sample.dwg"
        expect_input_error(lambda: validate_inputs(bad_inputs, source_lock), "fixture path was accepted")

        bad_lock = json.loads(json.dumps(source_lock))
        bad_lock["libreCAD"]["snapshotRevision"] = "d" * 40
        source_lock_path.write_text(json.dumps(bad_lock), encoding="utf-8")
        inputs_path.write_text(json.dumps(inputs), encoding="utf-8")
        expect_input_error(lambda: check(inputs_path, source_lock_path, repo), "snapshot mismatch was accepted")
        expect_input_error(
            lambda: decode_target_text(b"\0", "scripts/inventory.py"),
            "non-text input was accepted",
        )
    print("check_parity_inventory_inputs self-test: PASS")


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--inputs", type=Path, default=Path("metadata/librecad-parity-inventory-inputs-v1.json"))
    parser.add_argument("--source-lock", type=Path, default=Path("metadata/libdxfrw-target-lock.json"))
    parser.add_argument("--target-repo", type=Path)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args(argv)
    try:
        if args.self_test:
            self_test()
            return 0
        if args.target_repo is None:
            parser.error("--target-repo is required unless --self-test is used")
        check(args.inputs, args.source_lock, args.target_repo)
        return 0
    except (OSError, UnicodeError, ValueError, InputError, AssertionError) as exc:
        print("parity inventory input check: FAIL: %s" % exc, file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
