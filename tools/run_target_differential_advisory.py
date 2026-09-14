#!/usr/bin/env python3
"""Compare two libdxfrw runners on an external DWG corpus.

This is advisory evidence only.  It records source/output hashes, versions,
sizes, exit categories, and a normalized relation between the two outputs. No
DWG/DXF payload is retained, copied into the repository, or treated as a
support-promoting fixture.
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


class DifferentialError(RuntimeError):
    pass


def digest(path: Path) -> str:
    hasher = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            hasher.update(chunk)
    return hasher.hexdigest()


def version(path: Path) -> str:
    try:
        value = path.read_bytes()[:6].decode("ascii", "replace")
    except OSError:
        return "UNKNOWN"
    return value if re.fullmatch(r"AC\d{4}", value) else "UNKNOWN"


def run_runner(runner: Path, source: Path, output: Path,
               timeout_seconds: float) -> dict:
    command = [str(runner), str(source), "-y", "-v2010", str(output)]
    try:
        result = subprocess.run(
            command,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            check=False,
            timeout=timeout_seconds if timeout_seconds > 0 else None,
        )
    except subprocess.TimeoutExpired:
        return {"status": "timeout", "diagnosticCode": "timeout"}
    row = {
        "status": "converted" if result.returncode == 0 else "failed",
        "exitStatus": result.returncode,
        "diagnosticCode": "success" if result.returncode == 0
        else "conversion-failed",
    }
    if output.is_file():
        row["outputSha256"] = digest(output)
        row["outputSize"] = output.stat().st_size
    return row


def relation(left: dict, right: dict) -> str:
    if left["status"] == "timeout" or right["status"] == "timeout":
        return "timeout"
    if left["status"] == "converted" and right["status"] == "converted":
        if (left.get("outputSha256") == right.get("outputSha256")
                and left.get("outputSize") == right.get("outputSize")):
            return "equal"
        return "delta"
    if left["status"] == "converted":
        return "standalone-only"
    if right["status"] == "converted":
        return "target-only"
    return "both-failed"


def compare(root: Path, standalone: Path, target: Path, limit: int,
            timeout_seconds: float, versions: set[str] | None = None) -> dict:
    files = sorted(root.rglob("*.dwg"))
    if versions:
        files = [path for path in files if version(path) in versions]
    if limit >= 0:
        files = files[:limit]
    rows = []
    with tempfile.TemporaryDirectory(prefix="libdxfrw-differential-") as temp:
        output_root = Path(temp)
        for index, source in enumerate(files):
            left = run_runner(standalone, source, output_root / f"{index}-standalone.dxf",
                              timeout_seconds)
            right = run_runner(target, source, output_root / f"{index}-target.dxf",
                               timeout_seconds)
            rows.append({
                "sourceSha256": digest(source),
                "sourceSize": source.stat().st_size,
                "inputVersion": version(source),
                "standalone": left,
                "target": right,
                "relation": relation(left, right),
            })
    rows.sort(key=lambda item: (item["inputVersion"], item["sourceSha256"]))
    relations = Counter(row["relation"] for row in rows)
    return {
        "schema": 1,
        "advisory": True,
        "fixturePolicy": "external-only; never committed",
        "inputCount": len(rows),
        "relationCounts": dict(sorted(relations.items())),
        "rows": rows,
    }


def self_test() -> None:
    with tempfile.TemporaryDirectory(prefix="libdxfrw-differential-test-") as temp:
        root = Path(temp)
        sample = root / "sample.dwg"
        sample.write_bytes(b"AC1024" + b"synthetic")
        runner = root / "runner.py"
        runner.write_text(
            "#!/usr/bin/env python3\n"
            "import pathlib, sys\n"
            "pathlib.Path(sys.argv[-1]).write_bytes(b'normalized')\n",
            encoding="utf-8",
        )
        runner.chmod(0o755)
        report = compare(root, runner, runner, -1, 1.0)
        assert report["relationCounts"] == {"equal": 1}
        broken = root / "broken.py"
        broken.write_text(
            "#!/usr/bin/env python3\nimport sys\nraise SystemExit(1)\n",
            encoding="utf-8",
        )
        broken.chmod(0o755)
        failed = compare(root, runner, broken, -1, 1.0)
        assert failed["relationCounts"] == {"standalone-only": 1}
    print("run_target_differential_advisory self-test: PASS")


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--root", type=Path)
    parser.add_argument("--standalone", type=Path)
    parser.add_argument("--target", type=Path)
    parser.add_argument("--limit", type=int, default=30)
    parser.add_argument("--versions", default="",
                        help="comma-separated DWG AC versions to include")
    parser.add_argument("--timeout", type=float, default=0.0,
                        help="per-runner timeout in seconds (0 disables)")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args(argv)
    try:
        if args.self_test:
            self_test()
            return 0
        if args.root is None or args.standalone is None or args.target is None:
            parser.error("--root, --standalone, and --target are required unless --self-test is used")
        if args.limit < -1:
            parser.error("--limit must be -1 or non-negative")
        if args.timeout < 0:
            parser.error("--timeout must be non-negative")
        versions = {item.strip() for item in args.versions.split(",") if item.strip()}
        if any(not re.fullmatch(r"AC\d{4}", item) for item in versions):
            parser.error("--versions entries must look like AC1024")
        report = compare(args.root, args.standalone, args.target, args.limit,
                         args.timeout, versions or None)
        encoded = json.dumps(report, sort_keys=True, indent=2) + "\n"
        if args.output:
            args.output.write_text(encoded, encoding="utf-8")
        else:
            sys.stdout.write(encoded)
        return 0
    except (DifferentialError, OSError, UnicodeError,
            subprocess.SubprocessError) as exc:
        print(f"target differential advisory: FAIL: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
