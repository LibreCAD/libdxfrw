#!/usr/bin/env python3
"""API- and artifact-verify native qualification receipts after upload.

The immutable receipt contains only facts available inside the running job.
This J364 verifier joins it to numeric job/artifact IDs, authenticated GitHub
API responses, and the downloaded seven-file artifact through the mutable,
implementation-digest-excluded status overlay.
"""

from __future__ import annotations

import argparse
import base64
import copy
import hashlib
import json
import os
import stat
import sys
import tempfile
import urllib.request
import warnings
import zipfile
from pathlib import Path, PurePosixPath
from typing import Any

import emit_native_qualification_receipt as receipt_tool
import run_native_qualification as native_runner


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_STATUS = ROOT / "metadata/qualified-format-status-v1.json"
DEFAULT_CLAIMS = ROOT / "metadata/qualified-format-claims-v1.json"
DEFAULT_TESTS = ROOT / "metadata/qualification-required-tests-v1.json"
DEFAULT_SCHEMA = ROOT / "metadata/native-qualification-receipt-schema-v1.json"
DEFAULT_INPUTS = ROOT / "metadata/qualification-implementation-inputs-v1.json"

HEX = set("0123456789abcdef")
MATRIX_KEYS = {"linux-gcc", "macos-clang", "windows-msvc"}
ARTIFACT_ENTRIES = {
    "qualification-status.json",
    "ctest-inventory.json",
    "ctest.log",
    "ctest.xml",
    "native-qualification-observation-v1.json",
    "native-qualification-receipt-v1.json",
    "native-qualification-receipt-v1.sha256",
}
API_CONCLUSIONS = {
    "success", "failure", "cancelled", "timed_out", "action_required",
    "neutral", "skipped", "stale", "startup_failure",
}
REF_KEYS = {
    "id", "suite", "matrixKey", "repository", "workflowPath", "headSha",
    "runId", "runAttempt", "logicalJobKey", "apiJobId", "apiJobName",
    "artifactId", "artifactName", "receiptContentSha256", "runApiUrl",
    "runApiPath", "runApiSha256", "jobsApiUrl", "jobsApiPath",
    "jobsApiSha256", "artifactsApiUrl", "artifactsApiPath",
    "artifactsApiSha256", "workflowApiUrl", "workflowApiPath",
    "workflowApiSha256", "artifactDownloadUrl", "artifactArchivePath",
    "downloadedArchiveSha256", "artifactApiDigestSha256",
    "requiredStepName", "qualificationConclusion", "apiConclusion",
    "accepted", "failureClass", "supersededBy",
}


class VerificationError(ValueError):
    pass


def read_json(path: Path) -> dict[str, Any]:
    try:
        value = native_runner.read_json(path)
    except native_runner.QualificationError as exc:
        raise VerificationError(str(exc)) from exc
    if not isinstance(value, dict):
        raise VerificationError("%s must contain an object" % path)
    return value


def exact_keys(value: object, expected: set[str], label: str) -> dict[str, Any]:
    if not isinstance(value, dict):
        raise VerificationError("%s must be an object" % label)
    actual = set(value)
    if actual != expected:
        raise VerificationError(
            "%s keys differ (missing=%s unexpected=%s)"
            % (label, sorted(expected - actual), sorted(actual - expected))
        )
    return value


def hex_digest(value: object, lengths: set[int], label: str) -> str:
    if not isinstance(value, str) or len(value) not in lengths or any(char not in HEX for char in value):
        raise VerificationError("%s is not a lowercase hexadecimal digest" % label)
    return value


def positive(value: object, label: str) -> int:
    if not isinstance(value, int) or isinstance(value, bool) or value < 1:
        raise VerificationError("%s must be a positive integer" % label)
    return value


def nonempty(value: object, label: str) -> str:
    if not isinstance(value, str) or not value or "\x00" in value:
        raise VerificationError("%s must be a non-empty NUL-free string" % label)
    return value


def safe_relative(value: object, label: str) -> str:
    text = nonempty(value, label)
    if "\\" in text:
        raise VerificationError("%s is not a POSIX relative path" % label)
    path = PurePosixPath(text)
    if path.is_absolute() or any(part in {"", ".", ".."} for part in path.parts):
        raise VerificationError("%s is not a safe relative path" % label)
    return text


def sha256_bytes(value: bytes) -> str:
    return hashlib.sha256(value).hexdigest()


def sha256_file(path: Path) -> str:
    try:
        return hashlib.sha256(path.read_bytes()).hexdigest()
    except OSError as exc:
        raise VerificationError("cannot hash %s: %s" % (path, exc)) from exc


def canonical_digest(value: Any) -> str:
    encoded = json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=False).encode("utf-8")
    return sha256_bytes(encoded)


def git_blob_id(data: bytes, length: int) -> str:
    hasher = hashlib.sha1() if length == 40 else hashlib.sha256()
    hasher.update(("blob %d\0" % len(data)).encode("ascii"))
    hasher.update(data)
    return hasher.hexdigest()


def endpoint_urls(repository: str, ref: dict[str, Any]) -> dict[str, str]:
    api = "https://api.github.com/repos/%s" % repository
    run_id = ref["runId"]
    attempt = ref["runAttempt"]
    head = ref["headSha"]
    return {
        "runApiUrl": "%s/actions/runs/%d/attempts/%d" % (api, run_id, attempt),
        "jobsApiUrl": "%s/actions/runs/%d/attempts/%d/jobs?per_page=100" % (api, run_id, attempt),
        "artifactsApiUrl": "%s/actions/runs/%d/artifacts?per_page=100" % (api, run_id),
        "workflowApiUrl": "%s/contents/.github/workflows/build.yml?ref=%s" % (api, head),
    }


