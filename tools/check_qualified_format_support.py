#!/usr/bin/env python3
"""Fail-closed join for field-scoped qualified-format support claims.

The source support matrix remains an inventory.  Only this immutable claims
document joined with its deliberately small status overlay can advertise a
qualified route.  Local S387 validation accepts no promoted status; J364 must
add API-verified native receipts, a complete broad matrix, and authenticated
live API/artifact replay before ``--allow-promoted`` can advertise anything.
"""

from __future__ import annotations

import argparse
import copy
import hashlib
import json
import math
import os
import subprocess
import sys
import tempfile
import unicodedata
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_CLAIMS = ROOT / "metadata/qualified-format-claims-v1.json"
DEFAULT_STATUS = ROOT / "metadata/qualified-format-status-v1.json"
DEFAULT_ROUTES = ROOT / "metadata/parity-source-routes-v1.json"
DEFAULT_MATRIX = ROOT / "metadata/support-matrix-v1.json"
DEFAULT_INPUTS = ROOT / "metadata/qualification-implementation-inputs-v1.json"
DEFAULT_TESTS = ROOT / "metadata/qualification-required-tests-v1.json"
DEFAULT_RECEIPT_SCHEMA = ROOT / "metadata/native-qualification-receipt-schema-v1.json"

HEX64 = set("0123456789abcdef")
HEX40 = set("0123456789abcdef")
TUPLE_FIELDS = (
    "facade", "format", "direction", "ACVersion", "recordKind",
    "recordClass", "fieldPath", "contractKind",
)
CONTRACT_KINDS = {
    "value-read", "value-write", "callback-order", "ownership-edge",
    "opaque-preservation", "first-failure",
}
PRE_NATIVE_STATUSES = {"EXPERIMENTAL", "PENDING_NATIVE"}
ALL_STATUSES = PRE_NATIVE_STATUSES | {"PROMOTED"}
INDEPENDENT_AUTHORITIES = {"independent-semantic-reader", "independent-auditor"}
ADMISSIONS = {"locked-repository-blob", "local-from-scratch-runtime"}
IDENTITY_FIELDS = {"recordClass", "handle"}

CLAIMS_KEYS = {
    "schema", "kind", "freezeState", "fixturePolicy", "promotionAuthority",
    "claimIdCanonicalization", "evidence", "claims", "routeRequirements",
    "excludedCandidates", "pendingFinalization",
}
AUTHORITY_KEYS = {
    "soleAuthority", "sourceInventoryIsAuthority", "archivedReportsAreAuthority",
    "externalOnlyEvidenceIsAuthority", "selfReadIsIndependentEvidence",
    "allowedContractKinds", "allowedPreNativeStatuses",
}
CANONICALIZATION_KEYS = {
    "algorithm", "tupleFields", "normalization", "separator", "terminator",
    "forbiddenCodePoints",
}
EVIDENCE_KEYS = {
    "id", "authorityKind", "facade", "format", "direction",
    "artifactPath", "artifactSha256", "checkerPath", "checkerSha256",
    "recordedByCommit",
    "fixtureAdmission", "independentTool", "semanticOutput",
    "targetStandaloneDifferential", "fieldOutcome", "integrityDiagnostics",
    "silentLoss", "qualificationEligible", "qualificationBlockers",
    "candidateFieldIds",
}
CLAIM_KEYS = {
    "id", "tuple", "routeIds", "evidenceIds", "evidenceFieldId",
    "normalization", "tolerance",
}
REQUIREMENT_KEYS = {"routeId", "requiredClaimIds"}
EXCLUDED_KEYS = {"recordClass", "ACVersion", "reason", "sourceFieldIds"}
PENDING_KEYS = {
    "slice", "item", "artifactPath", "requiredBeforeFreeze", "reason",
}
STATUS_KEYS = {
    "schema", "kind", "freezeState", "implementationDigestSha256",
    "claimsDigestSha256", "allowedTransitions", "claimStatus",
    "nativeReceiptRefs", "broadMatrixReceiptRefs",
}
STATUS_ROW_KEYS = {"claimId", "status", "blockers", "receiptRefs"}


class SupportError(ValueError):
    """An immutable claim or mutable status violates the authority model."""


