#!/usr/bin/env python3
"""Apply the replayed FreeCAD candidate to a clean disposable Git checkout.

This modifies the selected checkout. It checks the pinned archive and tracked
base, restricts every patch to the five importer files, and restores their
original bytes on an ordinary failure. Use a disposable checkout: abrupt
termination, concurrent edits, ignored files and submodules are not covered.
"""
import argparse
import json
from pathlib import Path
import subprocess
import sys

import replay_freecad_import_patches as replay


def git(root, *args, check=True):
    return subprocess.run(["git", "-C", str(root), *args], capture_output=True,
                          text=True, check=check)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", required=True, type=Path)
    parser.add_argument("--source-archive", required=True, type=Path)
    parser.add_argument("--patch-dir", type=Path,
                        default=Path(__file__).resolve().parents[2] / "docs" / "patches")
    args = parser.parse_args()
    root = args.source_root.resolve(strict=True)
    archive = args.source_archive.resolve(strict=True)
    patch_dir = args.patch_dir.resolve(strict=True)
    replay_args = [sys.executable, str(Path(replay.__file__).resolve()),
                   "--source-archive", str(archive), "--patch-dir", str(patch_dir)]
    prepared = subprocess.run(replay_args, capture_output=True, text=True, check=False)
    if prepared.returncode:
        raise RuntimeError("candidate source replay failed:\n" + prepared.stderr.strip())
    expected = json.loads(prepared.stdout)
    if Path(git(root, "rev-parse", "--show-toplevel").stdout.strip()).resolve() != root:
        raise ValueError("source root must be the Git worktree root")
    if git(root, "rev-parse", "HEAD").stdout.strip() != expected["sourceCommit"]:
        raise ValueError("source checkout HEAD differs from the archive commit")
    if git(root, "status", "--porcelain", "--untracked-files=all", "--ignore-submodules=all").stdout:
        raise ValueError("source checkout contains tracked or untracked changes")
    originals = {}
    for relative in replay.FILES:
        item = root / relative
        if not item.is_file() or item.is_symlink():
            raise ValueError("candidate source must be a regular file: " + relative)
        originals[relative] = item.read_bytes()
    for name in replay.PATCHES:
        patch = patch_dir / name
        output = git(root, "apply", "--numstat", "-z", str(patch)).stdout
        paths = [row.split("\t", 2)[-1] for row in output.split("\0") if row]
        if not paths or any(path not in replay.FILES for path in paths):
            raise ValueError("patch touches an unexpected path: " + name)
    try:
        for name in replay.PATCHES:
            patch = patch_dir / name
            git(root, "apply", "--unidiff-zero", "--check", str(patch))
            git(root, "apply", "--unidiff-zero", str(patch))
        compared = subprocess.run(replay_args + ["--candidate-root", str(root)],
                                  capture_output=True, text=True, check=False)
        if compared.returncode:
            raise RuntimeError("candidate comparison failed:\n" + compared.stderr.strip())
        result = json.loads(compared.stdout)
        changed = set(git(root, "diff", "--name-only", "HEAD", "--ignore-submodules=all").stdout.splitlines())
        if changed != set(replay.FILES):
            raise ValueError("applied candidate changed an unexpected tracked-file set")
    except Exception as exc:
        failures = []
        for relative, contents in originals.items():
            try:
                target = root / relative
                target.write_bytes(contents)
                if target.read_bytes() != contents:
                    failures.append(relative + ": restored bytes differ")
            except OSError as restore_error:
                failures.append(relative + ": " + str(restore_error))
        if failures:
            raise RuntimeError("candidate application failed; rollback incomplete: " + "; ".join(failures)) from exc
        raise RuntimeError("candidate application failed; original file bytes restored") from exc
    print(json.dumps({"ok": True, "sourceCommit": result["sourceCommit"],
                      "archiveSha256": result["archiveSha256"], "patchCount": result["patchCount"],
                      "outputSha256": result["outputSha256"]}, sort_keys=True))


if __name__ == "__main__":
    main()
