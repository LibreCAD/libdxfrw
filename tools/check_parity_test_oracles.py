#!/usr/bin/env python3
"""Check the pinned LibreCAD test/oracle registry.

The registry records target-side test sources, CMake registrations, and
oracle routes without copying any DWG/DXF bytes.  It is execution evidence
metadata only: feature/source parity rows may link to a route, but a test or
oracle entry never promotes a format-support claim by itself.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import subprocess
import sys
import tempfile
from collections import Counter
from pathlib import Path


SCHEMA = 1
BLOB_RE = re.compile(r"^[0-9a-f]{40}$")
MODE_RE = re.compile(r"^100(?:644|755)$")
DRAWING_SUFFIXES = {".dwg", ".dxf"}
FIXTURE_POLICY = "no-drawing-payloads; source-and-metadata-only"
ALLOWED_CLASSIFICATIONS = {"portable", "LibreCAD-only", "fixture-blocked", "external-advisory"}
ALLOWED_FIXTURE_DISPOSITIONS = {"no-drawing-fixture", "requires-admitted-fixture", "external-only"}


class RegistryError(RuntimeError):
    """Raised when a registry is malformed or does not match the target."""


def read_json(path: Path) -> dict:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as exc:
        raise RegistryError("cannot read JSON %s: %s" % (path, exc)) from exc
    if not isinstance(value, dict):
        raise RegistryError("JSON root must be an object: %s" % path)
    return value


def git_bytes(repo: Path, *args: str) -> bytes:
    try:
        return subprocess.check_output(["git", "-C", str(repo), *args], stderr=subprocess.STDOUT)
    except (OSError, subprocess.CalledProcessError) as exc:
        raise RegistryError("git command failed: %s" % exc) from exc


def git_text(repo: Path, *args: str) -> str:
    try:
        return git_bytes(repo, *args).decode("utf-8")
    except UnicodeDecodeError as exc:
        raise RegistryError("target Git output is not UTF-8") from exc


def target_metadata(source_lock: dict) -> dict:
    target = source_lock.get("libreCAD")
    if not isinstance(target, dict) or not BLOB_RE.fullmatch(str(target.get("commit", ""))):
        raise RegistryError("source lock has no valid LibreCAD commit")
    if not target.get("repository"):
        raise RegistryError("source lock has no LibreCAD repository")
    return target


def decode_text(data: bytes, path: str) -> str:
    if b"\0" in data:
        raise RegistryError("registry entry is not text: %s" % path)
    if data.startswith(b"AC10") or data.startswith(b"AutoCAD Binary DXF"):
        raise RegistryError("registry entry resembles a drawing payload: %s" % path)
    try:
        text = data.decode("utf-8")
    except UnicodeDecodeError as exc:
        raise RegistryError("registry entry is not UTF-8: %s" % path) from exc
    first = text.lstrip("\ufeff\r\n\t ")
    if first.startswith("0\nSECTION") or first.startswith("0\r\nSECTION"):
        raise RegistryError("registry entry resembles ASCII DXF: %s" % path)
    return text


def target_entries(repo: Path, commit: str, paths: list[str]) -> dict[str, tuple[str, str]]:
    git_text(repo, "cat-file", "-e", commit + "^{commit}")
    output = git_text(repo, "ls-tree", "-r", "--format=%(objectmode)|%(objectname)|%(path)", commit, "--", *paths)
    result: dict[str, tuple[str, str]] = {}
    for line in output.splitlines():
        mode, blob, path = line.split("|", 2)
        result[path] = (mode, blob)
    return result


def load_source_lock(path: Path) -> dict:
    return read_json(path)


def validate_entry(entry: object, index: int) -> dict:
    if not isinstance(entry, dict):
        raise RegistryError("entry %d is not an object" % index)
    required = {"id", "path", "mode", "blob", "classification", "role", "registeredTargets", "fixtureDisposition"}
    missing = required - set(entry)
    if missing:
        raise RegistryError("entry %d is missing %s" % (index, sorted(missing)))
    for field in ("id", "path", "mode", "blob", "classification", "role", "fixtureDisposition"):
        if not isinstance(entry[field], str) or not entry[field]:
            raise RegistryError("entry %d has invalid %s" % (index, field))
    if not MODE_RE.fullmatch(entry["mode"]):
        raise RegistryError("entry %d has invalid mode" % index)
    if not BLOB_RE.fullmatch(entry["blob"]):
        raise RegistryError("entry %d has invalid blob" % index)
    if entry["classification"] not in ALLOWED_CLASSIFICATIONS:
        raise RegistryError("entry %d has invalid classification" % index)
    if entry["fixtureDisposition"] not in ALLOWED_FIXTURE_DISPOSITIONS:
        raise RegistryError("entry %d has invalid fixture disposition" % index)
    if not isinstance(entry["registeredTargets"], list) or any(not isinstance(x, str) or not x for x in entry["registeredTargets"]):
        raise RegistryError("entry %d has invalid registeredTargets" % index)
    if Path(entry["path"]).suffix.lower() in DRAWING_SUFFIXES or "/testdata/" in "/" + entry["path"]:
        raise RegistryError("registry cannot include drawing/testdata bytes: %s" % entry["path"])
    if entry["classification"] == "fixture-blocked" and entry["fixtureDisposition"] != "requires-admitted-fixture":
        raise RegistryError("fixture-blocked entry must require an admitted fixture: %s" % entry["id"])
    if entry["classification"] != "fixture-blocked" and entry["fixtureDisposition"] == "requires-admitted-fixture":
        raise RegistryError("only fixture-blocked entries may require fixtures: %s" % entry["id"])
    return entry


def validate_route(route: object, index: int) -> dict:
    if not isinstance(route, dict):
        raise RegistryError("route %d is not an object" % index)
    required = {"id", "kind", "testTargets", "entryIds", "promotesSupport"}
    missing = required - set(route)
    if missing:
        raise RegistryError("route %d is missing %s" % (index, sorted(missing)))
    if not isinstance(route["id"], str) or not route["id"]:
        raise RegistryError("route %d has invalid id" % index)
    if not isinstance(route["kind"], str) or not route["kind"]:
        raise RegistryError("route %d has invalid kind" % index)
    for field in ("testTargets", "entryIds"):
        if not isinstance(route[field], list) or any(not isinstance(x, str) or not x for x in route[field]):
            raise RegistryError("route %d has invalid %s" % (index, field))
    if route["promotesSupport"] is not False:
        raise RegistryError("oracle routes must not promote support: %s" % route["id"])
    return route


def validate_feature_link(link: object, index: int, route_ids: set[str]) -> dict:
    if not isinstance(link, dict):
        raise RegistryError("feature link %d is not an object" % index)
    required = {"selector", "oracleRouteIds", "evidenceRole", "promotesSupport"}
    missing = required - set(link)
    if missing:
        raise RegistryError("feature link %d is missing %s" % (index, sorted(missing)))
    selector = link["selector"]
    if not isinstance(selector, dict) or set(selector) != {"facade", "category"}:
        raise RegistryError("feature link %d has invalid selector" % index)
    if not all(isinstance(selector[x], str) and selector[x] for x in ("facade", "category")):
        raise RegistryError("feature link %d has empty selector" % index)
    routes = link["oracleRouteIds"]
    if not isinstance(routes, list) or not routes or any(x not in route_ids for x in routes):
        raise RegistryError("feature link %d references an unknown route" % index)
    if not isinstance(link["evidenceRole"], str) or not link["evidenceRole"]:
        raise RegistryError("feature link %d has no evidence role" % index)
    if link["promotesSupport"] is not False:
        raise RegistryError("feature links must not promote support")
    return link


def validate_registry(registry: dict, source_lock: dict, target_repo: Path, mapping_path: Path | None = None) -> None:
    if registry.get("schema") != SCHEMA or registry.get("kind") != "librecad-parity-test-oracle-registry":
        raise RegistryError("unexpected registry schema or kind")
    if registry.get("fixturePolicy") != FIXTURE_POLICY:
        raise RegistryError("registry has unsafe fixture policy")
    target = target_metadata(source_lock)
    locked_target = registry.get("target")
    if not isinstance(locked_target, dict) or locked_target.get("commit") != target["commit"] or locked_target.get("repository") != target["repository"]:
        raise RegistryError("registry target differs from source lock")
    entries_value = registry.get("entries")
    if not isinstance(entries_value, list) or not entries_value:
        raise RegistryError("registry needs entries")
    entries = [validate_entry(entry, i + 1) for i, entry in enumerate(entries_value)]
    ids = [entry["id"] for entry in entries]
    paths = [entry["path"] for entry in entries]
    if ids != sorted(ids) or len(ids) != len(set(ids)):
        raise RegistryError("entry IDs must be sorted and unique")
    if paths != sorted(paths) or len(paths) != len(set(paths)):
        raise RegistryError("entry paths must be sorted and unique")
    expected = {entry["path"]: (entry["mode"], entry["blob"]) for entry in entries}
    actual = target_entries(target_repo, target["commit"], paths)
    if actual != expected:
        missing = sorted(set(expected) - set(actual))
        changed = sorted(path for path in set(expected) & set(actual) if expected[path] != actual[path])
        raise RegistryError("target registry entries differ: missing=%s changed=%s" % (missing, changed))
    entry_by_id = {entry["id"]: entry for entry in entries}
    for entry in entries:
        text = decode_text(git_bytes(target_repo, "show", "%s:%s" % (target["commit"], entry["path"])), entry["path"])
        has_fixture_reference = bool(re.search(r"(?i)(?:testdata/|LIBRECAD_TEST_DIR)", text))
        if entry["path"] in {"CMakeLists.txt", "libraries/libdxfrw/CMakeLists.txt"}:
            has_fixture_reference = False
        if has_fixture_reference and entry["classification"] != "fixture-blocked":
            raise RegistryError("fixture reference must be marked fixture-blocked: %s" % entry["id"])
    routes_value = registry.get("routes")
    if not isinstance(routes_value, list) or not routes_value:
        raise RegistryError("registry needs oracle routes")
    routes = [validate_route(route, i + 1) for i, route in enumerate(routes_value)]
    route_ids = [route["id"] for route in routes]
    if route_ids != sorted(route_ids) or len(route_ids) != len(set(route_ids)):
        raise RegistryError("route IDs must be sorted and unique")
    for route in routes:
        if any(entry_id not in entry_by_id for entry_id in route["entryIds"]):
            raise RegistryError("route references unknown entry: %s" % route["id"])
    links_value = registry.get("featureLinks")
    if not isinstance(links_value, list) or not links_value:
        raise RegistryError("registry needs feature links")
    links = [validate_feature_link(link, i + 1, set(route_ids)) for i, link in enumerate(links_value)]
    selectors = [(link["selector"]["facade"], link["selector"]["category"]) for link in links]
    if selectors != sorted(selectors) or len(selectors) != len(set(selectors)):
        raise RegistryError("feature selectors must be sorted and unique")
    if mapping_path is not None:
        mapping = read_json(mapping_path).get("mapping", {})
        rows = mapping.get("rows")
        if not isinstance(rows, list) or not rows:
            raise RegistryError("parity mapping has no rows")
        selector_set = set(selectors)
        observed = {(row.get("facade"), row.get("category")) for row in rows}
        missing = sorted(observed - selector_set)
        extra = sorted(selector_set - observed)
        if missing or extra:
            raise RegistryError("feature-link selector coverage differs: missing=%s extra=%s" % (missing, extra))
    print("parity test/oracle registry: PASS (%d source entries; %d routes; %d feature selectors; no drawing payloads)" % (len(entries), len(routes), len(links)))


def cmake_source_set(cmake: str, variable: str) -> set[str]:
    match = re.search(r"set\(" + re.escape(variable) + r"(?P<body>.*?)\n\s*\)", cmake, re.DOTALL)
    if not match:
        return set()
    return set(re.findall(r"(?:libraries|librecad)/[^\s)]+\.(?:cpp|h)", match.group("body")))


def classify_source(path: str, text: str, pure_dwg: set[str], pure_dxf: set[str]) -> tuple[str, str, str, list[str]]:
    lower = text.lower()
    manifest = path in {"CMakeLists.txt", "libraries/libdxfrw/CMakeLists.txt"}
    fixture = bool(re.search(r"(?i)(?:testdata/|librecad_test_dir)", text))
    external = path == "libraries/libdxfrw/tests/fuzz/fuzz_dwg_read.cpp" or path.startswith("scripts/") or path.startswith("tests/fixtures/")
    if manifest:
        classification = "portable"
        disposition = "no-drawing-fixture"
    elif fixture:
        classification = "fixture-blocked"
        disposition = "requires-admitted-fixture"
    elif external:
        classification = "external-advisory"
        disposition = "external-only"
    elif path.startswith("libraries/libdxfrw/tests/") or path in pure_dwg or path in pure_dxf:
        classification = "portable"
        disposition = "no-drawing-fixture"
    elif path.startswith("librecad/src/lib/filters/tests/"):
        classification = "LibreCAD-only"
        disposition = "no-drawing-fixture"
    else:
        classification = "portable"
        disposition = "no-drawing-fixture"
    if path.endswith("fuzz_dwg_read.cpp"):
        role = "dwg-runtime-oracle"
        targets = []
    elif path.endswith("fuzz_dxf_read.cpp"):
        role = "dxf-runtime-oracle"
        targets = []
    elif path.endswith("fuzz_null_interface.h"):
        role = "shared-runtime-oracle"
        targets = []
    elif path in pure_dxf:
        role = "dxf-runtime-oracle"
        targets = ["libdxfrw_dxf_fast_tests", "libdxfrw_" + Path(path).stem]
    elif path in pure_dwg:
        role = "dwg-runtime-oracle"
        targets = ["libdxfrw_dwg_fast_tests", "libdxfrw_" + Path(path).stem]
    elif manifest:
        role = "test-manifest"
        targets = ["libdxfrw_dxf_fast_tests", "libdxfrw_dwg_fast_tests", "libdxfrw_fast_tests"]
    elif path.startswith("scripts/") or path.startswith("tests/fixtures/"):
        role = "external-oracle-receipt"
        targets = []
    elif "dxf" in path.lower() or "dxf" in lower:
        role = "dxf-runtime-oracle"
        targets = ["librecad_tests"]
    elif "dwg" in path.lower() or "dwg" in lower:
        role = "dwg-runtime-oracle"
        targets = ["librecad_tests"]
    else:
        role = "shared-runtime-oracle"
        targets = ["libdxfrw_fast_tests"]
    return classification, role, disposition, targets


def generate(args: argparse.Namespace) -> dict:
    source_lock = load_source_lock(args.source_lock)
    target = target_metadata(source_lock)
    commit = target["commit"]
    paths: set[str] = {"CMakeLists.txt", "libraries/libdxfrw/CMakeLists.txt"}
    for line in git_text(args.target_repo, "ls-tree", "-r", "--name-only", commit, "--", "libraries/libdxfrw/tests", "librecad/src/lib/filters/tests", "scripts", "tests/fixtures").splitlines():
        if (line.endswith((".cpp", ".h", ".cmake", ".json", ".py")) and "/testdata/" not in "/" + line and ("tests" in line or line in {"scripts/oracle_receipts.py", "scripts/test_external_oracle_receipts.py"})):
            paths.add(line)
    entries = []
    cmake = decode_text(git_bytes(args.target_repo, "show", "%s:CMakeLists.txt" % commit), "CMakeLists.txt")
    pure_dwg = cmake_source_set(cmake, "LIBDXFRW_FAST_DWG_TEST_SOURCES") | cmake_source_set(cmake, "LIBDXFRW_SLOW_DWG_TEST_SOURCES")
    pure_dxf = cmake_source_set(cmake, "LIBDXFRW_FAST_DXF_TEST_SOURCES")
    for path in sorted(paths):
        mode, blob = target_entries(args.target_repo, commit, [path])[path]
        text = decode_text(git_bytes(args.target_repo, "show", "%s:%s" % (commit, path)), path)
        classification, role, disposition, targets = classify_source(path, text, pure_dwg, pure_dxf)
        entries.append({"id": "source-%04d" % (len(entries) + 1), "path": path, "mode": mode, "blob": blob, "classification": classification, "role": role, "registeredTargets": sorted(set(targets)), "fixtureDisposition": disposition})
    route_specs = [
        ("oracle:dxf-fast", "fast-runtime", ["libdxfrw_dxf_fast_tests"], "dxf-runtime-oracle"),
        ("oracle:dwg-fast", "fast-runtime", ["libdxfrw_dwg_fast_tests"], "dwg-runtime-oracle"),
        ("oracle:shared-fast", "fast-runtime", ["libdxfrw_fast_tests"], "shared-runtime-oracle"),
        ("oracle:source-lexical", "source-only", [], "test-manifest"),
        ("oracle:external-advisory", "external-advisory", [], "external-oracle-receipt"),
    ]
    routes = []
    for route_id, kind, targets, role in route_specs:
        matching = [entry["id"] for entry in entries if entry["role"] == role and (kind not in {"fast-runtime"} or entry["classification"] != "fixture-blocked")]
        if not matching and role == "test-manifest":
            matching = [entry["id"] for entry in entries if entry["path"] == "CMakeLists.txt"]
        routes.append({"id": route_id, "kind": kind, "testTargets": targets, "entryIds": matching, "promotesSupport": False})
    rows = read_json(args.mapping).get("mapping", {}).get("rows", [])
    observed = sorted({(row["facade"], row["category"]) for row in rows})
    links = []
    for facade, category in observed:
        if facade == "dxfRW":
            route = "oracle:dxf-fast"
        elif facade == "dwgRW":
            route = "oracle:dwg-fast"
        elif category in {"source-unit", "source-unit-coverage", "pipeline-unit", "implementation-anchor"}:
            route = "oracle:source-lexical"
        elif category in {"public-method", "public-inline-method", "public-inline-function", "public-type", "public-enum", "public-header", "interface-method", "callback", "model-codec", "public-model", "internal-model-codec"}:
            route = "oracle:shared-fast"
        else:
            route = "oracle:external-advisory"
        links.append({"selector": {"facade": facade, "category": category}, "oracleRouteIds": [route], "evidenceRole": "execution-oracle-candidate", "promotesSupport": False})
    routes.sort(key=lambda route: route["id"])
    return {"schema": SCHEMA, "kind": "librecad-parity-test-oracle-registry", "target": {"repository": target["repository"], "commit": commit}, "fixturePolicy": FIXTURE_POLICY, "routes": routes, "entries": entries, "featureLinks": links}


def self_test() -> None:
    with tempfile.TemporaryDirectory() as directory:
        root = Path(directory)
        repo = root / "target"
        repo.mkdir()
        subprocess.check_call(["git", "-C", str(repo), "init", "-q"])
        (repo / "CMakeLists.txt").write_text("add_test(NAME libdxfrw_dxf_fast_tests)\n", encoding="utf-8")
        subprocess.check_call(["git", "-C", str(repo), "add", "CMakeLists.txt"])
        subprocess.check_call(["git", "-C", str(repo), "-c", "user.name=test", "-c", "user.email=test@example.invalid", "commit", "-qm", "seed"])
        commit = git_text(repo, "rev-parse", "HEAD").strip()
        blob = git_text(repo, "rev-parse", "HEAD:CMakeLists.txt").strip()
        registry = {"schema": 1, "kind": "librecad-parity-test-oracle-registry", "target": {"repository": "local", "commit": commit}, "fixturePolicy": FIXTURE_POLICY, "routes": [{"id": "oracle:source-lexical", "kind": "source-only", "testTargets": [], "entryIds": ["source-0001"], "promotesSupport": False}], "entries": [{"id": "source-0001", "path": "CMakeLists.txt", "mode": "100644", "blob": blob, "classification": "portable", "role": "test-manifest", "registeredTargets": ["libdxfrw_dxf_fast_tests"], "fixtureDisposition": "no-drawing-fixture"}], "featureLinks": [{"selector": {"facade": "shared", "category": "source-unit"}, "oracleRouteIds": ["oracle:source-lexical"], "evidenceRole": "execution-oracle-candidate", "promotesSupport": False}]}
        lock = {"libreCAD": {"repository": "local", "commit": commit}}
        validate_registry(registry, lock, repo)
        bad = json.loads(json.dumps(registry))
        bad["featureLinks"][0]["promotesSupport"] = True
        try:
            validate_registry(bad, lock, repo)
        except RegistryError:
            pass
        else:
            raise AssertionError("support-promoting link was accepted")
    print("check_parity_test_oracles self-test: PASS")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--write", action="store_true")
    parser.add_argument("--target-repo", type=Path, default=Path("/private/tmp/librecad-system-s16"))
    parser.add_argument("--source-lock", type=Path, default=Path("metadata/libdxfrw-target-lock.json"))
    parser.add_argument("--mapping", type=Path, default=Path("metadata/parity-source-routes-v1.json"))
    parser.add_argument("--registry", type=Path, default=Path("metadata/parity-test-oracles-v1.json"))
    args = parser.parse_args(argv)
    try:
        if args.self_test:
            self_test()
            return 0
        if args.write:
            registry = generate(args)
            args.registry.write_text(json.dumps(registry, indent=2, sort_keys=False) + "\n", encoding="utf-8")
        registry = read_json(args.registry)
        validate_registry(registry, load_source_lock(args.source_lock), args.target_repo, args.mapping)
        return 0
    except (OSError, UnicodeError, json.JSONDecodeError, RegistryError, subprocess.CalledProcessError) as exc:
        print("parity test/oracle registry: FAIL: %s" % exc, file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
