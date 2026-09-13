#!/usr/bin/env python3
"""Summarize an external DWG corpus without admitting its bytes to Git.

The report is intentionally non-reconstructive: it records only source/output
SHA-256 digests, six-byte version signatures, sizes, exit statuses, and bounded
diagnostic categories. Converted DXF files remain in a temporary directory.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
import sys
import tempfile
from collections import Counter
from pathlib import Path
import re


def version(path: Path) -> str:
    try:
        value = path.read_bytes()[:6].decode("ascii", "replace")
    except OSError:
        return "UNKNOWN"
    return value if re.fullmatch(r"AC\d{4}", value) else "UNKNOWN"


def digest(path: Path) -> str:
    hasher = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            hasher.update(chunk)
    return hasher.hexdigest()


def summarize(root: Path, converter: Path | None, limit: int) -> dict:
    files = sorted(root.rglob("*.dwg"))
    if limit >= 0:
        files = files[:limit]
    rows = []
    with tempfile.TemporaryDirectory(prefix="libdxfrw-advisory-") as temp:
        output_root = Path(temp)
        for index, path in enumerate(files):
            row = {
                "sourceSha256": digest(path),
                "sourceSize": path.stat().st_size,
                "inputVersion": version(path),
                "status": "inventoried",
            }
            if converter is not None:
                output = output_root / (str(index) + ".dxf")
                result = subprocess.run(
                    [str(converter), str(path), "-y", "-v2010", str(output)],
                    capture_output=True,
                    text=True,
                    check=False,
                )
                row["exitStatus"] = result.returncode
                row["status"] = "converted" if result.returncode == 0 else "failed"
                if output.is_file():
                    row["outputSha256"] = digest(output)
                    row["outputSize"] = output.stat().st_size
                # Do not retain paths, temporary names, or tool prose in the
                # committed/advisory summary; only a stable category belongs
                # in this non-reconstructive evidence.
                row["diagnosticCode"] = "success" if result.returncode == 0 else "conversion-failed"
            rows.append(row)
    rows.sort(key=lambda item: (item["inputVersion"], item["sourceSha256"]))
    statuses = Counter(row["status"] for row in rows)
    versions = Counter(row["inputVersion"] for row in rows)
    return {
        "schema": 1,
        "advisory": True,
        "fixturePolicy": "external-only; never committed",
        "inputCount": len(rows),
        "statusCounts": dict(sorted(statuses.items())),
        "versionCounts": dict(sorted(versions.items())),
        "rows": rows,
    }


def self_test() -> None:
    with tempfile.TemporaryDirectory(prefix="libdxfrw-advisory-test-") as temp:
        root = Path(temp)
        sample = root / "sample.dwg"
        sample.write_bytes(b"AC1021" + b"not-a-drawing")
        result = summarize(root, None, -1)
        assert result["inputCount"] == 1
        assert result["rows"][0]["inputVersion"] == "AC1021"
        assert result["rows"][0]["status"] == "inventoried"
    print("run_external_advisory self-test: PASS")


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--root", type=Path)
    parser.add_argument("--converter", type=Path)
    parser.add_argument("--limit", type=int, default=8)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args(argv)
    try:
        if args.self_test:
            self_test()
            return 0
        if args.root is None:
            parser.error("--root is required unless --self-test is used")
        report = summarize(args.root, args.converter, args.limit)
        encoded = json.dumps(report, sort_keys=True, indent=2) + "\n"
        if args.output:
            args.output.write_text(encoded, encoding="utf-8")
        else:
            sys.stdout.write(encoded)
        return 0
    except (OSError, UnicodeError, subprocess.SubprocessError) as exc:
        print("external advisory: FAIL: %s" % exc, file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
