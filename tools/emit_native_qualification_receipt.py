#!/usr/bin/env python3
"""Turn one exact native-run observation into a canonical immutable receipt.

The pre-upload receipt intentionally has no GitHub numeric job ID, artifact ID,
API digest, or archive digest.  J364 learns those values only after upload and
binds them in the digest-excluded status overlay.
"""

from __future__ import annotations

import argparse
import copy
import hashlib
import json
import re
import sys
import tempfile
from datetime import datetime
from pathlib import Path
from typing import Any

import check_qualification_digest as digest_tool
import run_native_qualification as native_runner


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_CLAIMS = ROOT / "metadata/qualified-format-claims-v1.json"
DEFAULT_STATUS = ROOT / "metadata/qualified-format-status-v1.json"
DEFAULT_INPUTS = ROOT / "metadata/qualification-implementation-inputs-v1.json"
DEFAULT_TESTS = ROOT / "metadata/qualification-required-tests-v1.json"
DEFAULT_SCHEMA = ROOT / "metadata/native-qualification-receipt-schema-v1.json"

HEX = set("0123456789abcdef")
OBSERVATION_KEYS = {
    "schema", "kind", "suite", "generatedAtUtc", "artifactPolicy",
    "repository", "workflow", "run", "platform", "toolchain",
    "environment", "contracts", "inventory", "tests", "summary",
    "ctestExitCode", "qualificationConclusion",
}
RECEIPT_KEYS = OBSERVATION_KEYS | {
    "observationSha256", "qualificationDigests",
    "suppliesIndependentSemanticEvidence",
}
REPOSITORY_KEYS = {"githubRepository", "githubRepositoryId", "checkedOutCommit"}
WORKFLOW_KEYS = {"path", "gitBlobAtHead", "workingTreeGitBlob", "sha256"}
RUN_KEYS = {"id", "attempt", "number", "eventName", "job", "ref", "githubSha", "workflowRef"}
PLATFORM_KEYS = {
    "key", "declaredRunnerLabel", "declaredRunnerOS", "declaredRunnerArch",
    "observedRunnerOS", "observedRunnerArch", "imageOS", "imageVersion",
    "system", "release", "version", "machine",
}
TOOLCHAIN_KEYS = {
    "declared", "generator", "generatorPlatform", "generatorToolset",
    "buildType", "cxxStandard", "cxxFlags", "cxxFlagsRelease",
    "compilerPath", "compilerId", "compilerVersion", "compilerTarget",
    "compilerArchitectureId", "pointerSize", "tools",
}
TOOL_KEYS = {"path", "sha256", "versionOutput", "versionExitCode"}
CONTRACT_KEYS = {
    "requiredTestsPath", "requiredTestsSha256", "focusedNameDigest",
    "broadNameDigest", "broadGeneratedAtCommit",
}
INVENTORY_KEYS = {
    "count", "nameDigest", "names", "selectedCount", "selectedNameDigest",
    "focusedLabel",
}
TEST_KEYS = {
    "testId", "expectedExecutionMode", "observedExecutionMode",
    "expectedCommandBasename", "observedCommandBasename", "commandShape",
    "commandShapeDigest", "suppliesIndependentSemanticEvidence", "status",
}
SUMMARY_KEYS = {"total", "passed", "failed", "skipped", "notRun"}
QUALIFICATION_DIGEST_KEYS = {"implementation", "immutableClaims", "receiptSchema"}

MATRIX = {
    "linux-gcc": ("ubuntu-24.04", "Linux", "X64", "gcc-13", "Ninja", "GNU"),
    "macos-clang": ("macos-15", "macOS", "ARM64", "apple-clang", "Ninja", "AppleClang"),
    "windows-msvc": ("windows-2022", "Windows", "X64", "msvc-vs2022-x64", "Visual Studio 17 2022", "MSVC"),
}


class ReceiptError(ValueError):
    pass


def read_json(path: Path) -> dict[str, Any]:
    try:
        value = native_runner.read_json(path)
    except native_runner.QualificationError as exc:
        raise ReceiptError(str(exc)) from exc
    if not isinstance(value, dict):
        raise ReceiptError("%s must contain one object" % path)
    return value