def validate_urls(ref: dict[str, Any]) -> None:
    expected = endpoint_urls(ref["repository"], ref)
    for key, value in expected.items():
        if ref.get(key) != value:
            raise VerificationError("%s does not bind the exact GitHub API endpoint" % key)
    download = nonempty(ref.get("artifactDownloadUrl"), "artifactDownloadUrl")
    expected_download = "https://api.github.com/repos/%s/actions/artifacts/%d/zip" % (
        ref["repository"], ref["artifactId"],
    )
    if download != expected_download:
        raise VerificationError("artifactDownloadUrl differs from the exact artifact endpoint")


def fetch_authenticated(url: str, token: str) -> bytes:
    request = urllib.request.Request(
        url,
        headers={
            "Accept": "application/vnd.github+json",
            "Authorization": "Bearer " + token,
            "X-GitHub-Api-Version": "2022-11-28",
            "User-Agent": "libdxfrw-qualification-verifier-v1",
        },
    )
    try:
        with urllib.request.urlopen(request, timeout=30) as response:
            return response.read()
    except OSError as exc:
        raise VerificationError("authenticated GitHub fetch failed for %s: %s" % (url, exc)) from exc


def live_check_reference(ref: dict[str, Any], root: Path, token: str) -> None:
    for url_key, path_key, digest_key in (
        ("runApiUrl", "runApiPath", "runApiSha256"),
        ("jobsApiUrl", "jobsApiPath", "jobsApiSha256"),
        ("artifactsApiUrl", "artifactsApiPath", "artifactsApiSha256"),
        ("workflowApiUrl", "workflowApiPath", "workflowApiSha256"),
        ("artifactDownloadUrl", "artifactArchivePath", "downloadedArchiveSha256"),
    ):
        observed = fetch_authenticated(ref[url_key], token)
        saved = (root / safe_relative(ref[path_key], path_key)).read_bytes()
        if observed != saved or sha256_bytes(observed) != ref[digest_key]:
            raise VerificationError("live authenticated %s differs from saved bound evidence" % url_key)


def read_bound_json(root: Path, ref: dict[str, Any], path_key: str, digest_key: str) -> dict[str, Any]:
    path = root / safe_relative(ref.get(path_key), path_key)
    if sha256_file(path) != ref.get(digest_key):
        raise VerificationError("%s digest differs" % path_key)
    return read_json(path)


def artifact_binding(artifact: dict[str, Any]) -> dict[str, Any]:
    workflow_run = artifact.get("workflow_run")
    if not isinstance(workflow_run, dict):
        raise VerificationError("artifact lacks workflow_run")
    binding = {
        "id": artifact.get("id"),
        "name": artifact.get("name"),
        "expired": artifact.get("expired"),
        "size_in_bytes": artifact.get("size_in_bytes"),
        "digest": artifact.get("digest"),
        "archive_download_url": artifact.get("archive_download_url"),
        "workflow_run": {"id": workflow_run.get("id"), "head_sha": workflow_run.get("head_sha")},
    }
    if binding["expired"] is not False or not isinstance(binding["size_in_bytes"], int) or binding["size_in_bytes"] < 1:
        raise VerificationError("artifact is expired or has an invalid size")
    return binding


def extract_artifact(archive: Path) -> dict[str, bytes]:
    try:
        with zipfile.ZipFile(archive, "r") as bundle:
            infos = bundle.infolist()
            names = [info.filename for info in infos]
            if len(names) != len(set(names)):
                raise VerificationError("artifact ZIP contains duplicate entry names")
            if set(names) != ARTIFACT_ENTRIES:
                raise VerificationError(
                    "artifact entries differ (missing=%s unexpected=%s)"
                    % (sorted(ARTIFACT_ENTRIES - set(names)), sorted(set(names) - ARTIFACT_ENTRIES))
                )
            result = {}
            total = 0
            for info in infos:
                path = PurePosixPath(info.filename)
                if path.is_absolute() or len(path.parts) != 1 or any(part in {"", ".", ".."} for part in path.parts):
                    raise VerificationError("artifact ZIP contains an unsafe path")
                mode = (info.external_attr >> 16) & 0xFFFF
                if info.is_dir() or (mode and not stat.S_ISREG(mode)):
                    raise VerificationError("artifact ZIP contains a non-regular entry")
                total += info.file_size
                if total > 128 * 1024 * 1024:
                    raise VerificationError("artifact ZIP expands beyond the metadata bound")
                result[info.filename] = bundle.read(info)
            return result
    except (OSError, zipfile.BadZipFile, KeyError) as exc:
        raise VerificationError("cannot read qualification artifact %s: %s" % (archive, exc)) from exc


def _json_bytes(value: bytes, label: str) -> dict[str, Any]:
    try:
        parsed = native_runner.loads_json(value.decode("utf-8"), label)
    except (UnicodeDecodeError, native_runner.QualificationError) as exc:
        raise VerificationError("%s is not strict UTF-8 JSON: %s" % (label, exc)) from exc
    if not isinstance(parsed, dict):
        raise VerificationError("%s must contain an object" % label)
    return parsed


