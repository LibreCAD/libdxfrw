#!/usr/bin/env python3
"""Verify the three admitted J284 fixtures against semantic adapters and LibreDWG.

The lane is intentionally field-scoped and non-promoting.  It runs the same
receipt-bound target/standalone adapters used by qualified differential v2,
then compares only the frozen RTEXT, MPOLYGON, and identity-only LARGE_RADIAL
contracts with a pinned LibreDWG full-JSON reader.  Drawing and oracle JSON
payloads stay temporary.
"""

from __future__ import annotations

import argparse
import copy
import hashlib
import json
import math
import os
import platform
import re
import subprocess
import sys
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_CONTRACT = ROOT / "metadata/admitted-candidate-fields-v1.json"
DEFAULT_MANIFEST = ROOT / "metadata/qualified-differential-v2.json"
HEX40 = re.compile(r"[0-9a-f]{40}")
HEX64 = re.compile(r"[0-9a-f]{64}")
HANDLE = re.compile(r"(?:0|[1-9a-f][0-9a-f]*)")
EXPECTED_DIFFERENTIAL_OPTIONS = {
    "applyExtrusion": False,
    "compatibilityProfile": "default",
}
ORACLE_ENVIRONMENT = {
    "LANG": "C",
    "LC_ALL": "C",
    "PATH": "/usr/bin:/bin",
    "SOURCE_DATE_EPOCH": "0",
    "TZ": "UTC",
}
ORACLE_FORBIDDEN_ENV_PREFIXES = ("DYLD_", "LIBREDWG_")
ORACLE_FORBIDDEN_ENV_NAMES = {
    "DWG_DEBUG", "DWG_LOGLEVEL", "DWG_TRACE", "HOME", "XDG_CONFIG_HOME",
}

CONTRACT_KEYS = {
    "schema", "kind", "status", "fixturePolicy", "promotionBoundary",
    "sharedDifferential", "oracle", "fixtures", "claims", "exclusions",
}
PROMOTION_KEYS = {
    "promotesSupport", "unlistedFieldDisposition", "allowedStatuses",
    "forbiddenClaimScopes",
}
DIFFERENTIAL_KEYS = {
    "manifest", "facade", "direction", "options",
    "deterministicRunsPerSide", "requireNoMismatches",
}
ORACLE_KEYS = {
    "name", "version", "versionOutput", "license", "outputFormat",
    "timeoutSeconds", "command", "source", "buildRecipe", "platform",
    "dependencyInspection", "artifacts", "closureDigest",
}
FIXTURE_KEYS = {
    "id", "path", "sha256", "byteSize", "version", "repository",
    "commit", "sourcePath", "sourceBlob", "oracleJsonSha256", "records",
}
RECORD_KEYS = {
    "semanticRecordClass", "oracleEntity", "expectedHandle", "callback",
    "expectedIncidentEdgeCount", "carrierDisposition",
}
CLAIM_KEYS = {
    "id", "fixtureId", "semantic", "oracle", "normalization", "tolerance",
}
SIDE_KEYS = {"recordClass", "field", "expected"}
ORACLE_SIDE_KEYS = {"entity", "field", "expected"}
EXCLUSION_KEYS = {
    "id", "fixtureId", "semanticRecordClass", "oracleEntity", "scope",
    "semanticFields", "oracleFields", "disposition", "reason",
}

EXPECTED_FIXTURES = {
    "fixture:large_radial": (
        "tests/fixtures/dwg/large_radial.dwg",
        (("LARGE_RADIAL", "LARGE_RADIAL_DIMENSION", "2f", "addDimRadial"),),
    ),
    "fixture:mpolygon_solid": (
        "tests/fixtures/dwg/mpolygon_solid.dwg",
        (("MPOLYGON", "MPOLYGON", "2f", "addMPolygon"),),
    ),
    "fixture:rtext_arctext": (
        "tests/fixtures/dwg/rtext_arctext.dwg",
        (
            ("ARCALIGNEDTEXT", "ARCALIGNEDTEXT", "5a1", "addText"),
            ("RTEXT", "RTEXT", "5a0", "addText"),
        ),
    ),
}
FIXTURE_PROVENANCE_KEYS = (
    "path", "sha256", "byteSize", "version", "repository", "commit",
    "sourcePath", "sourceBlob", "oracleJsonSha256",
)
EXPECTED_FIXTURE_PROVENANCE_DIGESTS = {
    "fixture:large_radial": (
        "363bd1de78d59609bac40b3fce8a65ac9b9cc3a9d2a30082da07f609b3227516"
    ),
    "fixture:mpolygon_solid": (
        "3478382004bf987d4d91e0290c419edd3706f21df596864823b376230c73ad8b"
    ),
    "fixture:rtext_arctext": (
        "167ff051cc8d86a68223b88aaf41d9d92d6ab6798782d6e80a90a4859e02eda3"
    ),
}

# This is the feature boundary, not expected test data.  Adding a field must be
# a new reviewed contract revision rather than an oracle-discovery side effect.
EXPECTED_CLAIMS = {
    "large-radial-handle": (
        "fixture:large_radial", "LARGE_RADIAL", "handle",
        "LARGE_RADIAL_DIMENSION", "handle", "handleHex",
    ),
    "large-radial-record-class": (
        "fixture:large_radial", "LARGE_RADIAL", "recordClass",
        "LARGE_RADIAL_DIMENSION", "entity", "mappedIdentity",
    ),
    "mpolygon-associative": (
        "fixture:mpolygon_solid", "MPOLYGON", "associative",
        "MPOLYGON", "is_associative", "boolean01",
    ),
    "mpolygon-base-point-z": (
        "fixture:mpolygon_solid", "MPOLYGON", "basePoint.z",
        "MPOLYGON", "elevation", "floatTolerance",
    ),
    "mpolygon-extrusion-x": (
        "fixture:mpolygon_solid", "MPOLYGON", "extPoint.x",
        "MPOLYGON", "extrusion.0", "floatTolerance",
    ),
    "mpolygon-extrusion-y": (
        "fixture:mpolygon_solid", "MPOLYGON", "extPoint.y",
        "MPOLYGON", "extrusion.1", "floatTolerance",
    ),
    "mpolygon-extrusion-z": (
        "fixture:mpolygon_solid", "MPOLYGON", "extPoint.z",
        "MPOLYGON", "extrusion.2", "floatTolerance",
    ),
    "mpolygon-name": (
        "fixture:mpolygon_solid", "MPOLYGON", "name",
        "MPOLYGON", "name", "exact",
    ),
    "mpolygon-pattern-type": (
        "fixture:mpolygon_solid", "MPOLYGON", "hpattern",
        "MPOLYGON", "pattern_type", "exact",
    ),
    "mpolygon-solid": (
        "fixture:mpolygon_solid", "MPOLYGON", "solid",
        "MPOLYGON", "is_solid_fill", "boolean01",
    ),
    "rtext-handle": (
        "fixture:rtext_arctext", "RTEXT", "handle",
        "RTEXT", "handle", "handleHex",
    ),
    "rtext-record-class": (
        "fixture:rtext_arctext", "RTEXT", "recordClass",
        "RTEXT", "entity", "exact",
    ),
    "rtext-text": (
        "fixture:rtext_arctext", "RTEXT", "text",
        "RTEXT", "text_value", "exact",
    ),
}

EXPECTED_EXCLUSIONS = {
    "arcalignedtext-payload": (
        "fixture:rtext_arctext", "ARCALIGNEDTEXT", "ARCALIGNEDTEXT",
        "payload-fields-excluded; record-class-and-handle-selector-only-nonclaim",
        "libredwg-ac1032-split-string-decoder-misalignment",
    ),
    "large-radial-payload": (
        "fixture:large_radial", "LARGE_RADIAL", "LARGE_RADIAL_DIMENSION",
        "all-payload-fields-except-record-class-and-handle",
        "identity-only-independent-evidence",
    ),
    "mpolygon-color": (
        "fixture:mpolygon_solid", "MPOLYGON", "MPOLYGON",
        "entity-and-hatch-color-fields",
        "aci-rgb-entity-color-normalization-unproved",
    ),
}
EXPECTED_EXCLUSION_FIELD_DIGESTS = {
    "arcalignedtext-payload": (
        "94a4dc79b8937bdfe1f2d5d6bd3d27f3d9f52fa471005c7bebd03c18c777aeb3"
    ),
    "large-radial-payload": (
        "43f7d002816cd0186ca514ddf929a9fa4edb939b8c28d054625b014e789164ac"
    ),
    "mpolygon-color": (
        "6102c8d5523d1d82ef2b308e80a8885a1df8b7cf061f0c5c391245d6cec66e2f"
    ),
}
EXPECTED_FLOAT_TOLERANCE = {"absolute": 1e-12, "relative": 1e-12}
EXPECTED_DIFFERENTIAL_DEBT_RULES = {
    "fixture:large_radial": "debt-s386-classes-crc-large-radial",
    "fixture:mpolygon_solid": "debt-s386-classes-crc-mpolygon-solid",
    "fixture:rtext_arctext": "debt-s386-classes-crc-rtext-arctext",
}

