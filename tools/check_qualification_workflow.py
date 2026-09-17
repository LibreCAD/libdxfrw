#!/usr/bin/env python3
"""Fail-closed audit of the native qualification GitHub Actions workflow.

This checker intentionally understands the repository's constrained workflow
shape instead of depending on a permissive YAML loader.  It pins action commit
IDs, fixed hosted labels, matrix architecture/toolchain declarations, event
routing, the seven-test focused profile, manual-only broad validation, and a
metadata-only artifact allow-list.  The CTest runner independently rechecks
the registered test commands and labels at execution time.
"""

from __future__ import annotations

import argparse
import copy
import hashlib
import re
import sys
from pathlib import Path
from typing import Any

sys.path.insert(0, str(Path(__file__).resolve().parent))
from run_native_qualification import (  # noqa: E402
    QualificationError,
    canonical_bytes,
    read_json,
    test_name_digest,
    validate_contract,
)


CHECKOUT_SHA = "3d3c42e5aac5ba805825da76410c181273ba90b1"
UPLOAD_SHA = "043fb46d1a93c77aae656e7c1c64a875d1fc6a0a"
EXPECTED_WORKFLOW_SHA256 = "72c3c75af1d3a8836564bc11ac695d787f9c8fe0f9e132b8c0571d3542f51473"
ACTION_RE = re.compile(r"^\s*uses:\s*([^\s#]+)", re.MULTILINE)
FULL_ACTION_RE = re.compile(r"[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+@[0-9a-f]{40}")
EXPECTED_ACTIONS = {
    "actions/checkout@" + CHECKOUT_SHA: 4,
    "actions/upload-artifact@" + UPLOAD_SHA: 2,
}
EXPECTED_JOBS = {
    "workflow-contract", "native-focused", "native-final-broad", "long-fuzz",
}
EXPECTED_MATRIX = {
    "linux-gcc": {
        "runnerLabel": "ubuntu-24.04",
        "runnerOS": "Linux",
        "runnerArch": "X64",
        "toolchain": "gcc-13",
        "pythonCommand": "python3",
        "configureArgs": "-G Ninja -DCMAKE_C_COMPILER=gcc-13 -DCMAKE_CXX_COMPILER=g++-13",
    },
    "macos-clang": {
        "runnerLabel": "macos-15",
        "runnerOS": "macOS",
        "runnerArch": "ARM64",
        "toolchain": "apple-clang",
        "pythonCommand": "python3",
        "configureArgs": "-G Ninja -DCMAKE_C_COMPILER=/usr/bin/clang -DCMAKE_CXX_COMPILER=/usr/bin/clang++",
    },
    "windows-msvc": {
        "runnerLabel": "windows-2022",
        "runnerOS": "Windows",
        "runnerArch": "X64",
        "toolchain": "msvc-vs2022-x64",
        "pythonCommand": "python",
        "configureArgs": '-G "Visual Studio 17 2022" -A x64',
    },
}
PROFILE_MARKERS = {
    "-DCMAKE_BUILD_TYPE=Release",
    "-DBUILD_SHARED_LIBS=OFF",
    "-DLIBDXFRW_BUILD_DOC=OFF",
    "-DLIBDXFRW_BUILD_DWG2DXF=ON",
    "-DLIBDXFRW_BUILD_TESTS=ON",
    "-DLIBDXFRW_BUILD_QUALIFICATION=OFF",
    "-DLIBDXFRW_BUILD_LONG_FUZZ=OFF",
}
FOCUSED_TARGETS = {
    "libdxfrw_dwg_local_roundtrip",
    "libdxfrw_dwg_reader_matrix_tests",
    "libdxfrw_diagnostic_tests",
    "libdxfrw_dwg_fixture_tests",
}
ARTIFACT_PATHS = {
    "qualification-output/qualification-status.json",
    "qualification-output/ctest-inventory.json",
    "qualification-output/ctest.log",
    "qualification-output/ctest.xml",
    "qualification-output/native-qualification-observation-v1.json",
    "qualification-output/native-qualification-receipt-v1.json",
    "qualification-output/native-qualification-receipt-v1.sha256",
}
RECEIPT_INPUTS = {
    "--claims metadata/qualified-format-claims-v1.json",
    "--status metadata/qualified-format-status-v1.json",
    "--implementation-inputs metadata/qualification-implementation-inputs-v1.json",
    "--required-tests metadata/qualification-required-tests-v1.json",
}