def _drawing_signature(value: bytes) -> bool:
    if len(value) >= 6 and value[:2] == b"AC" and value[2:6].isdigit():
        return True
    normalized = value[:512].replace(b"\r\n", b"\n")
    return normalized.startswith(b"0\nSECTION") and b"$ACADVER" in normalized


def _validate_artifact_contents(
    files: dict[str, bytes], receipt: dict[str, Any], contract: dict[str, Any],
    ref: dict[str, Any],
) -> None:
    if any(_drawing_signature(value) for value in files.values()):
        raise VerificationError("artifact contains DWG/DXF bytes under a metadata filename")
    receipt_bytes = files["native-qualification-receipt-v1.json"]
    if receipt_tool.canonical_bytes(receipt) != receipt_bytes:
        raise VerificationError("receipt is not canonical JSON")
    if sha256_bytes(receipt_bytes) != ref["receiptContentSha256"]:
        raise VerificationError("receipt content SHA differs")
    detached = files["native-qualification-receipt-v1.sha256"]
    expected_detached = (ref["receiptContentSha256"] + "  native-qualification-receipt-v1.json\n").encode("ascii")
    if detached != expected_detached:
        raise VerificationError("detached receipt SHA file differs")

    observation_bytes = files["native-qualification-observation-v1.json"]
    observation = _json_bytes(observation_bytes, "native observation")
    if native_runner.canonical_bytes(observation) != observation_bytes:
        raise VerificationError("native observation is not canonical")
    if sha256_bytes(observation_bytes) != receipt["observationSha256"]:
        raise VerificationError("receipt does not bind the artifact observation")
    try:
        receipt_tool.validate_observation(observation, contract)
    except receipt_tool.ReceiptError as exc:
        raise VerificationError("artifact observation: %s" % exc) from exc
    projected = {
        key: copy.deepcopy(value) for key, value in receipt.items()
        if key not in {"observationSha256", "qualificationDigests", "suppliesIndependentSemanticEvidence"}
    }
    projected["kind"] = native_runner.OBSERVATION_KIND
    if observation != projected:
        raise VerificationError("receipt does not preserve the exact observation")

    status = _json_bytes(files["qualification-status.json"], "qualification status")
    expected_status = {
        "schema": 1,
        "kind": native_runner.STATUS_KIND,
        "suite": receipt["suite"],
        "platformKey": receipt["platform"]["key"],
        "conclusion": receipt["qualificationConclusion"],
        "error": None if receipt["qualificationConclusion"] == "success" else status.get("error"),
    }
    if status != expected_status or (status["conclusion"] == "failure" and not status["error"]):
        raise VerificationError("qualification status differs from the receipt")

    inventory = _json_bytes(files["ctest-inventory.json"], "CTest inventory")
    tests = inventory.get("tests")
    if not isinstance(tests, list):
        raise VerificationError("CTest inventory has no tests array")
    names = []
    for row in tests:
        if not isinstance(row, dict) or not isinstance(row.get("name"), str):
            raise VerificationError("CTest inventory contains a malformed test")
        names.append(row["name"])
    names = native_runner.sorted_test_names(names)
    if names != receipt["inventory"]["names"]:
        raise VerificationError("artifact CTest inventory differs from the receipt")

    with tempfile.TemporaryDirectory(prefix="libdxfrw-receipt-junit-") as directory:
        junit = Path(directory) / "ctest.xml"
        junit.write_bytes(files["ctest.xml"])
        try:
            rows, summary = native_runner.parse_junit(
                junit, [row["testId"] for row in receipt["tests"]],
            )
        except native_runner.QualificationError as exc:
            raise VerificationError("artifact JUnit: %s" % exc) from exc
    statuses = {row["testId"]: row["status"] for row in rows}
    if summary != receipt["summary"] or any(statuses[row["testId"]] != row["status"] for row in receipt["tests"]):
        raise VerificationError("artifact JUnit outcomes differ from the receipt")
    try:
        log_text = files["ctest.log"].decode("utf-8")
    except UnicodeDecodeError as exc:
        raise VerificationError("CTest log is not UTF-8") from exc
    if "\x00" in log_text or any(name.lower().endswith((".dwg", ".dxf")) for name in files):
        raise VerificationError("artifact contains binary/drawing payload evidence")


def _one(rows: list[dict[str, Any]], label: str) -> dict[str, Any]:
    if len(rows) != 1:
        raise VerificationError("expected exactly one %s, found %d" % (label, len(rows)))
    return rows[0]