ORACLE_PIN = {
    "name": "LibreDWG dwgread",
    "version": "0.14",
    "versionOutput": "dwgread 0.14",
    "license": "GPL-3.0-or-later",
    "outputFormat": "JSON",
    "source": {
        "release": "0.14",
        "url": "https://ftpmirror.gnu.org/gnu/libredwg/libredwg-0.14.tar.gz",
        "sha256": "cb6ee0b078c6d9e0f09d66f1feac33ba6342df88ae544e9f9335fab475218351",
    },
    "buildRecipe": {
        "kind": "homebrew-formula-and-bottle",
        "formulaSha256": "bf4063db732dc93e63b3e10bbde5f1831b7dad695e95ebfc50a51342bdcf8f95",
        "bottleSha256": "2e5efe7bb02a7067cd67e028da20b90a56d45fb126352d7d963de8ff8c6a2298",
        "statementRepresentation": "symbolic-homebrew-formula-argv-expressions",
        "statements": [
            {
                "callee": "system",
                "argvExpression": [
                    "./configure", "--disable-silent-rules", "*std_configure_args",
                ],
            },
            {"callee": "system", "argvExpression": ["make"]},
            {"callee": "system", "argvExpression": ["make", "install"]},
        ],
    },
}

ORACLE_ARTIFACTS = [
    {
        "role": "executable",
        "basename": "dwgread",
        "byteSize": 51568,
        "sha256": "536eef295f1dfdc2e63b4f1cd2961905f98c55bbd1fe017ef3574c0f1345f1ff",
    },
    {
        "role": "library",
        "basename": "libredwg.0.dylib",
        "byteSize": 19787920,
        "sha256": "0dd60856c25aa0dad6ff80f6c753957d0f27cc39a29f8466637db68dbd0a8847",
    },
]
ORACLE_CLOSURE_DIGEST = (
    "4170272163a077731dad3b4d77602dbf38d32f3232c877b135d6b67e0895f1b0"
)


class CandidateFieldError(RuntimeError):
    """The field contract or its live evidence is invalid."""


class JsonObject(dict[str, Any]):
    """JSON object retaining key occurrences from LibreDWG full JSON."""

    def __init__(self, pairs: list[tuple[str, Any]]) -> None:
        super().__init__(pairs)
        self.pairs = pairs

    def occurrences(self, key: str) -> list[Any]:
        return [value for name, value in self.pairs if name == key]