class WorkflowError(ValueError):
    """Raised when a workflow can run a non-attested or over-broad profile."""


def require(condition: bool, message: str) -> None:
    if not condition:
        raise WorkflowError(message)


def compact(text: str) -> str:
    return re.sub(r"\s+", " ", text).strip()


def job_blocks(text: str) -> dict[str, str]:
    lines = text.splitlines(keepends=True)
    jobs_index = next((index for index, line in enumerate(lines)
                       if line.rstrip() == "jobs:" and not line.startswith(" ")), None)
    if jobs_index is None:
        raise WorkflowError("workflow has no top-level jobs section")
    starts: list[tuple[str, int]] = []
    for index in range(jobs_index + 1, len(lines)):
        line = lines[index]
        if line.strip() and not line.startswith(" "):
            break
        match = re.match(r"^  ([A-Za-z0-9_-]+):\s*$", line.rstrip("\n\r"))
        if match:
            starts.append((match.group(1), index))
    if not starts:
        raise WorkflowError("workflow jobs section is empty")
    blocks: dict[str, str] = {}
    for offset, (name, start) in enumerate(starts):
        end = starts[offset + 1][1] if offset + 1 < len(starts) else len(lines)
        if name in blocks:
            raise WorkflowError("duplicate workflow job %s" % name)
        blocks[name] = "".join(lines[start:end])
    return blocks


def parse_matrix_rows(block: str) -> dict[str, dict[str, str]]:
    lines = block.splitlines()
    rows: dict[str, dict[str, str]] = {}
    index = 0
    while index < len(lines):
        match = re.match(r"^\s+- platformKey:\s*([^\s#]+)\s*$", lines[index])
        if not match:
            index += 1
            continue
        key = match.group(1)
        if key in rows:
            raise WorkflowError("duplicate matrix platform %s" % key)
        row: dict[str, str] = {}
        index += 1
        while index < len(lines) and not re.match(r"^\s+- platformKey:", lines[index]):
            field = re.match(
                r"^\s+(runnerLabel|runnerOS|runnerArch|toolchain|pythonCommand|configureArgs):\s*(.*?)\s*$",
                lines[index],
            )
            if field:
                value = field.group(2)
                if value.startswith('"') and value.endswith('"'):
                    value = value[1:-1].replace('\\"', '"')
                row[field.group(1)] = value
            index += 1
        rows[key] = row
    return rows


def parse_literal_paths(text: str) -> list[set[str]]:
    lines = text.splitlines()
    results: list[set[str]] = []
    for index, line in enumerate(lines):
        match = re.match(r"^(\s*)path:\s*\|\s*$", line)
        if not match:
            continue
        indentation = len(match.group(1))
        values: set[str] = set()
        cursor = index + 1
        while cursor < len(lines):
            candidate = lines[cursor]
            stripped = candidate.strip()
            if stripped and len(candidate) - len(candidate.lstrip()) <= indentation:
                break
            if stripped:
                values.add(stripped)
            cursor += 1
        results.append(values)
    return results


def validate_actions(text: str) -> None:
    actions = ACTION_RE.findall(text)
    require(actions, "workflow contains no actions")
    for action in actions:
        require(FULL_ACTION_RE.fullmatch(action) is not None,
                "action is not pinned to a full lowercase commit: %s" % action)
    actual = {action: actions.count(action) for action in set(actions)}
    require(actual == EXPECTED_ACTIONS,
            "action pins/counts differ: expected=%s actual=%s" % (
                EXPECTED_ACTIONS, actual))
    require("actions/checkout@%s # v7.0.1" % CHECKOUT_SHA in text,
            "checkout pin lacks its verified v7.0.1 annotation")
    require("actions/upload-artifact@%s # v7.0.1" % UPLOAD_SHA in text,
            "upload-artifact pin lacks its verified v7.0.1 annotation")