def validate_reference(
    ref: dict[str, Any], *, root: Path, status: dict[str, Any],
    contract: dict[str, Any], contract_path: Path, schema_path: Path,
    implementation_inputs_path: Path, live_token: str | None,
) -> dict[str, Any]:
    exact_keys(ref, REF_KEYS, "native receipt reference")
    reference_id = nonempty(ref.get("id"), "receipt reference ID")
    if not reference_id.startswith("receipt:"):
        raise VerificationError("receipt reference ID has the wrong namespace")
    if ref.get("suite") not in {"focused", "final-broad"} or ref.get("matrixKey") not in MATRIX_KEYS:
        raise VerificationError("receipt reference suite/matrix differs")
    repository = nonempty(ref.get("repository"), "repository")
    if repository.count("/") != 1 or ref.get("workflowPath") != ".github/workflows/build.yml":
        raise VerificationError("receipt reference repository/workflow differs")
    hex_digest(ref.get("headSha"), {40}, "headSha")
    for key in ("runId", "runAttempt", "apiJobId", "artifactId"):
        positive(ref.get(key), key)
    for key in ("logicalJobKey", "apiJobName", "artifactName", "requiredStepName"):
        nonempty(ref.get(key), key)
    for key in (
        "receiptContentSha256", "runApiSha256", "jobsApiSha256",
        "artifactsApiSha256", "workflowApiSha256", "downloadedArchiveSha256",
        "artifactApiDigestSha256",
    ):
        hex_digest(ref.get(key), {64}, key)
    validate_urls(ref)
    if ref.get("qualificationConclusion") not in {"success", "failure"}:
        raise VerificationError("qualificationConclusion differs")
    if ref.get("apiConclusion") not in API_CONCLUSIONS:
        raise VerificationError("apiConclusion is not a GitHub conclusion")
    if not isinstance(ref.get("accepted"), bool):
        raise VerificationError("accepted must be boolean")
    if ref["accepted"]:
        if ref["qualificationConclusion"] != "success" or ref["apiConclusion"] != "success" or ref.get("failureClass") is not None or ref.get("supersededBy") is not None:
            raise VerificationError("accepted receipt has failure/supersession state")
    else:
        failure_class = ref.get("failureClass")
        if failure_class not in {"semantic", "retryable-infrastructure", "cancelled"}:
            raise VerificationError("unaccepted receipt has no exact failure class")
        if failure_class == "semantic" and ref["qualificationConclusion"] != "failure":
            raise VerificationError("semantic failure did not fail qualification")
        if failure_class in {"retryable-infrastructure", "cancelled"} and ref["apiConclusion"] not in {"cancelled", "timed_out", "startup_failure", "stale"}:
            raise VerificationError("retryable failure does not map to an actual API conclusion")
        superseded = ref.get("supersededBy")
        if superseded is not None and (not isinstance(superseded, str) or not superseded.startswith("receipt:")):
            raise VerificationError("supersededBy is invalid")

    if live_token is not None:
        live_check_reference(ref, root, live_token)
    run_api = read_bound_json(root, ref, "runApiPath", "runApiSha256")
    if run_api.get("id") != ref["runId"] or run_api.get("run_attempt") != ref["runAttempt"] or run_api.get("head_sha") != ref["headSha"] or run_api.get("event") not in {"push", "pull_request", "workflow_dispatch"} or run_api.get("conclusion") != ref["apiConclusion"]:
        raise VerificationError("workflow-run API identity differs")
    api_repo = run_api.get("repository")
    if not isinstance(api_repo, dict) or api_repo.get("full_name") != repository:
        raise VerificationError("workflow-run API repository differs")
    run_path = run_api.get("path")
    if not isinstance(run_path, str):
        raise VerificationError("workflow-run API path is missing")
    bare_path, separator, run_ref = run_path.partition("@")
    if bare_path != ref["workflowPath"]:
        raise VerificationError("workflow-run API path differs")

    workflow_api = read_bound_json(root, ref, "workflowApiPath", "workflowApiSha256")
    if workflow_api.get("path") != ref["workflowPath"] or workflow_api.get("type") != "file" or workflow_api.get("encoding") != "base64":
        raise VerificationError("workflow contents API path/type/encoding differs")
    workflow_blob = hex_digest(workflow_api.get("sha"), {40, 64}, "workflow API blob")
    try:
        workflow_bytes = base64.b64decode(workflow_api.get("content", ""), validate=False)
    except (ValueError, TypeError) as exc:
        raise VerificationError("workflow contents API base64 is invalid") from exc
    if git_blob_id(workflow_bytes, len(workflow_blob)) != workflow_blob:
        raise VerificationError("workflow contents do not match the API blob")

    jobs_api = read_bound_json(root, ref, "jobsApiPath", "jobsApiSha256")
    jobs = jobs_api.get("jobs")
    if not isinstance(jobs, list) or jobs_api.get("total_count") != len(jobs):
        raise VerificationError("jobs API response is incomplete or paginated")
    job = _one([row for row in jobs if isinstance(row, dict) and row.get("id") == ref["apiJobId"]], "matching job")
    if job.get("run_id") != ref["runId"] or job.get("name") != ref["apiJobName"] or job.get("conclusion") != ref["apiConclusion"]:
        raise VerificationError("job API identity/conclusion differs")
    labels = job.get("labels")
    if not isinstance(labels, list):
        raise VerificationError("job API labels are missing")
    steps = job.get("steps")
    if not isinstance(steps, list):
        raise VerificationError("job API steps are missing")
    step = _one([row for row in steps if isinstance(row, dict) and row.get("name") == ref["requiredStepName"]], "qualification step")
    if ref["accepted"] and step.get("conclusion") != "success":
        raise VerificationError("accepted qualification step did not succeed")

    artifacts_api = read_bound_json(root, ref, "artifactsApiPath", "artifactsApiSha256")
    artifacts = artifacts_api.get("artifacts")
    if not isinstance(artifacts, list) or artifacts_api.get("total_count") != len(artifacts):
        raise VerificationError("artifact API response is incomplete or paginated")
    artifact = _one([row for row in artifacts if isinstance(row, dict) and row.get("id") == ref["artifactId"] and row.get("name") == ref["artifactName"]], "matching artifact")
    binding = artifact_binding(artifact)
    if binding["workflow_run"] != {"id": ref["runId"], "head_sha": ref["headSha"]} or binding["archive_download_url"] != ref["artifactDownloadUrl"]:
        raise VerificationError("artifact API run/download binding differs")
    if canonical_digest(binding) != ref["artifactApiDigestSha256"]:
        raise VerificationError("artifact API binding digest differs")

    archive = root / safe_relative(ref.get("artifactArchivePath"), "artifactArchivePath")
    archive_digest = sha256_file(archive)
    if archive_digest != ref["downloadedArchiveSha256"]:
        raise VerificationError("downloaded archive digest differs")
    if binding["digest"] != "sha256:" + archive_digest:
        raise VerificationError("artifact API digest does not bind the downloaded ZIP")
    if binding["size_in_bytes"] != archive.stat().st_size:
        raise VerificationError("artifact API size does not match the downloaded ZIP")
    files = extract_artifact(archive)
    receipt = _json_bytes(files["native-qualification-receipt-v1.json"], "native receipt")
    try:
        receipt_tool.validate_receipt(receipt, contract)
    except receipt_tool.ReceiptError as exc:
        raise VerificationError("receipt: %s" % exc) from exc
    _validate_artifact_contents(files, receipt, contract, ref)

    if receipt["repository"]["githubRepository"] != repository or receipt["repository"]["checkedOutCommit"] != ref["headSha"] or receipt["run"]["id"] != str(ref["runId"]) or receipt["run"]["attempt"] != str(ref["runAttempt"]) or receipt["run"]["eventName"] != run_api["event"] or receipt["run"]["job"] != ref["logicalJobKey"]:
        raise VerificationError("receipt and API/overlay run identity differ")
    if separator and run_ref not in {receipt["run"]["ref"], receipt["run"]["workflowRef"].rsplit("@", 1)[-1]}:
        raise VerificationError("workflow-run API @ref differs from the receipt")
    if receipt["workflow"]["gitBlobAtHead"] != workflow_blob or receipt["workflow"]["sha256"] != sha256_bytes(workflow_bytes):
        raise VerificationError("receipt workflow bytes/blob differ from contents API")
    if receipt["platform"]["key"] != ref["matrixKey"] or receipt["platform"]["declaredRunnerLabel"] not in labels or receipt["suite"] != ref["suite"] or receipt["qualificationConclusion"] != ref["qualificationConclusion"]:
        raise VerificationError("receipt platform/suite/conclusion differs")
    expected_digests = {
        "implementation": status.get("implementationDigestSha256"),
        "claims": status.get("claimsDigestSha256"),
    }
    observed_digests = receipt["qualificationDigests"]
    if (
        receipt["contracts"]["requiredTestsSha256"] != sha256_file(contract_path)
        or observed_digests["implementation"]["manifestSha256"] != sha256_file(implementation_inputs_path)
        or observed_digests["implementation"]["value"] != expected_digests["implementation"]
        or observed_digests["immutableClaims"]["sha256"] != expected_digests["claims"]
        or observed_digests["receiptSchema"]["sha256"] != sha256_file(schema_path)
    ):
        raise VerificationError("receipt binds stale implementation/claims/schema digests")
    return receipt