def _strict_object(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    result: dict[str, Any] = {}
    for key, value in pairs:
        if key in result:
            raise CandidateFieldError(f"duplicate JSON key: {key}")
        result[key] = value
    return result


def _reject_constant(value: str) -> None:
    raise CandidateFieldError(f"non-finite JSON number: {value}")


def _read_json(path: Path) -> dict[str, Any]:
    try:
        value = json.loads(
            path.read_text(encoding="utf-8"),
            object_pairs_hook=_strict_object,
            parse_constant=_reject_constant,
        )
    except (OSError, UnicodeError, json.JSONDecodeError) as exc:
        raise CandidateFieldError(f"could not read {path}: {exc}") from exc
    if not isinstance(value, dict):
        raise CandidateFieldError(f"{path} must contain a JSON object")
    return value


def _exact(value: Any, keys: set[str], path: str) -> dict[str, Any]:
    if not isinstance(value, dict):
        raise CandidateFieldError(f"{path} must be an object")
    actual = set(value)
    if actual != keys:
        raise CandidateFieldError(
            f"{path} keys differ (missing={sorted(keys - actual)}, "
            f"extra={sorted(actual - keys)})"
        )
    return value


def _text(value: Any, path: str) -> str:
    if not isinstance(value, str) or not value:
        raise CandidateFieldError(f"{path} must be a non-empty string")
    return value


def _digest(value: Any, path: str, length: int = 64) -> str:
    text = _text(value, path)
    pattern = HEX64 if length == 64 else HEX40
    if pattern.fullmatch(text) is None:
        raise CandidateFieldError(f"{path} must be {length} lowercase hex digits")
    return text


def _positive_int(value: Any, path: str, *, zero: bool = False) -> int:
    minimum = 0 if zero else 1
    if isinstance(value, bool) or not isinstance(value, int) or value < minimum:
        word = "non-negative" if zero else "positive"
        raise CandidateFieldError(f"{path} must be a {word} integer")
    return value


def _sorted_unique_strings(value: Any, path: str, *, nonempty: bool = True) -> list[str]:
    if not isinstance(value, list) or (nonempty and not value):
        raise CandidateFieldError(f"{path} must be a non-empty list")
    result = [_text(item, f"{path}[{index}]") for index, item in enumerate(value)]
    if result != sorted(result) or len(result) != len(set(result)):
        raise CandidateFieldError(f"{path} must be sorted and unique")
    return result


def _sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def _canonical_digest(value: Any) -> str:
    encoded = json.dumps(
        value, sort_keys=True, separators=(",", ":"), allow_nan=False,
    ).encode("utf-8")
    return hashlib.sha256(encoded).hexdigest()


def validate_contract(value: Any) -> dict[str, Any]:
    contract = _exact(value, CONTRACT_KEYS, "contract")
    if contract["schema"] != 1 or contract["kind"] != (
        "libdxfrw-admitted-independent-candidate-fields"
    ):
        raise CandidateFieldError("contract schema/kind is invalid")
    if contract["status"] != "EXPERIMENTAL":
        raise CandidateFieldError("candidate contract must remain EXPERIMENTAL")
    if contract["fixturePolicy"] != (
        "exactly-three-locked-repository-blobs; runtime-json-only; "
        "no-new-drawing-payloads"
    ):
        raise CandidateFieldError("fixture policy changed")

    promotion = _exact(
        contract["promotionBoundary"], PROMOTION_KEYS,
        "contract.promotionBoundary",
    )
    if promotion != {
        "promotesSupport": False,
        "unlistedFieldDisposition": "excluded",
        "allowedStatuses": ["EXPERIMENTAL", "PENDING_NATIVE"],
        "forbiddenClaimScopes": ["fixture", "version", "whole-format"],
    }:
        raise CandidateFieldError("promotion boundary changed")

    differential = _exact(
        contract["sharedDifferential"], DIFFERENTIAL_KEYS,
        "contract.sharedDifferential",
    )
    if differential != {
        "manifest": "metadata/qualified-differential-v2.json",
        "facade": "dwgRW",
        "direction": "read",
        "options": EXPECTED_DIFFERENTIAL_OPTIONS,
        "deterministicRunsPerSide": 2,
        "requireNoMismatches": True,
    }:
        raise CandidateFieldError("shared differential contract changed")

    oracle = _exact(contract["oracle"], ORACLE_KEYS, "contract.oracle")
    for key, expected in ORACLE_PIN.items():
        if oracle.get(key) != expected:
            raise CandidateFieldError(f"oracle pin changed at {key}")
    if oracle["timeoutSeconds"] != 10:
        raise CandidateFieldError("oracle timeout changed")
    if oracle["command"] != ["{executable}", "-O", "JSON", "{input}"]:
        raise CandidateFieldError("oracle command changed")
    if oracle["platform"] != {
        "system": "Darwin", "architecture": "arm64", "osVersion": "26.5.2",
    }:
        raise CandidateFieldError("oracle platform profile changed")
    if oracle["dependencyInspection"] != {
        "tool": "/usr/bin/otool -L",
        "algorithm": (
            "sha256-sort-role-basename-sha256-then-role-nul-basename-nul-"
            "size-decimal-nul-sha256-lf-v1"
        ),
        "systemPrefixes": ["/System/Library/", "/usr/lib/"],
    }:
        raise CandidateFieldError("oracle dependency inspection contract changed")
    _digest(oracle["closureDigest"], "contract.oracle.closureDigest")
    artifacts = oracle["artifacts"]
    if not isinstance(artifacts, list) or len(artifacts) != 2:
        raise CandidateFieldError("oracle must pin exactly executable and library")
    roles: list[str] = []
    for index, item in enumerate(artifacts):
        row = _exact(
            item, {"role", "basename", "byteSize", "sha256"},
            f"contract.oracle.artifacts[{index}]",
        )
        role = _text(row["role"], f"contract.oracle.artifacts[{index}].role")
        if role not in {"executable", "library"}:
            raise CandidateFieldError("oracle artifact role is invalid")
        roles.append(role)
        if Path(_text(row["basename"], "oracle artifact basename")).name != row["basename"]:
            raise CandidateFieldError("oracle artifact basename is not a basename")
        _positive_int(row["byteSize"], "oracle artifact byteSize")
        _digest(row["sha256"], "oracle artifact sha256")
    if roles != ["executable", "library"]:
        raise CandidateFieldError("oracle artifacts must be executable then library")
    if artifacts != ORACLE_ARTIFACTS or oracle["closureDigest"] != ORACLE_CLOSURE_DIGEST:
        raise CandidateFieldError("oracle artifact/closure pin changed")

    fixtures = contract["fixtures"]
    if not isinstance(fixtures, list) or len(fixtures) != 3:
        raise CandidateFieldError("contract must contain exactly three fixtures")
    fixture_ids: list[str] = []
    record_pairs: set[tuple[str, str, str]] = set()
    for index, item in enumerate(fixtures):
        path = f"contract.fixtures[{index}]"
        row = _exact(item, FIXTURE_KEYS, path)
        fixture_id = _text(row["id"], f"{path}.id")
        fixture_ids.append(fixture_id)
        if fixture_id not in EXPECTED_FIXTURES:
            raise CandidateFieldError(f"unexpected fixture ID: {fixture_id}")
        expected_path, expected_records = EXPECTED_FIXTURES[fixture_id]
        if row["path"] != expected_path:
            raise CandidateFieldError(f"{fixture_id} path changed")
        _digest(row["sha256"], f"{path}.sha256")
        _positive_int(row["byteSize"], f"{path}.byteSize")
        if row["version"] != "AC1032" or row["repository"] != "LibreCAD/LibreCAD":
            raise CandidateFieldError(f"{fixture_id} version/repository changed")
        _digest(row["commit"], f"{path}.commit", 40)
        _text(row["sourcePath"], f"{path}.sourcePath")
        _digest(row["sourceBlob"], f"{path}.sourceBlob", 40)
        _digest(row["oracleJsonSha256"], f"{path}.oracleJsonSha256")
        provenance_digest = _canonical_digest({
            key: row[key] for key in FIXTURE_PROVENANCE_KEYS
        })
        if provenance_digest != EXPECTED_FIXTURE_PROVENANCE_DIGESTS[fixture_id]:
            raise CandidateFieldError(
                f"exact fixture provenance changed: {fixture_id}"
            )
        records = row["records"]
        if not isinstance(records, list) or len(records) != len(expected_records):
            raise CandidateFieldError(f"{fixture_id} record inventory changed")
        observed_records: list[tuple[str, str, str, str]] = []
        for record_index, record_value in enumerate(records):
            record = _exact(
                record_value, RECORD_KEYS, f"{path}.records[{record_index}]",
            )
            observed_records.append((
                _text(record["semanticRecordClass"], "semanticRecordClass"),
                _text(record["oracleEntity"], "oracleEntity"),
                _text(record["expectedHandle"], "expectedHandle"),
                _text(record["callback"], "callback"),
            ))
            if HANDLE.fullmatch(record["expectedHandle"]) is None:
                raise CandidateFieldError("expectedHandle is not canonical lowercase hex")
            if record["expectedIncidentEdgeCount"] != 1:
                raise CandidateFieldError("admitted record graph expectation changed")
            if record["carrierDisposition"] != "preserved":
                raise CandidateFieldError("carrier disposition changed")
            pair = (
                fixture_id, record["semanticRecordClass"], record["oracleEntity"],
            )
            if pair in record_pairs:
                raise CandidateFieldError("duplicate admitted record selector")
            record_pairs.add(pair)
        if tuple(observed_records) != expected_records:
            raise CandidateFieldError(f"{fixture_id} record contract changed")
    if fixture_ids != sorted(EXPECTED_FIXTURES):
        raise CandidateFieldError("fixture IDs must be exact, sorted, and unique")

    claims = contract["claims"]
    if not isinstance(claims, list) or len(claims) != len(EXPECTED_CLAIMS):
        raise CandidateFieldError("candidate claim count changed")
    claim_ids: list[str] = []
    for index, item in enumerate(claims):
        path = f"contract.claims[{index}]"
        row = _exact(item, CLAIM_KEYS, path)
        claim_id = _text(row["id"], f"{path}.id")
        claim_ids.append(claim_id)
        semantic = _exact(row["semantic"], SIDE_KEYS, f"{path}.semantic")
        independent = _exact(row["oracle"], ORACLE_SIDE_KEYS, f"{path}.oracle")
        observed = (
            row["fixtureId"], semantic["recordClass"], semantic["field"],
            independent["entity"], independent["field"], row["normalization"],
        )
        if EXPECTED_CLAIMS.get(claim_id) != observed:
            raise CandidateFieldError(f"candidate claim shape changed: {claim_id}")
        if (row["fixtureId"], semantic["recordClass"], independent["entity"]) not in record_pairs:
            raise CandidateFieldError(f"claim {claim_id} has no admitted record selector")
        if row["normalization"] == "floatTolerance":
            tolerance = _exact(
                row["tolerance"], {"absolute", "relative"}, f"{path}.tolerance",
            )
            if tolerance != EXPECTED_FLOAT_TOLERANCE:
                raise CandidateFieldError(
                    f"{claim_id} tolerance must remain exactly "
                    "absolute=relative=1e-12"
                )
        elif row["tolerance"] is not None:
            raise CandidateFieldError(f"{claim_id} must not carry a tolerance")
    if claim_ids != sorted(EXPECTED_CLAIMS):
        raise CandidateFieldError("claim IDs must be exact, sorted, and unique")

    exclusions = contract["exclusions"]
    if not isinstance(exclusions, list) or len(exclusions) != len(EXPECTED_EXCLUSIONS):
        raise CandidateFieldError("exclusion inventory changed")
    exclusion_ids: list[str] = []
    for index, item in enumerate(exclusions):
        path = f"contract.exclusions[{index}]"
        row = _exact(item, EXCLUSION_KEYS, path)
        exclusion_id = _text(row["id"], f"{path}.id")
        exclusion_ids.append(exclusion_id)
        observed = (
            row["fixtureId"], row["semanticRecordClass"], row["oracleEntity"],
            row["scope"], row["reason"],
        )
        if EXPECTED_EXCLUSIONS.get(exclusion_id) != observed:
            raise CandidateFieldError(f"exclusion shape changed: {exclusion_id}")
        if row["disposition"] != "excluded":
            raise CandidateFieldError(f"{exclusion_id} is not excluded")
        for side_name in ("semanticFields", "oracleFields"):
            _sorted_unique_strings(row[side_name], f"{path}.{side_name}")
        field_digest = _canonical_digest({
            "semanticFields": row["semanticFields"],
            "oracleFields": row["oracleFields"],
        })
        if field_digest != EXPECTED_EXCLUSION_FIELD_DIGESTS[exclusion_id]:
            raise CandidateFieldError(
                f"exclusion field inventory changed: {exclusion_id}"
            )
    if exclusion_ids != sorted(EXPECTED_EXCLUSIONS):
        raise CandidateFieldError("exclusion IDs must be exact, sorted, and unique")

    for claim in claims:
        for exclusion in exclusions:
            if claim["fixtureId"] != exclusion["fixtureId"]:
                continue
            if (
                claim["semantic"]["recordClass"] == exclusion["semanticRecordClass"]
                and claim["semantic"]["field"] in exclusion["semanticFields"]
            ) or (
                claim["oracle"]["entity"] == exclusion["oracleEntity"]
                and claim["oracle"]["field"].split(".", 1)[0]
                in exclusion["oracleFields"]
            ):
                raise CandidateFieldError(
                    f"claim {claim['id']} overlaps exclusion {exclusion['id']}"
                )
    return contract


def validate_registry_bindings(contract: dict[str, Any], root: Path) -> None:
    registry = _read_json(root / "metadata/fixture-registry.json")
    rows = registry.get("fixtures")
    if not isinstance(rows, list):
        raise CandidateFieldError("fixture registry is malformed")
    by_path = {
        row.get("path"): row for row in rows
        if isinstance(row, dict) and isinstance(row.get("path"), str)
    }
    for fixture in contract["fixtures"]:
        row = by_path.get(fixture["path"])
        if row is None:
            raise CandidateFieldError(f"fixture is not registry-admitted: {fixture['path']}")
        expected = {
            "sha256": fixture["sha256"],
            "size": fixture["byteSize"],
            "format": "DWG",
            "version": fixture["version"],
            "originKind": "lockedRepositoryBlob",
            "sourceRepository": fixture["repository"],
            "sourceCommit": fixture["commit"],
            "sourcePath": fixture["sourcePath"],
            "sourceBlob": fixture["sourceBlob"],
        }
        for key, value in expected.items():
            if row.get(key) != value:
                raise CandidateFieldError(
                    f"fixture registry binding changed for {fixture['path']}:{key}"
                )


def _oracle_object(pairs: list[tuple[str, Any]]) -> JsonObject:
    return JsonObject(pairs)


def _walk_finite(value: Any, path: str = "$") -> None:
    if isinstance(value, float) and not math.isfinite(value):
        raise CandidateFieldError(f"oracle JSON has non-finite value at {path}")
    if isinstance(value, JsonObject):
        for index, (key, item) in enumerate(value.pairs):
            _walk_finite(item, f"{path}.{key}#{index}")
    elif isinstance(value, list):
        for index, item in enumerate(value):
            _walk_finite(item, f"{path}[{index}]")


def parse_oracle_json(payload: bytes) -> JsonObject:
    try:
        text = payload.decode("utf-8", errors="strict")
        value = json.loads(
            text, object_pairs_hook=_oracle_object,
            parse_constant=_reject_constant,
        )
    except (UnicodeError, json.JSONDecodeError) as exc:
        raise CandidateFieldError(f"LibreDWG output is not full JSON: {exc}") from exc
    if not isinstance(value, JsonObject):
        raise CandidateFieldError("LibreDWG JSON root is not an object")
    _walk_finite(value)
    return value


def _single(value: Any, key: str, path: str) -> Any:
    if isinstance(value, JsonObject):
        matches = value.occurrences(key)
        if len(matches) != 1:
            raise CandidateFieldError(
                f"{path}.{key} must occur exactly once, found {len(matches)}"
            )
        return matches[0]
    if not isinstance(value, dict) or key not in value:
        raise CandidateFieldError(f"{path}.{key} is missing")
    return value[key]


def _oracle_records(payload: JsonObject, expected_version: str) -> list[Any]:
    if _single(payload, "created_by", "oracle") != "LibreDWG 0.14":
        raise CandidateFieldError("LibreDWG created_by identity changed")
    header = _single(payload, "FILEHEADER", "oracle")
    if _single(header, "version", "oracle.FILEHEADER") != expected_version:
        raise CandidateFieldError("LibreDWG FILEHEADER.version changed")
    records = _single(payload, "OBJECTS", "oracle")
    if not isinstance(records, list):
        raise CandidateFieldError("LibreDWG OBJECTS is not a list")
    return records


def _find_oracle_record(records: list[Any], entity: str) -> JsonObject:
    matches: list[JsonObject] = []
    for record in records:
        if not isinstance(record, JsonObject):
            continue
        entities = record.occurrences("entity")
        if len(entities) == 1 and entities[0] == entity:
            matches.append(record)
    if len(matches) != 1:
        raise CandidateFieldError(
            f"expected exactly one LibreDWG {entity} record, found {len(matches)}"
        )
    return matches[0]


def _semantic_fields(record: dict[str, Any]) -> dict[str, Any]:
    result: dict[str, Any] = {}
    fields = record.get("fields")
    if not isinstance(fields, list):
        raise CandidateFieldError("semantic record fields are malformed")
    for field in fields:
        if not isinstance(field, dict) or set(field) != {"name", "type", "value"}:
            raise CandidateFieldError("semantic typed field is malformed")
        name = field["name"]
        if not isinstance(name, str) or name in result:
            raise CandidateFieldError("semantic typed field names are invalid/duplicate")
        result[name] = field["value"]
    return result


def _find_semantic_record(result: dict[str, Any], record_class: str) -> dict[str, Any]:
    matches = [
        row for row in result["records"]
        if isinstance(row, dict) and row.get("recordClass") == record_class
    ]
    if len(matches) != 1:
        raise CandidateFieldError(
            f"expected exactly one semantic {record_class} record, found {len(matches)}"
        )
    return matches[0]


def _resolve(value: Any, field: str, path: str) -> Any:
    current = value
    for component in field.split("."):
        if isinstance(current, list):
            if not component.isdigit() or int(component) >= len(current):
                raise CandidateFieldError(f"{path}.{field} has an invalid list index")
            current = current[int(component)]
        else:
            current = _single(current, component, path)
    return current


def _strict_equal(left: Any, right: Any) -> bool:
    return type(left) is type(right) and left == right


def _normalized_handle(value: Any, path: str) -> str:
    if isinstance(value, str):
        if HANDLE.fullmatch(value) is None:
            raise CandidateFieldError(f"{path} is not canonical lowercase hex")
        return value
    if not isinstance(value, list) or len(value) != 3 or any(
        isinstance(item, bool) or not isinstance(item, int) or item < 0
        for item in value
    ):
        raise CandidateFieldError(f"{path} is not a LibreDWG handle tuple")
    code, size, absolute = value
    expected_size = 0 if absolute == 0 else (absolute.bit_length() + 7) // 8
    if code != 0 or size != expected_size:
        raise CandidateFieldError(f"{path} has an invalid LibreDWG handle shape")
    return format(absolute, "x")


def evaluate_claim(
    claim: dict[str, Any], semantic_record: dict[str, Any], oracle_record: JsonObject,
) -> dict[str, Any]:
    semantic_fields = _semantic_fields(semantic_record)
    semantic_field = claim["semantic"]["field"]
    if semantic_field not in semantic_fields:
        raise CandidateFieldError(
            f"semantic.{claim['semantic']['recordClass']}.{semantic_field} is missing"
        )
    semantic_value = semantic_fields[semantic_field]
    oracle_value = _resolve(
        oracle_record, claim["oracle"]["field"],
        f"oracle.{claim['oracle']['entity']}",
    )
    normalization = claim["normalization"]
    if normalization == "handleHex":
        semantic_normalized = _normalized_handle(semantic_value, "semantic handle")
        oracle_normalized = _normalized_handle(oracle_value, "oracle handle")
        if (
            semantic_normalized != claim["semantic"]["expected"]
            or oracle_normalized != claim["oracle"]["expected"]
            or semantic_normalized != oracle_normalized
        ):
            raise CandidateFieldError(f"claim {claim['id']} handle mismatch")
        normalized: Any = semantic_normalized
    elif normalization == "boolean01":
        if type(semantic_value) is not bool or type(oracle_value) is not int:
            raise CandidateFieldError(f"claim {claim['id']} boolean types changed")
        if oracle_value not in {0, 1}:
            raise CandidateFieldError(f"claim {claim['id']} oracle boolean is not 0/1")
        if (
            not _strict_equal(semantic_value, claim["semantic"]["expected"])
            or not _strict_equal(oracle_value, claim["oracle"]["expected"])
            or semantic_value != bool(oracle_value)
        ):
            raise CandidateFieldError(f"claim {claim['id']} boolean mismatch")
        normalized = semantic_value
    elif normalization == "floatTolerance":
        values = (
            semantic_value, oracle_value, claim["semantic"]["expected"],
            claim["oracle"]["expected"],
        )
        if any(
            isinstance(item, bool) or not isinstance(item, (int, float))
            or not math.isfinite(float(item)) for item in values
        ):
            raise CandidateFieldError(f"claim {claim['id']} has non-finite/non-numeric data")
        tolerance = claim["tolerance"]
        close = lambda left, right: math.isclose(  # noqa: E731
            float(left), float(right), rel_tol=float(tolerance["relative"]),
            abs_tol=float(tolerance["absolute"]),
        )
        if (
            not close(semantic_value, claim["semantic"]["expected"])
            or not close(oracle_value, claim["oracle"]["expected"])
            or not close(semantic_value, oracle_value)
        ):
            raise CandidateFieldError(f"claim {claim['id']} numeric mismatch")
        normalized = float(semantic_value)
    elif normalization == "mappedIdentity":
        if (
            not _strict_equal(semantic_value, claim["semantic"]["expected"])
            or not _strict_equal(oracle_value, claim["oracle"]["expected"])
        ):
            raise CandidateFieldError(f"claim {claim['id']} mapped identity mismatch")
        normalized = {
            "semantic": semantic_value,
            "oracle": oracle_value,
        }
    elif normalization == "exact":
        if (
            not _strict_equal(semantic_value, claim["semantic"]["expected"])
            or not _strict_equal(oracle_value, claim["oracle"]["expected"])
            or not _strict_equal(semantic_value, oracle_value)
        ):
            raise CandidateFieldError(f"claim {claim['id']} exact mismatch")
        normalized = semantic_value
    else:  # validate_contract prevents this path.
        raise CandidateFieldError(f"claim {claim['id']} normalization is invalid")
    return {
        "id": claim["id"],
        "status": "EXPERIMENTAL",
        "outcome": (
            "independentlyToleranceNormalized"
            if normalization == "floatTolerance" else "independentlyExact"
        ),
        "normalizedValueDigest": _canonical_digest(normalized),
    }


def _validate_result_health(result: dict[str, Any], side: str) -> None:
    status = result["status"]
    if status["outcome"] != "success" or status["operationSucceeded"] is not True:
        raise CandidateFieldError(f"{side} semantic adapter did not succeed")
    if status["firstFailure"] != {
        "stage": None, "code": None, "path": None, "diagnosticId": None,
    }:
        raise CandidateFieldError(f"{side} semantic adapter reports first failure")
    if any(row.get("severity") == "error" for row in result["diagnostics"]):
        raise CandidateFieldError(f"{side} semantic adapter reports an error diagnostic")


def validate_record_assertion(
    expected: dict[str, Any], result: dict[str, Any], oracle_record: JsonObject,
    side: str,
) -> None:
    record = _find_semantic_record(result, expected["semanticRecordClass"])
    fields = _semantic_fields(record)
    handle = _normalized_handle(_resolve(fields, "handle", "semantic"), "semantic handle")
    if handle != expected["expectedHandle"] or record["sourceHandle"] != handle:
        raise CandidateFieldError(f"{side} semantic record handle is inconsistent")
    oracle_handle = _normalized_handle(
        _single(oracle_record, "handle", "oracle record"), "oracle handle",
    )
    if oracle_handle != expected["expectedHandle"]:
        raise CandidateFieldError("LibreDWG record handle is inconsistent")

    callbacks = [row for row in result["callbacks"] if row["recordId"] == record["id"]]
    if len(callbacks) != 1:
        raise CandidateFieldError(f"{side} record callback cardinality changed")
    callback = callbacks[0]
    if (
        callback["kind"] != expected["callback"]
        or callback["ordinal"] != record["callbackOrdinal"]
        or callback["blockContextId"] != record["blockId"]
    ):
        raise CandidateFieldError(f"{side} callback/record relationship changed")

    nodes = [row for row in result["graphNodes"] if row["recordId"] == record["id"]]
    if len(nodes) != 1 or nodes[0]["handle"] != handle:
        raise CandidateFieldError(f"{side} graph record node is invalid")
    node_id = nodes[0]["id"]
    incident = [
        edge for edge in result["graphEdges"]
        if edge["from"] == node_id or edge["to"] == node_id
    ]
    if len(incident) != expected["expectedIncidentEdgeCount"]:
        raise CandidateFieldError(f"{side} graph incident-edge count changed")

    if len(callback["carrierIds"]) != 1:
        raise CandidateFieldError(f"{side} advanced record carrier count changed")
    carriers = [
        carrier for carrier in result["opaqueCarriers"]
        if carrier["id"] == callback["carrierIds"][0]
    ]
    if len(carriers) != 1:
        raise CandidateFieldError(f"{side} advanced record carrier is missing")
    carrier = carriers[0]
    if (
        carrier["recordId"] != record["id"]
        or carrier["sourceHandle"] != handle
        or carrier["disposition"] != expected["carrierDisposition"]
        or not carrier["source"].endswith("/entityProxyGraphics")
        or carrier["byteSize"] <= 0
        or HEX64.fullmatch(carrier["sha256"]) is None
    ):
        raise CandidateFieldError(f"{side} advanced record carrier is inconsistent")

    unsupported = [
        row for row in result["unsupportedContent"]
        if row["recordId"] == record["id"] and row["kind"] == "semanticFields"
    ]
    if len(unsupported) != 1 or unsupported[0]["disposition"] != "excluded" or (
        unsupported[0]["reason"] != "adapterFieldCoverageExcluded"
    ):
        raise CandidateFieldError(f"{side} semantic exclusion disposition changed")


def validate_exclusions(
    exclusions: list[dict[str, Any]], fixture_id: str,
    semantic_results: list[dict[str, Any]], oracle_records: list[Any],
) -> list[dict[str, str]]:
    verified: list[dict[str, str]] = []
    for exclusion in exclusions:
        if exclusion["fixtureId"] != fixture_id:
            continue
        oracle_record = _find_oracle_record(oracle_records, exclusion["oracleEntity"])
        for side_result in semantic_results:
            record = _find_semantic_record(
                side_result, exclusion["semanticRecordClass"],
            )
            fields = _semantic_fields(record)
            missing = sorted(set(exclusion["semanticFields"]) - set(fields))
            if missing:
                raise CandidateFieldError(
                    f"exclusion {exclusion['id']} semantic fields disappeared: {missing}"
                )
        for field in exclusion["oracleFields"]:
            # Excluded fields are presence assertions only.  LibreDWG full
            # JSON legitimately repeats some common/subclass keys (notably
            # ARCALIGNEDTEXT color); ambiguity is itself why these values
            # cannot become candidate evidence.
            occurrences = oracle_record.occurrences(field)
            if not occurrences:
                raise CandidateFieldError(
                    f"exclusion {exclusion['id']} oracle field disappeared: {field}"
                )
        verified.append({
            "id": exclusion["id"],
            "disposition": "excluded",
            "reason": exclusion["reason"],
        })
    return verified


def _oracle_environment(source: dict[str, str] | None = None) -> dict[str, str]:
    """Return a fixed environment; no loader/debug/config state is inherited."""
    inherited = os.environ if source is None else source
    # Start from a copy so removal is explicit and testable, then retain only
    # the fixed allowlist.  Absolute executable paths plus this locale,
    # timezone, and reproducible-build epoch are sufficient for the oracle.
    result = dict(inherited)
    for name in list(result):
        if (
            name not in ORACLE_ENVIRONMENT
            or name.startswith(ORACLE_FORBIDDEN_ENV_PREFIXES)
            or name in ORACLE_FORBIDDEN_ENV_NAMES
        ):
            del result[name]
    result.update(ORACLE_ENVIRONMENT)
    if any(
        name.startswith(ORACLE_FORBIDDEN_ENV_PREFIXES)
        or name in ORACLE_FORBIDDEN_ENV_NAMES
        for name in result
    ):
        raise CandidateFieldError("oracle environment allowlist is unsafe")
    return result


def _non_system_dependencies(path: Path, oracle: dict[str, Any]) -> list[Path]:
    try:
        completed = subprocess.run(
            ["/usr/bin/otool", "-L", str(path)], capture_output=True, text=True,
            check=False, timeout=10,
            env=_oracle_environment(),
        )
    except (OSError, subprocess.SubprocessError) as exc:
        raise CandidateFieldError(f"cannot inspect LibreDWG dependencies: {exc}") from exc
    if completed.returncode != 0:
        raise CandidateFieldError("otool -L failed for LibreDWG artifact")
    result: list[Path] = []
    for line in completed.stdout.splitlines()[1:]:
        token = line.strip().split(" (", 1)[0]
        if not token or any(
            token.startswith(prefix)
            for prefix in oracle["dependencyInspection"]["systemPrefixes"]
        ):
            continue
        if token.startswith("@loader_path/"):
            dependency = path.parent / token.removeprefix("@loader_path/")
        elif token.startswith("@executable_path/"):
            dependency = path.parent / token.removeprefix("@executable_path/")
        elif token.startswith("@"):
            raise CandidateFieldError(f"unresolved LibreDWG load path: {token}")
        else:
            dependency = Path(token)
        result.append(dependency.resolve())
    return result


def verify_oracle_identity(executable: Path, oracle: dict[str, Any]) -> dict[str, Any]:
    executable = executable.resolve()
    if not executable.is_file():
        raise CandidateFieldError(f"LibreDWG executable is absent: {executable}")
    if (
        platform.system() != oracle["platform"]["system"]
        or platform.machine() != oracle["platform"]["architecture"]
        or platform.mac_ver()[0] != oracle["platform"]["osVersion"]
    ):
        raise CandidateFieldError("host does not match the pinned LibreDWG profile")
    try:
        version = subprocess.run(
            [str(executable), "--version"], capture_output=True, text=True,
            check=False, timeout=10,
            env=_oracle_environment(),
        )
    except (OSError, subprocess.SubprocessError) as exc:
        raise CandidateFieldError(f"cannot identify LibreDWG: {exc}") from exc
    if version.returncode != 0 or version.stdout.strip() != oracle["versionOutput"]:
        raise CandidateFieldError("LibreDWG version output does not match the pin")

    cellar = executable.parent.parent
    formula = cellar / ".brew/libredwg.rb"
    if not formula.is_file() or _sha256_file(formula) != oracle["buildRecipe"]["formulaSha256"]:
        raise CandidateFieldError("LibreDWG Homebrew formula does not match the pin")
    sbom_path = cellar / "sbom.spdx.json"
    sbom = _read_json(sbom_path)
    packages = sbom.get("packages")
    if not isinstance(packages, list):
        raise CandidateFieldError("LibreDWG Homebrew SBOM is malformed")
    source_matches = [
        package for package in packages
        if isinstance(package, dict)
        and package.get("name") == "libredwg"
        and package.get("versionInfo") == oracle["source"]["release"]
        and package.get("downloadLocation") == oracle["source"]["url"]
        and {item.get("checksumValue") for item in package.get("checksums", [])
             if isinstance(item, dict)} == {oracle["source"]["sha256"]}
    ]
    bottle_matches = [
        package for package in packages
        if isinstance(package, dict)
        and package.get("name") == "libredwg"
        and package.get("versionInfo") == oracle["source"]["release"]
        and package.get("downloadLocation", "").endswith(
            oracle["buildRecipe"]["bottleSha256"]
        )
        and {item.get("checksumValue") for item in package.get("checksums", [])
             if isinstance(item, dict)} == {oracle["buildRecipe"]["bottleSha256"]}
    ]
    if len(source_matches) != 1 or len(bottle_matches) != 1:
        raise CandidateFieldError("LibreDWG SBOM source/bottle provenance does not match")

    queue = [executable]
    seen: set[Path] = set()
    artifacts: list[dict[str, Any]] = []
    while queue:
        path = queue.pop(0).resolve()
        if path in seen:
            continue
        seen.add(path)
        if not path.is_file():
            raise CandidateFieldError(f"LibreDWG dependency is absent: {path}")
        role = "executable" if path == executable else "library"
        artifacts.append({
            "role": role,
            "basename": path.name,
            "byteSize": path.stat().st_size,
            "sha256": _sha256_file(path),
        })
        queue.extend(_non_system_dependencies(path, oracle))
    artifacts.sort(key=lambda row: (row["role"], row["basename"], row["sha256"]))
    expected = sorted(
        oracle["artifacts"],
        key=lambda row: (row["role"], row["basename"], row["sha256"]),
    )
    if artifacts != expected:
        raise CandidateFieldError("LibreDWG transitive non-system closure changed")
    # The pinned algorithm sorts by (role, basename, sha256), then frames each
    # row as role NUL basename NUL decimal-size NUL sha256 LF.
    payload = b"".join(
        row["role"].encode("utf-8") + b"\0"
        + row["basename"].encode("utf-8") + b"\0"
        + str(row["byteSize"]).encode("ascii") + b"\0"
        + row["sha256"].encode("ascii") + b"\n"
        for row in artifacts
    )
    closure_digest = hashlib.sha256(payload).hexdigest()
    if closure_digest != oracle["closureDigest"]:
        raise CandidateFieldError("LibreDWG closure digest changed")
    return {
        "versionOutput": version.stdout.strip(),
        "sourceArchiveSha256": oracle["source"]["sha256"],
        "bottleSha256": oracle["buildRecipe"]["bottleSha256"],
        "closureDigest": closure_digest,
        "platform": oracle["platform"],
        "artifacts": artifacts,
    }


def run_oracle_twice(
    executable: Path, input_path: Path, oracle: dict[str, Any], expected_version: str,
) -> tuple[JsonObject, str]:
    command = [
        str(executable.resolve()) if token == "{executable}"
        else str(input_path.resolve()) if token == "{input}"
        else token
        for token in oracle["command"]
    ]
    outputs: list[tuple[bytes, bytes]] = []
    for _ in range(2):
        try:
            completed = subprocess.run(
                command, capture_output=True, check=False,
                timeout=float(oracle["timeoutSeconds"]),
                env=_oracle_environment(),
            )
        except (OSError, subprocess.SubprocessError) as exc:
            raise CandidateFieldError(f"LibreDWG full-JSON execution failed: {exc}") from exc
        if completed.returncode != 0 or completed.stderr.strip() != b"SUCCESS":
            raise CandidateFieldError("LibreDWG full-JSON execution was unsuccessful")
        outputs.append((completed.stdout, completed.stderr))
    if outputs[0] != outputs[1]:
        raise CandidateFieldError("LibreDWG full-JSON rerun is non-deterministic")
    payload = parse_oracle_json(outputs[0][0])
    _oracle_records(payload, expected_version)
    return payload, hashlib.sha256(outputs[0][0]).hexdigest()


def _load_differential_module() -> Any:
    tools_path = str(ROOT / "tools")
    if tools_path not in sys.path:
        sys.path.insert(0, tools_path)
    try:
        import run_qualified_differential_v2 as qualified  # type: ignore
    except ImportError as exc:
        raise CandidateFieldError(f"cannot load qualified differential v2: {exc}") from exc
    return qualified


def _validate_candidate_differential_state(
    report: dict[str, Any], fixture_id: str,
) -> None:
    counts = report["outcomeCounts"]
    reviewed_rows = [
        row for row in report["comparisons"] if row["outcome"] != "exact"
    ]
    expected_rule = EXPECTED_DIFFERENTIAL_DEBT_RULES.get(fixture_id)
    if (
        expected_rule is None
        or report["status"] != "reviewedNonPromoting"
        or counts["mismatch"] != 0
        or counts["excluded"] != 0
        or counts["toleranceNormalized"] != 0
        or counts["reviewedTargetDebt"] != 1
        or isinstance(counts["exact"], bool)
        or not isinstance(counts["exact"], int)
        or counts["exact"] <= 0
        or report["completeness"]["missingTargetCount"] != 0
        or report["completeness"]["missingStandaloneCount"] != 0
        or report["deterministicRunsPerSide"] != 2
        or report["promotesSupport"] is not False
        or report["claimEligible"] is not False
        or len(reviewed_rows) != 1
        or reviewed_rows[0]["outcome"] != "reviewedTargetDebt"
        or reviewed_rows[0]["path"] != "/diagnostics/0/messageDigest"
        or reviewed_rows[0]["ruleId"] != expected_rule
        or reviewed_rows[0]["claimEligible"] is not False
    ):
        raise CandidateFieldError(
            "qualified differential state is not the exact reviewed, "
            "non-promoting candidate-field state"
        )


def _run_semantic_pair(
    qualified: Any, root: Path, manifest_path: Path,
    target_receipt_path: Path, standalone_receipt_path: Path,
    input_git_dir: Path, fixture: dict[str, Any],
) -> tuple[dict[str, Any], dict[str, Any], dict[str, Any]]:
    manifest = qualified.validate_manifest(
        qualified._read_json(manifest_path), root=root,
    )
    manifest_digest = qualified._sha256_file(manifest_path)
    source_digest = qualified._sha256_file(root / manifest["adapterSource"]["path"])
    target_receipt = qualified.validate_receipt(
        qualified._read_json(target_receipt_path),
        receipt_path=target_receipt_path, check_artifacts=True,
    )
    standalone_receipt = qualified.validate_receipt(
        qualified._read_json(standalone_receipt_path),
        receipt_path=standalone_receipt_path, check_artifacts=True,
    )
    qualified._validate_receipt_pair(target_receipt, standalone_receipt)
    if target_receipt["targetLock"] != manifest["targetLock"]:
        raise CandidateFieldError("target receipt does not bind the differential target lock")
    if standalone_receipt["targetLock"] is not None:
        raise CandidateFieldError("standalone receipt claims target provenance")
    input_path = root / fixture["path"]
    provenance = qualified._resolve_input_provenance(
        root=root, manifest=manifest, input_path=input_path,
        input_id=fixture["id"], input_path_hint=fixture["path"],
        origin_kind="lockedRepositoryBlob",
        input_repository=fixture["repository"], input_commit=fixture["commit"],
        generator_digest=None, source_digest=source_digest,
        input_git_dir=input_git_dir,
    )
    common = {
        "manifest": manifest,
        "manifest_digest": manifest_digest,
        "source_digest": source_digest,
        "input_path": input_path,
        "input_id": fixture["id"],
        "origin_kind": "lockedRepositoryBlob",
        "input_path_hint": fixture["path"],
        "input_repository": fixture["repository"],
        "input_commit": fixture["commit"],
        "input_source_path": provenance["sourcePath"],
        "input_source_blob": provenance["sourceBlob"],
        "input_registry_digest": provenance["registryDigest"],
        "expected_detected_format": provenance["detectedFormat"],
        "expected_detected_version": provenance["detectedVersion"],
        "generator_digest": None,
        "facade": "dwgRW",
        "direction": "read",
        "timeout_seconds": 60.0,
        "expected_operation_succeeded": True,
        "output_probe": "none",
    }
    target = qualified._run_live_side(
        side="target", receipt=target_receipt,
        receipt_path=target_receipt_path, **common,
    )
    standalone = qualified._run_live_side(
        side="standalone", receipt=standalone_receipt,
        receipt_path=standalone_receipt_path, **common,
    )
    expected_options = EXPECTED_DIFFERENTIAL_OPTIONS
    expected_invocation = {
        "facade": "dwgRW",
        "direction": "read",
        "options": expected_options,
        "optionsDigest": _canonical_digest(expected_options),
    }
    if (
        target["invocation"] != expected_invocation
        or standalone["invocation"] != expected_invocation
    ):
        raise CandidateFieldError("semantic invocation options changed")
    report = qualified.compare_results(
        target, standalone, manifest["comparison"],
        manifest_digest=manifest_digest, target_lock=manifest["targetLock"],
        expected_operation_succeeded=True,
    )
    _validate_candidate_differential_state(report, fixture["id"])
    return target, standalone, report


def run_semantic_pair(
    qualified: Any, root: Path, manifest_path: Path,
    target_receipt_path: Path, standalone_receipt_path: Path,
    input_git_dir: Path, fixture: dict[str, Any],
) -> tuple[dict[str, Any], dict[str, Any], dict[str, Any]]:
    try:
        return _run_semantic_pair(
            qualified, root, manifest_path, target_receipt_path,
            standalone_receipt_path, input_git_dir, fixture,
        )
    except qualified.QualifiedDifferentialError as exc:
        raise CandidateFieldError(f"qualified differential v2 failed: {exc}") from exc


def run_live(
    *, root: Path, contract_path: Path, manifest_path: Path,
    target_receipt_path: Path, standalone_receipt_path: Path,
    input_git_dir: Path, dwgread: Path,
) -> dict[str, Any]:
    contract = validate_contract(_read_json(contract_path))
    validate_registry_bindings(contract, root)
    if manifest_path.resolve() != (root / contract["sharedDifferential"]["manifest"]).resolve():
        raise CandidateFieldError("live manifest path does not match the contract")
    oracle_identity = verify_oracle_identity(dwgread, contract["oracle"])
    qualified = _load_differential_module()
    fixture_reports: list[dict[str, Any]] = []
    for fixture in contract["fixtures"]:
        input_path = root / fixture["path"]
        if (
            not input_path.is_file()
            or input_path.stat().st_size != fixture["byteSize"]
            or _sha256_file(input_path) != fixture["sha256"]
        ):
            raise CandidateFieldError(f"admitted fixture bytes changed: {fixture['path']}")
        oracle_payload, oracle_output_digest = run_oracle_twice(
            dwgread, input_path, contract["oracle"], fixture["version"],
        )
        if oracle_output_digest != fixture["oracleJsonSha256"]:
            raise CandidateFieldError(
                f"pinned full-JSON evidence changed: {fixture['id']}"
            )
        oracle_records = _oracle_records(oracle_payload, fixture["version"])
        target, standalone, differential = run_semantic_pair(
            qualified, root, manifest_path, target_receipt_path,
            standalone_receipt_path, input_git_dir, fixture,
        )
        for side, result in (("target", target), ("standalone", standalone)):
            _validate_result_health(result, side)
            for expected in fixture["records"]:
                oracle_record = _find_oracle_record(
                    oracle_records, expected["oracleEntity"],
                )
                validate_record_assertion(expected, result, oracle_record, side)
        claim_reports: list[dict[str, Any]] = []
        for claim in contract["claims"]:
            if claim["fixtureId"] != fixture["id"]:
                continue
            oracle_record = _find_oracle_record(
                oracle_records, claim["oracle"]["entity"],
            )
            target_claim = evaluate_claim(
                claim,
                _find_semantic_record(target, claim["semantic"]["recordClass"]),
                oracle_record,
            )
            standalone_claim = evaluate_claim(
                claim,
                _find_semantic_record(standalone, claim["semantic"]["recordClass"]),
                oracle_record,
            )
            if target_claim != standalone_claim:
                raise CandidateFieldError(
                    f"claim {claim['id']} differs between target and standalone"
                )
            claim_reports.append(target_claim)
        exclusions = validate_exclusions(
            contract["exclusions"], fixture["id"],
            [target, standalone], oracle_records,
        )
        fixture_reports.append({
            "id": fixture["id"],
            "inputSha256": fixture["sha256"],
            "oracleFullJsonSha256": oracle_output_digest,
            "differentialStatus": differential["status"],
            "differentialOutcomeCounts": differential["outcomeCounts"],
            "claims": claim_reports,
            "exclusions": exclusions,
            "graphPreservationAndErrorAssertions": "verified",
        })
    return {
        "schema": 1,
        "kind": "libdxfrw-admitted-independent-candidate-field-result",
        "status": "EXPERIMENTAL",
        "promotesSupport": False,
        "contractDigest": _sha256_file(contract_path),
        "manifestDigest": _sha256_file(manifest_path),
        "oracle": oracle_identity,
        "fixtures": fixture_reports,
    }


def _synthetic_record(record_class: str, fields: dict[str, Any]) -> dict[str, Any]:
    return {
        "recordClass": record_class,
        "fields": [
            {"name": name, "type": "synthetic", "value": value}
            for name, value in fields.items()
        ],
    }


def _synthetic_assertion_result() -> tuple[dict[str, Any], dict[str, Any], JsonObject]:
    """Return one internally consistent MPOLYGON assertion triplet."""
    expected = {
        "semanticRecordClass": "MPOLYGON",
        "oracleEntity": "MPOLYGON",
        "expectedHandle": "2f",
        "callback": "addMPolygon",
        "expectedIncidentEdgeCount": 1,
        "carrierDisposition": "preserved",
    }
    result = {
        "status": {
            "outcome": "success",
            "operationSucceeded": True,
            "firstFailure": {
                "stage": None, "code": None, "path": None,
                "diagnosticId": None,
            },
        },
        "records": [{
            "id": "record:mpolygon:0",
            "recordClass": "MPOLYGON",
            "sourceHandle": "2f",
            "callbackOrdinal": 0,
            "blockId": None,
            "fields": [{"name": "handle", "type": "handle", "value": "2f"}],
        }],
        "callbacks": [{
            "id": "callback:0",
            "ordinal": 0,
            "kind": "addMPolygon",
            "recordId": "record:mpolygon:0",
            "blockContextId": None,
            "carrierIds": ["carrier:mpolygon:0"],
        }],
        "graphNodes": [{
            "id": "node:record:mpolygon:0",
            "kind": "record",
            "recordId": "record:mpolygon:0",
            "handle": "2f",
        }],
        "graphEdges": [{
            "id": "edge:mpolygon:handle",
            "kind": "handleReference",
            "from": "node:record:mpolygon:0",
            "to": "node:handle:2f",
            "disposition": "observed",
        }],
        "opaqueCarriers": [{
            "id": "carrier:mpolygon:0",
            "source": "/records/0/entityProxyGraphics",
            "byteSize": 8,
            "sha256": "0" * 64,
            "disposition": "preserved",
            "recordId": "record:mpolygon:0",
            "sourceHandle": "2f",
        }],
        "unsupportedContent": [{
            "id": "unsupported:mpolygon:0",
            "kind": "semanticFields",
            "source": "/records/0/unmodeledFields",
            "recordId": "record:mpolygon:0",
            "carrierId": None,
            "diagnosticId": None,
            "disposition": "excluded",
            "reason": "adapterFieldCoverageExcluded",
        }],
        "diagnostics": [],
    }
    oracle = JsonObject([
        ("entity", "MPOLYGON"), ("handle", [0, 1, 0x2F]),
    ])
    return expected, result, oracle


def _expect_candidate_failure(action: Any, message: str) -> None:
    try:
        action()
    except CandidateFieldError:
        return
    raise AssertionError(message)


def self_test() -> None:
    contract = validate_contract(_read_json(DEFAULT_CONTRACT))
    validate_registry_bindings(contract, ROOT)

    hostile_environment = {
        "DYLD_INSERT_LIBRARIES": "/tmp/injected.dylib",
        "LIBREDWG_TRACE": "9",
        "DWG_LOGLEVEL": "9",
        "HOME": "/tmp/hostile-home",
        "PATH": "/tmp/hostile-bin",
    }
    sanitized_environment = _oracle_environment(hostile_environment)
    if sanitized_environment != ORACLE_ENVIRONMENT or any(
        name in sanitized_environment
        for name in (
            "DYLD_INSERT_LIBRARIES", "LIBREDWG_TRACE", "DWG_LOGLEVEL", "HOME",
        )
    ):
        raise AssertionError("oracle environment inherited hostile state")

    semantic_by_class: dict[str, dict[str, Any]] = {}
    oracle_pairs_by_entity: dict[str, list[tuple[str, Any]]] = {}
    for claim in contract["claims"]:
        record_class = claim["semantic"]["recordClass"]
        semantic = semantic_by_class.setdefault(
            record_class, _synthetic_record(record_class, {}),
        )
        semantic["fields"].append({
            "name": claim["semantic"]["field"],
            "type": "synthetic",
            "value": claim["semantic"]["expected"],
        })
        entity = claim["oracle"]["entity"]
        oracle_value: Any = claim["oracle"]["expected"]
        if claim["normalization"] == "handleHex":
            handle_value = int(oracle_value, 16)
            handle_size = 0 if handle_value == 0 else (
                handle_value.bit_length() + 7
            ) // 8
            oracle_value = [0, handle_size, handle_value]
        field_parts = claim["oracle"]["field"].split(".")
        if len(field_parts) == 1:
            oracle_pairs_by_entity.setdefault(entity, []).append(
                (field_parts[0], oracle_value)
            )
        else:
            pairs = oracle_pairs_by_entity.setdefault(entity, [])
            root_name, index_text = field_parts
            existing = next((value for name, value in pairs if name == root_name), None)
            if existing is None:
                existing = [None, None, None]
                pairs.append((root_name, existing))
            existing[int(index_text)] = oracle_value
    for claim in contract["claims"]:
        semantic = semantic_by_class[claim["semantic"]["recordClass"]]
        oracle = JsonObject(oracle_pairs_by_entity[claim["oracle"]["entity"]])
        evaluated = evaluate_claim(claim, semantic, oracle)
        expected_outcome = (
            "independentlyToleranceNormalized"
            if claim["normalization"] == "floatTolerance"
            else "independentlyExact"
        )
        if evaluated["outcome"] != expected_outcome:
            raise AssertionError(f"claim outcome changed: {claim['id']}")

    float_claim = next(
        row for row in contract["claims"]
        if row["id"] == "mpolygon-base-point-z"
    )
    near_semantic = copy.deepcopy(semantic_by_class["MPOLYGON"])
    near_field = next(
        row for row in near_semantic["fields"]
        if row["name"] == float_claim["semantic"]["field"]
    )
    near_field["value"] = float(float_claim["semantic"]["expected"]) + 5e-13
    near_oracle = JsonObject(oracle_pairs_by_entity["MPOLYGON"])
    near_result = evaluate_claim(float_claim, near_semantic, near_oracle)
    if near_result["outcome"] != "independentlyToleranceNormalized":
        raise AssertionError("within-bound numeric difference was labeled exact")

    text_claim = next(row for row in contract["claims"] if row["id"] == "rtext-text")
    bad_oracle = JsonObject([
        ("entity", "RTEXT"), ("text_value", "WRONG"),
    ])
    try:
        evaluate_claim(text_claim, semantic_by_class["RTEXT"], bad_oracle)
    except CandidateFieldError:
        pass
    else:
        raise AssertionError("RTEXT identity incorrectly implied text evidence")

    duplicate_oracle = JsonObject([
        ("entity", "RTEXT"),
        ("text_value", "RTEXT-DIESEL-TEST"),
        ("text_value", "RTEXT-DIESEL-TEST"),
    ])
    try:
        evaluate_claim(text_claim, semantic_by_class["RTEXT"], duplicate_oracle)
    except CandidateFieldError:
        pass
    else:
        raise AssertionError("duplicate claimed oracle key was accepted")

    _expect_candidate_failure(
        lambda: _normalized_handle([0, 1, 0x2F, 0], "oracle handle"),
        "LibreDWG handle tuple with trailing data was accepted",
    )

    expected, valid_result, assertion_oracle = _synthetic_assertion_result()
    _validate_result_health(valid_result, "synthetic")
    validate_record_assertion(
        expected, valid_result, assertion_oracle, "synthetic",
    )

    mutated = copy.deepcopy(valid_result)
    mutated["callbacks"][0]["kind"] = "addHatch"
    _expect_candidate_failure(
        lambda: validate_record_assertion(
            expected, mutated, assertion_oracle, "synthetic",
        ),
        "wrong callback kind was accepted",
    )

    mutated = copy.deepcopy(valid_result)
    mutated["callbacks"][0]["recordId"] = "record:wrong"
    _expect_candidate_failure(
        lambda: validate_record_assertion(
            expected, mutated, assertion_oracle, "synthetic",
        ),
        "wrong callback/record relationship was accepted",
    )

    mutated = copy.deepcopy(valid_result)
    mutated["graphNodes"].append(copy.deepcopy(mutated["graphNodes"][0]))
    mutated["graphNodes"][-1]["id"] = "node:record:mpolygon:duplicate"
    _expect_candidate_failure(
        lambda: validate_record_assertion(
            expected, mutated, assertion_oracle, "synthetic",
        ),
        "duplicate graph record node was accepted",
    )

    mutated = copy.deepcopy(valid_result)
    mutated["graphEdges"].append({
        "from": "node:record:mpolygon:0", "to": "node:other",
    })
    _expect_candidate_failure(
        lambda: validate_record_assertion(
            expected, mutated, assertion_oracle, "synthetic",
        ),
        "unexpected incident graph edge was accepted",
    )

    mutated = copy.deepcopy(valid_result)
    mutated["opaqueCarriers"].clear()
    _expect_candidate_failure(
        lambda: validate_record_assertion(
            expected, mutated, assertion_oracle, "synthetic",
        ),
        "missing carrier was accepted",
    )

    mutated = copy.deepcopy(valid_result)
    mutated["opaqueCarriers"][0]["disposition"] = "dropped"
    _expect_candidate_failure(
        lambda: validate_record_assertion(
            expected, mutated, assertion_oracle, "synthetic",
        ),
        "altered carrier disposition was accepted",
    )

    mutated = copy.deepcopy(valid_result)
    mutated["diagnostics"].append({"severity": "error"})
    _expect_candidate_failure(
        lambda: _validate_result_health(mutated, "synthetic"),
        "error diagnostic was accepted",
    )

    invalid = copy.deepcopy(contract)
    invalid["claims"].append(copy.deepcopy(invalid["claims"][0]))
    invalid["claims"][-1]["id"] = "mpolygon-color"
    invalid["claims"][-1]["semantic"]["recordClass"] = "MPOLYGON"
    invalid["claims"][-1]["semantic"]["field"] = "color"
    try:
        validate_contract(invalid)
    except CandidateFieldError:
        pass
    else:
        raise AssertionError("unreviewed/color candidate field was accepted")

    invalid = copy.deepcopy(contract)
    invalid["exclusions"] = [
        row for row in invalid["exclusions"]
        if row["id"] != "arcalignedtext-payload"
    ]
    try:
        validate_contract(invalid)
    except CandidateFieldError:
        pass
    else:
        raise AssertionError("missing ARCALIGNEDTEXT payload exclusion was accepted")

    invalid = copy.deepcopy(contract)
    invalid["claims"][0]["semantic"]["field"] = "center"
    try:
        validate_contract(invalid)
    except CandidateFieldError:
        pass
    else:
        raise AssertionError("LARGE_RADIAL payload claim was accepted")

    invalid = copy.deepcopy(contract)
    invalid["fixtures"][0]["sha256"] = "0" * 64
    _expect_candidate_failure(
        lambda: validate_contract(invalid),
        "mutated exact fixture provenance was accepted",
    )

    for altered_tolerance in (0, 1e-15, 1e-9):
        invalid = copy.deepcopy(contract)
        float_row = next(
            row for row in invalid["claims"]
            if row["normalization"] == "floatTolerance"
        )
        float_row["tolerance"]["absolute"] = altered_tolerance
        _expect_candidate_failure(
            lambda invalid=invalid: validate_contract(invalid),
            f"altered float tolerance {altered_tolerance} was accepted",
        )

    invalid = copy.deepcopy(contract)
    color_exclusion = next(
        row for row in invalid["exclusions"] if row["id"] == "mpolygon-color"
    )
    color_exclusion["semanticFields"].pop()
    _expect_candidate_failure(
        lambda: validate_contract(invalid),
        "shrunken exclusion field inventory was accepted",
    )

    invalid = copy.deepcopy(contract)
    color_exclusion = next(
        row for row in invalid["exclusions"] if row["id"] == "mpolygon-color"
    )
    color_exclusion["oracleFields"].append("zzUnexpected")
    _expect_candidate_failure(
        lambda: validate_contract(invalid),
        "expanded exclusion field inventory was accepted",
    )

    frozen_differential = {
        "status": "reviewedNonPromoting",
        "outcomeCounts": {
            "exact": 1,
            "excluded": 0,
            "mismatch": 0,
            "reviewedTargetDebt": 1,
            "toleranceNormalized": 0,
        },
        "completeness": {
            "missingTargetCount": 0,
            "missingStandaloneCount": 0,
        },
        "deterministicRunsPerSide": 2,
        "promotesSupport": False,
        "claimEligible": False,
        "comparisons": [{
            "outcome": "reviewedTargetDebt",
            "path": "/diagnostics/0/messageDigest",
            "ruleId": "debt-s386-classes-crc-large-radial",
            "claimEligible": False,
        }],
    }
    _validate_candidate_differential_state(
        frozen_differential, "fixture:large_radial",
    )
    for outcome in ("excluded", "mismatch", "toleranceNormalized"):
        altered = copy.deepcopy(frozen_differential)
        altered["outcomeCounts"][outcome] = 1
        _expect_candidate_failure(
            lambda altered=altered: _validate_candidate_differential_state(
                altered, "fixture:large_radial",
            ),
            f"unexpected differential {outcome} was accepted",
        )
    for key, value in (
        ("path", "/graphEdges/0/disposition"),
        ("ruleId", "debt-wrong-rule"),
    ):
        altered = copy.deepcopy(frozen_differential)
        altered["comparisons"][0][key] = value
        _expect_candidate_failure(
            lambda altered=altered: _validate_candidate_differential_state(
                altered, "fixture:large_radial",
            ),
            f"wrong reviewed-debt {key} was accepted",
        )

    try:
        parse_oracle_json(b'{"value":NaN}')
    except CandidateFieldError:
        pass
    else:
        raise AssertionError("non-finite oracle JSON was accepted")
    print("check_admitted_candidate_fields self-test: PASS")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--root", type=Path, default=ROOT)
    parser.add_argument("--contract", type=Path, default=DEFAULT_CONTRACT)
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument("--target-receipt", type=Path)
    parser.add_argument("--standalone-receipt", type=Path)
    parser.add_argument("--input-git-dir", type=Path)
    parser.add_argument("--dwgread", type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args(argv)
    try:
        if args.self_test:
            self_test()
            return 0
        required = {
            "--target-receipt": args.target_receipt,
            "--standalone-receipt": args.standalone_receipt,
            "--input-git-dir": args.input_git_dir,
            "--dwgread": args.dwgread,
        }
        missing = [name for name, value in required.items() if value is None]
        if missing:
            parser.error("live mode requires " + ", ".join(missing))
        # Keep the malformed-contract/field negatives in the single focused
        # live gate so qualification does not need another CTest entry.
        self_test()
        report = run_live(
            root=args.root.resolve(), contract_path=args.contract.resolve(),
            manifest_path=args.manifest.resolve(),
            target_receipt_path=args.target_receipt.resolve(),
            standalone_receipt_path=args.standalone_receipt.resolve(),
            input_git_dir=args.input_git_dir.resolve(),
            dwgread=args.dwgread.resolve(),
        )
        encoded = json.dumps(report, sort_keys=True, indent=2, allow_nan=False) + "\n"
        if args.output is None:
            sys.stdout.write(encoded)
        else:
            args.output.write_text(encoded, encoding="utf-8")
            print("admitted independent candidate fields: PASS")
        return 0
    except (CandidateFieldError, AssertionError, OSError, UnicodeError) as exc:
        print(f"admitted independent candidate fields: FAIL: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
