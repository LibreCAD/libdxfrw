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
import contextlib
import hashlib
import io
import json
import re
import subprocess
import sys
import tempfile
from pathlib import Path, PurePosixPath


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


def normalize_registry(value: dict) -> dict:
    if not isinstance(value, dict):
        raise ValueError("fixture registry must be an object")
    numeric_v1 = type(value.get("schema")) is int and value["schema"] == 1
    if not numeric_v1 and value.get("schema") != "libdxfrw-fixture-registry-v1":
        raise ValueError("unsupported fixture registry schema")
    if numeric_v1 and value.get("policy") != "lockedRepositoryBlob-or-localFromScratch":
        raise ValueError("unsupported fixture registry policy")
    if not isinstance(value.get("fixtures"), list):
        raise ValueError("fixture registry must contain a fixtures list")
    fixtures = []
    paths = set()
    for row in value["fixtures"]:
        if not isinstance(row, dict):
            raise ValueError("fixture registry rows must be objects")
        normalized = dict(row)
        if numeric_v1:
            required = {"path", "version", "expectedFirstStage", "semanticCounts"}
            if required - row.keys():
                raise ValueError("numeric-v1 fixture lacks version/semantic fields")
            if (not isinstance(row["expectedFirstStage"], str)
                    or not row["expectedFirstStage"]
                    or not isinstance(row["semanticCounts"], dict)
                    or any(not isinstance(key, str) or not key
                           or type(count) is not int or count < 0
                           for key, count in row["semanticCounts"].items())):
                raise ValueError("numeric-v1 fixture semantic fields are invalid")
            # The tracked v1 registry uses source-prefixed provenance names.
            # Adapt names only: retain every origin/hash/attestation check.
            aliases = {
                "id": row["path"], "formatVersion": row["version"],
                "expectedSemantics": {
                    "firstStage": row["expectedFirstStage"],
                    "counts": row["semanticCounts"],
                },
            }
            if row.get("originKind") == "lockedRepositoryBlob":
                for source, target in (
                        ("sourceRepository", "repository"),
                        ("sourceCommit", "commit"), ("sourcePath", "gitPath"),
                        ("sourceBlob", "blob")):
                    if source not in row:
                        raise ValueError(f"locked fixture lacks {source}")
                    aliases[target] = row[source]
            for key, expected in aliases.items():
                if key in row and row[key] != expected:
                    raise ValueError(f"conflicting fixture field {key}")
                normalized[key] = expected
        validate_fixture(normalized)
        if normalized["path"] in paths:
            raise ValueError(f"duplicate fixture path {normalized['path']}")
        paths.add(normalized["path"])
        fixtures.append(normalized)
    return {**value, "fixtures": fixtures}


def load_registry(path: Path) -> dict:
    return normalize_registry(json.loads(path.read_text(encoding="utf-8")))


def is_relative_path(value: object) -> bool:
    return (isinstance(value, str) and bool(value) and value != "."
            and "\\" not in value and ":" not in value
            and not any(c in value for c in ("\0", "\n", "\r"))
            and not PurePosixPath(value).is_absolute()
            and ".." not in PurePosixPath(value).parts
            and PurePosixPath(value).as_posix() == value)


def validate_fixture(row: dict) -> None:
    required = {"id", "path", "originKind", "sha256", "size", "formatVersion",
                "features", "expectedSemantics", "license"}
    missing = sorted(required - set(row))
    if missing:
        raise ValueError(f"fixture {row.get('id', '<unknown>')} missing {missing}")
    if not isinstance(row["id"], str) or not row["id"] or not is_relative_path(row["path"]):
        raise ValueError("fixture ID/path is invalid")
    if (not isinstance(row["sha256"], str)
            or re.fullmatch(r"[0-9a-f]{64}", row["sha256"]) is None
            or type(row["size"]) is not int or row["size"] < 0):
        raise ValueError(f"fixture {row['id']} hash/size is invalid")
    if (not isinstance(row["formatVersion"], str) or not row["formatVersion"]
            or not isinstance(row["features"], list)
            or any(not isinstance(item, str) for item in row["features"])
            or not isinstance(row["expectedSemantics"], dict)
            or not isinstance(row["license"], str) or not row["license"]):
        raise ValueError(f"fixture {row['id']} semantic/license fields are invalid")
    if row["originKind"] == "lockedRepositoryBlob":
        for key in ("repository", "commit", "gitPath", "blob"):
            if not row.get(key):
                raise ValueError(f"locked fixture {row['id']} missing {key}")
        if (row["repository"] not in {"LibreCAD/LibreCAD", "LibreCAD/libdxfrw"}
                or not is_relative_path(row["gitPath"])
                or not isinstance(row["commit"], str)
                or re.fullmatch(r"[0-9a-f]{40}", row["commit"]) is None
                or not isinstance(row["blob"], str)
                or re.fullmatch(r"[0-9a-f]{40}", row["blob"]) is None):
            raise ValueError(f"locked fixture {row['id']} provenance is invalid")
    elif row["originKind"] == "localFromScratch":
        for key in ("creator", "recipe", "noExternalInputAttestation"):
            if not row.get(key):
                raise ValueError(f"local fixture {row['id']} missing {key}")
        if (not isinstance(row["creator"], str) or not isinstance(row["recipe"], str)
                or row["noExternalInputAttestation"] is not True):
            raise ValueError(f"local fixture {row['id']} attestation is invalid")
    else:
        raise ValueError(f"fixture {row['id']} has disallowed originKind")


