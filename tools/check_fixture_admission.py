#!/usr/bin/env python3
"""Reject unregistered drawing payloads from staged Git content.

The guard is deliberately conservative.  It checks staged paths by default,
then inspects drawing magic and common archive/source-literal forms.  A
registered row must prove either an exact pre-lock repository blob or a
local-from-scratch recipe and attestation.  The guard never downloads or
rewrites a fixture.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import subprocess
import sys
from pathlib import Path


DRAWING_EXTENSIONS = {".dwg", ".dxf"}
ARCHIVE_EXTENSIONS = {".7z", ".gz", ".rar", ".tar", ".tgz", ".zip"}
TEXT_METADATA_EXTENSIONS = {".cmake", ".json", ".md", ".py", ".txt", ".yaml", ".yml"}
SOURCE_EXTENSIONS = {".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx"}
DWG_MAGIC = re.compile(rb"^AC10[0-9A-Z]{2}")
DXF_MARKERS = (b"SECTION", b"ENTITIES", b"HEADER", b"EOF")


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def is_drawing_candidate(path: Path, data: bytes) -> tuple[bool, str]:
    suffix = path.suffix.lower()
    if suffix in DRAWING_EXTENSIONS:
        return True, f"drawing extension {suffix}"
    if suffix in ARCHIVE_EXTENSIONS:
        return True, f"archive extension {suffix}"
    if DWG_MAGIC.match(data[:6]):
        return True, "DWG magic"
    # Human-readable plans, registries, and scanner source routinely mention
    # format words and version strings.  They are not payloads merely because
    # those words occur.  C/C++ byte-array literals are checked separately
    # below; the guard's review attestation covers other source forms.
    if suffix in TEXT_METADATA_EXTENSIONS:
        return False, ""
    # Catch common source-literal forms without trying to prove that an
    # arbitrary encoder is a fixture.  Plain source strings such as
    # ``SECTION`` or ``AC1021`` are intentionally ignored; only escaped or
    # numeric byte literals can represent an embedded drawing payload.
    if suffix in SOURCE_EXTENSIONS and re.search(
            rb"(?:\\x41\\x43\\x31\\x30|0x41\\s*,\\s*0x43\\s*,\\s*0x31\\s*,\\s*0x30).{0,256}(?:\\x00|0x00|SECTION)",
            data, re.DOTALL | re.IGNORECASE):
        return True, "encoded drawing signature"
    if suffix in SOURCE_EXTENSIONS:
        return False, ""
    upper = data[:4096].upper()
    if sum(marker in upper for marker in DXF_MARKERS) >= 2:
        return True, "DXF structural markers"
    return False, ""


def load_registry(path: Path) -> dict:
    value = json.loads(path.read_text(encoding="utf-8"))
    if value.get("schema") != "libdxfrw-fixture-registry-v1":
        raise ValueError("unsupported fixture registry schema")
    if not isinstance(value.get("fixtures"), list):
        raise ValueError("fixture registry must contain a fixtures list")
    return value


def validate_fixture(row: dict) -> None:
    required = {"id", "path", "originKind", "sha256", "size", "formatVersion",
                "features", "expectedSemantics", "license"}
    missing = sorted(required - set(row))
    if missing:
        raise ValueError(f"fixture {row.get('id', '<unknown>')} missing {missing}")
    if row["originKind"] == "lockedRepositoryBlob":
        for key in ("repository", "commit", "gitPath", "blob"):
            if not row.get(key):
                raise ValueError(f"locked fixture {row['id']} missing {key}")
    elif row["originKind"] == "localFromScratch":
        for key in ("creator", "recipe", "noExternalInputAttestation"):
            if not row.get(key):
                raise ValueError(f"local fixture {row['id']} missing {key}")
    else:
        raise ValueError(f"fixture {row['id']} has disallowed originKind")


def staged_paths(repo: Path, base: str) -> list[Path]:
    output = subprocess.check_output(
        ["git", "-C", str(repo), "diff", "--cached", "--name-only", base],
        text=True)
    return [repo / line for line in output.splitlines() if line]


def check(args: argparse.Namespace) -> int:
    registry = load_registry(Path(args.registry))
    rows = {row.get("path"): row for row in registry["fixtures"]}
    for row in rows.values():
        validate_fixture(row)
    repo = Path(args.repo).resolve()
    paths = [Path(item).resolve() for item in args.path] if args.path else staged_paths(repo, args.base)
    violations: list[str] = []
    for path in paths:
        try:
            relative = path.relative_to(repo).as_posix()
        except ValueError:
            continue
        if not path.exists() or not path.is_file():
            continue
        data = path.read_bytes()
        candidate, reason = is_drawing_candidate(path, data)
        if not candidate:
            continue
        row = rows.get(relative)
        digest = sha256(data)
        if row is None:
            violations.append(f"{relative}: {reason}; no registry row")
            continue
        if row.get("sha256") != digest or int(row.get("size", -1)) != len(data):
            violations.append(f"{relative}: registry hash/size mismatch")
    if violations:
        for violation in violations:
            print(f"ERROR: {violation}", file=sys.stderr)
        return 1
    print(f"Fixture admission: PASS ({len(paths)} staged/path candidates checked)")
    return 0


def self_test() -> None:
    assert is_drawing_candidate(Path("x.dwg"), b"not a drawing")[0]
    assert is_drawing_candidate(Path("x.bin"), b"AC1021")[0]
    assert is_drawing_candidate(Path("x.bin"), b"0\nSECTION\n2\nHEADER\n0\nENDSEC\n")[0]
    assert not is_drawing_candidate(Path("x.txt"), b"ordinary source text")[0]
    assert not is_drawing_candidate(Path("x.cpp"), b'const char* x = "AC1021 SECTION HEADER";')[0]
    assert is_drawing_candidate(Path("x.cpp"), b'const unsigned char x[] = "\\x41\\x43\\x31\\x30\\x32\\x31\\x00SECTION";')[0]
    row = {
        "id": "local-1", "path": "tests/generated/x.dxf", "originKind": "localFromScratch",
        "sha256": "0" * 64, "size": 0, "formatVersion": "AC1021",
        "features": [], "expectedSemantics": {}, "license": "GPL-2.0-or-later",
        "creator": "self-test", "recipe": "empty", "noExternalInputAttestation": True,
    }
    validate_fixture(row)
    print("check_fixture_admission self-test: PASS")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--registry", default="metadata/fixture-registry.json")
    parser.add_argument("--repo", default=".")
    parser.add_argument("--base", default="HEAD")
    parser.add_argument("--path", action="append", default=[])
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args(argv)
    if args.self_test:
        self_test()
        return 0
    try:
        return check(args)
    except (OSError, ValueError, subprocess.CalledProcessError, json.JSONDecodeError) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