def exact_keys(value: object, expected: set[str], label: str) -> dict[str, Any]:
    if not isinstance(value, dict):
        raise ReceiptError("%s must be an object" % label)
    actual = set(value)
    if actual != expected:
        raise ReceiptError(
            "%s keys differ (missing=%s unexpected=%s)"
            % (label, sorted(expected - actual), sorted(actual - expected))
        )
    return value


def hex_digest(value: object, lengths: set[int], label: str) -> str:
    if not isinstance(value, str) or len(value) not in lengths or any(char not in HEX for char in value):
        raise ReceiptError("%s is not a lowercase hexadecimal digest" % label)
    return value


def nonempty(value: object, label: str) -> str:
    if not isinstance(value, str) or not value or "\x00" in value:
        raise ReceiptError("%s must be a non-empty NUL-free string" % label)
    return value


def nullable_string(value: object, label: str) -> str | None:
    if value is not None and (not isinstance(value, str) or "\x00" in value):
        raise ReceiptError("%s must be a string or null" % label)
    return value


def nonnegative(value: object, label: str) -> int:
    if not isinstance(value, int) or isinstance(value, bool) or value < 0:
        raise ReceiptError("%s must be a nonnegative integer" % label)
    return value


def canonical_bytes(value: Any) -> bytes:
    return native_runner.canonical_bytes(value)