def validate_receipt_set(
    status: dict[str, Any], *, root: Path, claims_path: Path,
    contract_path: Path, schema_path: Path, implementation_inputs_path: Path,
    require_complete: bool, live_token: str | None,
) -> dict[str, Any]:
    if status.get("schema") != 1 or status.get("kind") != "libdxfrw-qualified-format-status":
        raise VerificationError("unexpected status overlay")
    refs = status.get("nativeReceiptRefs")
    broad_ids = status.get("broadMatrixReceiptRefs")
    if not isinstance(refs, list) or not isinstance(broad_ids, list):
        raise VerificationError("status receipt references must be lists")
    if not refs:
        if require_complete or broad_ids:
            raise VerificationError("native receipt evidence is incomplete")
        return {
            "acceptedFocused": 0, "acceptedBroad": 0, "attempts": 0,
            "acceptedFocusedIds": [], "acceptedBroadIds": [],
        }
    if status.get("freezeState") != "FROZEN" or sha256_file(claims_path) != status.get("claimsDigestSha256"):
        raise VerificationError("receipt set binds an unfrozen/stale claims status")
    hex_digest(status.get("implementationDigestSha256"), {64}, "implementation digest")
    try:
        contract = native_runner.validate_contract(read_json(contract_path), require_final=True)
    except native_runner.QualificationError as exc:
        raise VerificationError("required-test contract: %s" % exc) from exc
    verified: dict[str, tuple[dict[str, Any], dict[str, Any]]] = {}
    for raw in refs:
        if not isinstance(raw, dict):
            raise VerificationError("native receipt reference is not an object")
        receipt = validate_reference(
            raw, root=root, status=status, contract=contract,
            contract_path=contract_path, schema_path=schema_path,
            implementation_inputs_path=implementation_inputs_path,
            live_token=live_token,
        )
        if raw["id"] in verified:
            raise VerificationError("duplicate receipt reference ID")
        verified[raw["id"]] = (raw, receipt)
    if len(broad_ids) != len(set(broad_ids)) or any(item not in verified for item in broad_ids):
        raise VerificationError("broad matrix receipt references are invalid")
    accepted_focused = [ref for ref, _ in verified.values() if ref["accepted"] and ref["suite"] == "focused"]
    accepted_broad = [ref for ref, _ in verified.values() if ref["accepted"] and ref["suite"] == "final-broad"]
    if set(broad_ids) != {ref["id"] for ref in accepted_broad}:
        raise VerificationError("broad matrix references are not the exact accepted broad receipts")
    if require_complete:
        if len(accepted_focused) != 3 or {ref["matrixKey"] for ref in accepted_focused} != MATRIX_KEYS:
            raise VerificationError("focused receipts are not exactly one per native platform")
        if len(accepted_broad) != 3 or {ref["matrixKey"] for ref in accepted_broad} != MATRIX_KEYS:
            raise VerificationError("final-broad receipts are not exactly one per native platform")
        broad_run = {(ref["runId"], ref["runAttempt"], ref["headSha"]) for ref in accepted_broad}
        if len(broad_run) != 1 or any(receipt["run"]["eventName"] != "workflow_dispatch" for ref, receipt in verified.values() if ref in accepted_broad):
            raise VerificationError("accepted broad receipts are not one manual three-OS matrix")
        focused_ids = {ref["id"] for ref in accepted_focused}
        for row in status.get("claimStatus", []):
            if isinstance(row, dict) and row.get("status") == "PROMOTED" and set(row.get("receiptRefs", [])) != focused_ids:
                raise VerificationError("promoted claim lacks the exact three focused receipts")
    return {
        "acceptedFocused": len(accepted_focused),
        "acceptedBroad": len(accepted_broad),
        "attempts": len(refs),
        "acceptedFocusedIds": sorted(ref["id"] for ref in accepted_focused),
        "acceptedBroadIds": sorted(ref["id"] for ref in accepted_broad),
    }