def validate_triggers(text: str) -> None:
    header = text.split("\njobs:\n", 1)[0]
    for marker in (
        "on:\n", "  push:\n", "  pull_request:\n", "  workflow_dispatch:\n",
        "  schedule:\n", "      qualification_suite:\n", "        type: choice\n",
        "        default: focused\n", "          - focused\n",
        "          - final-broad\n", "          - extended-fuzz\n",
    ):
        require(marker in header, "workflow trigger/input is missing %r" % marker.strip())
    require(header.count("- final-broad") == 1,
            "final-broad dispatch option must occur exactly once")
    require("permissions:\n  contents: read\n" in header,
            "workflow must have contents:read as its only top-level permission")


def validate_matrix(block: str, label: str) -> None:
    require("runs-on: ${{ matrix.runnerLabel }}" in block,
            "%s does not dispatch on the declared runner label" % label)
    require("fail-fast: false" in block, "%s must use fail-fast:false" % label)
    rows = parse_matrix_rows(block)
    require(rows == EXPECTED_MATRIX,
            "%s matrix differs: expected=%s actual=%s" % (
                label, EXPECTED_MATRIX, rows))
    require("latest" not in block.lower(), "%s uses a drifting latest runner" % label)


def validate_profile(block: str, label: str) -> None:
    for marker in PROFILE_MARKERS:
        require(block.count(marker) == 1,
                "%s must contain fixed profile marker exactly once: %s" % (
                    label, marker))
    require("${{ matrix.configureArgs }}" in block,
            "%s does not use its declared toolchain arguments" % label)


def validate_receipt_lane(block: str, suite: str) -> None:
    collapsed = compact(block)
    for marker in (
        "--manifest metadata/qualification-required-tests-v1.json",
        "--workflow .github/workflows/build.yml",
        "--require-github-runner",
        "--declared-runner-label ${{ matrix.runnerLabel }}",
        "--declared-runner-os ${{ matrix.runnerOS }}",
        "--declared-runner-arch ${{ matrix.runnerArch }}",
        "--declared-toolchain ${{ matrix.toolchain }}",
        "--input qualification-output/native-qualification-observation-v1.json",
        "--output qualification-output/native-qualification-receipt-v1.json",
    ):
        require(marker in collapsed, "%s lane lacks %s" % (suite, marker))
    for marker in RECEIPT_INPUTS:
        require(marker in collapsed, "%s receipt lane lacks %s" % (suite, marker))
    require("--suite %s" % suite in collapsed,
            "%s lane invokes the wrong runner suite" % suite)
    require(block.count("continue-on-error: true") == 3,
            "%s must preserve qualification, receipt, and preflight failures" % suite)
    require(block.count("${{ matrix.pythonCommand }} -B tools/run_native_qualification.py enforce") == 2,
            "%s must preflight before upload and enforce after upload" % suite)
    require(block.count("${{ matrix.pythonCommand }} -B") == 4,
            "%s must use its declared Python command for all qualification steps" % suite)
    artifact_name = (
        "native-qualification-${{ github.run_id }}-${{ github.run_attempt }}-"
        + suite + "-${{ matrix.platformKey }}"
    )
    require(artifact_name in block,
            "%s artifact name does not bind run/attempt/platform" % suite)
    for marker in (
        "if-no-files-found: error", "include-hidden-files: false",
        "overwrite: false", "persist-credentials: false", "fetch-depth: 0",
    ):
        require(marker in block, "%s lane lacks %s" % (suite, marker))


