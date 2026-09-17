#!/usr/bin/env python3
"""Run and attest the bounded native qualification test profiles.

The hosted profile deliberately builds with ``LIBDXFRW_BUILD_QUALIFICATION``
disabled.  Under that profile the semantic differential tests are contract
self-tests, not live independent-oracle runs.  This runner inspects CTest's
registered command for every focused test and records the observed execution
mode so a green schema self-test cannot be mistaken for semantic evidence.

The runner never reads or copies a drawing.  Its output allow-list contains
only JSON metadata, JUnit XML, text logs, and a detached receipt digest.
"""

from __future__ import annotations

import argparse
import datetime as dt
import hashlib
import json
import math
import os
import platform
import re
import shutil
import subprocess
import sys
import tempfile
import unicodedata
import xml.etree.ElementTree as ET
from pathlib import Path
from typing import Any, Iterable


ROOT = Path(__file__).resolve().parents[1]
CONTRACT_SCHEMA = 1
CONTRACT_KIND = "libdxfrw-native-qualification-test-contract"
OBSERVATION_SCHEMA = 1
OBSERVATION_KIND = "libdxfrw-native-qualification-observation"
STATUS_KIND = "libdxfrw-native-qualification-status"
FOCUSED_LABEL = "native-qualification-focused"
MODES = {"native-executable", "contract-self-test"}
SHA256_RE = re.compile(r"[0-9a-f]{64}")
COMMIT_RE = re.compile(r"[0-9a-f]{40}")
SAFE_ID_RE = re.compile(r"[A-Za-z0-9][A-Za-z0-9_.+-]{0,127}")

CONTRACT_KEYS = {
    "schema", "kind", "canonicalization", "artifactPolicy", "hostedProfile",
    "focusedLabel", "semanticEvidencePolicy", "focusedTests", "focusedCount",
    "focusedNameDigest", "broadInventory",
}
CANONICALIZATION_KEYS = {"json", "testNameDigest"}
FOCUSED_TEST_KEYS = {
    "id", "expectedExecutionMode", "expectedCommandBasename",
    "suppliesIndependentSemanticEvidence", "reason",
}
BROAD_KEYS = {
    "status", "generatedAtCommit", "configurationDigest", "count",
    "nameDigest", "names",
}
HOSTED_PROFILE = {
    "BUILD_SHARED_LIBS": "OFF",
    "CMAKE_BUILD_TYPE": "Release",
    "LIBDXFRW_BUILD_DOC": "OFF",
    "LIBDXFRW_BUILD_DWG2DXF": "ON",
    "LIBDXFRW_BUILD_LONG_FUZZ": "OFF",
    "LIBDXFRW_BUILD_QUALIFICATION": "OFF",
    "LIBDXFRW_BUILD_TESTS": "ON",
}
HOSTED_MATRIX = {
    "linux-gcc": {
        "declaredToolchain": "gcc-13",
        "generator": "Ninja",
        "generatorPlatform": "",
        "compilerId": "GNU",
        "compilerBasenames": {"g++-13"},
    },
    "macos-clang": {
        "declaredToolchain": "apple-clang",
        "generator": "Ninja",
        "generatorPlatform": "",
        "compilerId": "AppleClang",
        "compilerBasenames": {"clang++"},
    },
    "windows-msvc": {
        "declaredToolchain": "msvc-vs2022-x64",
        "generator": "Visual Studio 17 2022",
        "generatorPlatform": "x64",
        "compilerId": "MSVC",
        "compilerBasenames": {"cl", "cl.exe"},
    },
}
CANONICAL_JSON = (
    "UTF-8; exact keys; no duplicate keys; no non-finite numbers; "
    "sort_keys=true; separators=(',', ':'); ensure_ascii=false; trailing LF"
)
CANONICAL_TEST_NAMES = (
    "SHA-256 over NFC-normalized, unique test names sorted by UTF-8 bytes, "
    "each followed by one LF"
)
ARTIFACT_POLICY = (
    "qualification artifacts contain metadata, logs, and test reports only; "
    "never DWG or DXF payloads"
)
SEMANTIC_EVIDENCE_POLICY = (
    "hosted contract self-tests validate schemas and bindings only; they never "
    "supply independent semantic support evidence"
)
EXPECTED_FOCUSED = {
    "libdxfrw_dwg_local_roundtrip": (
        "native-executable", "libdxfrw_dwg_local_roundtrip"),
    "libdxfrw_dwg_reader_matrix": (
        "native-executable", "libdxfrw_dwg_reader_matrix_tests"),
    "libdxfrw_diagnostic": (
        "native-executable", "libdxfrw_diagnostic_tests"),
    "libdxfrw_dwg_fixtures": (
        "native-executable", "libdxfrw_dwg_fixture_tests"),
    "libdxfrw_qualified_differential_v2": (
        "contract-self-test", "run_qualified_differential_v2.py"),
    "libdxfrw_qualified_semantic_fields": (
        "contract-self-test", "check_admitted_candidate_fields.py"),
    "libdxfrw_qualified_support_metadata": (
        "contract-self-test", "check_qualified_format_support.py"),
}
OUTPUT_ALLOWLIST = {
    "ctest-inventory.json",
    "ctest.log",
    "ctest.xml",
    "native-qualification-observation-v1.json",
    "native-qualification-receipt-v1.json",
    "native-qualification-receipt-v1.sha256",
    "qualification-status.json",
}
ENVIRONMENT_WHITELIST = (
    "CC", "CXX", "CPPFLAGS", "CFLAGS", "CXXFLAGS", "LDFLAGS",
    "CMAKE_GENERATOR", "CMAKE_GENERATOR_PLATFORM", "CMAKE_GENERATOR_TOOLSET",
    "CMAKE_PREFIX_PATH", "CMAKE_TOOLCHAIN_FILE", "CMAKE_BUILD_PARALLEL_LEVEL",
    "CTEST_PARALLEL_LEVEL", "MACOSX_DEPLOYMENT_TARGET", "SDKROOT",
    "DEVELOPER_DIR", "VSCMD_ARG_TGT_ARCH", "VisualStudioVersion",
    "WindowsSDKVersion", "INCLUDE", "LIB", "LIBPATH", "CL", "_CL_",
    "LINK", "_LINK_", "CPATH", "CPLUS_INCLUDE_PATH", "LIBRARY_PATH",
    "PKG_CONFIG_PATH", "CCACHE_DIR", "CCACHE_BASEDIR", "CCACHE_PREFIX",
    "PATH",
)


class QualificationError(ValueError):
    """Raised when qualification evidence is incomplete or ambiguous."""


def _reject_constant(token: str) -> Any:
    raise QualificationError("non-finite JSON token %r" % token)


