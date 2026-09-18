#!/usr/bin/env python3
"""Verify the imported source tree against the locked target manifest."""

from __future__ import annotations

import argparse
import hashlib
import json
import sys
from pathlib import Path


def git_blob(data: bytes) -> str:
    return hashlib.sha1(b"blob " + str(len(data)).encode() + b"\0" + data).hexdigest()


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def check(root: Path, manifest_path: Path, allowlist_path: Path) -> None:
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    if manifest.get("schema") != "libdxfrw-target-source-manifest-v1":
        raise ValueError("unsupported source manifest schema")
    target_prefix = "libraries/libdxfrw/"
    entries = {item["path"]: item for item in manifest["entries"]}
    allowlist = json.loads(allowlist_path.read_text(encoding="utf-8"))
    allowed = set(allowlist.get("allowedPaths", []))
    adaptations = {item.get("path"): item for item in allowlist.get("entries", [])
                   if isinstance(item, dict) and item.get("path")}
    for target_path, entry in entries.items():
        if not target_path.startswith(target_prefix):
            raise ValueError(f"manifest path outside target root: {target_path}")
        local = root / target_path[len(target_prefix):]
        if not local.is_file():
            raise ValueError(f"imported file missing: {local}")
        data = local.read_bytes()
        if git_blob(data) == entry["blob"]:
            continue
        local_name = target_path[len(target_prefix):]
        adaptation = adaptations.get(target_path) or adaptations.get(local_name)
        if adaptation is None:
            raise ValueError(f"unallowlisted imported blob change: {target_path}")
        if adaptation.get("targetBlob") != entry["blob"]:
            raise ValueError(f"allowlist target blob mismatch: {target_path}")
        if adaptation.get("adaptedSha256") != sha256(data):
            raise ValueError(f"allowlist adapted hash mismatch: {target_path}")
    for path in root.rglob("*"):
        if not path.is_file():
            continue
        relative = path.relative_to(root).as_posix()
        if not relative.startswith("src/") or relative == "src/Makefile.am":
            continue
        # Some standalone-only compatibility helpers are intentionally kept
        # outside the target snapshot.  Their paths are explicitly recorded
        # in the allowlist so this check remains closed-world without forcing
        # those helpers into the LibreCAD source manifest.
        if relative in allowed:
            continue
        if target_prefix + relative not in entries:
            raise ValueError(f"unlocked source-root file: {relative}")


def self_test() -> None:
    assert git_blob(b"int x;\n") == "6d1a0d47b7f73eacb962f3711df06b21ed11f7ca"
    print("check_import_scope self-test: PASS")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path("."))
    parser.add_argument("--manifest", type=Path,
                        default=Path("metadata/libdxfrw-target-source-manifest.json"))
    parser.add_argument("--allowlist", type=Path,
                        default=Path("metadata/adaptation-allowlist.json"))
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args(argv)
    if args.self_test:
        self_test()
        return 0
    try:
        check(args.root.resolve(), args.manifest.resolve(), args.allowlist.resolve())
        print("Import scope check: PASS")
        return 0
    except (OSError, ValueError, KeyError, json.JSONDecodeError) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