def validate_workflow_text(text: str) -> None:
    actual_digest = hashlib.sha256(text.encode("utf-8")).hexdigest()
    require(
        actual_digest == EXPECTED_WORKFLOW_SHA256,
        "workflow bytes differ from the frozen qualification workflow",
    )
    require("\t" not in text, "workflow contains tab indentation")
    require(not re.search(r"(?:ubuntu|macos|windows)-latest", text),
            "workflow uses a drifting latest runner label")
    validate_triggers(text)
    validate_actions(text)
    jobs = job_blocks(text)
    require(set(jobs) == EXPECTED_JOBS,
            "workflow jobs differ (missing=%s, extra=%s)" % (
                sorted(EXPECTED_JOBS - set(jobs)),
                sorted(set(jobs) - EXPECTED_JOBS)))

    contract = jobs["workflow-contract"]
    require("runs-on: ubuntu-24.04" in contract,
            "workflow-contract runner is not fixed")
    require("--allow-unfinalized" not in contract,
            "CI workflow contract must fail closed on an unfinalized inventory")
    require("tools/check_qualification_workflow.py" in contract,
            "workflow does not validate its own qualification contract")

    focused = jobs["native-focused"]
    validate_matrix(focused, "focused")
    validate_profile(focused, "focused")
    focused_compact = compact(focused)
    for marker in (
        "github.event_name == 'push'", "github.event_name == 'pull_request'",
        "github.event_name == 'workflow_dispatch'",
        "inputs.qualification_suite == 'focused'",
    ):
        require(marker in focused_compact, "focused routing lacks %s" % marker)
    require("github.event_name == 'schedule'" not in focused,
            "scheduled runs must not execute the focused matrix")
    require("--target" in focused, "focused lane does not build bounded targets")
    for target in FOCUSED_TARGETS:
        require(focused.count(target) == 1,
                "focused target %s is absent or duplicated" % target)
    validate_receipt_lane(focused, "focused")

    broad = jobs["native-final-broad"]
    validate_matrix(broad, "final-broad")
    validate_profile(broad, "final-broad")
    broad_compact = compact(broad)
    require("github.event_name == 'workflow_dispatch'" in broad_compact,
            "final-broad lane is not manual-only")
    require("inputs.qualification_suite == 'final-broad'" in broad_compact,
            "final-broad lane lacks the exact manual input gate")
    for forbidden in ("github.event_name == 'push'", "github.event_name == 'pull_request'",
                      "github.event_name == 'schedule'"):
        require(forbidden not in broad,
                "final-broad lane is reachable from %s" % forbidden)
    require("run: cmake --build build --config Release --parallel 2" in broad,
            "final-broad lane does not build the complete configured graph")
    validate_receipt_lane(broad, "final-broad")

    fuzz = jobs["long-fuzz"]
    fuzz_compact = compact(fuzz)
    require("runs-on: ubuntu-24.04" in fuzz,
            "extended fuzz must use fixed ubuntu-24.04")
    require("github.event_name == 'schedule'" in fuzz_compact,
            "extended fuzz lacks its scheduled route")
    require("inputs.qualification_suite == 'extended-fuzz'" in fuzz_compact,
            "extended fuzz lacks its manual route")
    require("-DLIBDXFRW_BUILD_LONG_FUZZ=ON" in fuzz,
            "extended fuzz is not explicitly enabled")
    require("-R '^libdxfrw_long_fuzz$'" in fuzz,
            "extended fuzz does not select only its exact CTest")
    require("upload-artifact" not in fuzz,
            "fuzz lane must not upload unbounded build/test artifacts")

    paths = parse_literal_paths(text)
    require(len(paths) == 2, "workflow must contain exactly two literal artifact paths")
    for index, values in enumerate(paths):
        require(values == ARTIFACT_PATHS,
                "artifact path set %d differs: expected=%s actual=%s" % (
                    index, sorted(ARTIFACT_PATHS), sorted(values)))
    lowered = "\n".join("\n".join(sorted(values)) for values in paths).lower()
    require(".dwg" not in lowered and ".dxf" not in lowered,
            "artifact upload includes a drawing suffix")
    require("build/" not in lowered and "build-" not in lowered,
            "artifact upload includes a build directory")


def validate_files(workflow_path: Path, manifest_path: Path, *, require_final: bool) -> None:
    try:
        workflow_text = workflow_path.read_text(encoding="utf-8")
    except (OSError, UnicodeError) as exc:
        raise WorkflowError("cannot read workflow %s: %s" % (workflow_path, exc)) from exc
    validate_workflow_text(workflow_text)
    try:
        validate_contract(read_json(manifest_path), require_final=require_final)
    except QualificationError as exc:
        raise WorkflowError("required-test contract: %s" % exc) from exc


def expect_rejected(callback: Any, label: str) -> None:
    try:
        callback()
    except (WorkflowError, QualificationError):
        return
    raise AssertionError("mutation was accepted: %s" % label)