def write_json(path: Path, value: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(native_runner.canonical_bytes(value))


def _fixture(root: Path) -> tuple[dict[str, Any], dict[str, Any], Path, Path, Path, Path]:
    contract = receipt_tool._fixture_contract()
    contract_path = root / "metadata/qualification-required-tests-v1.json"
    write_json(contract_path, contract)
    inputs_path = root / "metadata/qualification-implementation-inputs-v1.json"
    inputs_path.parent.mkdir(parents=True, exist_ok=True)
    inputs_path.write_bytes(b"fixture implementation inputs\n")
    observation = receipt_tool._fixture_observation(contract)
    observation["contracts"]["requiredTestsSha256"] = sha256_file(contract_path)
    observation_bytes = native_runner.canonical_bytes(observation)
    receipt = receipt_tool.build_receipt(
        observation, contract,
        observation_sha256=sha256_bytes(observation_bytes),
        implementation_manifest_sha256=sha256_file(inputs_path),
        implementation_sha256="8" * 64,
        claims_sha256="9" * 64,
        schema_sha256=sha256_file(DEFAULT_SCHEMA),
    )
    receipt_bytes = receipt_tool.canonical_bytes(receipt)
    files = {
        "qualification-status.json": native_runner.canonical_bytes({"schema": 1, "kind": native_runner.STATUS_KIND, "suite": "focused", "platformKey": "linux-gcc", "conclusion": "success", "error": None}),
        "ctest-inventory.json": native_runner.canonical_bytes({"kind": "ctestInfo", "tests": [{"name": name} for name in contract["broadInventory"]["names"]]}),
        "ctest.log": b"100% tests passed\n",
        "ctest.xml": ("<testsuite>" + "".join('<testcase name="%s" status="run"/>' % row["testId"] for row in receipt["tests"]) + "</testsuite>").encode("utf-8"),
        "native-qualification-observation-v1.json": observation_bytes,
        "native-qualification-receipt-v1.json": receipt_bytes,
        "native-qualification-receipt-v1.sha256": (sha256_bytes(receipt_bytes) + "  native-qualification-receipt-v1.json\n").encode("ascii"),
    }
    evidence = root / "metadata/qualification-receipts/focused-linux"
    archive = evidence / "artifact.zip"
    archive.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(archive, "w", compression=zipfile.ZIP_DEFLATED) as bundle:
        for name in sorted(files):
            info = zipfile.ZipInfo(name)
            info.external_attr = (stat.S_IFREG | 0o644) << 16
            bundle.writestr(info, files[name])
    archive_sha = sha256_file(archive)
    workflow_bytes = b"name: test\n"
    workflow_blob = git_blob_id(workflow_bytes, 40)
    receipt["workflow"]["gitBlobAtHead"] = workflow_blob
    receipt["workflow"]["workingTreeGitBlob"] = workflow_blob
    receipt["workflow"]["sha256"] = sha256_bytes(workflow_bytes)
    observation["workflow"] = copy.deepcopy(receipt["workflow"])
    observation_bytes = native_runner.canonical_bytes(observation)
    receipt["observationSha256"] = sha256_bytes(observation_bytes)
    receipt_bytes = receipt_tool.canonical_bytes(receipt)
    files["native-qualification-observation-v1.json"] = observation_bytes
    files["native-qualification-receipt-v1.json"] = receipt_bytes
    files["native-qualification-receipt-v1.sha256"] = (sha256_bytes(receipt_bytes) + "  native-qualification-receipt-v1.json\n").encode("ascii")
    with zipfile.ZipFile(archive, "w", compression=zipfile.ZIP_DEFLATED) as bundle:
        for name in sorted(files):
            info = zipfile.ZipInfo(name)
            info.external_attr = (stat.S_IFREG | 0o644) << 16
            bundle.writestr(info, files[name])
    archive_sha = sha256_file(archive)
    artifact_name = "native-qualification-101-1-focused-linux-gcc"
    download_url = "https://api.github.com/repos/LibreCAD/libdxfrw/actions/artifacts/303/zip"
    artifact = {"id": 303, "name": artifact_name, "expired": False, "size_in_bytes": archive.stat().st_size, "digest": "sha256:" + archive_sha, "archive_download_url": download_url, "workflow_run": {"id": 101, "head_sha": "3" * 40}}
    run_api = {"id": 101, "run_attempt": 1, "head_sha": "3" * 40, "path": ".github/workflows/build.yml@refs/heads/master", "event": "workflow_dispatch", "conclusion": "success", "repository": {"full_name": "LibreCAD/libdxfrw"}}
    jobs_api = {"total_count": 1, "jobs": [{"id": 202, "run_id": 101, "name": "focused / linux-gcc", "conclusion": "success", "labels": ["ubuntu-24.04"], "steps": [{"name": "Run focused native qualification", "conclusion": "success"}]}]}
    artifacts_api = {"total_count": 1, "artifacts": [artifact]}
    workflow_api = {"path": ".github/workflows/build.yml", "sha": workflow_blob, "type": "file", "encoding": "base64", "content": base64.b64encode(workflow_bytes).decode("ascii")}
    for name, value in (("run.json", run_api), ("jobs.json", jobs_api), ("artifacts.json", artifacts_api), ("workflow.json", workflow_api)):
        write_json(evidence / name, value)
    relative = lambda path: path.relative_to(root).as_posix()
    ref = {
        "id": "receipt:focused:linux-gcc:101:1:202", "suite": "focused", "matrixKey": "linux-gcc", "repository": "LibreCAD/libdxfrw", "workflowPath": ".github/workflows/build.yml", "headSha": "3" * 40,
        "runId": 101, "runAttempt": 1, "logicalJobKey": "native-focused", "apiJobId": 202, "apiJobName": "focused / linux-gcc", "artifactId": 303, "artifactName": artifact_name, "receiptContentSha256": sha256_bytes(receipt_bytes),
        "runApiUrl": "", "runApiPath": relative(evidence / "run.json"), "runApiSha256": sha256_file(evidence / "run.json"), "jobsApiUrl": "", "jobsApiPath": relative(evidence / "jobs.json"), "jobsApiSha256": sha256_file(evidence / "jobs.json"),
        "artifactsApiUrl": "", "artifactsApiPath": relative(evidence / "artifacts.json"), "artifactsApiSha256": sha256_file(evidence / "artifacts.json"), "workflowApiUrl": "", "workflowApiPath": relative(evidence / "workflow.json"), "workflowApiSha256": sha256_file(evidence / "workflow.json"),
        "artifactDownloadUrl": download_url, "artifactArchivePath": relative(archive), "downloadedArchiveSha256": archive_sha, "artifactApiDigestSha256": canonical_digest(artifact_binding(artifact)),
        "requiredStepName": "Run focused native qualification", "qualificationConclusion": "success", "apiConclusion": "success", "accepted": True, "failureClass": None, "supersededBy": None,
    }
    ref.update(endpoint_urls(ref["repository"], ref))
    claims = root / "claims.json"
    claims.write_bytes(b"claims\n")
    status = {"schema": 1, "kind": "libdxfrw-qualified-format-status", "freezeState": "FROZEN", "implementationDigestSha256": "8" * 64, "claimsDigestSha256": sha256_file(claims), "claimStatus": [], "nativeReceiptRefs": [ref], "broadMatrixReceiptRefs": []}
    receipt["qualificationDigests"]["immutableClaims"]["sha256"] = status["claimsDigestSha256"]
    receipt_bytes = receipt_tool.canonical_bytes(receipt)
    files["native-qualification-receipt-v1.json"] = receipt_bytes
    files["native-qualification-receipt-v1.sha256"] = (sha256_bytes(receipt_bytes) + "  native-qualification-receipt-v1.json\n").encode("ascii")
    ref["receiptContentSha256"] = sha256_bytes(receipt_bytes)
    with zipfile.ZipFile(archive, "w", compression=zipfile.ZIP_DEFLATED) as bundle:
        for name in sorted(files):
            info = zipfile.ZipInfo(name)
            info.external_attr = (stat.S_IFREG | 0o644) << 16
            bundle.writestr(info, files[name])
    archive_sha = sha256_file(archive)
    ref["downloadedArchiveSha256"] = archive_sha
    artifact["size_in_bytes"] = archive.stat().st_size
    artifact["digest"] = "sha256:" + archive_sha
    write_json(evidence / "artifacts.json", artifacts_api)
    ref["artifactsApiSha256"] = sha256_file(evidence / "artifacts.json")
    ref["artifactApiDigestSha256"] = canonical_digest(artifact_binding(artifact))
    return status, ref, claims, contract_path, DEFAULT_SCHEMA, inputs_path


def expect_error(callable_value: Any, label: str) -> None:
    try:
        callable_value()
    except VerificationError:
        return
    raise AssertionError("negative verifier vector was accepted: %s" % label)


def _rewrite_zip(source: Path, mutation: str) -> Path:
    files = extract_artifact(source)
    target = source.with_name("mutated-" + mutation + ".zip")
    names = sorted(files)
    if mutation == "extra":
        files["extra.json"] = b"{}\n"
        names = sorted(files)
    elif mutation == "missing":
        del files["ctest.log"]
        names = sorted(files)
    elif mutation == "traversal":
        del files["ctest.log"]
        files["../ctest.log"] = b"bad"
        names = sorted(files)
    with zipfile.ZipFile(target, "w") as bundle:
        for name in names:
            info = zipfile.ZipInfo(name)
            info.external_attr = (stat.S_IFREG | 0o644) << 16
            bundle.writestr(info, files[name])
        if mutation == "duplicate":
            info = zipfile.ZipInfo("ctest.log")
            info.external_attr = (stat.S_IFREG | 0o644) << 16
            with warnings.catch_warnings():
                warnings.simplefilter("ignore", UserWarning)
                bundle.writestr(info, b"duplicate")
    return target


def self_test() -> None:
    with tempfile.TemporaryDirectory(prefix="libdxfrw-api-receipt-") as directory:
        root = Path(directory)
        status, ref, claims, contract_path, schema_path, inputs_path = _fixture(root)
        contract = native_runner.validate_contract(read_json(contract_path), require_final=True)
        validate_reference(
            ref, root=root, status=status, contract=contract,
            contract_path=contract_path, schema_path=schema_path,
            implementation_inputs_path=inputs_path, live_token=None,
        )
        pre = copy.deepcopy(status)
        pre["nativeReceiptRefs"] = []
        validate_receipt_set(
            pre, root=root, claims_path=claims, contract_path=contract_path,
            schema_path=schema_path, implementation_inputs_path=inputs_path,
            require_complete=False, live_token=None,
        )
        archive = root / ref["artifactArchivePath"]
        assert set(extract_artifact(archive)) == ARTIFACT_ENTRIES
        for mutation in ("extra", "missing", "traversal", "duplicate"):
            changed = _rewrite_zip(archive, mutation)
            expect_error(lambda p=changed: extract_artifact(p), "artifact " + mutation)

        def invalid_ref(field: str, value: Any, label: str) -> None:
            changed = copy.deepcopy(ref)
            changed[field] = value
            expect_error(
                lambda: validate_reference(
                    changed, root=root, status=status, contract=contract,
                    contract_path=contract_path, schema_path=schema_path,
                    implementation_inputs_path=inputs_path, live_token=None,
                ),
                label,
            )

        invalid_ref("runAttempt", 2, "wrong run attempt")
        invalid_ref("receiptContentSha256", "0" * 64, "wrong receipt digest")
        invalid_ref("artifactId", 304, "wrong artifact ID/endpoint")
        invalid_ref("artifactApiDigestSha256", "0" * 64, "wrong artifact API digest")
        invalid_ref("downloadedArchiveSha256", "0" * 64, "wrong archive digest")
        invalid_ref("runApiUrl", "https://api.github.com/forged", "unbound API endpoint")
        changed = copy.deepcopy(ref)
        changed["apiConclusion"] = "infrastructure_failure"
        expect_error(
            lambda: validate_reference(
                changed, root=root, status=status, contract=contract,
                contract_path=contract_path, schema_path=schema_path,
                implementation_inputs_path=inputs_path, live_token=None,
            ),
            "synthetic API conclusion",
        )

        artifact_path = root / ref["artifactsApiPath"]
        artifact_doc = read_json(artifact_path)
        artifact_doc["artifacts"][0]["digest"] = "sha256:" + "f" * 64
        write_json(artifact_path, artifact_doc)
        changed = copy.deepcopy(ref)
        changed["artifactsApiSha256"] = sha256_file(artifact_path)
        changed["artifactApiDigestSha256"] = canonical_digest(artifact_binding(artifact_doc["artifacts"][0]))
        expect_error(
            lambda: validate_reference(
                changed, root=root, status=status, contract=contract,
                contract_path=contract_path, schema_path=schema_path,
                implementation_inputs_path=inputs_path, live_token=None,
            ),
            "API digest not linked to ZIP",
        )
    print("native qualification API/artifact verifier self-test: PASS")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--status", type=Path, default=DEFAULT_STATUS)
    parser.add_argument("--claims", type=Path, default=DEFAULT_CLAIMS)
    parser.add_argument("--required-tests", type=Path, default=DEFAULT_TESTS)
    parser.add_argument("--schema", type=Path, default=DEFAULT_SCHEMA)
    parser.add_argument("--implementation-inputs", type=Path, default=DEFAULT_INPUTS)
    parser.add_argument("--root", type=Path, default=ROOT)
    parser.add_argument("--pre-native", action="store_true")
    parser.add_argument("--live-token-env", help="authenticate and byte-compare every saved API response/download")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args(argv)
    try:
        if args.self_test:
            self_test()
            return 0
        token = None
        if args.live_token_env:
            token = os.environ.get(args.live_token_env)
            if not token:
                raise VerificationError("live token environment variable is empty")
        if not args.pre_native and token is None:
            raise VerificationError(
                "complete J364 verification requires --live-token-env for authenticated API/download replay"
            )
        result = validate_receipt_set(
            read_json(args.status), root=args.root.resolve(), claims_path=args.claims,
            contract_path=args.required_tests, schema_path=args.schema,
            implementation_inputs_path=args.implementation_inputs,
            require_complete=not args.pre_native, live_token=token,
        )
        print(
            "native qualification receipts: PASS (%d focused; %d broad; %d attempts)"
            % (result["acceptedFocused"], result["acceptedBroad"], result["attempts"])
        )
        return 0
    except (
        OSError, UnicodeError, json.JSONDecodeError, VerificationError,
        receipt_tool.ReceiptError, native_runner.QualificationError, AssertionError,
    ) as exc:
        print("native qualification receipts: FAIL: %s" % exc, file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
