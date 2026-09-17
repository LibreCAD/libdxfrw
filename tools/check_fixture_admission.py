#!/usr/bin/env python3
"""Guard staged drawings against the repository fixture-admission policy."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import subprocess
import sys
from pathlib import Path


DRAWING_EXTENSIONS = {".dwg", ".dxf"}
ARCHIVE_EXTENSIONS = {".zip", ".tar", ".gz", ".bz2", ".xz", ".7z"}
DRAWING_MAGIC = re.compile(rb"^AC10[0-9]{2}")
ASCII_DXF = re.compile(rb"0\r?\nSECTION\r?\n.*?2\r?\n(?:HEADER|CLASSES|TABLES|BLOCKS|ENTITIES|OBJECTS)\b", re.DOTALL)
ENCODED_DWG = re.compile(rb"(?:\\x41\\x43\\x31[0-9]{3}|bytes\(\s*\[\s*65\s*,\s*67\s*,\s*49)")


class AdmissionError(RuntimeError):
    pass


def run(*args):
    try:
        return subprocess.check_output(args, text=True, stderr=subprocess.STDOUT)
    except (OSError, subprocess.CalledProcessError) as exc:
        raise AdmissionError("command failed: %s" % exc) from exc


def candidate_kind(path, data):
    suffix = Path(path).suffix.lower()
    if suffix in DRAWING_EXTENSIONS:
        return "drawing-extension"
    if suffix in ARCHIVE_EXTENSIONS:
        return "archive-extension"
    if DRAWING_MAGIC.match(data[:8]):
        return "dwg-magic"
    if ASCII_DXF.search(data[:1024 * 1024]):
        return "dxf-content"
    if ENCODED_DWG.search(data[:1024 * 1024]):
        return "encoded-dwg-signature"
    return None


def staged_paths(base):
    output = run("git", "diff", "--name-only", "--diff-filter=AMR", base, "--")
    return [line for line in output.splitlines() if line]


def staged_bytes(path):
    try:
        return subprocess.check_output(["git", "show", ":" + path])
    except (OSError, subprocess.CalledProcessError) as exc:
        raise AdmissionError("cannot read staged path %s" % path) from exc


def load_registry(path):
    registry = json.loads(path.read_text(encoding="utf-8"))
    rows = {}
    for row in registry.get("fixtures", []):
        if row.get("path") in rows:
            raise AdmissionError("duplicate fixture registry path: %s" % row.get("path"))
        rows[row.get("path")] = row
    return rows


def validate_row(path, data, row):
    digest = hashlib.sha256(data).hexdigest()
    if row.get("sha256") != digest:
        raise AdmissionError("registry SHA-256 mismatch for %s" % path)
    if row.get("size") != len(data):
        raise AdmissionError("registry size mismatch for %s" % path)
    origin = row.get("originKind")
    if origin == "lockedRepositoryBlob":
        required = ("sourceRepository", "sourceCommit", "sourcePath", "sourceBlob")
    elif origin == "localFromScratch":
        required = ("creator", "creationMethod", "toolVersion", "recipe", "seed", "noExternalInputAttestation")
        if row.get("noExternalInputAttestation") is not True:
            raise AdmissionError("local-from-scratch fixture %s lacks a true no-external-input attestation" % path)
    else:
        raise AdmissionError("fixture %s has invalid originKind" % path)
    missing = [field for field in required if field not in row or row[field] in (None, "")]
    if missing:
        raise AdmissionError("fixture %s lacks: %s" % (path, ", ".join(missing)))


def check(args):
    rows = load_registry(args.registry)
    checked = []
    for path in staged_paths(args.base):
        data = staged_bytes(path)
        kind = candidate_kind(path, data)
        if kind is None:
            continue
        if kind == "archive-extension":
            raise AdmissionError("staged archive may contain an unadmitted drawing: %s" % path)
        if path not in rows:
            raise AdmissionError("detected %s without a fixture registry row: %s" % (kind, path))
        validate_row(path, data, rows[path])
        checked.append(path)
    print("fixture admission: PASS (%d staged drawing candidates checked)" % len(checked))


def self_test():
    assert candidate_kind("case.dwg", b"AC1027") == "drawing-extension"
    assert candidate_kind("case.txt", b"0\nSECTION\n2\nENTITIES\n0\nENDSEC") == "dxf-content"
    assert candidate_kind("payload.zip", b"PK\x03\x04") == "archive-extension"
    assert candidate_kind("reader.cpp", b"int main() { return 0; }") is None
    print("check_fixture_admission self-test: PASS")


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--base", default="HEAD")
    parser.add_argument("--registry", type=Path, default=Path("metadata/fixture-registry.json"))
    args = parser.parse_args(argv)
    try:
        if args.self_test:
            self_test()
            return 0
        check(args)
        return 0
    except (OSError, UnicodeError, json.JSONDecodeError, AdmissionError) as exc:
        print("fixture admission: FAIL: %s" % exc, file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
