#!/usr/bin/env python3
"""Verify that an imported source tree is exactly the locked target scope."""

from __future__ import annotations

import argparse
import hashlib
import json
import tempfile
from pathlib import Path


class ImportError(RuntimeError):
    pass


def git_blob(path: Path) -> str:
    data = path.read_bytes()
    return hashlib.sha1(b"blob " + str(len(data)).encode("ascii") + b"\0" + data).hexdigest()


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def entries(path: Path) -> dict:
    result = {}
    for number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        if not line or line.startswith("#"):
            continue
        fields = line.split("|")
        if len(fields) != 4:
            raise ImportError("malformed manifest line %d" % number)
        target_path, mode, blob, classification = fields
        result[target_path] = (mode, blob, classification)
    return result


def check(root: Path, manifest_path: Path, allowlist_path: Path) -> None:
    manifest = entries(manifest_path)
    allowlist = json.loads(allowlist_path.read_text(encoding="utf-8"))
    allowed_paths = set(allowlist.get("allowedPaths", []))
    adaptations = {
        entry.get("path"): entry for entry in allowlist.get("entries", [])
        if isinstance(entry, dict) and entry.get("path")
    }
    for target_path, (_, expected_blob, _) in manifest.items():
        if not target_path.startswith("libraries/libdxfrw/"):
            raise ImportError("manifest path is outside libdxfrw: %s" % target_path)
        local = root / target_path.removeprefix("libraries/libdxfrw/")
        if not local.is_file():
            raise ImportError("imported file is missing: %s" % local)
        actual_blob = git_blob(local)
        local_name = target_path.removeprefix("libraries/libdxfrw/")
        if actual_blob != expected_blob:
            adaptation = adaptations.get(target_path) or adaptations.get(local_name)
            if adaptation is None:
                raise ImportError("imported blob differs from manifest: %s" % target_path)
            if adaptation.get("targetBlob") != expected_blob:
                raise ImportError("adaptation target blob is not the manifest blob: %s" % target_path)
            if adaptation.get("adaptedSha256") != sha256(local):
                raise ImportError("adapted SHA-256 does not match allowlist: %s" % target_path)
    for path in sorted(root.rglob("*")):
        if not path.is_file():
            continue
        relative = path.relative_to(root).as_posix()
        if relative.startswith(".git/"):
            continue
        target_name = "libraries/libdxfrw/" + relative
        if target_name in manifest:
            continue
        # Existing standalone build metadata is retained until the source-list
        # activation slice; no imported source may be silently outside the lock.
        if relative == "src/Makefile.am" or relative in allowed_paths:
            continue
        if relative.startswith("build") or relative.startswith("install"):
            continue
        if relative.startswith("metadata/") or relative.startswith("tools/"):
            continue
        if relative == "LIBRECAD_DXFRW_UPGRADE_PLAN.md" or relative == "LIBRECAD_SYNC.md":
            continue
        # This checker is intentionally narrow: report only source-root extras.
        if relative.startswith("src/"):
            raise ImportError("unlocked source-root file: %s" % relative)


def self_test() -> None:
    with tempfile.TemporaryDirectory() as directory:
        root = Path(directory)
        (root / "src").mkdir()
        source = root / "src/example.cpp"
        source.write_text("int x;\n", encoding="utf-8")
        blob = git_blob(source)
        manifest = root / "manifest.txt"
        manifest.write_text("src/example.cpp|100644|%s|source\n" % blob, encoding="utf-8")
        allowlist = root / "allow.json"
        allowlist.write_text('{"allowedPaths": []}\n', encoding="utf-8")
        # The checker expects the repository-relative prefix; use a nested root.
        nested = root / "tree"
        (nested / "src").mkdir(parents=True)
        (nested / "src/example.cpp").write_text(source.read_text(encoding="utf-8"), encoding="utf-8")
        manifest.write_text("libraries/libdxfrw/src/example.cpp|100644|%s|source\n" % blob, encoding="utf-8")
        check(nested, manifest, allowlist)


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path)
    parser.add_argument("--manifest", type=Path)
    parser.add_argument("--allowlist", type=Path)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args(argv)
    try:
        if args.self_test:
            self_test()
            print("import scope self-test: PASS")
            return 0
        if not args.root or not args.manifest or not args.allowlist:
            parser.error("--root, --manifest, and --allowlist are required")
        check(args.root.resolve(), args.manifest.resolve(), args.allowlist.resolve())
        print("import scope check: PASS")
        return 0
    except (OSError, UnicodeError, KeyError, ValueError, ImportError) as exc:
        parser.error(str(exc))


if __name__ == "__main__":
    raise SystemExit(main())