def _read_json(path: Path) -> dict[str, Any]:
    def reject_pairs(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
        result: dict[str, Any] = {}
        for key, value in pairs:
            if key in result:
                raise SupportError("duplicate JSON key %r in %s" % (key, path))
            result[key] = value
        return result

    def reject_constant(token: str) -> Any:
        raise SupportError("non-finite JSON token %s in %s" % (token, path))

    try:
        value = json.loads(
            path.read_text(encoding="utf-8"),
            object_pairs_hook=reject_pairs,
            parse_constant=reject_constant,
        )
    except (OSError, UnicodeError, json.JSONDecodeError) as exc:
        raise SupportError("cannot read %s: %s" % (path, exc)) from exc
    if not isinstance(value, dict):
        raise SupportError("%s must contain one JSON object" % path)
    return value


def _exact_keys(value: dict[str, Any], expected: set[str], label: str) -> None:
    actual = set(value)
    if actual != expected:
        raise SupportError(
            "%s keys differ (missing=%s unexpected=%s)"
            % (label, sorted(expected - actual), sorted(actual - expected))
        )


def _is_hex64(value: object) -> bool:
    return isinstance(value, str) and len(value) == 64 and set(value) <= HEX64


def _is_hex40(value: object) -> bool:
    return isinstance(value, str) and len(value) == 40 and set(value) <= HEX40


def _string_list(value: object, label: str, *, nonempty: bool = False) -> list[str]:
    if not isinstance(value, list) or (nonempty and not value):
        raise SupportError("%s must be %sa list" % (label, "a non-empty " if nonempty else ""))
    if any(not isinstance(item, str) or not item for item in value):
        raise SupportError("%s must contain non-empty strings" % label)
    if len(value) != len(set(value)):
        raise SupportError("%s contains duplicates" % label)
    return value


def canonical_claim_id(claim_tuple: dict[str, Any]) -> str:
    _exact_keys(claim_tuple, set(TUPLE_FIELDS), "claim tuple")
    encoded = []
    for field in TUPLE_FIELDS:
        value = claim_tuple[field]
        if not isinstance(value, str) or not value:
            raise SupportError("claim tuple %s must be a non-empty string" % field)
        if "\x00" in value or "\n" in value:
            raise SupportError("claim tuple %s contains NUL/LF" % field)
        normalized = unicodedata.normalize("NFC", value)
        if normalized != value:
            raise SupportError("claim tuple %s is not UTF-8 NFC" % field)
        encoded.append(value.encode("utf-8"))
    return hashlib.sha256(b"\0".join(encoded) + b"\n").hexdigest()


def claims_digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def _artifact_in_implementation_scope(path: str, inputs: dict[str, Any]) -> bool:
    selection = inputs.get("selection")
    if not isinstance(selection, dict):
        return False
    exact = selection.get("includeExact", [])
    prefixes = selection.get("includePrefixes", [])
    excluded = set(selection.get("excludeExact", []))
    excluded_prefixes = selection.get("excludePrefixes", [])
    return (
        path not in excluded
        and not any(path.startswith(prefix) for prefix in excluded_prefixes)
        and (path in exact or any(path.startswith(prefix) for prefix in prefixes))
    )


def _validate_committed_file(
    root: Path, commit: str, path: str, expected_sha256: str, label: str,
) -> None:
    try:
        value = subprocess.run(
            ["git", "show", "%s:%s" % (commit, path)],
            cwd=root,
            check=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        ).stdout
    except (OSError, subprocess.CalledProcessError) as exc:
        raise SupportError("%s is not available at recorded commit" % label) from exc
    if hashlib.sha256(value).hexdigest() != expected_sha256:
        raise SupportError("%s digest does not match recorded commit" % label)


def _validate_authority(document: dict[str, Any]) -> None:
    authority = document.get("promotionAuthority")
    if not isinstance(authority, dict):
        raise SupportError("promotionAuthority must be an object")
    _exact_keys(authority, AUTHORITY_KEYS, "promotionAuthority")
    required_false = (
        "sourceInventoryIsAuthority", "archivedReportsAreAuthority",
        "externalOnlyEvidenceIsAuthority", "selfReadIsIndependentEvidence",
    )
    if authority.get("soleAuthority") is not True:
        raise SupportError("claims join must be the sole promotion authority")
    if any(authority.get(key) is not False for key in required_false):
        raise SupportError("source/archive/external/self-read authority is forbidden")
    if set(_string_list(authority.get("allowedContractKinds"), "allowedContractKinds", nonempty=True)) != CONTRACT_KINDS:
        raise SupportError("contract-kind authority set drifted")
    if set(_string_list(authority.get("allowedPreNativeStatuses"), "allowedPreNativeStatuses", nonempty=True)) != PRE_NATIVE_STATUSES:
        raise SupportError("pre-native status authority set drifted")

    canonical = document.get("claimIdCanonicalization")
    if not isinstance(canonical, dict):
        raise SupportError("claimIdCanonicalization must be an object")
    _exact_keys(canonical, CANONICALIZATION_KEYS, "claimIdCanonicalization")
    expected = {
        "algorithm": "sha256-utf8-nfc-nul-tuple-lf-v1",
        "tupleFields": list(TUPLE_FIELDS),
        "normalization": "UTF-8 NFC",
        "separator": "NUL",
        "terminator": "LF",
        "forbiddenCodePoints": ["U+0000", "U+000A"],
    }
    if canonical != expected:
        raise SupportError("claim-ID canonicalization contract drifted")


def _validate_evidence(
    document: dict[str, Any], root: Path, inputs: dict[str, Any]
) -> tuple[
    dict[str, dict[str, Any]],
    dict[str, set[str]],
    dict[str, dict[str, dict[str, Any]]],
]:
    rows = document.get("evidence")
    if not isinstance(rows, list) or not rows:
        raise SupportError("evidence must be a non-empty list")
    by_id: dict[str, dict[str, Any]] = {}
    field_sets: dict[str, set[str]] = {}
    field_details: dict[str, dict[str, dict[str, Any]]] = {}
    for index, row in enumerate(rows):
        if not isinstance(row, dict):
            raise SupportError("evidence[%d] must be an object" % index)
        _exact_keys(row, EVIDENCE_KEYS, "evidence[%d]" % index)
        evidence_id = row.get("id")
        if not isinstance(evidence_id, str) or not evidence_id.startswith("evidence:"):
            raise SupportError("evidence[%d] has an invalid ID" % index)
        if evidence_id in by_id:
            raise SupportError("duplicate evidence ID %s" % evidence_id)
        if row.get("authorityKind") not in INDEPENDENT_AUTHORITIES:
            raise SupportError("%s is not independent semantic evidence" % evidence_id)
        if (
            row.get("facade") not in {"dxfRW", "dwgRW"}
            or row.get("format") not in {"DXF", "DWG"}
            or row.get("direction") not in {"read", "write"}
        ):
            raise SupportError("%s has an invalid facade/format/direction" % evidence_id)
        if row.get("fixtureAdmission") not in ADMISSIONS:
            raise SupportError("%s has external-only or unadmitted inputs" % evidence_id)
        if row.get("semanticOutput") not in {"full-JSON", "auditor-records"}:
            raise SupportError("%s lacks full semantic output" % evidence_id)
        if row.get("fieldOutcome") not in {"exact", "toleranceNormalized"}:
            raise SupportError("%s has a non-qualifying field outcome" % evidence_id)
        if row.get("silentLoss") != "none-observed-in-scoped-records":
            raise SupportError("%s has unresolved silent loss" % evidence_id)
        blockers = _string_list(row.get("qualificationBlockers"), evidence_id + " blockers")
        eligible = row.get("qualificationEligible")
        if not isinstance(eligible, bool):
            raise SupportError("%s eligibility must be boolean" % evidence_id)
        if eligible and (blockers or row.get("integrityDiagnostics") != "resolved-none"):
            raise SupportError("%s is eligible despite unresolved blockers" % evidence_id)
        if not eligible and not blockers:
            raise SupportError("%s is ineligible without a blocker" % evidence_id)
        recorded_commit = row.get("recordedByCommit")
        if not _is_hex40(recorded_commit):
            raise SupportError("%s has an invalid recorded commit" % evidence_id)
        bound_files = (
            (row.get("artifactPath"), row.get("artifactSha256"), "artifact"),
            (row.get("checkerPath"), row.get("checkerSha256"), "checker"),
        )
        for bound_path, bound_digest, role in bound_files:
            if (
                not isinstance(bound_path, str)
                or bound_path.startswith("/")
                or ".." in Path(bound_path).parts
            ):
                raise SupportError("%s has an unsafe %s path" % (evidence_id, role))
            if not _is_hex64(bound_digest):
                raise SupportError("%s has an invalid %s digest" % (evidence_id, role))
            if not _artifact_in_implementation_scope(bound_path, inputs):
                raise SupportError(
                    "%s %s is outside the implementation digest" % (evidence_id, role)
                )
            bound_file = root / bound_path
            if (
                not bound_file.is_file()
                or hashlib.sha256(bound_file.read_bytes()).hexdigest() != bound_digest
            ):
                raise SupportError("%s %s is missing or stale" % (evidence_id, role))
            _validate_committed_file(
                root, recorded_commit, bound_path, bound_digest,
                "%s %s" % (evidence_id, role),
            )
        artifact_path = row["artifactPath"]
        artifact = root / artifact_path
        fields = set(_string_list(row.get("candidateFieldIds"), evidence_id + " candidateFieldIds", nonempty=True))
        source = _read_json(artifact)
        source_by_id: dict[str, dict[str, Any]] = {}
        if source.get("kind") == "libdxfrw-admitted-independent-candidate-fields":
            source_claims = source.get("claims")
            source_fixtures = source.get("fixtures")
            if not isinstance(source_claims, list) or not isinstance(source_fixtures, list):
                raise SupportError("%s artifact has no field claims/fixtures" % evidence_id)
            fixture_versions = {}
            for fixture in source_fixtures:
                if not isinstance(fixture, dict) or not isinstance(fixture.get("id"), str) or not isinstance(fixture.get("version"), str):
                    raise SupportError("%s artifact has a malformed fixture" % evidence_id)
                fixture_versions[fixture["id"]] = fixture["version"]
            for source_claim in source_claims:
                if not isinstance(source_claim, dict) or not isinstance(source_claim.get("id"), str):
                    raise SupportError("%s artifact has a malformed claim" % evidence_id)
                source_id = source_claim["id"]
                if source_id in source_by_id:
                    raise SupportError("%s artifact has duplicate claim %s" % (evidence_id, source_id))
                fixture_id = source_claim.get("fixtureId")
                if fixture_id not in fixture_versions:
                    raise SupportError("%s artifact claim %s has no fixture version" % (evidence_id, source_id))
                source_claim = dict(source_claim)
                source_claim["_ACVersion"] = fixture_versions[fixture_id]
                source_by_id[source_id] = source_claim
        elif source.get("kind") == "libdxfrw-local-ac1024-independent-candidate-fields-successor":
            if source.get("schema") != 2 or source.get("status") != "EXPERIMENTAL":
                raise SupportError("%s has an unexpected successor contract" % evidence_id)
            boundary = source.get("promotionBoundary")
            if (
                not isinstance(boundary, dict)
                or boundary.get("promotesSupport") is not False
                or boundary.get("wholeFixtureClaimsForbidden") is not True
                or boundary.get("wholeVersionClaimsForbidden") is not True
            ):
                raise SupportError("%s successor promotion boundary drifted" % evidence_id)
            source_claims = source.get("claimOverrides")
            if not isinstance(source_claims, list) or len(source_claims) != 4:
                raise SupportError("%s artifact lacks the four v2 overrides" % evidence_id)
            for source_claim in source_claims:
                if not isinstance(source_claim, dict) or not isinstance(source_claim.get("id"), str):
                    raise SupportError("%s artifact has a malformed v2 override" % evidence_id)
                source_id = source_claim["id"]
                if source_id in source_by_id:
                    raise SupportError("%s artifact has duplicate claim %s" % (evidence_id, source_id))
                source_claim = dict(source_claim)
                source_claim["_ACVersion"] = "AC1024"
                source_claim["_recordClass"] = "RTEXT"
                source_by_id[source_id] = source_claim
        else:
            raise SupportError("%s artifact kind is not admitted" % evidence_id)
        if not fields <= set(source_by_id):
            raise SupportError("%s candidate fields do not resolve in the artifact" % evidence_id)
        by_id[evidence_id] = row
        field_sets[evidence_id] = fields
        field_details[evidence_id] = source_by_id
    return by_id, field_sets, field_details


def _route_inventory(routes: dict[str, Any], matrix: dict[str, Any]) -> set[str]:
    if routes.get("schema") != 1 or routes.get("kind") != "libdxfrw-source-route-inventory":
        raise SupportError("unexpected source-route inventory schema")
    rows = routes.get("mapping", {}).get("rows")
    if not isinstance(rows, list) or not rows:
        raise SupportError("source-route inventory is empty")
    route_ids = []
    for row in rows:
        if not isinstance(row, dict) or not isinstance(row.get("targetRouteId"), str):
            raise SupportError("source-route inventory contains a malformed row")
        route_ids.append(row["targetRouteId"])
    if len(route_ids) != len(set(route_ids)):
        raise SupportError("source-route inventory contains duplicate route IDs")
    if matrix.get("schema") != 1 or matrix.get("kind") != "libdxfrw-support-matrix":
        raise SupportError("unexpected support-matrix schema")
    claim_policy = matrix.get("claimPolicy")
    if (
        not isinstance(claim_policy, dict)
        or claim_policy.get("advertisedRows") != 0
        or claim_policy.get("qualifiedFormatParityRows") != 0
    ):
        raise SupportError("source support matrix conflicts with the joined claims authority")
    for facade in ("dxfRW", "dwgRW"):
        selected = sorted(row["targetRouteId"] for row in rows if row.get("facade") == facade)
        summary = matrix.get("facades", {}).get(facade, {})
        digest = hashlib.sha256("\n".join(selected).encode("utf-8")).hexdigest()
        if summary.get("routeCount") != len(selected) or summary.get("routeIdsSha256") != digest:
            raise SupportError("support matrix is stale for %s" % facade)
    return set(route_ids)


def validate_claims(
    document: dict[str, Any], *, root: Path, inputs: dict[str, Any],
    routes: dict[str, Any], matrix: dict[str, Any], allow_draft: bool,
) -> tuple[dict[str, dict[str, Any]], dict[str, dict[str, Any]]]:
    _exact_keys(document, CLAIMS_KEYS, "claims document")
    if document.get("schema") != 1 or document.get("kind") != "libdxfrw-qualified-format-claims":
        raise SupportError("unexpected claims schema")
    if document.get("fixturePolicy") != "locked-repository-blob-or-local-from-scratch-runtime; no-generated-drawing-payloads":
        raise SupportError("claims fixture policy drifted")
    freeze_state = document.get("freezeState")
    if freeze_state != "FROZEN" and not allow_draft:
        raise SupportError("immutable claims are not frozen")
    _validate_authority(document)
    evidence, evidence_fields, evidence_details = _validate_evidence(document, root, inputs)
    inventory_ids = _route_inventory(routes, matrix)

    claims = document.get("claims")
    if not isinstance(claims, list) or not claims:
        raise SupportError("claims must be a non-empty list")
    by_id: dict[str, dict[str, Any]] = {}
    route_membership: dict[str, set[str]] = {}
    used_evidence_fields: set[tuple[str, str]] = set()
    for index, claim in enumerate(claims):
        if not isinstance(claim, dict):
            raise SupportError("claims[%d] must be an object" % index)
        _exact_keys(claim, CLAIM_KEYS, "claims[%d]" % index)
        claim_id = claim.get("id")
        claim_tuple = claim.get("tuple")
        if not isinstance(claim_tuple, dict) or claim_id != canonical_claim_id(claim_tuple):
            raise SupportError("claim %s has a non-canonical ID" % claim_id)
        if claim_id in by_id:
            raise SupportError("duplicate claim ID %s" % claim_id)
        if claim_tuple["facade"] not in {"dxfRW", "dwgRW"}:
            raise SupportError("claim %s has an unknown facade" % claim_id)
        if claim_tuple["format"] not in {"DXF", "DWG"}:
            raise SupportError("claim %s has an unknown format" % claim_id)
        if claim_tuple["direction"] not in {"read", "write"}:
            raise SupportError("claim %s has an unknown direction" % claim_id)
        if claim_tuple["contractKind"] not in CONTRACT_KINDS:
            raise SupportError("claim %s has an unknown contract kind" % claim_id)
        if claim_tuple["fieldPath"] in {"*", "/", "all", "whole-format", "fixture"} or "*" in claim_tuple["fieldPath"]:
            raise SupportError("claim %s is a blanket or whole-format claim" % claim_id)
        route_ids = _string_list(claim.get("routeIds"), claim_id + " routeIds", nonempty=True)
        if route_ids != sorted(route_ids):
            raise SupportError("claim %s route IDs are not canonical" % claim_id)
        if any(route not in inventory_ids for route in route_ids):
            raise SupportError("claim %s names an unknown source route" % claim_id)
        evidence_ids = _string_list(claim.get("evidenceIds"), claim_id + " evidenceIds", nonempty=True)
        if evidence_ids != sorted(evidence_ids):
            raise SupportError("claim %s evidence IDs are not canonical" % claim_id)
        if any(item not in evidence for item in evidence_ids):
            raise SupportError("claim %s has unresolved evidence" % claim_id)
        field_id = claim.get("evidenceFieldId")
        if not isinstance(field_id, str) or not field_id:
            raise SupportError("claim %s lacks an evidence field ID" % claim_id)
        if any(field_id not in evidence_fields[item] for item in evidence_ids):
            raise SupportError("claim %s field is absent from its evidence" % claim_id)
        for evidence_id in evidence_ids:
            evidence_row = evidence[evidence_id]
            source_claim = evidence_details[evidence_id][field_id]
            semantic = source_claim.get("semantic")
            if isinstance(semantic, dict):
                source_record_class = semantic.get("recordClass")
                source_field_path = semantic.get("field")
            else:
                source_record_class = source_claim.get("_recordClass")
                source_field_path = source_claim.get("semanticField")
            if (
                claim_tuple["facade"] != evidence_row["facade"]
                or claim_tuple["format"] != evidence_row["format"]
                or claim_tuple["direction"] != evidence_row["direction"]
                or claim_tuple["ACVersion"] != source_claim["_ACVersion"]
                or claim_tuple["recordClass"] != source_record_class
                or claim_tuple["fieldPath"] != source_field_path
                or claim.get("normalization") != source_claim.get("normalization")
                or claim.get("tolerance") != source_claim.get("tolerance")
            ):
                raise SupportError("claim %s does not exactly crosswalk its evidence field" % claim_id)
        for evidence_id in evidence_ids:
            pair = (evidence_id, field_id)
            if pair in used_evidence_fields:
                raise SupportError("evidence field %s is reused by multiple claims" % field_id)
            used_evidence_fields.add(pair)
        normalization = claim.get("normalization")
        tolerance = claim.get("tolerance")
        if normalization == "floatTolerance":
            if (
                not isinstance(tolerance, dict)
                or set(tolerance) != {"absolute", "relative"}
                or any(not isinstance(tolerance[key], (int, float)) or tolerance[key] < 0 for key in tolerance)
            ):
                raise SupportError("claim %s has an invalid float tolerance" % claim_id)
        elif normalization in {"finiteNumber", "degreesFromRadians"}:
            if (
                not isinstance(tolerance, (int, float))
                or isinstance(tolerance, bool)
                or not math.isfinite(float(tolerance))
                or tolerance < 0
            ):
                raise SupportError("claim %s has an invalid numeric tolerance" % claim_id)
        elif tolerance is not None:
            raise SupportError("claim %s has a tolerance without float normalization" % claim_id)
        if not isinstance(normalization, str) or not normalization:
            raise SupportError("claim %s lacks a normalization" % claim_id)
        by_id[claim_id] = claim
        for route in route_ids:
            route_membership.setdefault(route, set()).add(claim_id)

    requirements = document.get("routeRequirements")
    if not isinstance(requirements, list) or not requirements:
        raise SupportError("routeRequirements must be a non-empty list")
    by_route: dict[str, dict[str, Any]] = {}
    for index, row in enumerate(requirements):
        if not isinstance(row, dict):
            raise SupportError("routeRequirements[%d] must be an object" % index)
        _exact_keys(row, REQUIREMENT_KEYS, "routeRequirements[%d]" % index)
        route_id = row.get("routeId")
        if not isinstance(route_id, str) or route_id in by_route:
            raise SupportError("duplicate or invalid route requirement")
        required = _string_list(row.get("requiredClaimIds"), route_id + " requiredClaimIds", nonempty=True)
        if required != sorted(required):
            raise SupportError("%s required claims are not canonical" % route_id)
        if set(required) != route_membership.get(route_id, set()):
            raise SupportError("%s has a partial, extra, or wrong-route required claim set" % route_id)
        if all(by_id[item]["tuple"]["fieldPath"] in IDENTITY_FIELDS for item in required):
            raise SupportError("%s is identity-only and cannot advertise payload support" % route_id)
        by_route[route_id] = row
    if set(by_route) != set(route_membership):
        raise SupportError("route requirements omit or add a candidate route")

    excluded = document.get("excludedCandidates")
    if not isinstance(excluded, list):
        raise SupportError("excludedCandidates must be a list")
    for index, row in enumerate(excluded):
        if not isinstance(row, dict):
            raise SupportError("excludedCandidates[%d] must be an object" % index)
        _exact_keys(row, EXCLUDED_KEYS, "excludedCandidates[%d]" % index)
        _string_list(row.get("sourceFieldIds"), "excluded sourceFieldIds", nonempty=True)
        if row.get("reason") == "identity-only-independent-evidence":
            forbidden_class = row.get("recordClass")
            if any(claim["tuple"]["recordClass"] == forbidden_class for claim in claims):
                raise SupportError("identity-only excluded class became a claim")

    pending = document.get("pendingFinalization")
    if not isinstance(pending, list):
        raise SupportError("pendingFinalization must be a list")
    for index, row in enumerate(pending):
        if not isinstance(row, dict):
            raise SupportError("pendingFinalization[%d] must be an object" % index)
        _exact_keys(row, PENDING_KEYS, "pendingFinalization[%d]" % index)
        if row.get("requiredBeforeFreeze") is not True:
            raise SupportError("pending finalization row must fail closed")
    if freeze_state == "FROZEN" and pending:
        raise SupportError("frozen claims retain pending finalization inputs")
    return by_id, by_route


def validate_status(
    status: dict[str, Any], claims_path: Path,
    claim_by_id: dict[str, dict[str, Any]], route_by_id: dict[str, dict[str, Any]],
    evidence_by_id: dict[str, dict[str, Any]], *, allow_draft: bool,
    accepted_focused_receipt_ids: set[str] | None,
) -> dict[str, str]:
    _exact_keys(status, STATUS_KEYS, "status overlay")
    if status.get("schema") != 1 or status.get("kind") != "libdxfrw-qualified-format-status":
        raise SupportError("unexpected status overlay schema")
    freeze_state = status.get("freezeState")
    if freeze_state != "FROZEN" and not allow_draft:
        raise SupportError("status overlay is not frozen")
    recorded_claims_digest = status.get("claimsDigestSha256")
    implementation_digest = status.get("implementationDigestSha256")
    if freeze_state == "FROZEN":
        if not _is_hex64(recorded_claims_digest) or recorded_claims_digest != claims_digest(claims_path):
            raise SupportError("status overlay has a stale claims digest")
        if not _is_hex64(implementation_digest):
            raise SupportError("status overlay lacks a frozen implementation digest")
    elif recorded_claims_digest is not None or implementation_digest is not None:
        raise SupportError("draft status must not pretend to freeze digests")

    expected_transitions = {
        "EXPERIMENTAL": ["EXPERIMENTAL", "PENDING_NATIVE"],
        "PENDING_NATIVE": ["EXPERIMENTAL", "PENDING_NATIVE", "PROMOTED"],
        "PROMOTED": ["EXPERIMENTAL", "PROMOTED"],
    }
    if status.get("allowedTransitions") != expected_transitions:
        raise SupportError("status transition policy drifted")
    native_refs = status.get("nativeReceiptRefs")
    broad_refs = status.get("broadMatrixReceiptRefs")
    if not isinstance(native_refs, list) or not isinstance(broad_refs, list):
        raise SupportError("receipt reference collections must be lists")
    native_ids: list[str] = []
    for index, ref in enumerate(native_refs):
        if not isinstance(ref, dict) or not isinstance(ref.get("id"), str) or not ref["id"]:
            raise SupportError("nativeReceiptRefs[%d] lacks a non-empty ID" % index)
        native_ids.append(ref["id"])
    if len(native_ids) != len(set(native_ids)):
        raise SupportError("native receipt reference IDs contain duplicates")
    checked_broad_refs = _string_list(broad_refs, "broadMatrixReceiptRefs")
    if not set(checked_broad_refs) <= set(native_ids):
        raise SupportError("broad matrix receipt references are not native receipt IDs")
    if (accepted_focused_receipt_ids is not None and
            len(accepted_focused_receipt_ids) != 3):
        raise SupportError("verified focused receipt set must contain exactly three IDs")

    rows = status.get("claimStatus")
    if not isinstance(rows, list):
        raise SupportError("claimStatus must be a list")
    statuses: dict[str, str] = {}
    for index, row in enumerate(rows):
        if not isinstance(row, dict):
            raise SupportError("claimStatus[%d] must be an object" % index)
        _exact_keys(row, STATUS_ROW_KEYS, "claimStatus[%d]" % index)
        claim_id = row.get("claimId")
        if claim_id not in claim_by_id or claim_id in statuses:
            raise SupportError("status row has an unknown or duplicate claim ID")
        state = row.get("status")
        if state not in ALL_STATUSES or (
                state == "PROMOTED" and accepted_focused_receipt_ids is None):
            raise SupportError("claim %s has a forbidden status" % claim_id)
        blockers = _string_list(row.get("blockers"), claim_id + " blockers")
        receipt_refs = _string_list(row.get("receiptRefs"), claim_id + " receiptRefs")
        if not set(receipt_refs) <= set(native_ids):
            raise SupportError("claim %s references unknown native receipts" % claim_id)
        claim = claim_by_id[claim_id]
        claim_evidence = [evidence_by_id[item] for item in claim["evidenceIds"]]
        evidence_blockers = {
            blocker for evidence in claim_evidence
            for blocker in evidence["qualificationBlockers"]
        }
        if any(not evidence["qualificationEligible"] for evidence in claim_evidence):
            if state != "EXPERIMENTAL" or not evidence_blockers <= set(blockers):
                raise SupportError("ineligible claim %s escaped EXPERIMENTAL" % claim_id)
        if state == "PENDING_NATIVE" and (blockers or receipt_refs or freeze_state != "FROZEN"):
            raise SupportError("PENDING_NATIVE claim %s has blockers, receipts, or unfrozen inputs" % claim_id)
        if state == "PROMOTED" and (
                freeze_state != "FROZEN" or blockers or
                set(receipt_refs) != accepted_focused_receipt_ids):
            raise SupportError(
                "PROMOTED claim %s lacks the exact API-verified focused receipt set" %
                claim_id)
        if state == "EXPERIMENTAL" and receipt_refs:
            raise SupportError("EXPERIMENTAL claim %s references accepting receipts" % claim_id)
        statuses[claim_id] = state
    if set(statuses) != set(claim_by_id):
        raise SupportError("status overlay is missing or adds immutable claims")

    advertised = {}
    for route_id, requirement in route_by_id.items():
        route_states = [statuses[item] for item in requirement["requiredClaimIds"]]
        promoted_count = route_states.count("PROMOTED")
        advertised[route_id] = (
            "QUALIFIED_FORMAT_PARITY"
            if promoted_count == len(route_states) else "EXPERIMENTAL"
        )
    return advertised


def validate_all(
    claims_path: Path, status_path: Path, routes_path: Path, matrix_path: Path,
    inputs_path: Path, *, allow_draft: bool,
    accepted_focused_receipt_ids: set[str] | None,
) -> dict[str, Any]:
    claims_document = _read_json(claims_path)
    status_document = _read_json(status_path)
    inputs_document = _read_json(inputs_path)
    evidence_by_id = {
        row["id"]: row for row in claims_document.get("evidence", [])
        if isinstance(row, dict) and isinstance(row.get("id"), str)
    }
    claim_by_id, route_by_id = validate_claims(
        claims_document,
        root=ROOT,
        inputs=inputs_document,
        routes=_read_json(routes_path),
        matrix=_read_json(matrix_path),
        allow_draft=allow_draft,
    )
    advertised = validate_status(
        status_document, claims_path, claim_by_id, route_by_id, evidence_by_id,
        allow_draft=allow_draft,
        accepted_focused_receipt_ids=accepted_focused_receipt_ids,
    )
    return {
        "claimCount": len(claim_by_id),
        "candidateRouteCount": len(route_by_id),
        "advertisedRouteCount": sum(value == "QUALIFIED_FORMAT_PARITY" for value in advertised.values()),
        "claimsDigestSha256": claims_digest(claims_path),
        "routes": advertised,
    }


def verify_promoted_authority(
    *, root: Path, status_path: Path, claims_path: Path, inputs_path: Path,
    required_tests_path: Path, receipt_schema_path: Path,
    live_token_env: str | None,
) -> set[str]:
    if not live_token_env:
        raise SupportError(
            "promoted support requires --live-token-env for authenticated API/artifact replay")
    token = os.environ.get(live_token_env)
    if not token:
        raise SupportError("promotion verification token environment variable is empty")
    try:
        import check_qualification_digest as digest_tool
        import verify_native_qualification_receipts as receipt_verifier

        inputs_document = digest_tool._read_json(inputs_path)
        digest_tool.validate_manifest(inputs_document, allow_draft=False)
        digest_result = digest_tool.compute_digest(
            root.resolve(), inputs_document, allow_selected_untracked=False)
        digest_tool.verify_status_digest(
            digest_result, status_path, allow_draft=False)
        receipt_result = receipt_verifier.validate_receipt_set(
            receipt_verifier.read_json(status_path), root=root.resolve(),
            claims_path=claims_path, contract_path=required_tests_path,
            schema_path=receipt_schema_path,
            implementation_inputs_path=inputs_path, require_complete=True,
            live_token=token,
        )
    except (OSError, UnicodeError, ValueError) as exc:
        raise SupportError("promotion authority verification failed: %s" % exc) from exc
    accepted = receipt_result.get("acceptedFocusedIds")
    if not isinstance(accepted, list) or any(
            not isinstance(item, str) or not item for item in accepted):
        raise SupportError("receipt verifier returned no exact focused receipt IDs")
    return set(accepted)


def _expect_rejected(callable_value: Any, label: str) -> None:
    try:
        callable_value()
    except SupportError:
        return
    raise AssertionError("negative vector was accepted: %s" % label)


def self_test() -> None:
    claims = _read_json(DEFAULT_CLAIMS)
    live_status = _read_json(DEFAULT_STATUS)
    # Contract self-tests must remain runnable after a legitimate J364
    # promotion.  Normalize only promoted rows back to their immediately
    # preceding PENDING_NATIVE state; the authenticated production path above
    # validates the real promoted overlay and receipt set.
    status = copy.deepcopy(live_status)
    for row in status.get("claimStatus", []):
        if isinstance(row, dict) and row.get("status") == "PROMOTED":
            row["status"] = "PENDING_NATIVE"
            row["receiptRefs"] = []
    routes = _read_json(DEFAULT_ROUTES)
    matrix = _read_json(DEFAULT_MATRIX)
    inputs = _read_json(DEFAULT_INPUTS)
    claim_by_id, route_by_id = validate_claims(
        claims, root=ROOT, inputs=inputs, routes=routes, matrix=matrix,
        allow_draft=True,
    )
    evidence = {row["id"]: row for row in claims["evidence"]}
    validate_status(
        status, DEFAULT_CLAIMS, claim_by_id, route_by_id, evidence,
        allow_draft=True, accepted_focused_receipt_ids=None,
    )

    def invalid_claims(mutator: Any) -> None:
        changed = copy.deepcopy(claims)
        mutator(changed)
        validate_claims(
            changed, root=ROOT, inputs=inputs, routes=routes, matrix=matrix,
            allow_draft=True,
        )

    _expect_rejected(lambda: invalid_claims(lambda d: d["evidence"][0].update(authorityKind="self-read")), "self-read authority")
    _expect_rejected(lambda: invalid_claims(lambda d: d["evidence"][0].update(fixtureAdmission="external-only")), "external-only evidence")
    _expect_rejected(lambda: invalid_claims(lambda d: d["evidence"][0].update(artifactSha256="0" * 64)), "stale evidence digest")
    _expect_rejected(lambda: invalid_claims(lambda d: d["evidence"][0].update(qualificationEligible=True)), "unresolved integrity eligibility")
    _expect_rejected(lambda: invalid_claims(lambda d: d["routeRequirements"][0].update(requiredClaimIds=[])), "empty route requirements")
    _expect_rejected(lambda: invalid_claims(lambda d: d["routeRequirements"][0]["requiredClaimIds"].append(d["routeRequirements"][0]["requiredClaimIds"][0])), "duplicate route requirements")
    _expect_rejected(lambda: invalid_claims(lambda d: d["routeRequirements"][0]["requiredClaimIds"].pop()), "partial route requirements")
    _expect_rejected(lambda: invalid_claims(lambda d: d["claims"][0]["tuple"].update(fieldPath="*")), "blanket claim")
    _expect_rejected(lambda: invalid_claims(lambda d: d["claims"][0].update(routeIds=["dwgRW/unknown/route"])), "unknown route")
    _expect_rejected(lambda: invalid_claims(lambda d: d["claims"][0].update(evidenceFieldId="rtext-handle")), "wrong evidence crosswalk")
    v2_evidence_index = next(
        index for index, row in enumerate(claims["evidence"])
        if row["id"] == "evidence:s386b-ac1024-rtext-insertion-v2"
    )
    insertion_claim_index = next(
        index for index, row in enumerate(claims["claims"])
        if row["evidenceFieldId"] == "rtext-insertion-x"
    )
    rotation_claim_index = next(
        index for index, row in enumerate(claims["claims"])
        if row["evidenceFieldId"] == "rtext-rotation"
    )
    _expect_rejected(
        lambda: invalid_claims(
            lambda d: d["evidence"][v2_evidence_index].update(
                checkerSha256="0" * 64
            )
        ),
        "stale v2 checker digest",
    )
    _expect_rejected(
        lambda: invalid_claims(
            lambda d: d["evidence"][v2_evidence_index].update(
                recordedByCommit="0" * 40
            )
        ),
        "unbound v2 recorded commit",
    )
    _expect_rejected(
        lambda: invalid_claims(
            lambda d: d["claims"][insertion_claim_index].update(
                evidenceFieldId="rtext-insertion-y"
            )
        ),
        "wrong v2 insertion crosswalk",
    )
    _expect_rejected(
        lambda: invalid_claims(
            lambda d: d["claims"][rotation_claim_index].update(
                tolerance=0.0
            )
        ),
        "wrong v2 rotation tolerance",
    )

    identity = copy.deepcopy(claims)
    payload_ids = {
        row["id"] for row in identity["claims"]
        if row["tuple"]["recordClass"] == "RTEXT"
        and row["tuple"]["fieldPath"] not in IDENTITY_FIELDS
    }
    identity["claims"] = [
        row for row in identity["claims"] if row["id"] not in payload_ids
    ]
    identity["routeRequirements"][1]["requiredClaimIds"] = [
        item for item in identity["routeRequirements"][1]["requiredClaimIds"]
        if item not in payload_ids
    ]
    _expect_rejected(
        lambda: validate_claims(identity, root=ROOT, inputs=inputs, routes=routes, matrix=matrix, allow_draft=True),
        "identity-only route",
    )

    def invalid_status(mutator: Any) -> None:
        changed = copy.deepcopy(status)
        mutator(changed)
        validate_status(
            changed, DEFAULT_CLAIMS, claim_by_id, route_by_id, evidence,
            allow_draft=True, accepted_focused_receipt_ids=None,
        )

    _expect_rejected(lambda: invalid_status(lambda d: d["claimStatus"].pop()), "missing status")
    _expect_rejected(lambda: invalid_status(lambda d: d["claimStatus"].append(copy.deepcopy(d["claimStatus"][0]))), "duplicate status")
    _expect_rejected(lambda: invalid_status(lambda d: d["claimStatus"][0].update(tuple={})), "immutable data in overlay")
    _expect_rejected(lambda: invalid_status(lambda d: d["claimStatus"][0].update(status="PROMOTED")), "pre-native promotion")
    _expect_rejected(lambda: invalid_status(lambda d: d["claimStatus"][0].update(status="PENDING_NATIVE")), "blocked pending-native")
    draft_claims = copy.deepcopy(claims)
    draft_claims["freezeState"] = "DRAFT_WORKFLOW_AND_DIGEST_PENDING"
    _expect_rejected(
        lambda: validate_claims(
            draft_claims, root=ROOT, inputs=inputs, routes=routes,
            matrix=matrix, allow_draft=False),
        "unfrozen claims",
    )

    eligible_claim = next(
        claim_id for claim_id, claim in claim_by_id.items()
        if all(evidence[item]["qualificationEligible"] for item in claim["evidenceIds"])
    )
    receipt_ids = {"receipt:linux", "receipt:macos", "receipt:windows"}

    def draft_promotion() -> None:
        changed = copy.deepcopy(status)
        changed["freezeState"] = "DRAFT_WORKFLOW_AND_DIGEST_PENDING"
        changed["claimsDigestSha256"] = None
        changed["implementationDigestSha256"] = None
        changed["nativeReceiptRefs"] = [{"id": item} for item in sorted(receipt_ids)]
        row = next(item for item in changed["claimStatus"] if item["claimId"] == eligible_claim)
        row.update(status="PROMOTED", blockers=[], receiptRefs=sorted(receipt_ids))
        validate_status(
            changed, DEFAULT_CLAIMS, claim_by_id, route_by_id, evidence,
            allow_draft=True, accepted_focused_receipt_ids=receipt_ids,
        )

    _expect_rejected(draft_promotion, "draft promotion with opaque receipt IDs")
    _expect_rejected(
        lambda: verify_promoted_authority(
            root=ROOT, status_path=DEFAULT_STATUS,
            claims_path=DEFAULT_CLAIMS, inputs_path=DEFAULT_INPUTS,
            required_tests_path=DEFAULT_TESTS,
            receipt_schema_path=DEFAULT_RECEIPT_SCHEMA,
            live_token_env=None,
        ),
        "promotion without authenticated live replay",
    )

    frozen_partial = copy.deepcopy(status)
    frozen_partial["freezeState"] = "FROZEN"
    frozen_partial["claimsDigestSha256"] = claims_digest(DEFAULT_CLAIMS)
    frozen_partial["implementationDigestSha256"] = "0" * 64
    frozen_partial["nativeReceiptRefs"] = [
        {"id": item} for item in sorted(receipt_ids)
    ]
    for row in frozen_partial["claimStatus"]:
        claim = claim_by_id[row["claimId"]]
        if all(evidence[item]["qualificationEligible"] for item in claim["evidenceIds"]):
            row.update(
                status="PROMOTED", blockers=[], receiptRefs=sorted(receipt_ids))
    partial_routes = validate_status(
        frozen_partial, DEFAULT_CLAIMS, claim_by_id, route_by_id, evidence,
        allow_draft=False, accepted_focused_receipt_ids=receipt_ids,
    )
    if any(value != "EXPERIMENTAL" for value in partial_routes.values()):
        raise AssertionError("partial field promotion advertised an aggregate route")

    with tempfile.TemporaryDirectory(prefix="libdxfrw-claims-test-") as directory:
        encoded = Path(directory) / "claims.json"
        encoded.write_text(json.dumps(claims, sort_keys=True), encoding="utf-8")
        if not _is_hex64(claims_digest(encoded)):
            raise AssertionError("claims digest is not SHA-256")
    print("qualified format support self-test: PASS (15 claims; 2 exact candidate routes; 0 advertised)")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--claims", type=Path, default=DEFAULT_CLAIMS)
    parser.add_argument("--status", type=Path, default=DEFAULT_STATUS)
    parser.add_argument("--routes", type=Path, default=DEFAULT_ROUTES)
    parser.add_argument("--support-matrix", type=Path, default=DEFAULT_MATRIX)
    parser.add_argument("--implementation-inputs", type=Path, default=DEFAULT_INPUTS)
    parser.add_argument("--required-tests", type=Path, default=DEFAULT_TESTS)
    parser.add_argument("--receipt-schema", type=Path, default=DEFAULT_RECEIPT_SCHEMA)
    parser.add_argument("--root", type=Path, default=ROOT)
    parser.add_argument("--allow-draft", action="store_true")
    parser.add_argument("--allow-promoted", action="store_true")
    parser.add_argument("--live-token-env")
    parser.add_argument("--json", action="store_true")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args(argv)
    try:
        if args.self_test:
            self_test()
            return 0
        status_document = _read_json(args.status)
        status_rows = status_document.get("claimStatus")
        has_promoted = isinstance(status_rows, list) and any(
            isinstance(row, dict) and row.get("status") == "PROMOTED"
            for row in status_rows
        )
        accepted_focused_receipt_ids = None
        if has_promoted:
            if not args.allow_promoted:
                raise SupportError("promoted claims require --allow-promoted")
            accepted_focused_receipt_ids = verify_promoted_authority(
                root=args.root, status_path=args.status,
                claims_path=args.claims, inputs_path=args.implementation_inputs,
                required_tests_path=args.required_tests,
                receipt_schema_path=args.receipt_schema,
                live_token_env=args.live_token_env,
            )
        result = validate_all(
            args.claims, args.status, args.routes, args.support_matrix,
            args.implementation_inputs,
            allow_draft=args.allow_draft,
            accepted_focused_receipt_ids=accepted_focused_receipt_ids,
        )
        if args.json:
            print(json.dumps(result, sort_keys=True, separators=(",", ":")))
        else:
            print(
                "qualified format support: PASS (%d claims; %d candidate routes; %d advertised)"
                % (result["claimCount"], result["candidateRouteCount"], result["advertisedRouteCount"])
            )
        return 0
    except (OSError, UnicodeError, json.JSONDecodeError, SupportError, AssertionError) as exc:
        print("qualified format support: FAIL: %s" % exc, file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
