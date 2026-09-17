#!/usr/bin/env python3
"""Compute and verify the non-self-referential qualification implementation digest."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import stat
import subprocess
import sys
import tempfile
import unicodedata
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_MANIFEST = ROOT / "metadata/qualification-implementation-inputs-v1.json"
DEFAULT_STATUS = ROOT / "metadata/qualified-format-status-v1.json"
REGULAR_MODES = {"100644", "100755"}
HEX64 = set("0123456789abcdef")
MANIFEST_KEYS = {
    "schema", "kind", "freezeState", "digestAlgorithm", "selection",
    "excludedMutableOutputs", "requiredFinalizationPaths",
}
ALGORITHM_KEYS = {
    "name", "record", "ordering", "hash", "pathRules", "fileRules",
    "manifestSelfReference",
}
SELECTION_KEYS = {
    "includeExact", "includePrefixes", "excludeExact", "excludePrefixes",
}
EXCLUDED_KEYS = {
    "statusOverlay", "receiptDirectory", "releaseProse", "generatedReports",
}


class DigestError(ValueError):
    pass


def _read_json(path: Path) -> dict[str, Any]:
    def reject_pairs(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
        result: dict[str, Any] = {}
        for key, value in pairs:
            if key in result:
                raise DigestError("duplicate JSON key %r in %s" % (key, path))
            result[key] = value
        return result

    def reject_constant(token: str) -> Any:
        raise DigestError("non-finite JSON token %s in %s" % (token, path))

    try:
        value = json.loads(
            path.read_text(encoding="utf-8"),
            object_pairs_hook=reject_pairs,
            parse_constant=reject_constant,
        )
    except (OSError, UnicodeError, json.JSONDecodeError) as exc:
        raise DigestError("cannot read %s: %s" % (path, exc)) from exc
    if not isinstance(value, dict):
        raise DigestError("%s must contain an object" % path)
    return value


def _exact_keys(value: dict[str, Any], expected: set[str], label: str) -> None:
    actual = set(value)
    if actual != expected:
        raise DigestError(
            "%s keys differ (missing=%s unexpected=%s)"
            % (label, sorted(expected - actual), sorted(actual - expected))
        )


def _strings(value: object, label: str, *, nonempty: bool = False) -> list[str]:
    if not isinstance(value, list) or (nonempty and not value):
        raise DigestError("%s must be %sa list" % (label, "a non-empty " if nonempty else ""))
    if any(not isinstance(item, str) or not item for item in value):
        raise DigestError("%s must contain non-empty strings" % label)
    if len(value) != len(set(value)):
        raise DigestError("%s contains duplicates" % label)
    return value


def _validate_path(path: str, *, prefix: bool = False) -> None:
    if path.startswith("/") or "\\" in path or "\x00" in path or "\n" in path:
        raise DigestError("unsafe qualification path %r" % path)
    if unicodedata.normalize("NFC", path) != path:
        raise DigestError("qualification path is not NFC: %r" % path)
    parts = Path(path).parts
    if not parts or any(part in {"", ".", ".."} for part in parts):
        raise DigestError("unsafe qualification path %r" % path)
    if prefix != path.endswith("/"):
        raise DigestError("qualification %s has the wrong trailing slash: %s" % ("prefix" if prefix else "file", path))


def validate_manifest(document: dict[str, Any], *, allow_draft: bool) -> None:
    _exact_keys(document, MANIFEST_KEYS, "implementation-input manifest")
    if document.get("schema") != 1 or document.get("kind") != "libdxfrw-qualification-implementation-inputs":
        raise DigestError("unexpected implementation-input schema")
    if document.get("freezeState") != "FROZEN" and not allow_draft:
        raise DigestError("implementation-input definition is not frozen")
    algorithm = document.get("digestAlgorithm")
    if not isinstance(algorithm, dict):
        raise DigestError("digestAlgorithm must be an object")
    _exact_keys(algorithm, ALGORITHM_KEYS, "digestAlgorithm")
    if algorithm.get("name") != "sha256-git-blob-records-v1":
        raise DigestError("unknown implementation digest algorithm")
    required_algorithm = {
        "record": "path UTF-8, NUL, six-digit Git mode, NUL, lowercase Git blob ID, LF",
        "ordering": "records sorted by unsigned UTF-8 path bytes",
        "hash": "SHA-256 over the concatenated records",
        "pathRules": "repository-relative NFC; no NUL, LF, absolute path, dot, dot-dot, or backslash",
        "fileRules": "tracked regular files only; symlinks, submodules, conflicts, sparse omissions, untracked scoped files, and worktree/index drift are rejected",
        "manifestSelfReference": "this definition is selected and hashed as a Git blob but contains no recorded implementation digest",
    }
    for key, expected in required_algorithm.items():
        if algorithm.get(key) != expected:
            raise DigestError("implementation digest %s drifted" % key)
    selection = document.get("selection")
    if not isinstance(selection, dict):
        raise DigestError("selection must be an object")
    _exact_keys(selection, SELECTION_KEYS, "selection")
    exact = _strings(selection.get("includeExact"), "includeExact", nonempty=True)
    prefixes = _strings(selection.get("includePrefixes"), "includePrefixes", nonempty=True)
    excluded = _strings(selection.get("excludeExact"), "excludeExact")
    excluded_prefixes = _strings(selection.get("excludePrefixes"), "excludePrefixes")
    for path in exact + excluded:
        _validate_path(path)
    for path in prefixes + excluded_prefixes:
        _validate_path(path, prefix=True)
    if exact != sorted(exact) or prefixes != sorted(prefixes) or excluded != sorted(excluded) or excluded_prefixes != sorted(excluded_prefixes):
        raise DigestError("implementation path selections must be canonical")
    if set(exact) & set(excluded):
        raise DigestError("implementation exact include/exclude sets overlap")
    if any(path.startswith(prefix) for path in exact for prefix in excluded_prefixes):
        raise DigestError("an exact input is under an excluded prefix")
    excluded_outputs = document.get("excludedMutableOutputs")
    if not isinstance(excluded_outputs, dict):
        raise DigestError("excludedMutableOutputs must be an object")
    _exact_keys(excluded_outputs, EXCLUDED_KEYS, "excludedMutableOutputs")
    if excluded_outputs.get("statusOverlay") not in excluded:
        raise DigestError("status overlay is not explicitly excluded")
    if excluded_outputs.get("receiptDirectory") not in excluded_prefixes:
        raise DigestError("receipt directory is not explicitly excluded")
    release_prose = _strings(excluded_outputs.get("releaseProse"), "releaseProse", nonempty=True)
    if any(path in exact or any(path.startswith(prefix) for prefix in prefixes) for path in release_prose):
        raise DigestError("release prose enters the implementation digest")
    finalization = _strings(document.get("requiredFinalizationPaths"), "requiredFinalizationPaths", nonempty=True)
    if any(path not in exact for path in finalization):
        raise DigestError("a required finalization path is not an exact input")


def _run_git(args: list[str], root: Path) -> bytes:
    try:
        result = subprocess.run(
            ["git", *args], cwd=root, stdout=subprocess.PIPE,
            stderr=subprocess.PIPE, check=False,
        )
    except OSError as exc:
        raise DigestError("cannot execute git: %s" % exc) from exc
    if result.returncode != 0:
        raise DigestError("git %s failed: %s" % (" ".join(args), result.stderr.decode("utf-8", "replace").strip()))
    return result.stdout


def _index_entries(root: Path) -> dict[str, tuple[str, str]]:
    output = _run_git(["ls-files", "-s", "-z"], root)
    entries: dict[str, tuple[str, str]] = {}
    for raw in output.split(b"\0"):
        if not raw:
            continue
        try:
            header, encoded_path = raw.split(b"\t", 1)
            mode, object_id, stage = header.decode("ascii").split(" ")
            path = encoded_path.decode("utf-8")
        except (ValueError, UnicodeDecodeError) as exc:
            raise DigestError("malformed git index entry") from exc
        _validate_path(path)
        if stage != "0":
            raise DigestError("unmerged qualification input %s" % path)
        if path in entries:
            raise DigestError("duplicate index path %s" % path)
        entries[path] = (mode, object_id)
    return entries


def _object_format(root: Path) -> str:
    value = _run_git(["rev-parse", "--show-object-format"], root).decode("ascii", "strict").strip()
    if value not in {"sha1", "sha256"}:
        raise DigestError("unsupported Git object format %s" % value)
    return value


def _blob_id(data: bytes, object_format: str) -> str:
    hasher = hashlib.sha1() if object_format == "sha1" else hashlib.sha256()
    hasher.update(("blob %d\0" % len(data)).encode("ascii"))
    hasher.update(data)
    return hasher.hexdigest()


def _tracked_mode_matches(index_mode: str, filesystem_mode: int,
                          *, platform_name: str) -> bool:
    if index_mode not in REGULAR_MODES:
        return False
    if platform_name == "nt":
        # Native Windows does not expose Git's POSIX executable bit.  The
        # validated index mode remains authoritative and is still bound into
        # the digest record.
        return True
    expected = "100755" if filesystem_mode & stat.S_IXUSR else "100644"
    return index_mode == expected


def _selected(path: str, selection: dict[str, Any]) -> bool:
    if path in selection["excludeExact"] or any(path.startswith(prefix) for prefix in selection["excludePrefixes"]):
        return False
    return path in selection["includeExact"] or any(path.startswith(prefix) for prefix in selection["includePrefixes"])


def digest_records(entries: list[tuple[str, str, str]]) -> tuple[str, bytes]:
    paths = [path for path, _mode, _blob in entries]
    if len(paths) != len(set(paths)):
        raise DigestError("digest input paths are duplicated")
    records = []
    for path, mode, blob in sorted(entries, key=lambda item: item[0].encode("utf-8")):
        _validate_path(path)
        if mode not in REGULAR_MODES:
            raise DigestError("non-regular mode %s for %s" % (mode, path))
        if not blob or any(char not in HEX64 for char in blob):
            raise DigestError("invalid Git blob ID for %s" % path)
        records.append(path.encode("utf-8") + b"\0" + mode.encode("ascii") + b"\0" + blob.encode("ascii") + b"\n")
    payload = b"".join(records)
    return hashlib.sha256(payload).hexdigest(), payload


def compute_digest(
    root: Path, manifest: dict[str, Any], *, allow_selected_untracked: bool,
) -> dict[str, Any]:
    selection = manifest["selection"]
    index = _index_entries(root)
    object_format = _object_format(root)
    selected_paths = {path for path in index if _selected(path, selection)}
    missing = set(selection["includeExact"]) - selected_paths
    if missing and not allow_selected_untracked:
        raise DigestError("exact implementation inputs are not tracked: %s" % sorted(missing))

    untracked_raw = _run_git(["ls-files", "--others", "-z"], root)
    untracked = []
    for raw in untracked_raw.split(b"\0"):
        if not raw:
            continue
        try:
            path = raw.decode("utf-8")
        except UnicodeDecodeError as exc:
            raise DigestError("untracked path is not UTF-8") from exc
        _validate_path(path)
        if _selected(path, selection):
            untracked.append(path)
    permitted_untracked = set(selection["includeExact"]) if allow_selected_untracked else set()
    unexpected_untracked = set(untracked) - permitted_untracked
    if unexpected_untracked:
        raise DigestError("untracked files enter an implementation scope: %s" % sorted(unexpected_untracked))
    selected_paths.update(set(untracked) & permitted_untracked)
    missing = set(selection["includeExact"]) - selected_paths
    if missing:
        raise DigestError("exact implementation inputs are missing: %s" % sorted(missing))

    if manifest.get("freezeState") == "FROZEN":
        missing_final = [path for path in manifest["requiredFinalizationPaths"] if path not in selected_paths]
        if missing_final:
            raise DigestError("frozen implementation lacks finalization inputs: %s" % missing_final)

    entries = []
    for path in selected_paths:
        absolute = root / path
        try:
            info = absolute.lstat()
        except OSError as exc:
            raise DigestError("selected input %s is missing: %s" % (path, exc)) from exc
        if not stat.S_ISREG(info.st_mode):
            raise DigestError("selected input is not a regular file: %s" % path)
        data = absolute.read_bytes()
        worktree_blob = _blob_id(data, object_format)
        if path in index:
            mode, index_blob = index[path]
            if not _tracked_mode_matches(mode, info.st_mode, platform_name=os.name):
                raise DigestError("worktree/index mode drift for %s" % path)
            if worktree_blob != index_blob and not allow_selected_untracked:
                raise DigestError("worktree/index content drift for %s" % path)
        else:
            mode = "100755" if info.st_mode & stat.S_IXUSR else "100644"
        entries.append((path, mode, worktree_blob))
    digest, payload = digest_records(entries)
    return {
        "schema": 1,
        "kind": "libdxfrw-qualification-implementation-digest",
        "algorithm": "sha256-git-blob-records-v1",
        "objectFormat": object_format,
        "inputCount": len(entries),
        "implementationDigestSha256": digest,
        "recordBytes": len(payload),
        "paths": sorted(selected_paths, key=lambda value: value.encode("utf-8")),
    }


def verify_status_digest(result: dict[str, Any], status_path: Path, *, allow_draft: bool) -> None:
    status = _read_json(status_path)
    if status.get("kind") != "libdxfrw-qualified-format-status":
        raise DigestError("unexpected status overlay")
    recorded = status.get("implementationDigestSha256")
    if status.get("freezeState") == "FROZEN":
        if recorded != result["implementationDigestSha256"]:
            raise DigestError("status overlay implementation digest is stale")
    elif not allow_draft:
        raise DigestError("status overlay is not frozen")
    elif recorded is not None:
        raise DigestError("draft status must not record an implementation digest")


def _expect_error(callable_value: Any, label: str) -> None:
    try:
        callable_value()
    except DigestError:
        return
    raise AssertionError("negative digest vector was accepted: %s" % label)


def self_test() -> None:
    document = _read_json(DEFAULT_MANIFEST)
    validate_manifest(document, allow_draft=False)
    first = [("src/a.cpp", "100644", "1" * 40), ("src/b.cpp", "100755", "2" * 40)]
    second = list(reversed(first))
    digest_a, payload_a = digest_records(first)
    digest_b, payload_b = digest_records(second)
    if digest_a != digest_b or payload_a != payload_b:
        raise AssertionError("record ordering is nondeterministic")
    changed = [("src/a.cpp", "100644", "1" * 40), ("src/b.cpp", "100644", "2" * 40)]
    if digest_records(changed)[0] == digest_a:
        raise AssertionError("mode is not bound into the digest")
    non_executable = stat.S_IFREG | 0o644
    if not _tracked_mode_matches("100755", non_executable, platform_name="nt"):
        raise AssertionError("Windows rejected authoritative executable index mode")
    if _tracked_mode_matches("100755", non_executable, platform_name="posix"):
        raise AssertionError("POSIX accepted executable-mode drift")
    if _tracked_mode_matches("120000", non_executable, platform_name="nt"):
        raise AssertionError("Windows accepted a non-regular index mode")
    _expect_error(lambda: digest_records(first + [first[0]]), "duplicate path")
    _expect_error(lambda: digest_records([("../escape", "100644", "1" * 40)]), "unsafe path")
    _expect_error(lambda: digest_records([("src/link", "120000", "1" * 40)]), "symlink mode")
    invalid = json.loads(json.dumps(document))
    invalid["selection"]["includeExact"].append("metadata/qualified-format-status-v1.json")
    invalid["selection"]["includeExact"].sort()
    _expect_error(lambda: validate_manifest(invalid, allow_draft=True), "mutable status input")
    unfrozen = json.loads(json.dumps(document))
    unfrozen["freezeState"] = "DRAFT_WORKFLOW_AND_DIGEST_PENDING"
    validate_manifest(unfrozen, allow_draft=True)
    _expect_error(lambda: validate_manifest(unfrozen, allow_draft=False), "unfrozen manifest")
    with tempfile.TemporaryDirectory(prefix="libdxfrw-digest-test-") as directory:
        data = b"qualification-digest\n"
        path = Path(directory) / "input"
        path.write_bytes(data)
        if _blob_id(path.read_bytes(), "sha1") != hashlib.sha1(b"blob 21\0" + data).hexdigest():
            raise AssertionError("Git blob hashing drifted")
    print("qualification digest self-test: PASS")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument("--status", type=Path, default=DEFAULT_STATUS)
    parser.add_argument("--root", type=Path, default=ROOT)
    parser.add_argument("--allow-draft", action="store_true")
    parser.add_argument("--allow-selected-untracked", action="store_true")
    parser.add_argument("--skip-status", action="store_true")
    parser.add_argument("--json", action="store_true")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args(argv)
    try:
        if args.self_test:
            self_test()
            return 0
        document = _read_json(args.manifest)
        validate_manifest(document, allow_draft=args.allow_draft)
        result = compute_digest(
            args.root.resolve(), document,
            allow_selected_untracked=args.allow_selected_untracked,
        )
        if not args.skip_status:
            verify_status_digest(result, args.status, allow_draft=args.allow_draft)
        if args.json:
            print(json.dumps(result, sort_keys=True, separators=(",", ":")))
        else:
            print(
                "implementation digest: %s (%d inputs)"
                % (result["implementationDigestSha256"], result["inputCount"])
            )
        return 0
    except (OSError, UnicodeError, json.JSONDecodeError, DigestError, AssertionError) as exc:
        print("qualification digest: FAIL: %s" % exc, file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
