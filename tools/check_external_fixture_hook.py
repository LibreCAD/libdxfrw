#!/usr/bin/env python3
"""Run the external-corpus admission hook without fetching or copying files."""

from __future__ import annotations

import argparse
import hashlib
import json
import sys
from pathlib import Path


class ExternalHookError(RuntimeError):
    pass


def verify(manifest, root):
    registry = json.loads(manifest.read_text(encoding="utf-8"))
    reports = []
    for row in registry.get("externalAdvisories", []):
        relative = row.get("path")
        if not relative or Path(relative).is_absolute():
            raise ExternalHookError("external advisory path must be relative")
        path = root / relative
        if not path.is_file():
            raise ExternalHookError("external advisory file is missing: %s" % path)
        data = path.read_bytes()
        digest = hashlib.sha256(data).hexdigest()
        if digest != row.get("sha256"):
            raise ExternalHookError("external advisory SHA-256 mismatch: %s" % relative)
        if row.get("size") != len(data):
            raise ExternalHookError("external advisory size mismatch: %s" % relative)
        reports.append({"path": relative, "sha256": digest, "version": row.get("version"), "status": "observed"})
    return {"schema": 1, "advisory": True, "fixtures": reports}


def self_test():
    import tempfile

    with tempfile.TemporaryDirectory() as directory:
        root = Path(directory)
        sample = root / "sample.dwg"
        data = b"external sample bytes"
        sample.write_bytes(data)
        manifest = root / "manifest.json"
        manifest.write_text(json.dumps({"externalAdvisories": [{"path": "sample.dwg", "sha256": hashlib.sha256(data).hexdigest(), "size": len(data), "version": "AC1027"}]}), encoding="utf-8")
        result = verify(manifest, root)
        assert result["advisory"] is True and result["fixtures"][0]["status"] == "observed"
    print("check_external_fixture_hook self-test: PASS")


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--manifest", type=Path, default=Path("metadata/fixture-registry.json"))
    parser.add_argument("--root", type=Path, default=Path("."))
    args = parser.parse_args(argv)
    try:
        if args.self_test:
            self_test()
            return 0
        print(json.dumps(verify(args.manifest, args.root), sort_keys=True, indent=2))
        return 0
    except (OSError, UnicodeError, json.JSONDecodeError, ExternalHookError) as exc:
        print("external fixture hook: FAIL: %s" % exc, file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