def self_test(workflow_path: Path, manifest_path: Path) -> None:
    workflow = workflow_path.read_text(encoding="utf-8")
    manifest = read_json(manifest_path)
    validate_workflow_text(workflow)
    validate_contract(manifest, require_final=True)

    expect_rejected(
        lambda: validate_workflow_text(workflow.replace(
            "actions/checkout@" + CHECKOUT_SHA, "actions/checkout@v7", 1)),
        "tag action pin",
    )
    expect_rejected(
        lambda: validate_workflow_text(workflow.replace("ubuntu-24.04", "ubuntu-latest", 1)),
        "latest runner",
    )
    expect_rejected(
        lambda: validate_workflow_text(workflow.replace(
            "github.event_name == 'pull_request'", "github.event_name == 'issues'", 1)),
        "missing pull-request focus",
    )
    expect_rejected(
        lambda: validate_workflow_text(workflow.replace(
            "inputs.qualification_suite == 'final-broad'",
            "inputs.qualification_suite == 'focused'", 1)),
        "broad route weakened",
    )
    expect_rejected(
        lambda: validate_workflow_text(workflow.replace(
            "qualification-output/ctest.log", "build/ctest.log", 1)),
        "build artifact uploaded",
    )
    expect_rejected(
        lambda: validate_workflow_text(workflow.replace("runnerArch: ARM64", "runnerArch: X64", 1)),
        "macOS architecture drift",
    )
    expect_rejected(
        lambda: validate_workflow_text(workflow.replace(
            "permissions:\n  contents: read\n",
            "permissions:\n  contents: read\n  id-token: write\n", 1)),
        "broadened top-level permission",
    )
    expect_rejected(
        lambda: validate_workflow_text(workflow.replace(
            "inputs.qualification_suite == 'focused')",
            "inputs.qualification_suite == 'focused') && false", 1)),
        "silently disabled focused route",
    )
    expect_rejected(
        lambda: validate_workflow_text(workflow.replace(
            "    needs: workflow-contract\n", "", 1)),
        "removed workflow-contract dependency",
    )
    expect_rejected(
        lambda: validate_workflow_text(workflow.replace(
            "-DBUILD_SHARED_LIBS=OFF", "-DBUILD_SHARED_LIBS=OFF -DBUILD_SHARED_LIBS=ON", 1)),
        "contradictory effective configure flag",
    )

    finalized = copy.deepcopy(manifest)
    names = sorted((row["id"] for row in finalized["focusedTests"]),
                   key=lambda item: item.encode("utf-8"))
    finalized["broadInventory"].update({
        "status": "FINAL",
        "generatedAtCommit": "0" * 40,
        "count": len(names),
        "nameDigest": test_name_digest(names),
        "names": names,
    })
    validate_contract(finalized, require_final=True)
    promoted = copy.deepcopy(finalized)
    promoted["focusedTests"][-1]["suppliesIndependentSemanticEvidence"] = True
    expect_rejected(lambda: validate_contract(promoted, require_final=True),
                    "self-test semantic promotion")
    stale = copy.deepcopy(finalized)
    stale["broadInventory"]["nameDigest"] = "0" * 64
    expect_rejected(lambda: validate_contract(stale, require_final=True),
                    "stale broad inventory digest")
    unfinalized = copy.deepcopy(finalized)
    unfinalized["broadInventory"].update({
        "status": "REQUIRES_FINAL_REGENERATION",
        "generatedAtCommit": None,
        "count": None,
        "nameDigest": None,
        "names": [],
    })
    validate_contract(unfinalized, require_final=False)
    expect_rejected(lambda: validate_contract(unfinalized, require_final=True),
                    "unfinalized broad inventory")
    assert canonical_bytes(finalized).endswith(b"\n")
    print("check_qualification_workflow self-test: PASS")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--workflow", type=Path,
                        default=Path(".github/workflows/build.yml"))
    parser.add_argument("--manifest", type=Path,
                        default=Path("metadata/qualification-required-tests-v1.json"))
    parser.add_argument("--allow-unfinalized", action="store_true",
                        help="development-only; CI intentionally never passes this")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args(argv)
    try:
        if args.self_test:
            self_test(args.workflow, args.manifest)
        else:
            validate_files(
                args.workflow, args.manifest,
                require_final=not args.allow_unfinalized,
            )
            suffix = " (broad inventory pending)" if args.allow_unfinalized else ""
            print("qualification workflow contract: PASS%s" % suffix)
        return 0
    except (OSError, UnicodeError, WorkflowError, QualificationError, AssertionError) as exc:
        print("qualification workflow contract: FAIL: %s" % exc, file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