def staged_paths(repo: Path, base: str) -> list[str]:
    output = subprocess.check_output(
        ["git", "-C", str(repo), "diff", "--cached", "--name-only", "-z",
         "--diff-filter=ACMRT", base, "--"])
    return [item.decode("utf-8", errors="surrogateescape")
            for item in output.split(b"\0") if item]


def index_bytes(repo: Path, relative: str) -> bytes:
    entries = subprocess.check_output(
        ["git", "-C", str(repo), "--literal-pathspecs", "ls-files",
         "--stage", "-z", "--", relative]).split(b"\0")
    entries = [entry for entry in entries if entry]
    if len(entries) != 1:
        raise ValueError(f"{relative!r}: missing or unmerged index entry")
    metadata, name = entries[0].split(b"\t", 1)
    mode, blob, stage = metadata.split()
    if (mode not in (b"100644", b"100755") or stage != b"0"
            or name.decode("utf-8", errors="surrogateescape") != relative):
        raise ValueError(f"{relative!r}: index entry is not a regular file")
    return subprocess.check_output(
        ["git", "-C", str(repo), "cat-file", "blob", blob.decode("ascii")])


def check(args: argparse.Namespace) -> int:
    repo = Path(args.repo).resolve()
    registry_path = Path(args.registry)
    if not registry_path.is_absolute():
        registry_path = repo / registry_path
    # Do not resolve staged paths through the worktree: an unstaged symlink
    # or deletion must not change which Git blob is reviewed.
    registry_relative = registry_path.relative_to(repo).as_posix()
    if not is_relative_path(registry_relative):
        raise ValueError("fixture registry must be a repository-relative file")
    if args.path:
        registry_path = registry_path.resolve(strict=True)
        registry_path.relative_to(repo)
        registry = load_registry(registry_path)
        paths = []
        for item in args.path:
            path = Path(item)
            if not path.is_absolute():
                path = repo / path
            path = path.resolve(strict=True)
            relative = path.relative_to(repo).as_posix()
            if not path.is_file():
                raise ValueError(f"{relative!r}: manual path is not a file")
            paths.append((relative, path.read_bytes()))
        mode = "worktree"
    else:
        if subprocess.check_output(
                ["git", "-C", str(repo), "ls-files", "--unmerged", "-z"]):
            raise ValueError("fixture admission requires a merged index")
        registry = normalize_registry(json.loads(
            index_bytes(repo, registry_relative).decode("utf-8")))
        paths = [(relative, index_bytes(repo, relative))
                 for relative in staged_paths(repo, args.base)]
        mode = "index"
    rows = {row["path"]: row for row in registry["fixtures"]}
    violations: list[str] = []
    for relative, data in paths:
        candidate, reason = is_drawing_candidate(Path(relative), data)
        if not candidate:
            continue
        row = rows.get(relative)
        digest = sha256(data)
        if row is None:
            violations.append(f"{relative!r}: {reason}; no registry row")
            continue
        if row.get("sha256") != digest or int(row.get("size", -1)) != len(data):
            violations.append(f"{relative!r}: registry hash/size mismatch")
    if violations:
        for violation in violations:
            print(f"ERROR: {violation}", file=sys.stderr)
        return 1
    print(f"Fixture admission: PASS ({len(paths)} {mode} candidates checked)")
    return 0