def sha256_file(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def _timestamp(value: object) -> None:
    text = nonempty(value, "generatedAtUtc")
    try:
        parsed = datetime.fromisoformat(text.replace("Z", "+00:00"))
    except ValueError as exc:
        raise ReceiptError("generatedAtUtc is not RFC3339-compatible") from exc
    if parsed.tzinfo is None:
        raise ReceiptError("generatedAtUtc has no timezone")


def _validate_tool(record: object, label: str) -> None:
    row = exact_keys(record, TOOL_KEYS, label)
    nonempty(row.get("path"), label + ".path")
    hex_digest(row.get("sha256"), {64}, label + ".sha256")
    nonempty(row.get("versionOutput"), label + ".versionOutput")
    if row.get("versionExitCode") != 0:
        raise ReceiptError("%s version probe did not succeed" % label)


def validate_observation(
    document: dict[str, Any], contract: dict[str, Any], *,
    expected_observation_sha256: str | None = None,
) -> None:
    exact_keys(document, OBSERVATION_KEYS, "native observation")
    if document.get("schema") != 1 or document.get("kind") != native_runner.OBSERVATION_KIND:
        raise ReceiptError("unexpected native observation schema")
    suite = document.get("suite")
    if suite not in {"focused", "final-broad"}:
        raise ReceiptError("native observation has an invalid suite")
    _timestamp(document.get("generatedAtUtc"))
    if document.get("artifactPolicy") != contract["artifactPolicy"]:
        raise ReceiptError("artifact policy differs from the frozen test contract")

    repository = exact_keys(document.get("repository"), REPOSITORY_KEYS, "repository")
    repo_name = nonempty(repository.get("githubRepository"), "repository.githubRepository")
    if re.fullmatch(r"[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+", repo_name) is None:
        raise ReceiptError("repository.githubRepository is not owner/name")
    nonempty(repository.get("githubRepositoryId"), "repository.githubRepositoryId")
    commit = hex_digest(repository.get("checkedOutCommit"), {40}, "checkedOutCommit")

    workflow = exact_keys(document.get("workflow"), WORKFLOW_KEYS, "workflow")
    if workflow.get("path") != ".github/workflows/build.yml":
        raise ReceiptError("unexpected workflow path")
    head_blob = hex_digest(workflow.get("gitBlobAtHead"), {40, 64}, "workflow.gitBlobAtHead")
    if workflow.get("workingTreeGitBlob") != head_blob:
        raise ReceiptError("workflow working tree differs from its committed blob")
    hex_digest(workflow.get("sha256"), {64}, "workflow.sha256")

    run = exact_keys(document.get("run"), RUN_KEYS, "run")
    for key in ("id", "attempt", "number"):
        if re.fullmatch(r"[1-9][0-9]*", nonempty(run.get(key), "run." + key)) is None:
            raise ReceiptError("run.%s is not a positive decimal string" % key)
    if run.get("eventName") not in {"push", "pull_request", "workflow_dispatch"}:
        raise ReceiptError("run.eventName is not an accepted event")
    for key in ("job", "ref", "workflowRef"):
        nonempty(run.get(key), "run." + key)
    if run.get("githubSha") != commit:
        raise ReceiptError("run.githubSha differs from checked-out commit")

    platform = exact_keys(document.get("platform"), PLATFORM_KEYS, "platform")
    key = platform.get("key")
    if key not in MATRIX:
        raise ReceiptError("unknown native platform key")
    expected_label, expected_os, expected_arch, expected_toolchain, expected_generator, expected_compiler = MATRIX[key]
    expected_platform = {
        "declaredRunnerLabel": expected_label,
        "declaredRunnerOS": expected_os,
        "declaredRunnerArch": expected_arch,
        "observedRunnerOS": expected_os,
        "observedRunnerArch": expected_arch,
    }
    for field, expected in expected_platform.items():
        if platform.get(field) != expected:
            raise ReceiptError("%s %s differs" % (key, field))
    for field in ("imageOS", "imageVersion", "system", "release", "version", "machine"):
        nonempty(platform.get(field), "platform." + field)

    toolchain = exact_keys(document.get("toolchain"), TOOLCHAIN_KEYS, "toolchain")
    if toolchain.get("declared") != expected_toolchain:
        raise ReceiptError("declared toolchain differs for %s" % key)
    if toolchain.get("generator") != expected_generator:
        raise ReceiptError("generator differs for %s" % key)
    if toolchain.get("compilerId") != expected_compiler:
        raise ReceiptError("compiler ID differs for %s" % key)
    if toolchain.get("buildType") not in {"Release", "" if key == "windows-msvc" else "Release"}:
        raise ReceiptError("native qualification is not a Release build")
    if toolchain.get("cxxStandard") != "17":
        raise ReceiptError("native qualification C++ standard differs")
    for field in ("compilerPath", "compilerVersion", "pointerSize", "cxxFlagsRelease"):
        nonempty(toolchain.get(field), "toolchain." + field)
    for field in ("generatorPlatform", "generatorToolset", "cxxFlags", "compilerTarget", "compilerArchitectureId"):
        nullable_string(toolchain.get(field), "toolchain." + field)
    tools = exact_keys(toolchain.get("tools"), {"cmake", "ctest", "python", "git", "cxxCompiler"}, "toolchain.tools")
    for tool_name, record in tools.items():
        _validate_tool(record, "toolchain.tools." + tool_name)

    environment = document.get("environment")
    if not isinstance(environment, list) or len(environment) != len(native_runner.ENVIRONMENT_WHITELIST):
        raise ReceiptError("environment does not cover the exact whitelist")
    names = []
    for index, raw in enumerate(environment):
        row = exact_keys(raw, {"name", "present", "value"}, "environment[%d]" % index)
        name = nonempty(row.get("name"), "environment name")
        if not isinstance(row.get("present"), bool):
            raise ReceiptError("environment present flag is not boolean")
        nullable_string(row.get("value"), "environment value")
        if row["present"] != (row["value"] is not None):
            raise ReceiptError("environment presence/value differs for %s" % name)
        names.append(name)
    if names != list(native_runner.ENVIRONMENT_WHITELIST):
        raise ReceiptError("environment whitelist order or membership differs")

    contracts = exact_keys(document.get("contracts"), CONTRACT_KEYS, "contracts")
    if contracts.get("requiredTestsPath") != "metadata/qualification-required-tests-v1.json":
        raise ReceiptError("required-test contract path differs")
    for key_name in ("requiredTestsSha256", "focusedNameDigest", "broadNameDigest"):
        hex_digest(contracts.get(key_name), {64}, "contracts." + key_name)
    hex_digest(contracts.get("broadGeneratedAtCommit"), {40}, "contracts.broadGeneratedAtCommit")
    if contracts["focusedNameDigest"] != contract["focusedNameDigest"]:
        raise ReceiptError("focused test digest differs from the contract")
    broad = contract["broadInventory"]
    if contracts["broadNameDigest"] != broad["nameDigest"] or contracts["broadGeneratedAtCommit"] != broad["generatedAtCommit"]:
        raise ReceiptError("broad inventory contract binding differs")

    inventory = exact_keys(document.get("inventory"), INVENTORY_KEYS, "inventory")
    inventory_names = inventory.get("names")
    if not isinstance(inventory_names, list) or inventory_names != broad["names"]:
        raise ReceiptError("receipt inventory differs from the frozen broad inventory")
    if inventory.get("count") != broad["count"] or inventory.get("nameDigest") != broad["nameDigest"]:
        raise ReceiptError("receipt broad inventory count/digest differs")
    focused_rows = {row["id"]: row for row in contract["focusedTests"]}
    selected = sorted(focused_rows, key=lambda value: value.encode("utf-8")) if suite == "focused" else broad["names"]
    if (
        inventory.get("selectedCount") != len(selected)
        or inventory.get("selectedNameDigest") != native_runner.test_name_digest(selected)
        or inventory.get("focusedLabel") != native_runner.FOCUSED_LABEL
    ):
        raise ReceiptError("selected test inventory differs")

    tests = document.get("tests")
    if not isinstance(tests, list) or len(tests) != len(selected):
        raise ReceiptError("test results have the wrong cardinality")
    actual_ids = []
    status_counts = {"passed": 0, "failed": 0, "skipped": 0, "notRun": 0}
    for index, raw in enumerate(tests):
        row = exact_keys(raw, TEST_KEYS, "tests[%d]" % index)
        test_id = nonempty(row.get("testId"), "test ID")
        actual_ids.append(test_id)
        if row.get("status") not in status_counts:
            raise ReceiptError("test %s has an invalid status" % test_id)
        status_counts[row["status"]] += 1
        if row.get("suppliesIndependentSemanticEvidence") is not False:
            raise ReceiptError("test %s falsely claims independent semantic evidence" % test_id)
        if test_id in focused_rows:
            expected = focused_rows[test_id]
            if (
                row.get("expectedExecutionMode") != expected["expectedExecutionMode"]
                or row.get("observedExecutionMode") != expected["expectedExecutionMode"]
                or row.get("expectedCommandBasename") != expected["expectedCommandBasename"]
                or row.get("observedCommandBasename") != expected["expectedCommandBasename"]
            ):
                raise ReceiptError("focused test %s mode/command attestation differs" % test_id)
            shape = row.get("commandShape")
            if not isinstance(shape, list) or not shape or any(not isinstance(token, str) for token in shape):
                raise ReceiptError("focused test %s lacks its exact command shape" % test_id)
            expected_shape_digest = hashlib.sha256(json.dumps(shape, ensure_ascii=False, separators=(",", ":")).encode()).hexdigest()
            if row.get("commandShapeDigest") != expected_shape_digest:
                raise ReceiptError("focused test %s command-shape digest differs" % test_id)
        else:
            if any(row.get(field) is not None for field in ("expectedCommandBasename", "observedCommandBasename", "commandShape", "commandShapeDigest")):
                raise ReceiptError("broad-only test %s has invented command evidence" % test_id)
            if row.get("expectedExecutionMode") != "broad-inventory-test" or row.get("observedExecutionMode") != "broad-inventory-test":
                raise ReceiptError("broad-only test %s mode differs" % test_id)
    if actual_ids != selected:
        raise ReceiptError("test result order/membership differs from the frozen inventory")

    summary = exact_keys(document.get("summary"), SUMMARY_KEYS, "summary")
    for name, count in summary.items():
        nonnegative(count, "summary." + name)
    if summary["total"] != len(selected) or any(summary[name] != status_counts[name] for name in status_counts):
        raise ReceiptError("test summary differs from per-test results")
    exit_code = document.get("ctestExitCode")
    if not isinstance(exit_code, int) or isinstance(exit_code, bool):
        raise ReceiptError("ctestExitCode must be an integer")
    conclusion = document.get("qualificationConclusion")
    success = exit_code == 0 and summary == {
        "total": len(selected), "passed": len(selected), "failed": 0,
        "skipped": 0, "notRun": 0,
    }
    if conclusion != ("success" if success else "failure"):
        raise ReceiptError("qualification conclusion differs from exact test outcomes")
    if expected_observation_sha256 is not None:
        hex_digest(expected_observation_sha256, {64}, "observation digest")


def build_receipt(
    observation: dict[str, Any], contract: dict[str, Any], *,
    observation_sha256: str, implementation_manifest_sha256: str,
    implementation_sha256: str, claims_sha256: str, schema_sha256: str,
) -> dict[str, Any]:
    validate_observation(observation, contract, expected_observation_sha256=observation_sha256)
    for label, value in (
        ("observation", observation_sha256),
        ("implementation manifest", implementation_manifest_sha256),
        ("implementation", implementation_sha256),
        ("claims", claims_sha256),
        ("receipt schema", schema_sha256),
    ):
        hex_digest(value, {64}, label + " digest")
    receipt = copy.deepcopy(observation)
    receipt["kind"] = "libdxfrw-native-qualification-receipt"
    receipt["observationSha256"] = observation_sha256
    receipt["qualificationDigests"] = {
        "implementation": {
            "algorithm": "sha256-git-blob-records-v1",
            "manifestPath": "metadata/qualification-implementation-inputs-v1.json",
            "manifestSha256": implementation_manifest_sha256,
            "value": implementation_sha256,
        },
        "immutableClaims": {
            "path": "metadata/qualified-format-claims-v1.json",
            "sha256": claims_sha256,
        },
        "receiptSchema": {
            "path": "metadata/native-qualification-receipt-schema-v1.json",
            "sha256": schema_sha256,
        },
    }
    receipt["suppliesIndependentSemanticEvidence"] = False
    return receipt


def validate_receipt(document: dict[str, Any], contract: dict[str, Any]) -> None:
    exact_keys(document, RECEIPT_KEYS, "native receipt")
    if document.get("kind") != "libdxfrw-native-qualification-receipt":
        raise ReceiptError("unexpected native receipt kind")
    observation = {
        key: copy.deepcopy(value) for key, value in document.items()
        if key not in {"observationSha256", "qualificationDigests", "suppliesIndependentSemanticEvidence"}
    }
    observation["kind"] = native_runner.OBSERVATION_KIND
    validate_observation(observation, contract)
    if document.get("suppliesIndependentSemanticEvidence") is not False:
        raise ReceiptError("native receipt falsely claims independent semantic evidence")
    hex_digest(document.get("observationSha256"), {64}, "observationSha256")
    digests = exact_keys(document.get("qualificationDigests"), QUALIFICATION_DIGEST_KEYS, "qualificationDigests")
    implementation = exact_keys(digests.get("implementation"), {"algorithm", "manifestPath", "manifestSha256", "value"}, "implementation digest")
    if implementation.get("algorithm") != "sha256-git-blob-records-v1" or implementation.get("manifestPath") != "metadata/qualification-implementation-inputs-v1.json":
        raise ReceiptError("implementation digest identity differs")
    hex_digest(implementation.get("manifestSha256"), {64}, "implementation manifest digest")
    hex_digest(implementation.get("value"), {64}, "implementation digest")
    claims = exact_keys(digests.get("immutableClaims"), {"path", "sha256"}, "claims digest")
    if claims.get("path") != "metadata/qualified-format-claims-v1.json":
        raise ReceiptError("claims path differs")
    hex_digest(claims.get("sha256"), {64}, "claims digest")
    schema = exact_keys(digests.get("receiptSchema"), {"path", "sha256"}, "receipt schema digest")
    if schema.get("path") != "metadata/native-qualification-receipt-schema-v1.json":
        raise ReceiptError("receipt schema path differs")
    hex_digest(schema.get("sha256"), {64}, "receipt schema digest")


def _fixture_contract() -> dict[str, Any]:
    document = copy.deepcopy(read_json(DEFAULT_TESTS))
    names = sorted([row["id"] for row in document["focusedTests"]], key=lambda value: value.encode("utf-8"))
    document["broadInventory"] = {
        "status": "FINAL",
        "generatedAtCommit": "1" * 40,
        "configurationDigest": document["broadInventory"]["configurationDigest"],
        "count": len(names),
        "nameDigest": native_runner.test_name_digest(names),
        "names": names,
    }
    return native_runner.validate_contract(document, require_final=True)


def _fixture_observation(contract: dict[str, Any]) -> dict[str, Any]:
    focused = {row["id"]: row for row in contract["focusedTests"]}
    names = contract["broadInventory"]["names"]
    tool = {"path": "/usr/bin/tool", "sha256": "2" * 64, "versionOutput": "tool 1", "versionExitCode": 0}
    tests = []
    for name in names:
        row = focused[name]
        shape = ["$BUILD/" + row["expectedCommandBasename"]]
        tests.append({
            "testId": name,
            "expectedExecutionMode": row["expectedExecutionMode"],
            "observedExecutionMode": row["expectedExecutionMode"],
            "expectedCommandBasename": row["expectedCommandBasename"],
            "observedCommandBasename": row["expectedCommandBasename"],
            "commandShape": shape,
            "commandShapeDigest": hashlib.sha256(json.dumps(shape, separators=(",", ":")).encode()).hexdigest(),
            "suppliesIndependentSemanticEvidence": False,
            "status": "passed",
        })
    return {
        "schema": 1,
        "kind": native_runner.OBSERVATION_KIND,
        "suite": "focused",
        "generatedAtUtc": "2026-09-16T12:00:00Z",
        "artifactPolicy": contract["artifactPolicy"],
        "repository": {"githubRepository": "LibreCAD/libdxfrw", "githubRepositoryId": "42", "checkedOutCommit": "3" * 40},
        "workflow": {"path": ".github/workflows/build.yml", "gitBlobAtHead": "4" * 40, "workingTreeGitBlob": "4" * 40, "sha256": "5" * 64},
        "run": {"id": "101", "attempt": "1", "number": "9", "eventName": "workflow_dispatch", "job": "native-focused", "ref": "refs/heads/master", "githubSha": "3" * 40, "workflowRef": "LibreCAD/libdxfrw/.github/workflows/build.yml@refs/heads/master"},
        "platform": {"key": "linux-gcc", "declaredRunnerLabel": "ubuntu-24.04", "declaredRunnerOS": "Linux", "declaredRunnerArch": "X64", "observedRunnerOS": "Linux", "observedRunnerArch": "X64", "imageOS": "ubuntu24", "imageVersion": "20260901.1", "system": "Linux", "release": "6.11", "version": "#1", "machine": "x86_64"},
        "toolchain": {"declared": "gcc-13", "generator": "Ninja", "generatorPlatform": None, "generatorToolset": None, "buildType": "Release", "cxxStandard": "17", "cxxFlags": "", "cxxFlagsRelease": "-O3 -DNDEBUG", "compilerPath": "/usr/bin/g++-13", "compilerId": "GNU", "compilerVersion": "13.3.0", "compilerTarget": None, "compilerArchitectureId": None, "pointerSize": "8", "tools": {name: copy.deepcopy(tool) for name in ("cmake", "ctest", "python", "git", "cxxCompiler")}},
        "environment": [{"name": name, "present": name == "PATH", "value": "/usr/bin" if name == "PATH" else None} for name in native_runner.ENVIRONMENT_WHITELIST],
        "contracts": {"requiredTestsPath": "metadata/qualification-required-tests-v1.json", "requiredTestsSha256": "6" * 64, "focusedNameDigest": contract["focusedNameDigest"], "broadNameDigest": contract["broadInventory"]["nameDigest"], "broadGeneratedAtCommit": contract["broadInventory"]["generatedAtCommit"]},
        "inventory": {"count": len(names), "nameDigest": native_runner.test_name_digest(names), "names": names, "selectedCount": len(names), "selectedNameDigest": native_runner.test_name_digest(names), "focusedLabel": native_runner.FOCUSED_LABEL},
        "tests": tests,
        "summary": {"total": len(names), "passed": len(names), "failed": 0, "skipped": 0, "notRun": 0},
        "ctestExitCode": 0,
        "qualificationConclusion": "success",
    }


def _expect_error(callable_value: Any, label: str) -> None:
    try:
        callable_value()
    except (ReceiptError, native_runner.QualificationError):
        return
    raise AssertionError("negative receipt vector was accepted: %s" % label)


def self_test() -> None:
    contract = _fixture_contract()
    observation = _fixture_observation(contract)
    observation_bytes = canonical_bytes(observation)
    receipt = build_receipt(
        observation, contract,
        observation_sha256=hashlib.sha256(observation_bytes).hexdigest(),
        implementation_manifest_sha256="7" * 64,
        implementation_sha256="8" * 64,
        claims_sha256="9" * 64,
        schema_sha256="a" * 64,
    )
    validate_receipt(receipt, contract)
    if canonical_bytes(receipt) != canonical_bytes(json.loads(canonical_bytes(receipt))):
        raise AssertionError("receipt canonicalization is nondeterministic")
    invalid = copy.deepcopy(observation)
    invalid["run"]["id"] = None
    _expect_error(lambda: validate_observation(invalid, contract), "missing run ID")
    invalid = copy.deepcopy(observation)
    invalid["platform"]["declaredRunnerLabel"] = "ubuntu-latest"
    _expect_error(lambda: validate_observation(invalid, contract), "drifting runner label")
    invalid = copy.deepcopy(observation)
    invalid["tests"][0]["observedExecutionMode"] = "broad-inventory-test"
    _expect_error(lambda: validate_observation(invalid, contract), "lost exact focused execution mode")
    invalid = copy.deepcopy(observation)
    invalid["tests"][0]["suppliesIndependentSemanticEvidence"] = True
    _expect_error(lambda: validate_observation(invalid, contract), "false semantic authority")
    invalid = copy.deepcopy(observation)
    invalid["tests"].pop()
    _expect_error(lambda: validate_observation(invalid, contract), "missing focused test")
    invalid_receipt = copy.deepcopy(receipt)
    invalid_receipt["jobId"] = 123
    _expect_error(lambda: validate_receipt(invalid_receipt, contract), "pre-upload numeric job ID")
    with tempfile.TemporaryDirectory(prefix="libdxfrw-receipt-") as directory:
        path = Path(directory) / "receipt.json"
        path.write_bytes(canonical_bytes(receipt))
        if sha256_file(path) != hashlib.sha256(canonical_bytes(receipt)).hexdigest():
            raise AssertionError("receipt content hash drifted")
    print("native qualification receipt emitter self-test: PASS")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--claims", type=Path, default=DEFAULT_CLAIMS)
    parser.add_argument("--status", type=Path, default=DEFAULT_STATUS)
    parser.add_argument("--implementation-inputs", type=Path, default=DEFAULT_INPUTS)
    parser.add_argument("--required-tests", type=Path, default=DEFAULT_TESTS)
    parser.add_argument("--schema", type=Path, default=DEFAULT_SCHEMA)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args(argv)
    try:
        if args.self_test:
            self_test()
            return 0
        if args.input is None or args.output is None:
            raise ReceiptError("--input and --output are required")
        schema = read_json(args.schema)
        if schema.get("$schema") != "https://json-schema.org/draft/2020-12/schema" or schema.get("title") != "libdxfrw native qualification receipt v1":
            raise ReceiptError("unexpected native receipt schema")
        contract = native_runner.validate_contract(read_json(args.required_tests), require_final=True)
        input_bytes = args.input.read_bytes()
        observation = read_json(args.input)
        if canonical_bytes(observation) != input_bytes:
            raise ReceiptError("native observation is not canonical JSON with one trailing LF")
        if observation["contracts"]["requiredTestsSha256"] != sha256_file(args.required_tests):
            raise ReceiptError("observation binds a stale required-test contract")
        claims_sha = sha256_file(args.claims)
        status = read_json(args.status)
        if status.get("freezeState") != "FROZEN" or status.get("claimsDigestSha256") != claims_sha:
            raise ReceiptError("claims/status digests are not frozen")
        inputs_document = digest_tool._read_json(args.implementation_inputs)
        digest_tool.validate_manifest(inputs_document, allow_draft=False)
        computed = digest_tool.compute_digest(ROOT, inputs_document, allow_selected_untracked=False)
        digest_tool.verify_status_digest(computed, args.status, allow_draft=False)
        receipt = build_receipt(
            observation, contract,
            observation_sha256=hashlib.sha256(input_bytes).hexdigest(),
            implementation_manifest_sha256=sha256_file(args.implementation_inputs),
            implementation_sha256=computed["implementationDigestSha256"],
            claims_sha256=claims_sha,
            schema_sha256=sha256_file(args.schema),
        )
        validate_receipt(receipt, contract)
        encoded = canonical_bytes(receipt)
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_bytes(encoded)
        print("native receipt: PASS (%s)" % hashlib.sha256(encoded).hexdigest())
        return 0
    except (
        OSError, UnicodeError, json.JSONDecodeError, ReceiptError,
        native_runner.QualificationError, digest_tool.DigestError, AssertionError,
    ) as exc:
        print("native receipt: FAIL: %s" % exc, file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
