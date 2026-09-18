#!/usr/bin/env python3
"""Create/check the pinned LibreCAD libdxfrw source manifest and lock.

The checker reads Git object data only.  It never copies from a working tree,
never downloads a drawing, and never mutates source files.  The generated
manifest includes every tracked file under the bundled ``src`` tree plus the
bundled source-list file, even when the target CMake list omits a header.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
import sys
from pathlib import Path


MANIFEST_SCHEMA = "libdxfrw-target-source-manifest-v1"
LOCK_SCHEMA = "libdxfrw-target-lock-v1"
TARGET_ROOT = "libraries/libdxfrw"


def git(git_dir: Path, *args: str, binary: bool = False) -> bytes | str:
    command = ["git", "--git-dir", str(git_dir), *args]
    return subprocess.check_output(command, stderr=subprocess.STDOUT,
                                   text=not binary)


def ls_tree(git_dir: Path, commit: str) -> list[dict[str, object]]:
    raw = git(git_dir, "ls-tree", "-r", "-l", commit, "--",
              f"{TARGET_ROOT}/src", f"{TARGET_ROOT}/libdxfrw_sources.cmake")
    result: list[dict[str, object]] = []
    for line in str(raw).splitlines():
        meta, path = line.split("\t", 1)
        mode, kind, oid, size = meta.split()
        if kind != "blob":
            continue
        result.append({"path": path, "mode": mode, "blob": oid,
                       "size": None if size == "-" else int(size)})
    return sorted(result, key=lambda item: str(item["path"]))


def blob_bytes(git_dir: Path, oid: str) -> bytes:
    return bytes(git(git_dir, "cat-file", "blob", oid, binary=True))


def manifest(git_dir: Path, commit: str) -> dict[str, object]:
    entries = []
    for entry in ls_tree(git_dir, commit):
        content = blob_bytes(git_dir, str(entry["blob"]))
        if entry["size"] is not None and int(entry["size"]) != len(content):
            raise ValueError(f"Git size mismatch for {entry['path']}")
        item = dict(entry)
        item["sha256"] = hashlib.sha256(content).hexdigest()
        entries.append(item)
    return {"schema": MANIFEST_SCHEMA, "targetCommit": commit,
            "targetRoot": TARGET_ROOT, "entries": entries}


def source_list(git_dir: Path, commit: str) -> tuple[list[str], list[str]]:
    path = f"{TARGET_ROOT}/libdxfrw_sources.cmake"
    text = str(git(git_dir, "show", f"{commit}:{path}"))
    listed = []
    for line in text.splitlines():
        marker = "/src/"
        if marker not in line:
            continue
        value = line.split(marker, 1)[1].split('"', 1)[0].strip()
        if value.endswith((".cpp", ".h")):
            listed.append("src/" + value)
    actual = [str(item["path"])[len(TARGET_ROOT) + 1:]
              for item in ls_tree(git_dir, commit)
              if str(item["path"]).startswith(f"{TARGET_ROOT}/src/")]
    return sorted(set(listed)), sorted(actual)


def archive_sha256(git_dir: Path, commit: str) -> str:
    data = bytes(git(git_dir, "archive", "--format=tar", commit,
                     f"{TARGET_ROOT}/src", f"{TARGET_ROOT}/libdxfrw_sources.cmake",
                     binary=True))
    return hashlib.sha256(data).hexdigest()


def snapshot_revision(git_dir: Path, commit: str) -> str:
    return str(git(git_dir, "show", f"{commit}:{TARGET_ROOT}/.snapshot-revision")).strip()


def write_json(path: Path, value: object) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n",
                    encoding="utf-8")


def generate(args: argparse.Namespace) -> tuple[dict[str, object], dict[str, object]]:
    target_git = Path(args.target_git_dir).resolve()
    standalone_git = Path(args.standalone_git_dir).resolve()
    target = args.target_commit or str(git(target_git, "rev-parse", "origin/master")).strip()
    standalone = args.standalone_commit or str(git(standalone_git, "rev-parse", "origin/master")).strip()
    generated = manifest(target_git, target)
    listed, actual = source_list(target_git, target)
    missing_from_list = sorted(set(actual) - set(listed))
    extra_in_list = sorted(set(listed) - set(actual))
    generated["sourceList"] = {
        "path": f"{TARGET_ROOT}/libdxfrw_sources.cmake",
        "listed": listed,
        "missingFromList": missing_from_list,
        "extraInList": extra_in_list,
    }
    manifest_bytes = json.dumps(generated, indent=2, sort_keys=True).encode() + b"\n"
    manifest_hash = hashlib.sha256(manifest_bytes).hexdigest()
    lock = {
        "schema": LOCK_SCHEMA,
        "standaloneRepository": "LibreCAD/libdxfrw",
        "standaloneCommit": standalone,
        "targetRepository": "LibreCAD/LibreCAD",
        "targetCommit": target,
        "bundledSnapshotRevision": snapshot_revision(target_git, target),
        "archiveSha256": archive_sha256(target_git, target),
        "manifestSha256": manifest_hash,
        "manifestPath": args.manifest,
        "sourceManifestEntries": len(generated["entries"]),
        "sourceListOmissions": missing_from_list,
        "sourceListExtras": extra_in_list,
        "verification": {
            "mode": "local-remote-tracking-ref",
            "networkFetchRequiredBeforeImport": True,
            "note": "Refresh and record live remote transport before Checkpoint A.",
        },
    }
    return generated, lock


def load(path: Path) -> object:
    return json.loads(path.read_text(encoding="utf-8"))


def check(args: argparse.Namespace) -> int:
    generated, lock = generate(args)
    manifest_path = Path(args.manifest)
    lock_path = Path(args.lock)
    if args.write:
        write_json(manifest_path, generated)
        write_json(lock_path, lock)
        print(f"Wrote {manifest_path} ({len(generated['entries'])} entries)")
        print(f"Wrote {lock_path}")
        return 0
    if not manifest_path.exists() or not lock_path.exists():
        print("ERROR: manifest/lock missing; use --write", file=sys.stderr)
        return 1
    expected_manifest = load(manifest_path)
    expected_lock = load(lock_path)
    actual_manifest = json.dumps(generated, indent=2, sort_keys=True) + "\n"
    saved_manifest = manifest_path.read_text(encoding="utf-8")
    if saved_manifest != actual_manifest:
        print("ERROR: target source manifest differs from Git object data", file=sys.stderr)
        return 1
    if expected_lock != lock:
        print("ERROR: target lock differs from current commits/manifest", file=sys.stderr)
        return 1
    if not isinstance(expected_manifest, dict) or expected_manifest.get("schema") != MANIFEST_SCHEMA:
        print("ERROR: invalid manifest schema", file=sys.stderr)
        return 1
    print(f"Sync check: PASS ({len(generated['entries'])} entries; "
          f"{len(generated['sourceList']['missingFromList'])} source-list omissions recorded)")
    return 0


def self_test() -> None:
    assert hashlib.sha256(b"abc").hexdigest() == (
        "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad")
    assert sorted({"b", "a"}) == ["a", "b"]
    print("check_libdxfrw_sync self-test: PASS")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--target-git-dir", default="/Users/dli/dev/LibreCAD/.git")
    parser.add_argument("--standalone-git-dir", default=".git")
    parser.add_argument("--target-commit")
    parser.add_argument("--standalone-commit")
    parser.add_argument("--manifest", default="metadata/libdxfrw-target-source-manifest.json")
    parser.add_argument("--lock", default="metadata/libdxfrw-target-lock.json")
    parser.add_argument("--write", action="store_true")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args(argv)
    if args.self_test:
        self_test()
        return 0
    try:
        return check(args)
    except (OSError, ValueError, subprocess.CalledProcessError) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