def self_test() -> None:
    assert is_drawing_candidate(Path("x.dwg"), b"not a drawing")[0]
    assert is_drawing_candidate(Path("x.zip"), b"not a drawing")[0]
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
    normalize_registry({"schema": "libdxfrw-fixture-registry-v1", "fixtures": [row]})
    locked = {
        "path": "tests/fixtures/local.dxf", "version": "AC1021",
        "expectedFirstStage": "none", "semanticCounts": {"lines": 1},
        "sha256": "0" * 64, "size": 1, "features": ["LINE"],
        "license": "GPL-2.0-or-later", "originKind": "lockedRepositoryBlob",
        "sourceRepository": "LibreCAD/LibreCAD", "sourceCommit": "1" * 40,
        "sourcePath": "testdata/local.dxf", "sourceBlob": "2" * 40,
    }
    numeric = {"schema": 1, "policy": "lockedRepositoryBlob-or-localFromScratch",
               "fixtures": [locked]}
    adapted = normalize_registry(numeric)["fixtures"][0]
    assert adapted["commit"] == locked["sourceCommit"]
    assert adapted["expectedSemantics"]["counts"] == {"lines": 1}
    invalid = [
        {**numeric, "schema": True}, {**numeric, "schema": 2},
        {**numeric, "policy": "external-downloads"},
        {**numeric, "fixtures": [locked, locked]},
    ]
    for edit in ({"sourceBlob": ""}, {"sourceRepository": "External/Files"},
                 {"path": "../outside.dxf"}, {"size": True},
                 {"path": "."}, {"path": "C:/outside.dxf"},
                 {"commit": "3" * 40}, {"semanticCounts": []},
                 {"semanticCounts": {"lines": True}},
                 {"semanticCounts": {"lines": -1}}):
        invalid.append({**numeric, "fixtures": [{**locked, **edit}]})
    for candidate in invalid:
        try:
            normalize_registry(candidate)
        except ValueError:
            pass
        else:
            raise AssertionError("invalid registry was accepted")
    index_self_test(row)
    print("check_fixture_admission self-test: PASS")


def index_self_test(local_row: dict) -> None:
    # All payload bytes are authored here; the temporary repository has no
    # remotes and never changes the caller's index or Git configuration.
    with tempfile.TemporaryDirectory(prefix="fixture-index-test-") as directory:
        repo = Path(directory)

        def git(*arguments: str) -> None:
            subprocess.run(["git", "-C", str(repo), *arguments], check=True,
                           stdout=subprocess.PIPE, stderr=subprocess.PIPE)

        git("init", "-q")
        registry_path = repo / "registry.json"
        empty = {"schema": "libdxfrw-fixture-registry-v1", "fixtures": []}
        registry_path.write_text(json.dumps(empty), encoding="utf-8")
        baseline = repo / "baseline.txt"
        baseline.write_text("local test", encoding="utf-8")
        git("add", "--", "registry.json", "baseline.txt")
        git("-c", "user.name=Fixture Test", "-c",
            "user.email=fixture-test@example.invalid", "-c", "commit.gpgsign=false",
            "commit", "-qm", "local baseline")
        args = argparse.Namespace(repo=str(repo), registry="registry.json",
                                  base="HEAD", path=[])

        def result() -> int:
            with contextlib.redirect_stdout(io.StringIO()), \
                    contextlib.redirect_stderr(io.StringIO()):
                return check(args)

        payload = b"AC1021local-from-scratch"
        drawing = repo / "local drawing.dxf"
        drawing.write_bytes(payload)
        git("add", "--", drawing.name)
        eligible = {**local_row, "path": drawing.name,
                    "sha256": sha256(payload), "size": len(payload)}
        registry_path.write_text(json.dumps({**empty, "fixtures": [eligible]}),
                                 encoding="utf-8")
        assert result() == 1  # Unstaged registry cannot admit staged bytes.
        git("add", "--", "registry.json")
        assert result() == 0
        drawing.write_bytes(b"different worktree bytes")
        assert result() == 0  # Eligible index bytes, not this worktree copy.
        git("add", "--", drawing.name)
        drawing.write_bytes(payload)
        assert result() == 1  # Eligible worktree cannot hide ineligible index.
        drawing.unlink()
        assert result() == 1  # Worktree deletion cannot hide a staged blob.
        git("add", "-u", "--", drawing.name)
        assert result() == 0  # Deleted index paths are not payload candidates.

        odd = repo / "odd name\n[fixture].dxf"
        odd.write_bytes(payload)
        git("add", "--", odd.name)
        assert result() == 1  # NUL-delimited discovery handles odd paths.
        odd.unlink()
        git("add", "-u", "--", odd.name)
        archive = repo / "drawing.zip"
        archive.write_bytes(b"local archive sentinel")
        git("add", "--", archive.name)
        assert result() == 1
        archive.unlink()
        git("add", "-u", "--", archive.name)
        link = repo / "fixture-link"
        link.symlink_to(baseline.name)
        git("add", "--", link.name)
        try:
            result()
        except ValueError:
            pass
        else:
            raise AssertionError("staged symlink was accepted")


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
