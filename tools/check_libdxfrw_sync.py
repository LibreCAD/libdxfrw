#!/usr/bin/env python3
"""Check a pinned LibreCAD libdxfrw source snapshot without mutating files."""

from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
import sys
import tempfile
from pathlib import Path


class SyncError(RuntimeError):
    pass


def run(*args):
    try:
        return subprocess.check_output(args, text=True, stderr=subprocess.STDOUT)
    except (OSError, subprocess.CalledProcessError) as exc:
        raise SyncError("command failed: %s" % exc) from exc


def manifest_entries(path):
    entries = {}
    for line_number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        if not line or line.startswith("#"):
            continue
        fields = line.split("|")
        if len(fields) != 4 or not all(fields):
            raise SyncError("malformed manifest line %d" % line_number)
        name, mode, blob, classification = fields
        if classification not in {"manifest", "source", "header-listed", "header-unlisted"}:
            raise SyncError("invalid classification on manifest line %d" % line_number)
        if name in entries:
            raise SyncError("duplicate manifest path: %s" % name)
        entries[name] = (mode, blob, classification)
    return entries


def git_entries(git_dir, commit):
    output = run("git", "--git-dir=" + str(git_dir), "ls-tree", "-r", "--format=%(path)|%(objectmode)|%(objectname)", commit, "--", "libraries/libdxfrw/src", "libraries/libdxfrw/libdxfrw_sources.cmake")
    entries = {}
    for line in output.splitlines():
        name, mode, blob = line.split("|", 2)
        entries[name] = (mode, blob)
    return entries


def source_list(git_dir, commit):
    text = run("git", "--git-dir=" + str(git_dir), "show", "%s:libraries/libdxfrw/libdxfrw_sources.cmake" % commit)
    names = set()
    for token in text.replace('"', " ").split():
        marker = "/src/"
        if marker in token:
            names.add("libraries/libdxfrw/src/" + token.split(marker, 1)[1])
    return names


def archive_hash(git_dir, commit):
    command = ["git", "--git-dir=" + str(git_dir), "archive", "--format=tar", "--prefix=libdxfrw/", commit, "libraries/libdxfrw/src", "libraries/libdxfrw/libdxfrw_sources.cmake"]
    process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    digest = hashlib.sha256()
    assert process.stdout is not None
    for chunk in iter(lambda: process.stdout.read(1024 * 1024), b""):
        digest.update(chunk)
    error = process.stderr.read().decode("utf-8", "replace") if process.stderr else ""
    status = process.wait()
    if status:
        raise SyncError("git archive failed: %s" % error.strip())
    return digest.hexdigest()


def check(args):
    lock = json.loads(args.lock.read_text(encoding="utf-8"))
    target_commit = lock["libreCAD"]["commit"]
    if target_commit != args.target_commit:
        raise SyncError("target commit differs from lock")
    if lock["standalone"]["commit"] != args.standalone_commit:
        raise SyncError("standalone baseline differs from lock")
    expected_manifest_hash = lock["manifest"]["sha256"]
    actual_manifest_hash = hashlib.sha256(args.manifest.read_bytes()).hexdigest()
    if actual_manifest_hash != expected_manifest_hash:
        raise SyncError("manifest SHA-256 differs from lock")
    expected_entries = int(lock["manifest"]["entries"])
    manifest = manifest_entries(args.manifest)
    if len(manifest) != expected_entries:
        raise SyncError("manifest has %d entries; lock requires %d" % (len(manifest), expected_entries))
    actual = git_entries(args.target_git_dir, args.target_commit)
    actual_content = {name: value[:2] for name, value in actual.items()}
    manifest_content = {name: value[:2] for name, value in manifest.items()}
    if actual_content != manifest_content:
        missing = sorted(set(actual) - set(manifest))
        extra = sorted(set(manifest) - set(actual))
        changed = sorted(name for name in set(actual) & set(manifest) if actual_content[name] != manifest_content[name])
        raise SyncError("manifest mismatch: missing=%s extra=%s changed=%s" % (missing, extra, changed))
    listed = source_list(args.target_git_dir, args.target_commit)
    for name, value in manifest.items():
        expected = "manifest" if name.endswith("libdxfrw_sources.cmake") else "source" if name.endswith(".cpp") else "header-listed" if name in listed else "header-unlisted"
        if value[2] != expected:
            raise SyncError("wrong source-list classification for %s: %s (expected %s)" % (name, value[2], expected))
    if any(name.endswith(".cpp") and name not in listed for name in manifest):
        raise SyncError("source-list omits a tracked implementation source")
    expected_archive = lock["archive"]["sha256"]
    actual_archive = archive_hash(args.target_git_dir, args.target_commit)
    if actual_archive != expected_archive:
        raise SyncError("archive SHA-256 differs from lock")
    print("sync check: PASS (%d manifest entries, source list closed, archive verified)" % len(manifest))


def self_test():
    with tempfile.TemporaryDirectory() as directory:
        path = Path(directory) / "manifest.txt"
        path.write_text("# path|mode|blob|classification\na|100644|abc|source\n", encoding="utf-8")
        assert manifest_entries(path) == {"a": ("100644", "abc", "source")}
        try:
            path.write_text("# path|mode|blob|classification\na|100644|abc|source\na|100644|def|source\n", encoding="utf-8")
            manifest_entries(path)
        except SyncError:
            pass
        else:
            raise AssertionError("duplicate manifest path was accepted")
    print("check_libdxfrw_sync self-test: PASS")


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--target-git-dir", type=Path)
    parser.add_argument("--target-commit")
    parser.add_argument("--standalone-commit")
    parser.add_argument("--manifest", type=Path)
    parser.add_argument("--lock", type=Path)
    args = parser.parse_args(argv)
    try:
        if args.self_test:
            self_test()
            return 0
        required = (args.target_git_dir, args.target_commit, args.standalone_commit, args.manifest, args.lock)
        if any(value is None for value in required):
            parser.error("all sync-check arguments are required unless --self-test is used")
        check(args)
        return 0
    except (OSError, UnicodeError, KeyError, ValueError, SyncError, AssertionError) as exc:
        print("sync check: FAIL: %s" % exc, file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