def _reject_duplicate_pairs(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    result: dict[str, Any] = {}
    for key, value in pairs:
        if key in result:
            raise QualificationError("duplicate JSON key %r" % key)
        result[key] = value
    return result


def loads_json(text: str, label: str) -> Any:
    try:
        return json.loads(
            text,
            object_pairs_hook=_reject_duplicate_pairs,
            parse_constant=_reject_constant,
        )
    except json.JSONDecodeError as exc:
        raise QualificationError("invalid JSON in %s: %s" % (label, exc)) from exc


def read_json(path: Path) -> Any:
    try:
        return loads_json(path.read_text(encoding="utf-8"), str(path))
    except (OSError, UnicodeError) as exc:
        raise QualificationError("cannot read %s: %s" % (path, exc)) from exc


def canonical_bytes(value: Any) -> bytes:
    return (
        json.dumps(
            value,
            sort_keys=True,
            separators=(",", ":"),
            ensure_ascii=False,
            allow_nan=False,
        )
        + "\n"
    ).encode("utf-8")


def write_json(path: Path, value: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    payload = canonical_bytes(value)
    temporary = path.with_name(path.name + ".tmp")
    temporary.write_bytes(payload)
    temporary.replace(path)


def sha256_bytes(payload: bytes) -> str:
    return hashlib.sha256(payload).hexdigest()


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    try:
        with path.open("rb") as stream:
            for chunk in iter(lambda: stream.read(1024 * 1024), b""):
                digest.update(chunk)
    except OSError as exc:
        raise QualificationError("cannot hash %s: %s" % (path, exc)) from exc
    return digest.hexdigest()


def exact_keys(value: Any, expected: set[str], label: str) -> dict[str, Any]:
    if not isinstance(value, dict):
        raise QualificationError("%s must be an object" % label)
    actual = set(value)
    if actual != expected:
        raise QualificationError(
            "%s keys differ (missing=%s, extra=%s)" % (
                label, sorted(expected - actual), sorted(actual - expected)))
    return value


def string(value: Any, label: str) -> str:
    if not isinstance(value, str) or not value or "\x00" in value:
        raise QualificationError("%s must be a non-empty NUL-free string" % label)
    return value


def integer(value: Any, label: str) -> int:
    if not isinstance(value, int) or isinstance(value, bool) or value < 0:
        raise QualificationError("%s must be a non-negative integer" % label)
    return value


def digest(value: Any, label: str) -> str:
    text = string(value, label)
    if not SHA256_RE.fullmatch(text):
        raise QualificationError("%s must be a lowercase SHA-256" % label)
    return text


def stable_name(value: Any, label: str) -> str:
    name = string(value, label)
    if "\n" in name or "\r" in name:
        raise QualificationError("%s contains a line break" % label)
    if unicodedata.normalize("NFC", name) != name:
        raise QualificationError("%s is not NFC-normalized" % label)
    return name


def sorted_test_names(names: Iterable[str]) -> list[str]:
    checked = [stable_name(name, "test name") for name in names]
    if len(set(checked)) != len(checked):
        raise QualificationError("test names are not unique")
    return sorted(checked, key=lambda item: item.encode("utf-8"))


def test_name_digest(names: Iterable[str]) -> str:
    ordered = sorted_test_names(names)
    payload = b"".join(name.encode("utf-8") + b"\n" for name in ordered)
    return sha256_bytes(payload)


def validate_contract(document: Any, *, require_final: bool) -> dict[str, Any]:
    root = exact_keys(document, CONTRACT_KEYS, "contract")
    if root["schema"] != CONTRACT_SCHEMA or root["kind"] != CONTRACT_KIND:
        raise QualificationError("unsupported qualification test contract")
    canonicalization = exact_keys(
        root["canonicalization"], CANONICALIZATION_KEYS,
        "contract.canonicalization")
    if canonicalization != {
            "json": CANONICAL_JSON,
            "testNameDigest": CANONICAL_TEST_NAMES,
    }:
        raise QualificationError("contract canonicalization rules drifted")
    if root["artifactPolicy"] != ARTIFACT_POLICY:
        raise QualificationError("contract artifact policy drifted")
    if root["hostedProfile"] != HOSTED_PROFILE:
        raise QualificationError("contract.hostedProfile is not the fixed hosted profile")
    if root["focusedLabel"] != FOCUSED_LABEL:
        raise QualificationError("contract.focusedLabel must be %s" % FOCUSED_LABEL)
    policy = string(root["semanticEvidencePolicy"], "semanticEvidencePolicy")
    if policy != SEMANTIC_EVIDENCE_POLICY:
        raise QualificationError("semantic-evidence policy is not fail-closed")
    if not isinstance(root["focusedTests"], list):
        raise QualificationError("contract.focusedTests must be an array")
    rows: dict[str, dict[str, Any]] = {}
    for index, raw in enumerate(root["focusedTests"]):
        row = exact_keys(raw, FOCUSED_TEST_KEYS, "focusedTests[%d]" % index)
        test_id = stable_name(row["id"], "focusedTests[%d].id" % index)
        if test_id in rows:
            raise QualificationError("duplicate focused test %s" % test_id)
        mode = string(row["expectedExecutionMode"], "expectedExecutionMode")
        if mode not in MODES:
            raise QualificationError("unsupported execution mode %s" % mode)
        basename = stable_name(row["expectedCommandBasename"],
                               "expectedCommandBasename")
        if row["suppliesIndependentSemanticEvidence"] is not False:
            raise QualificationError(
                "%s must not claim independent semantic evidence" % test_id)
        string(row["reason"], "focusedTests[%d].reason" % index)
        rows[test_id] = row
        if EXPECTED_FOCUSED.get(test_id) != (mode, basename):
            raise QualificationError("unexpected focused test contract for %s" % test_id)
    if set(rows) != set(EXPECTED_FOCUSED):
        raise QualificationError(
            "focused tests differ (missing=%s, extra=%s)" % (
                sorted(set(EXPECTED_FOCUSED) - set(rows)),
                sorted(set(rows) - set(EXPECTED_FOCUSED))))
    if root["focusedCount"] != len(rows):
        raise QualificationError("focusedCount does not match focusedTests")
    if digest(root["focusedNameDigest"], "focusedNameDigest") != test_name_digest(rows):
        raise QualificationError("focusedNameDigest does not match focusedTests")

    broad = exact_keys(root["broadInventory"], BROAD_KEYS, "broadInventory")
    expected_profile_digest = sha256_bytes(
        json.dumps(HOSTED_PROFILE, sort_keys=True, separators=(",", ":")).encode())
    if digest(broad["configurationDigest"], "configurationDigest") != expected_profile_digest:
        raise QualificationError("broad inventory configuration digest is wrong")
    status = broad["status"]
    if status == "REQUIRES_FINAL_REGENERATION":
        if (broad["generatedAtCommit"] is not None or broad["count"] is not None
                or broad["nameDigest"] is not None or broad["names"] != []):
            raise QualificationError("unfinalized broad inventory must contain null/empty values")
        if require_final:
            raise QualificationError(
                "broad inventory is unfinalized; run freeze-inventory after all tests are registered")
    elif status == "FINAL":
        commit = string(broad["generatedAtCommit"], "generatedAtCommit")
        if not COMMIT_RE.fullmatch(commit):
            raise QualificationError("generatedAtCommit must be a full lowercase commit")
        if not isinstance(broad["names"], list):
            raise QualificationError("broadInventory.names must be an array")
        names = sorted_test_names(broad["names"])
        if names != broad["names"]:
            raise QualificationError("broadInventory.names are not UTF-8-byte sorted")
        if integer(broad["count"], "broadInventory.count") != len(names):
            raise QualificationError("broadInventory.count does not match names")
        if digest(broad["nameDigest"], "broadInventory.nameDigest") != test_name_digest(names):
            raise QualificationError("broadInventory.nameDigest does not match names")
        if not set(rows).issubset(names):
            raise QualificationError("broad inventory omits focused tests")
    else:
        raise QualificationError("unsupported broadInventory.status %r" % status)
    return root


def run_capture(command: list[str], *, cwd: Path | None = None) -> subprocess.CompletedProcess[str]:
    try:
        return subprocess.run(
            command,
            cwd=str(cwd) if cwd else None,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            check=False,
        )
    except OSError as exc:
        raise QualificationError("cannot run %s: %s" % (command[0], exc)) from exc


def ctest_inventory(ctest: str, build_dir: Path) -> dict[str, Any]:
    result = run_capture([
        ctest, "--test-dir", str(build_dir), "--build-config", "Release",
        "--show-only=json-v1",
    ])
    if result.returncode != 0:
        raise QualificationError("CTest inventory failed:\n%s" % result.stdout)
    value = loads_json(result.stdout, "CTest JSON inventory")
    if not isinstance(value, dict) or value.get("kind") != "ctestInfo":
        raise QualificationError("CTest JSON inventory has the wrong kind")
    tests = value.get("tests")
    if not isinstance(tests, list) or not tests:
        raise QualificationError("CTest JSON inventory contains no tests")
    seen: set[str] = set()
    for index, test in enumerate(tests):
        if not isinstance(test, dict):
            raise QualificationError("CTest test %d is not an object" % index)
        name = stable_name(test.get("name"), "CTest test name")
        if name in seen:
            raise QualificationError("duplicate CTest test %s" % name)
        seen.add(name)
        # CTest omits ``command`` for executable tests whose target has not
        # been built yet.  That is valid for the configure-only inventory
        # freeze; live qualification below requires commands for every
        # focused test after the bounded build step.
        command = test.get("command")
        if command is not None and (
                not isinstance(command, list) or not command or not all(
                    isinstance(token, str) and "\x00" not in token
                    for token in command)):
            raise QualificationError("CTest test %s has an invalid command" % name)
    return value


def property_values(test: dict[str, Any], property_name: str) -> list[str]:
    result: list[str] = []
    properties = test.get("properties", [])
    if not isinstance(properties, list):
        raise QualificationError("CTest properties are not an array")
    for prop in properties:
        if not isinstance(prop, dict) or set(prop) != {"name", "value"}:
            raise QualificationError("malformed CTest property")
        if prop["name"] != property_name:
            continue
        value = prop["value"]
        if isinstance(value, str):
            result.extend(part for part in value.split(";") if part)
        elif isinstance(value, list) and all(isinstance(item, str) for item in value):
            result.extend(value)
        else:
            raise QualificationError("CTest %s property has invalid value" % property_name)
    return result


def command_basename(command: list[str], mode: str) -> str:
    if mode == "native-executable":
        basename = Path(command[0].replace("\\", "/")).name
        return basename[:-4] if basename.lower().endswith(".exe") else basename
    candidates = [Path(token.replace("\\", "/")).name
                  for token in command if token.lower().endswith(".py")]
    if len(candidates) != 1:
        raise QualificationError("contract self-test command must name exactly one Python script")
    return candidates[0]


def observed_mode(command: list[str]) -> str:
    first = Path(command[0].replace("\\", "/")).name.lower()
    python_driver = first.startswith("python") or first in {"py", "py.exe"}
    has_self_test = "--self-test" in command
    python_scripts = [token for token in command if token.lower().endswith(".py")]
    if python_driver and has_self_test and len(python_scripts) == 1:
        return "contract-self-test"
    if not python_driver and not has_self_test and not python_scripts:
        return "native-executable"
    raise QualificationError("cannot classify CTest command execution mode")


def normalized_command(command: list[str], root: Path, build_dir: Path) -> list[str]:
    replacements = sorted(
        ((str(root.resolve()).replace("\\", "/"), "$SOURCE"),
         (str(build_dir.resolve()).replace("\\", "/"), "$BUILD")),
        key=lambda item: len(item[0]),
        reverse=True,
    )
    normalized: list[str] = []
    for raw in command:
        token = raw.replace("\\", "/")
        for prefix, replacement in replacements:
            if token == prefix or token.startswith(prefix + "/"):
                token = replacement + token[len(prefix):]
                break
        normalized.append(token)
    return normalized


def validate_inventory(
    inventory: dict[str, Any],
    contract: dict[str, Any],
    root: Path,
    build_dir: Path,
) -> tuple[list[str], list[dict[str, Any]]]:
    tests = {test["name"]: test for test in inventory["tests"]}
    names = sorted_test_names(tests)
    broad = contract["broadInventory"]
    if broad["status"] != "FINAL":
        raise QualificationError("broad inventory is not final")
    if names != broad["names"] or len(names) != broad["count"]:
        raise QualificationError("live CTest inventory differs from the frozen broad inventory")
    if test_name_digest(names) != broad["nameDigest"]:
        raise QualificationError("live CTest inventory digest differs from the frozen digest")

    focused_rows = {row["id"]: row for row in contract["focusedTests"]}
    labelled = {
        name for name, test in tests.items()
        if FOCUSED_LABEL in property_values(test, "LABELS")
    }
    if labelled != set(focused_rows):
        raise QualificationError(
            "focused label membership differs (missing=%s, extra=%s)" % (
                sorted(set(focused_rows) - labelled),
                sorted(labelled - set(focused_rows))))

    attestations: list[dict[str, Any]] = []
    for test_id in sorted_test_names(focused_rows):
        row = focused_rows[test_id]
        command = tests[test_id].get("command")
        if not isinstance(command, list) or not command:
            raise QualificationError(
                "focused CTest %s has no command; build its target before qualification" %
                test_id)
        mode = observed_mode(command)
        basename = command_basename(command, mode)
        if mode != row["expectedExecutionMode"]:
            raise QualificationError(
                "%s mode is %s, expected %s" % (
                    test_id, mode, row["expectedExecutionMode"]))
        if basename != row["expectedCommandBasename"]:
            raise QualificationError(
                "%s command basename is %s, expected %s" % (
                    test_id, basename, row["expectedCommandBasename"]))
        shape = normalized_command(command, root, build_dir)
        attestations.append({
            "testId": test_id,
            "expectedExecutionMode": row["expectedExecutionMode"],
            "observedExecutionMode": mode,
            "expectedCommandBasename": row["expectedCommandBasename"],
            "observedCommandBasename": basename,
            "commandShape": shape,
            "commandShapeDigest": sha256_bytes(
                json.dumps(shape, ensure_ascii=False, separators=(",", ":")).encode()),
            "suppliesIndependentSemanticEvidence": False,
        })
    return names, attestations


def local_tag(tag: str) -> str:
    return tag.rsplit("}", 1)[-1]


def parse_junit(path: Path, expected_names: list[str]) -> tuple[list[dict[str, Any]], dict[str, int]]:
    try:
        root = ET.parse(path).getroot()
    except (OSError, ET.ParseError) as exc:
        raise QualificationError("cannot parse CTest JUnit XML: %s" % exc) from exc
    cases = [element for element in root.iter() if local_tag(element.tag) == "testcase"]
    by_name: dict[str, list[ET.Element]] = {}
    for case in cases:
        name = case.attrib.get("name")
        if not name:
            raise QualificationError("JUnit testcase has no name")
        by_name.setdefault(name, []).append(case)
    expected = set(expected_names)
    actual = set(by_name)
    if actual != expected:
        raise QualificationError(
            "JUnit test set differs (missing=%s, extra=%s)" % (
                sorted(expected - actual), sorted(actual - expected)))
    if any(len(items) != 1 for items in by_name.values()):
        raise QualificationError("JUnit contains duplicate testcases")

    rows: list[dict[str, Any]] = []
    counts = {"total": 0, "passed": 0, "failed": 0, "skipped": 0, "notRun": 0}
    for name in sorted_test_names(expected_names):
        case = by_name[name][0]
        children = {local_tag(child.tag) for child in case}
        status_attribute = case.attrib.get("status", "").lower()
        if "failure" in children or "error" in children:
            result = "failed"
        elif "skipped" in children:
            result = "skipped"
        elif status_attribute in {"notrun", "disabled", "skipped"}:
            result = "notRun"
        elif status_attribute == "run":
            result = "passed"
        else:
            raise QualificationError(
                "JUnit testcase %s has unknown success status %r" % (
                    name, status_attribute))
        counts["total"] += 1
        counts[result] += 1
        rows.append({"testId": name, "status": result})
    return rows, counts


def git_output(root: Path, *arguments: str) -> str:
    result = run_capture(["git", *arguments], cwd=root)
    if result.returncode != 0:
        raise QualificationError("git %s failed: %s" % (" ".join(arguments), result.stdout))
    return result.stdout.strip()


def git_bytes(root: Path, *arguments: str) -> bytes:
    try:
        result = subprocess.run(
            ["git", *arguments], cwd=str(root), stdout=subprocess.PIPE,
            stderr=subprocess.PIPE, check=False)
    except OSError as exc:
        raise QualificationError("cannot run git: %s" % exc) from exc
    if result.returncode != 0:
        detail = result.stderr.decode("utf-8", errors="replace").strip()
        raise QualificationError(
            "git %s failed: %s" % (" ".join(arguments), detail))
    return result.stdout


def cache_values(build_dir: Path) -> dict[str, str]:
    path = build_dir / "CMakeCache.txt"
    try:
        lines = path.read_text(encoding="utf-8", errors="strict").splitlines()
    except (OSError, UnicodeError) as exc:
        raise QualificationError("cannot read %s: %s" % (path, exc)) from exc
    values: dict[str, str] = {}
    for line in lines:
        if not line or line.startswith(("#", "//")) or "=" not in line:
            continue
        left, value = line.split("=", 1)
        key = left.split(":", 1)[0]
        values[key] = value
    return values


def compiler_metadata(build_dir: Path) -> dict[str, str | None]:
    result = {
        "compilerPath": None,
        "compilerId": None,
        "compilerVersion": None,
        "compilerTarget": None,
        "compilerArchitectureId": None,
        "pointerSize": None,
    }
    candidates = sorted((build_dir / "CMakeFiles").glob("*/CMakeCXXCompiler.cmake"))
    if not candidates:
        return result
    text = candidates[-1].read_text(encoding="utf-8", errors="replace")
    mappings = {
        "compilerPath": "CMAKE_CXX_COMPILER",
        "compilerId": "CMAKE_CXX_COMPILER_ID",
        "compilerVersion": "CMAKE_CXX_COMPILER_VERSION",
        "compilerTarget": "CMAKE_CXX_COMPILER_TARGET",
        "compilerArchitectureId": "CMAKE_CXX_COMPILER_ARCHITECTURE_ID",
        "pointerSize": "CMAKE_CXX_SIZEOF_DATA_PTR",
    }
    for output_key, cmake_key in mappings.items():
        match = re.search(
            r"set\(" + re.escape(cmake_key) + r'\s+"?([^"\)]+)"?\)', text)
        if match:
            result[output_key] = match.group(1)
    return result


def validate_hosted_build_profile(
    cache: dict[str, str], compiler_fields: dict[str, str | None], *,
    root: Path, platform_key: str, declared_toolchain: str,
) -> None:
    """Reject a configured tree that differs from the frozen hosted profile."""
    profile = HOSTED_MATRIX.get(platform_key)
    if profile is None:
        raise QualificationError("unknown hosted platform key %s" % platform_key)
    if declared_toolchain != profile["declaredToolchain"]:
        raise QualificationError(
            "declared toolchain %s differs from %s profile" % (
                declared_toolchain, platform_key))
    for key, expected in HOSTED_PROFILE.items():
        actual = cache.get(key)
        if actual != expected:
            raise QualificationError(
                "configured %s=%r, expected %r" % (key, actual, expected))
    home = cache.get("CMAKE_HOME_DIRECTORY")
    if home is None or Path(home).resolve() != root.resolve():
        raise QualificationError("CMake source directory differs from checked-out root")
    if cache.get("CMAKE_GENERATOR") != profile["generator"]:
        raise QualificationError("configured generator differs from hosted profile")
    if cache.get("CMAKE_GENERATOR_PLATFORM", "") != profile["generatorPlatform"]:
        raise QualificationError("configured generator platform differs from hosted profile")
    if compiler_fields.get("compilerId") != profile["compilerId"]:
        raise QualificationError("configured compiler ID differs from hosted profile")
    compiler_path = compiler_fields.get("compilerPath") or cache.get("CMAKE_CXX_COMPILER")
    if not compiler_path or Path(compiler_path).name.lower() not in {
            value.lower() for value in profile["compilerBasenames"]}:
        raise QualificationError("configured C++ compiler differs from hosted profile")


def tool_record(executable: str, version_arguments: list[str]) -> dict[str, Any]:
    resolved = shutil.which(executable) or executable
    path = Path(resolved)
    if not path.exists():
        raise QualificationError("tool does not exist: %s" % executable)
    version = run_capture([str(path), *version_arguments])
    output = version.stdout.strip()
    if not output:
        raise QualificationError("tool produced no version output: %s" % executable)
    if version.returncode != 0:
        raise QualificationError(
            "tool version probe failed for %s (exit %d)" % (
                executable, version.returncode))
    return {
        "path": str(path.resolve()),
        "sha256": sha256_file(path),
        "versionOutput": output,
        "versionExitCode": version.returncode,
    }


def collect_context(
    root: Path,
    build_dir: Path,
    workflow_path: Path,
    platform_key: str,
    declared_runner_label: str,
    declared_runner_os: str,
    declared_runner_arch: str,
    declared_toolchain: str,
    require_github_runner: bool,
) -> dict[str, Any]:
    for value, label in (
        (platform_key, "platform key"),
        (declared_runner_label, "runner label"),
        (declared_runner_os, "runner OS"),
        (declared_runner_arch, "runner architecture"),
        (declared_toolchain, "toolchain"),
    ):
        if not SAFE_ID_RE.fullmatch(value):
            raise QualificationError("invalid declared %s %r" % (label, value))
    observed_os = os.environ.get("RUNNER_OS")
    observed_arch = os.environ.get("RUNNER_ARCH")
    image_os = os.environ.get("ImageOS")
    image_version = os.environ.get("ImageVersion")
    if require_github_runner:
        required_environment = (
            ("RUNNER_OS", observed_os), ("RUNNER_ARCH", observed_arch),
            ("ImageOS", image_os), ("ImageVersion", image_version),
            ("GITHUB_REPOSITORY", os.environ.get("GITHUB_REPOSITORY")),
            ("GITHUB_RUN_ID", os.environ.get("GITHUB_RUN_ID")),
            ("GITHUB_RUN_ATTEMPT", os.environ.get("GITHUB_RUN_ATTEMPT")),
            ("GITHUB_RUN_NUMBER", os.environ.get("GITHUB_RUN_NUMBER")),
            ("GITHUB_EVENT_NAME", os.environ.get("GITHUB_EVENT_NAME")),
            ("GITHUB_JOB", os.environ.get("GITHUB_JOB")),
            ("GITHUB_REF", os.environ.get("GITHUB_REF")),
            ("GITHUB_SHA", os.environ.get("GITHUB_SHA")),
        )
        missing = [name for name, value in required_environment if not value]
        if missing:
            raise QualificationError("missing GitHub runner fields: %s" % ", ".join(missing))
        if observed_os != declared_runner_os or observed_arch != declared_runner_arch:
            raise QualificationError(
                "observed runner %s/%s differs from declared %s/%s" % (
                    observed_os, observed_arch, declared_runner_os, declared_runner_arch))

    head = git_output(root, "rev-parse", "HEAD")
    if not COMMIT_RE.fullmatch(head):
        raise QualificationError("HEAD is not a full commit")
    if require_github_runner:
        for name in ("GITHUB_RUN_ID", "GITHUB_RUN_ATTEMPT", "GITHUB_RUN_NUMBER"):
            if not re.fullmatch(r"[1-9][0-9]*", os.environ[name]):
                raise QualificationError("%s is not a positive decimal integer" % name)
        if os.environ["GITHUB_SHA"] != head:
            raise QualificationError("checked-out HEAD differs from GITHUB_SHA")
        if not re.fullmatch(
                r"[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+",
                os.environ["GITHUB_REPOSITORY"]):
            raise QualificationError("GITHUB_REPOSITORY is not owner/name")
    workflow_relative = workflow_path.resolve().relative_to(root.resolve()).as_posix()
    working_blob = git_output(root, "hash-object", "--", workflow_relative)
    head_blob = git_output(root, "rev-parse", "HEAD:" + workflow_relative)
    if working_blob != head_blob:
        raise QualificationError("workflow working tree bytes differ from checked-out commit")
    # Hash the committed blob bytes, not checkout bytes.  On Windows the
    # checkout may contain CRLF while Git and the GitHub contents API expose
    # the canonical LF blob; the receipt must bind the latter byte-for-byte.
    workflow_bytes = git_bytes(root, "cat-file", "blob", head_blob)

    cache = cache_values(build_dir)
    compiler_fields = compiler_metadata(build_dir)
    validate_hosted_build_profile(
        cache, compiler_fields, root=root, platform_key=platform_key,
        declared_toolchain=declared_toolchain)
    compiler = cache.get("CMAKE_CXX_COMPILER") or compiler_fields["compilerPath"] or "cl"
    # `cl /Bv` emits D8003 and exits nonzero when no source is supplied.  The
    # help probe is side-effect-free, exits successfully, and still includes
    # the compiler banner; the exact compiler version also comes from CMake.
    compiler_args = ["/?"] if Path(compiler).name.lower() in {"cl", "cl.exe"} else ["--version"]
    tools = {
        "cmake": tool_record(shutil.which("cmake") or "cmake", ["--version"]),
        "ctest": tool_record(shutil.which("ctest") or "ctest", ["--version"]),
        "python": tool_record(sys.executable, ["--version"]),
        "git": tool_record(shutil.which("git") or "git", ["--version"]),
        "cxxCompiler": tool_record(compiler, compiler_args),
    }
    environment = [
        {"name": name, "present": name in os.environ,
         "value": os.environ.get(name) if name in os.environ else None}
        for name in ENVIRONMENT_WHITELIST
    ]
    return {
        "repository": {
            "githubRepository": os.environ.get("GITHUB_REPOSITORY"),
            "githubRepositoryId": os.environ.get("GITHUB_REPOSITORY_ID"),
            "checkedOutCommit": head,
        },
        "workflow": {
            "path": workflow_relative,
            "gitBlobAtHead": head_blob,
            "workingTreeGitBlob": working_blob,
            "sha256": sha256_bytes(workflow_bytes),
        },
        "run": {
            "id": os.environ.get("GITHUB_RUN_ID"),
            "attempt": os.environ.get("GITHUB_RUN_ATTEMPT"),
            "number": os.environ.get("GITHUB_RUN_NUMBER"),
            "eventName": os.environ.get("GITHUB_EVENT_NAME"),
            "job": os.environ.get("GITHUB_JOB"),
            "ref": os.environ.get("GITHUB_REF"),
            "githubSha": os.environ.get("GITHUB_SHA"),
            "workflowRef": os.environ.get("GITHUB_WORKFLOW_REF"),
        },
        "platform": {
            "key": platform_key,
            "declaredRunnerLabel": declared_runner_label,
            "declaredRunnerOS": declared_runner_os,
            "declaredRunnerArch": declared_runner_arch,
            "observedRunnerOS": observed_os,
            "observedRunnerArch": observed_arch,
            "imageOS": image_os,
            "imageVersion": image_version,
            "system": platform.system(),
            "release": platform.release(),
            "version": platform.version(),
            "machine": platform.machine(),
        },
        "toolchain": {
            "declared": declared_toolchain,
            "generator": cache.get("CMAKE_GENERATOR"),
            "generatorPlatform": cache.get("CMAKE_GENERATOR_PLATFORM"),
            "generatorToolset": cache.get("CMAKE_GENERATOR_TOOLSET"),
            "buildType": cache.get("CMAKE_BUILD_TYPE"),
            "cxxStandard": cache.get("CMAKE_CXX_STANDARD", "17"),
            "cxxFlags": cache.get("CMAKE_CXX_FLAGS"),
            "cxxFlagsRelease": cache.get("CMAKE_CXX_FLAGS_RELEASE"),
            **compiler_fields,
            "tools": tools,
        },
        "environment": environment,
    }


def merge_test_rows(
    selected: list[str],
    focused_attestations: list[dict[str, Any]],
    junit_rows: list[dict[str, Any]],
) -> list[dict[str, Any]]:
    attestations = {row["testId"]: row for row in focused_attestations}
    statuses = {row["testId"]: row["status"] for row in junit_rows}
    rows: list[dict[str, Any]] = []
    for test_id in sorted_test_names(selected):
        if test_id in attestations:
            row = dict(attestations[test_id])
        else:
            row = {
                "testId": test_id,
                "expectedExecutionMode": "broad-inventory-test",
                "observedExecutionMode": "broad-inventory-test",
                "expectedCommandBasename": None,
                "observedCommandBasename": None,
                "commandShape": None,
                "commandShapeDigest": None,
                "suppliesIndependentSemanticEvidence": False,
            }
        row["status"] = statuses[test_id]
        rows.append(row)
    return rows


def run_qualification(args: argparse.Namespace) -> int:
    root = args.root.resolve()
    build_dir = args.build_dir.resolve()
    output_dir = args.output_dir.resolve()
    output_dir.mkdir(parents=True, exist_ok=True)
    status_path = output_dir / "qualification-status.json"
    status: dict[str, Any] = {
        "schema": 1,
        "kind": STATUS_KIND,
        "suite": args.suite,
        "platformKey": args.platform_key,
        "conclusion": "failure",
        "error": None,
    }
    try:
        contract = validate_contract(read_json(args.manifest), require_final=True)
        inventory = ctest_inventory(args.ctest, build_dir)
        write_json(output_dir / "ctest-inventory.json", inventory)
        all_names, attestations = validate_inventory(
            inventory, contract, root, build_dir)
        selected = (
            sorted_test_names(row["id"] for row in contract["focusedTests"])
            if args.suite == "focused" else all_names
        )
        junit_path = output_dir / "ctest.xml"
        command = [
            args.ctest, "--test-dir", str(build_dir), "--build-config", "Release",
            "--parallel", "2", "--no-tests=error", "--output-on-failure",
            "--output-junit", str(junit_path),
        ]
        if args.suite == "focused":
            command.extend(["-L", "^%s$" % FOCUSED_LABEL])
        result = run_capture(command)
        (output_dir / "ctest.log").write_text(result.stdout, encoding="utf-8")
        junit_rows, summary = parse_junit(junit_path, selected)
        context = collect_context(
            root, build_dir, args.workflow,
            args.platform_key, args.declared_runner_label,
            args.declared_runner_os, args.declared_runner_arch,
            args.declared_toolchain, args.require_github_runner,
        )
        contract_bytes = args.manifest.read_bytes()
        rows = merge_test_rows(selected, attestations, junit_rows)
        success = (
            result.returncode == 0
            and summary["failed"] == 0
            and summary["skipped"] == 0
            and summary["notRun"] == 0
            and summary["passed"] == len(selected)
        )
        observation = {
            "schema": OBSERVATION_SCHEMA,
            "kind": OBSERVATION_KIND,
            "suite": args.suite,
            "generatedAtUtc": dt.datetime.now(dt.timezone.utc).isoformat().replace("+00:00", "Z"),
            "artifactPolicy": contract["artifactPolicy"],
            **context,
            "contracts": {
                "requiredTestsPath": args.manifest.resolve().relative_to(root).as_posix(),
                "requiredTestsSha256": sha256_bytes(contract_bytes),
                "focusedNameDigest": contract["focusedNameDigest"],
                "broadNameDigest": contract["broadInventory"]["nameDigest"],
                "broadGeneratedAtCommit": contract["broadInventory"]["generatedAtCommit"],
            },
            "inventory": {
                "count": len(all_names),
                "nameDigest": test_name_digest(all_names),
                "names": all_names,
                "selectedCount": len(selected),
                "selectedNameDigest": test_name_digest(selected),
                "focusedLabel": FOCUSED_LABEL,
            },
            "tests": rows,
            "summary": summary,
            "ctestExitCode": result.returncode,
            "qualificationConclusion": "success" if success else "failure",
        }
        write_json(output_dir / "native-qualification-observation-v1.json", observation)
        if not success:
            raise QualificationError(
                "CTest qualification did not pass without skips (exit=%d, summary=%s)" % (
                    result.returncode, summary))
        status["conclusion"] = "success"
        write_json(status_path, status)
        print("native qualification: PASS (%s; %s; %d tests)" % (
            args.suite, args.platform_key, len(selected)))
        return 0
    except (OSError, UnicodeError, QualificationError) as exc:
        status["error"] = str(exc)
        try:
            write_json(status_path, status)
        except OSError:
            pass
        print("native qualification: FAIL: %s" % exc, file=sys.stderr)
        return 2


def validate_output_allowlist(output_dir: Path, *, require_sealed: bool) -> set[str]:
    if not output_dir.is_dir():
        raise QualificationError("qualification output directory does not exist")
    actual: set[str] = set()
    for path in output_dir.iterdir():
        if path.is_symlink() or not path.is_file():
            raise QualificationError("qualification output contains a non-regular file: %s" % path)
        actual.add(path.name)
        if path.suffix.lower() in {".dwg", ".dxf"}:
            raise QualificationError("drawing payload in qualification output: %s" % path)
    expected = (OUTPUT_ALLOWLIST if require_sealed else
                OUTPUT_ALLOWLIST - {"native-qualification-receipt-v1.sha256"})
    if actual != expected:
        raise QualificationError(
            "qualification output file set differs (expected %s %s, actual=%s)" % (
                "sealed" if require_sealed else "preseal",
                sorted(expected), sorted(actual)))
    return actual


def enforce_output(args: argparse.Namespace) -> int:
    errors: list[str] = []
    output_dir = args.output_dir.resolve()
    observation: dict[str, Any] = {}
    observation_bytes = b""
    contract: dict[str, Any] | None = None
    receipt_tool: Any = None
    try:
        output_names = validate_output_allowlist(
            output_dir, require_sealed=args.phase == "post-upload-verify")
    except QualificationError as exc:
        errors.append(str(exc))
        output_names = set()
    status_path = output_dir / "qualification-status.json"
    observation_path = output_dir / "native-qualification-observation-v1.json"
    receipt_path = output_dir / "native-qualification-receipt-v1.json"
    try:
        status = read_json(status_path)
        expected_status_keys = {
            "schema", "kind", "suite", "platformKey", "conclusion", "error",
        }
        if set(status) != expected_status_keys or status.get("schema") != 1:
            errors.append("qualification status has the wrong schema")
        if status.get("kind") != STATUS_KIND or status.get("conclusion") != "success":
            errors.append("qualification status is not successful")
        if status.get("error") is not None:
            errors.append("successful qualification status carries an error")
        if status.get("suite") != args.suite or status.get("platformKey") != args.platform_key:
            errors.append("qualification status identity differs")
    except QualificationError as exc:
        errors.append(str(exc))
    try:
        observation_bytes = observation_path.read_bytes()
        observation = loads_json(
            observation_bytes.decode("utf-8"), str(observation_path))
        if canonical_bytes(observation) != observation_bytes:
            errors.append("native observation is not canonical JSON with one trailing LF")
        contract_path = ROOT / "metadata/qualification-required-tests-v1.json"
        contract = validate_contract(read_json(contract_path), require_final=True)
        import emit_native_qualification_receipt as receipt_tool
        receipt_tool.validate_observation(observation, contract)
        if observation.get("suite") != args.suite:
            errors.append("qualification observation suite differs")
        if observation.get("platform", {}).get("key") != args.platform_key:
            errors.append("qualification observation platform differs")
        if observation.get("qualificationConclusion") != "success":
            errors.append("qualification observation conclusion is not success")
    except (OSError, UnicodeError, QualificationError, ValueError) as exc:
        errors.append(str(exc))

    if args.qualification_step_outcome != "success":
        errors.append("qualification workflow step outcome is not success")
    if args.receipt_step_outcome != "success":
        errors.append("receipt-emitter workflow step outcome is not success")
    if (args.phase == "post-upload-verify" and
            args.seal_step_outcome != "success"):
        errors.append("pre-upload seal workflow step outcome is not success")
    try:
        if contract is None or receipt_tool is None or not observation_bytes:
            raise QualificationError("validated observation/contract is unavailable")
        receipt_bytes = receipt_path.read_bytes()
        receipt = loads_json(receipt_bytes.decode("utf-8"), str(receipt_path))
        if canonical_bytes(receipt) != receipt_bytes:
            errors.append("native receipt is not canonical JSON with one trailing LF")
        receipt_tool.validate_receipt(receipt, contract)
        observation_digest = sha256_bytes(observation_bytes)
        if receipt.get("observationSha256") != observation_digest:
            errors.append("native receipt does not bind the exact observation bytes")
        if receipt.get("suite") != args.suite:
            errors.append("native receipt suite differs")
        if receipt.get("platform", {}).get("key") != args.platform_key:
            errors.append("native receipt platform differs")
        qualification_digests = receipt.get("qualificationDigests", {})
        claims_path = ROOT / "metadata/qualified-format-claims-v1.json"
        status_overlay_path = ROOT / "metadata/qualified-format-status-v1.json"
        inputs_path = ROOT / "metadata/qualification-implementation-inputs-v1.json"
        schema_path = ROOT / "metadata/native-qualification-receipt-schema-v1.json"
        status_overlay = read_json(status_overlay_path)
        if qualification_digests.get("immutableClaims", {}).get("sha256") != sha256_file(claims_path):
            errors.append("native receipt claims digest differs from repository bytes")
        if qualification_digests.get("receiptSchema", {}).get("sha256") != sha256_file(schema_path):
            errors.append("native receipt schema digest differs from repository bytes")
        implementation = qualification_digests.get("implementation", {})
        if implementation.get("manifestSha256") != sha256_file(inputs_path):
            errors.append("native receipt implementation-manifest digest differs")
        if implementation.get("value") != status_overlay.get("implementationDigestSha256"):
            errors.append("native receipt implementation digest differs from frozen status")
        if qualification_digests.get("immutableClaims", {}).get("sha256") != status_overlay.get("claimsDigestSha256"):
            errors.append("native receipt claims digest differs from frozen status")
        if observation.get("contracts", {}).get("requiredTestsSha256") != sha256_file(contract_path):
            errors.append("native observation required-test digest differs from repository bytes")
        receipt_digest = sha256_bytes(receipt_bytes)
        digest_line = receipt_digest + "  native-qualification-receipt-v1.json\n"
        digest_bytes = digest_line.encode("ascii")
        digest_path = output_dir / "native-qualification-receipt-v1.sha256"
        if args.phase == "post-upload-verify":
            if digest_path.read_bytes() != digest_bytes:
                errors.append("detached native-receipt digest is stale")
        if errors:
            raise QualificationError("receipt/output validation failed")
        if args.phase == "pre-upload-seal":
            digest_path.write_bytes(digest_bytes)
    except (OSError, UnicodeError, QualificationError, ValueError) as exc:
        errors.append("cannot seal native receipt: %s" % exc)

    if errors:
        print("native qualification enforcement: FAIL: %s" % "; ".join(errors),
              file=sys.stderr)
        return 2
    print("native qualification enforcement: PASS (%s; %s)" % (
        args.suite, args.platform_key))
    return 0


def freeze_inventory(args: argparse.Namespace) -> int:
    contract = validate_contract(read_json(args.manifest), require_final=False)
    inventory = ctest_inventory(args.ctest, args.build_dir.resolve())
    names = sorted_test_names(test["name"] for test in inventory["tests"])
    focused = {row["id"] for row in contract["focusedTests"]}
    labelled = {
        test["name"] for test in inventory["tests"]
        if FOCUSED_LABEL in property_values(test, "LABELS")
    }
    if labelled != focused:
        raise QualificationError(
            "cannot freeze: focused label membership differs (missing=%s, extra=%s)" % (
                sorted(focused - labelled), sorted(labelled - focused)))
    head = git_output(args.root.resolve(), "rev-parse", "HEAD")
    if not COMMIT_RE.fullmatch(head):
        raise QualificationError("cannot freeze against a non-commit HEAD")
    contract["broadInventory"] = {
        "status": "FINAL",
        "generatedAtCommit": head,
        "configurationDigest": contract["broadInventory"]["configurationDigest"],
        "count": len(names),
        "nameDigest": test_name_digest(names),
        "names": names,
    }
    validate_contract(contract, require_final=True)
    write_json(args.output, contract)
    print("frozen CTest inventory: %d tests; %s" % (
        len(names), contract["broadInventory"]["nameDigest"]))
    return 0


def fixture_contract() -> dict[str, Any]:
    rows = []
    for test_id, (mode, basename) in EXPECTED_FOCUSED.items():
        rows.append({
            "id": test_id,
            "expectedExecutionMode": mode,
            "expectedCommandBasename": basename,
            "suppliesIndependentSemanticEvidence": False,
            "reason": "self-test fixture; never independent evidence",
        })
    names = sorted_test_names(row["id"] for row in rows)
    return {
        "schema": CONTRACT_SCHEMA,
        "kind": CONTRACT_KIND,
        "canonicalization": {
            "json": CANONICAL_JSON,
            "testNameDigest": CANONICAL_TEST_NAMES,
        },
        "artifactPolicy": ARTIFACT_POLICY,
        "hostedProfile": dict(HOSTED_PROFILE),
        "focusedLabel": FOCUSED_LABEL,
        "semanticEvidencePolicy": SEMANTIC_EVIDENCE_POLICY,
        "focusedTests": rows,
        "focusedCount": len(rows),
        "focusedNameDigest": test_name_digest(names),
        "broadInventory": {
            "status": "FINAL",
            "generatedAtCommit": "1" * 40,
            "configurationDigest": sha256_bytes(json.dumps(
                HOSTED_PROFILE, sort_keys=True, separators=(",", ":")).encode()),
            "count": len(names),
            "nameDigest": test_name_digest(names),
            "names": names,
        },
    }


def fixture_inventory(contract: dict[str, Any]) -> dict[str, Any]:
    tests = []
    for row in contract["focusedTests"]:
        basename = row["expectedCommandBasename"]
        if row["expectedExecutionMode"] == "native-executable":
            command = ["/tmp/build/" + basename]
        else:
            command = [
                "python3", "-B", "/tmp/source/tools/" + basename, "--self-test",
            ]
        tests.append({
            "name": row["id"],
            "command": command,
            "properties": [{"name": "LABELS", "value": [FOCUSED_LABEL]}],
        })
    return {
        "kind": "ctestInfo",
        "version": {"major": 1, "minor": 0},
        "tests": tests,
    }


def self_test() -> None:
    names = ["z", "a", "é"]
    expected = hashlib.sha256("a\nz\né\n".encode("utf-8")).hexdigest()
    assert test_name_digest(names) == expected
    try:
        test_name_digest(["a", "a"])
        raise AssertionError("duplicate test name accepted")
    except QualificationError:
        pass
    try:
        loads_json('{"a":1,"a":2}', "duplicate")
        raise AssertionError("duplicate JSON key accepted")
    except QualificationError:
        pass
    try:
        loads_json('{"a":NaN}', "nonfinite")
        raise AssertionError("non-finite JSON accepted")
    except QualificationError:
        pass
    assert observed_mode(["python3", "-B", "check.py", "--self-test"]) == "contract-self-test"
    assert observed_mode(["/tmp/native-test"]) == "native-executable"
    try:
        observed_mode(["python3", "check.py"])
        raise AssertionError("ambiguous Python command accepted")
    except QualificationError:
        pass
    contract = fixture_contract()
    validate_contract(contract, require_final=True)
    inventory = fixture_inventory(contract)
    names, attestations = validate_inventory(
        inventory, contract, Path("/tmp/source"), Path("/tmp/build"))
    assert len(names) == len(EXPECTED_FOCUSED)
    assert len(attestations) == len(EXPECTED_FOCUSED)
    assert all(row["suppliesIndependentSemanticEvidence"] is False
               for row in attestations)
    broken_inventory = json.loads(json.dumps(inventory))
    for test in broken_inventory["tests"]:
        if test["name"] == "libdxfrw_qualified_semantic_fields":
            test["command"].remove("--self-test")
    try:
        validate_inventory(
            broken_inventory, contract, Path("/tmp/source"), Path("/tmp/build"))
        raise AssertionError("non-self-test semantic command accepted")
    except QualificationError:
        pass
    promoted = json.loads(json.dumps(contract))
    promoted["focusedTests"][-1]["suppliesIndependentSemanticEvidence"] = True
    try:
        validate_contract(promoted, require_final=True)
        raise AssertionError("hosted self-test semantic promotion accepted")
    except QualificationError:
        pass
    with tempfile.TemporaryDirectory(prefix="libdxfrw-native-qualification-") as temporary:
        xml = Path(temporary) / "ctest.xml"
        xml.write_text(
            '<?xml version="1.0"?><testsuite tests="2">'
            '<testcase name="b" status="run"/><testcase name="a" status="run"/>'
            '</testsuite>', encoding="utf-8")
        rows, summary = parse_junit(xml, ["a", "b"])
        assert [row["testId"] for row in rows] == ["a", "b"]
        assert summary == {"total": 2, "passed": 2, "failed": 0,
                           "skipped": 0, "notRun": 0}
        xml.write_text(
            '<testsuite><testcase name="a"><skipped/></testcase></testsuite>',
            encoding="utf-8")
        _, summary = parse_junit(xml, ["a"])
        assert summary["skipped"] == 1
        xml.write_text(
            '<testsuite><testcase name="a" status="failed"/></testsuite>',
            encoding="utf-8")
        try:
            parse_junit(xml, ["a"])
            raise AssertionError("unknown/failed JUnit status was accepted")
        except QualificationError:
            pass
        preseal = OUTPUT_ALLOWLIST - {"native-qualification-receipt-v1.sha256"}
        output = Path(temporary) / "output"
        output.mkdir()
        for name in preseal:
            (output / name).write_text("x", encoding="utf-8")
        assert validate_output_allowlist(output, require_sealed=False) == preseal
        try:
            validate_output_allowlist(output, require_sealed=True)
            raise AssertionError("preseal output accepted as sealed")
        except QualificationError:
            pass
        (output / "native-qualification-receipt-v1.sha256").write_text(
            "x", encoding="utf-8")
        assert validate_output_allowlist(
            output, require_sealed=True) == OUTPUT_ALLOWLIST
        try:
            validate_output_allowlist(output, require_sealed=False)
            raise AssertionError("sealed output accepted as preseal")
        except QualificationError:
            pass
        canonical_digest = ("0" * 64 +
                            "  native-qualification-receipt-v1.json\n").encode("ascii")
        digest_path = output / "native-qualification-receipt-v1.sha256"
        digest_path.write_bytes(canonical_digest)
        assert digest_path.read_bytes() == canonical_digest
        assert b"\r\n" not in digest_path.read_bytes()
        digest_path.unlink()
        (output / "ctest.log").unlink()
        try:
            validate_output_allowlist(output, require_sealed=False)
            raise AssertionError("missing qualification artifact was accepted")
        except QualificationError:
            pass
    print("run_native_qualification self-test: PASS")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test", action="store_true")
    subparsers = parser.add_subparsers(dest="command")

    check = subparsers.add_parser("check-manifest")
    check.add_argument("--manifest", type=Path,
                       default=Path("metadata/qualification-required-tests-v1.json"))
    check.add_argument("--allow-unfinalized", action="store_true")

    freeze = subparsers.add_parser("freeze-inventory")
    freeze.add_argument("--root", type=Path, default=Path("."))
    freeze.add_argument("--build-dir", type=Path, required=True)
    freeze.add_argument("--manifest", type=Path,
                        default=Path("metadata/qualification-required-tests-v1.json"))
    freeze.add_argument("--output", type=Path, required=True)
    freeze.add_argument("--ctest", default="ctest")

    run = subparsers.add_parser("run")
    run.add_argument("--root", type=Path, default=Path("."))
    run.add_argument("--build-dir", type=Path, required=True)
    run.add_argument("--manifest", type=Path,
                     default=Path("metadata/qualification-required-tests-v1.json"))
    run.add_argument("--workflow", type=Path,
                     default=Path(".github/workflows/build.yml"))
    run.add_argument("--output-dir", type=Path, required=True)
    run.add_argument("--suite", choices=("focused", "final-broad"), required=True)
    run.add_argument("--platform-key", required=True)
    run.add_argument("--declared-runner-label", required=True)
    run.add_argument("--declared-runner-os", required=True)
    run.add_argument("--declared-runner-arch", required=True)
    run.add_argument("--declared-toolchain", required=True)
    run.add_argument("--ctest", default="ctest")
    run.add_argument("--require-github-runner", action="store_true")

    enforce = subparsers.add_parser("enforce")
    enforce.add_argument("--output-dir", type=Path, required=True)
    enforce.add_argument("--suite", choices=("focused", "final-broad"), required=True)
    enforce.add_argument("--platform-key", required=True)
    enforce.add_argument("--qualification-step-outcome", required=True)
    enforce.add_argument("--receipt-step-outcome", required=True)
    enforce.add_argument("--phase", choices=("pre-upload-seal", "post-upload-verify"),
                         required=True)
    enforce.add_argument("--seal-step-outcome")
    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    try:
        if args.self_test:
            self_test()
            return 0
        if args.command == "check-manifest":
            validate_contract(read_json(args.manifest),
                              require_final=not args.allow_unfinalized)
            print("qualification required-test contract: PASS")
            return 0
        if args.command == "freeze-inventory":
            return freeze_inventory(args)
        if args.command == "run":
            return run_qualification(args)
        if args.command == "enforce":
            return enforce_output(args)
        parser.error("a command or --self-test is required")
        return 2
    except (OSError, UnicodeError, QualificationError, AssertionError) as exc:
        print("native qualification: FAIL: %s" % exc, file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
