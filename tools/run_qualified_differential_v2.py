#!/usr/bin/env python3
"""Validate adapter result schemas 2/3 and normalized differential report v2.

This gate deliberately separates three things which older advisory reports
mixed together:

* an adapter result has an exact, typed semantic schema;
* a build receipt establishes which source, package, binary, static library,
  configuration, and linked-library closure produced that result; and
* a differential classifies every comparable value without treating reviewed
  debt or exclusions as support evidence.

No drawing bytes are copied into a report.  Live runs accept only an admitted
repository blob or a local-from-scratch input and execute each adapter twice;
the two stdout byte streams must be identical before either is parsed.
"""

from __future__ import annotations

import argparse
import copy
import fnmatch
import hashlib
import inspect
import json
import math
import os
import re
import stat
import subprocess
import sys
import tempfile
from collections import Counter
from pathlib import Path, PurePosixPath
from typing import Any, Iterable


ADAPTER_RESULT_SCHEMA_V2 = 2
ADAPTER_RESULT_SCHEMA_V3 = 3
MANIFEST_SCHEMA_V2 = 2
MANIFEST_SCHEMA_V3 = 3
NORMALIZATION_REPORT_SCHEMA = 2
RESULT_KIND = "libdxfrw-semantic-adapter-result"
REPORT_KIND = "libdxfrw-qualified-differential-report"
MANIFEST_KIND = "libdxfrw-qualified-differential-runners"
RECEIPT_SCHEMA = 1
RECEIPT_KIND = "libdxfrw-semantic-adapter-build-receipt"
FIXTURE_POLICY = (
    "lockedRepositoryBlob-or-localFromScratch; no-drawing-payloads-in-reports"
)

SHA256_RE = re.compile(r"[0-9a-f]{64}")
COMMIT_RE = re.compile(r"[0-9a-f]{40}")
ID_RE = re.compile(r"[A-Za-z0-9][A-Za-z0-9_.:/@+-]{0,255}")
HANDLE_RE = re.compile(r"(?:0|[1-9a-f][0-9a-f]*)")

ORIGIN_KINDS = {"lockedRepositoryBlob", "localFromScratch"}
SIDES = {"target", "standalone"}
FACADES = {"dxfRW", "dwgRW"}
DIRECTIONS = {"read", "write"}
STATUS_OUTCOMES = {"success", "failure"}
FIELD_TYPES = {
    "bool",
    "int64",
    "uint64",
    "double",
    "string",
    "handle",
    "point3d",
}
RECORD_KINDS = {"entity", "table", "block", "unsupported", "artifact"}
GRAPH_NODE_KINDS = {"record", "block", "handle"}
GRAPH_EDGE_KINDS = {
    "handleReference", "historyReference", "owner", "blockMembership",
}
GRAPH_EDGE_DISPOSITIONS = {"observed", "resolved", "unresolved"}
OPAQUE_DISPOSITIONS = {"observed", "preserved", "preservedCanonical"}
UNSUPPORTED_DISPOSITIONS = {"excluded", "preserved", "preservedCanonical"}
DIAGNOSTIC_SEVERITIES = {"warning", "error"}
DIAGNOSTIC_STAGES = {
    "open", "version", "metadata", "fileHeader", "header", "handles",
    "classes", "tables", "blocks", "entities", "objects", "section",
    "parseCode", "unknown",
    "readback", "write", "writeResult", "callback", "integrity",
}
VALUE_OUTCOMES = {
    "exact",
    "toleranceNormalized",
    "reviewedTargetDebt",
    "excluded",
    "mismatch",
}

ROOT_KEYS_V2 = {
    "schema",
    "kind",
    "input",
    "adapter",
    "invocation",
    "status",
    "callbacks",
    "records",
    "graphNodes",
    "graphEdges",
    "opaqueCarriers",
    "unsupportedContent",
    "diagnostics",
    "cardinality",
}
ROOT_KEYS_V3 = ROOT_KEYS_V2 | {"observations"}
INPUT_KEYS = {
    "id",
    "pathHint",
    "originKind",
    "repository",
    "commit",
    "sourcePath",
    "sourceBlob",
    "registryDigest",
    "generatorDigest",
    "sha256",
    "byteSize",
    "detectedFormat",
    "detectedVersion",
}
ADAPTER_KEYS = {
    "name",
    "side",
    "package",
    "commit",
    "sourceDigest",
    "configDigest",
    "staticLibraryDigest",
    "binaryDigest",
    "linkedClosureDigest",
}
INVOCATION_KEYS = {"facade", "direction", "options", "optionsDigest"}
INVOCATION_OPTION_KEYS = {"applyExtrusion", "compatibilityProfile"}
STATUS_KEYS = {"outcome", "operationSucceeded", "exitCode", "firstFailure"}
FIRST_FAILURE_KEYS = {"stage", "code", "path", "diagnosticId"}
CALLBACK_KEYS = {
    "id", "ordinal", "kind", "recordId", "blockContextId", "carrierIds"
}
RECORD_KEYS = {
    "id",
    "callbackOrdinal",
    "kind",
    "recordClass",
    "entity",
    "sourceHandle",
    "ownerHandle",
    "blockId",
    "fields",
}
FIELD_KEYS = {"name", "type", "value"}
GRAPH_NODE_KEYS = {"id", "kind", "recordId", "handle"}
GRAPH_EDGE_KEYS = {"id", "kind", "from", "to", "disposition"}
OPAQUE_KEYS = {
    "id",
    "source",
    "byteSize",
    "sha256",
    "disposition",
    "recordId",
    "sourceHandle",
}
UNSUPPORTED_KEYS = {
    "id", "kind", "source", "recordId", "carrierId", "diagnosticId",
    "disposition", "reason"
}
DIAGNOSTIC_KEYS = {
    "id", "ordinal", "severity", "stage", "code", "path", "messageDigest"
}
CARDINALITY_KEYS = {
    "callbackCount",
    "recordCount",
    "fieldCount",
    "graphNodeCount",
    "graphEdgeCount",
    "opaqueCarrierCount",
    "unsupportedContentCount",
    "diagnosticCount",
}


class QualifiedDifferentialError(ValueError):
    """Raised when evidence is incomplete, malformed, or non-deterministic."""


def _loads_json(text: str, label: str) -> Any:
    def reject_duplicates(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
        result: dict[str, Any] = {}
        for key, value in pairs:
            if key in result:
                raise QualifiedDifferentialError(
                    f"duplicate JSON key {key!r} in {label}"
                )
            result[key] = value
        return result

    try:
        return json.loads(
            text, object_pairs_hook=reject_duplicates,
            parse_constant=lambda token: (_ for _ in ()).throw(
                QualifiedDifferentialError(
                    f"non-finite JSON token {token!r} in {label}"
                )
            ),
        )
    except json.JSONDecodeError as exc:
        raise QualifiedDifferentialError(f"invalid JSON in {label}: {exc}") from exc


def _read_json(path: Path) -> Any:
    try:
        return _loads_json(path.read_text(encoding="utf-8"), str(path))
    except (OSError, UnicodeError, json.JSONDecodeError) as exc:
        raise QualifiedDifferentialError(f"cannot read JSON {path}: {exc}") from exc


def _canonical_bytes(value: Any) -> bytes:
    return json.dumps(
        value, sort_keys=True, separators=(",", ":"), ensure_ascii=False,
        allow_nan=False,
    ).encode("utf-8")


def _value_digest(value: Any) -> str:
    return hashlib.sha256(_canonical_bytes(value)).hexdigest()


def _sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    try:
        with path.open("rb") as stream:
            for chunk in iter(lambda: stream.read(1024 * 1024), b""):
                digest.update(chunk)
    except OSError as exc:
        raise QualifiedDifferentialError(f"cannot hash {path}: {exc}") from exc
    return digest.hexdigest()


def _exact_keys(value: Any, expected: set[str], path: str) -> dict[str, Any]:
    if not isinstance(value, dict):
        raise QualifiedDifferentialError(f"{path} must be an object")
    actual = set(value)
    if actual != expected:
        missing = sorted(expected - actual)
        extra = sorted(actual - expected)
        raise QualifiedDifferentialError(
            f"{path} keys differ (missing={missing}, extra={extra})"
        )
    return value


def _validate_compile_contract_disposition(inputs: dict[str, Any]) -> str:
    """Preserve frozen receipts while keeping new direct-only evidence advisory."""
    mode = inputs.get("compileContractMode")
    if mode is None:
        if inputs.get("compileContractEnforced") is not True:
            raise QualifiedDifferentialError(
                "legacy adapter compile contract was not enforced"
            )
        return "legacy"
    if mode == "direct-options-only":
        if (inputs.get("compileOptionAdmissionEnforced") is not True
                or inputs.get("compileContractEnforced") is not False):
            raise QualifiedDifferentialError(
                "direct-only compile disposition is internally inconsistent"
            )
        profile = inputs.get("compileContractProfile")
        if isinstance(profile, dict) and profile.get("disposition") == "enforced":
            raise QualifiedDifferentialError(
                "direct-only compile contract cannot carry an enforced profile"
            )
        return mode
    if mode == "controlled-driver-profile-v1":
        profile = inputs.get("compileContractProfile")
        if (inputs.get("compileOptionAdmissionEnforced") is not True
                or inputs.get("compileContractEnforced") is not True
                or not isinstance(profile, dict)
                or profile.get("disposition") != "enforced"
                or profile.get("mode") != "controlled-fresh-build-v1"):
            raise QualifiedDifferentialError(
                "controlled compile contract lacks an enforced fresh-build profile"
            )
        return mode
    raise QualifiedDifferentialError("adapter compile-contract mode is unsupported")


def _validate_controlled_driver_context(value: Any, path: str) -> None:
    if (isinstance(value, dict)
            and set(value) == LEGACY_CONTROLLED_DRIVER_CONTEXT_KEYS):
        context = _exact_keys(value, LEGACY_CONTROLLED_DRIVER_CONTEXT_KEYS, path)
    else:
        context = _exact_keys(value, CONTROLLED_DRIVER_CONTEXT_KEYS, path)
    if isinstance(context["schema"], bool) or context["schema"] != 1:
        raise QualifiedDifferentialError(f"{path}.schema is unsupported")
    if context["mode"] != "controlled-driver-v1":
        raise QualifiedDifferentialError(f"{path}.mode is unsupported")
    _digest(context["environmentDigest"], f"{path}.environmentDigest")
    _string(context["compilerPath"], f"{path}.compilerPath")
    _digest(context["compilerPathDigest"], f"{path}.compilerPathDigest")
    _digest(context["compilerBinaryDigest"], f"{path}.compilerBinaryDigest")
    _digest(context["digest"], f"{path}.digest")
    expected_path_digest = hashlib.sha256(
        context["compilerPath"].encode("utf-8")
    ).hexdigest()
    if context["compilerPathDigest"] != expected_path_digest:
        raise QualifiedDifferentialError(f"{path}.compilerPathDigest is stale")
    if "cmakePath" in context:
        _string(context["cmakePath"], f"{path}.cmakePath")
        _digest(context["cmakePathDigest"], f"{path}.cmakePathDigest")
        _digest(context["cmakeBinaryDigest"], f"{path}.cmakeBinaryDigest")
        expected_cmake_path_digest = hashlib.sha256(
            context["cmakePath"].encode("utf-8")
        ).hexdigest()
        if context["cmakePathDigest"] != expected_cmake_path_digest:
            raise QualifiedDifferentialError(f"{path}.cmakePathDigest is stale")
    payload = {key: item for key, item in context.items() if key != "digest"}
    expected_digest = hashlib.sha256(_canonical_bytes(payload) + b"\n").hexdigest()
    if context["digest"] != expected_digest:
        raise QualifiedDifferentialError(f"{path}.digest is stale")


def _validate_compile_contract_profile(
    value: Any,
    *,
    driver_context_present: bool,
    compiler_id: str,
    path: str,
) -> dict[str, Any]:
    profile = _exact_keys(value, COMPILE_CONTRACT_PROFILE_KEYS, path)
    if isinstance(profile["schema"], bool) or profile["schema"] != 1:
        raise QualifiedDifferentialError(f"{path}.schema is unsupported")
    if profile["name"] != "effective-driver-contract-v1":
        raise QualifiedDifferentialError(f"{path}.name is unsupported")
    if profile["mode"] not in {
        "controlled-fresh-build-v1", "existing-build-inspection-v1",
    }:
        raise QualifiedDifferentialError(f"{path}.mode is unsupported")
    if profile["disposition"] not in {"enforced", "advisory", "unavailable"}:
        raise QualifiedDifferentialError(f"{path}.disposition is unsupported")
    checks = _exact_keys(profile["checks"], COMPILE_CONTRACT_CHECK_KEYS,
                         f"{path}.checks")
    statuses = {"pass", "partial", "pending", "unavailable", "not-applicable"}
    for name, status in checks.items():
        if status not in statuses:
            raise QualifiedDifferentialError(f"{path}.checks.{name} is unsupported")
    if checks["directOptions"] != "pass":
        raise QualifiedDifferentialError(f"{path}.checks.directOptions is not verified")
    clang = compiler_id in {"Clang", "AppleClang"}
    expected_clang_status = (
        "pass" if driver_context_present
        else "unavailable"
    ) if clang else "not-applicable"
    if checks["clangDefaultConfig"] != expected_clang_status:
        raise QualifiedDifferentialError(
            f"{path}.checks.clangDefaultConfig contradicts compiler/context identity"
        )
    if profile["mode"] == "controlled-fresh-build-v1":
        if not driver_context_present:
            raise QualifiedDifferentialError(
                f"{path}.mode requires a controlled driver context"
            )
        for name in ("driverIdentity", "freshBuild", "ambientOverrides"):
            if checks[name] != "pass":
                raise QualifiedDifferentialError(
                    f"{path}.checks.{name} is not verified for a controlled build"
                )
        if checks["implicitConfiguration"] not in {"partial", "pass"}:
            raise QualifiedDifferentialError(
                f"{path}.checks.implicitConfiguration is inconsistent"
            )
        if profile["disposition"] == "advisory":
            if (checks["effectiveOptionPolicy"] == "pass"
                    and checks["implicitConfiguration"] == "pass"):
                raise QualifiedDifferentialError(
                    f"{path}.disposition is advisory despite all compile checks passing"
                )
        elif profile["disposition"] == "enforced":
            if any(status not in {"pass", "not-applicable"}
                   for status in checks.values()):
                raise QualifiedDifferentialError(
                    f"{path}.disposition is enforced with incomplete compile checks"
                )
        else:
            raise QualifiedDifferentialError(
                f"{path}.disposition is unavailable for a controlled fresh build"
            )
    else:
        if driver_context_present:
            raise QualifiedDifferentialError(
                f"{path}.mode cannot carry a controlled driver context"
            )
        if profile["disposition"] != "unavailable":
            raise QualifiedDifferentialError(
                f"{path}.disposition must be unavailable for an inspected build"
            )
        for name in (
            "driverIdentity", "freshBuild", "ambientOverrides",
            "implicitConfiguration",
        ):
            if checks[name] != "unavailable":
                raise QualifiedDifferentialError(
                    f"{path}.checks.{name} overstates inspected-build evidence"
                )
        if checks["effectiveOptionPolicy"] != "pending":
            raise QualifiedDifferentialError(
                f"{path}.checks.effectiveOptionPolicy overstates inspected-build evidence"
            )
    _digest(profile["digest"], f"{path}.digest")
    payload = {key: item for key, item in profile.items() if key != "digest"}
    expected_digest = hashlib.sha256(_canonical_bytes(payload) + b"\n").hexdigest()
    if profile["digest"] != expected_digest:
        raise QualifiedDifferentialError(f"{path}.digest is stale")
    return profile


def _validate_interface_profile(value: Any, side: str, path: str) -> dict[str, Any]:
    profile = _exact_keys(value, INTERFACE_PROFILE_KEYS, path)
    if isinstance(profile["schema"], bool) or profile["schema"] != 1:
        raise QualifiedDifferentialError(f"{path}.schema is unsupported")
    profile_name = profile["name"]
    if profile_name not in STANDALONE_INTERFACE_PROFILE_INVENTORIES:
        raise QualifiedDifferentialError(f"{path}.name is unsupported")
    if profile["side"] != side:
        raise QualifiedDifferentialError(f"{path}.side does not match adapter side")
    if _integer(profile["commonMethodCount"], f"{path}.commonMethodCount",
                minimum=1) != EXPECTED_INTERFACE_METHOD_COUNT:
        raise QualifiedDifferentialError(f"{path}.commonMethodCount is stale")
    _digest(profile["commonMethodDigest"], f"{path}.commonMethodDigest")
    if profile["commonMethodDigest"] != EXPECTED_COMMON_INTERFACE_DIGEST:
        raise QualifiedDifferentialError(f"{path}.commonMethodDigest is stale")
    profile_inventory = STANDALONE_INTERFACE_PROFILE_INVENTORIES[profile_name]
    expected_interface_count = (
        170 if side == "target"
        else profile_inventory["methodCount"] if side == "standalone"
        else -1
    )
    if _integer(profile["interfaceMethodCount"], f"{path}.interfaceMethodCount",
                minimum=1) != expected_interface_count:
        raise QualifiedDifferentialError(f"{path}.interfaceMethodCount is stale")
    _digest(profile["interfaceMethodDigest"], f"{path}.interfaceMethodDigest")
    extras = _list(profile["defaultedInterfaceExtras"],
                   f"{path}.defaultedInterfaceExtras")
    for index, item in enumerate(extras):
        _identifier(item, f"{path}.defaultedInterfaceExtras[{index}]")
    expected_extras = (
        profile_inventory["extras"] if side == "standalone" else []
    )
    if extras != expected_extras:
        raise QualifiedDifferentialError(f"{path}.defaultedInterfaceExtras are stale")
    adapter_extras = _list(profile["adapterSupplementalMethods"],
                           f"{path}.adapterSupplementalMethods")
    for index, item in enumerate(adapter_extras):
        _identifier(item, f"{path}.adapterSupplementalMethods[{index}]")
    if adapter_extras != expected_extras:
        raise QualifiedDifferentialError(
            f"{path}.adapterSupplementalMethods are incomplete"
        )
    unobserved = _list(profile["unobservedDefaultedExtras"],
                       f"{path}.unobservedDefaultedExtras")
    for index, item in enumerate(unobserved):
        _identifier(item, f"{path}.unobservedDefaultedExtras[{index}]")
    if unobserved:
        raise QualifiedDifferentialError(f"{path}.unobservedDefaultedExtras are stale")
    return profile


def _validate_semantic_capability_profile(
    value: Any, side: str, path: str,
) -> dict[str, Any]:
    profile = _exact_keys(value, CAPABILITY_PROFILE_KEYS, path)
    if type(profile["schema"]) is not int or profile["schema"] != 1:
        raise QualifiedDifferentialError(f"{path}.schema is unsupported")
    if profile["name"] != EXPECTED_CAPABILITY_PROFILE_NAME:
        raise QualifiedDifferentialError(f"{path}.name is unsupported")
    if profile["side"] != side or side not in EXPECTED_CAPABILITIES_BY_SIDE:
        raise QualifiedDifferentialError(f"{path}.side does not match adapter side")
    capabilities = _exact_keys(
        profile["capabilities"], CAPABILITY_KEYS, f"{path}.capabilities"
    )
    expected = EXPECTED_CAPABILITIES_BY_SIDE[side]
    for name in sorted(CAPABILITY_KEYS):
        if type(capabilities[name]) is not bool:
            raise QualifiedDifferentialError(
                f"{path}.capabilities.{name} must be boolean"
            )
        if capabilities[name] is not expected[name]:
            raise QualifiedDifferentialError(
                f"{path}.capabilities.{name} contradicts the reviewed {side} profile"
            )
    return profile


SURFACE_RECORD_CLASSES = {
    "SURFACE", "PLANESURFACE", "EXTRUDEDSURFACE", "REVOLVEDSURFACE",
    "SWEPTSURFACE", "LOFTEDSURFACE", "NURBSURFACE",
}


def _validate_adapter_field_capabilities(
    records: list[dict[str, Any]], unsupported: list[dict[str, Any]], side: str,
) -> None:
    """Bind schema-3 optional modeler fields to the reviewed adapter profile."""

    if side not in EXPECTED_CAPABILITIES_BY_SIDE:
        raise QualifiedDifferentialError(
            "semantic adapter side has no reviewed field-capability profile"
        )
    capabilities = EXPECTED_CAPABILITIES_BY_SIDE[side]

    def check(record: dict[str, Any], feature: str,
              expected_fields: dict[str, str], excluded_name: str) -> None:
        record_id = record["id"]
        fields = {field["name"]: field for field in record["fields"]}
        available = capabilities[feature]
        for name, expected_type in expected_fields.items():
            field = fields.get(name)
            if available:
                if field is None or field["type"] != expected_type:
                    raise QualifiedDifferentialError(
                        f"record {record_id} is missing available {feature} field {name}"
                    )
            elif field is not None:
                raise QualifiedDifferentialError(
                    f"record {record_id} fabricated unavailable {feature} field {name}"
                )

        source = (
            f"/records/{_pointer_token(record_id)}/"
            f"{_pointer_token(excluded_name)}"
        )
        matching = [
            row for row in unsupported
            if row["recordId"] == record_id and row["source"] == source
        ]
        if available:
            if matching:
                raise QualifiedDifferentialError(
                    f"record {record_id} excludes available {feature} field"
                )
        elif (len(matching) != 1
              or matching[0]["kind"] != "semanticFields"
              or matching[0]["disposition"] != "excluded"
              or matching[0]["reason"] != "adapterFieldCoverageExcluded"):
            raise QualifiedDifferentialError(
                f"record {record_id} lacks the exact exclusion for unavailable {feature}"
            )

    for record in records:
        if record["kind"] != "entity":
            continue
        if record["recordClass"] == "MODELER_GEOMETRY":
            check(record, "modelerDwgSourceVersion",
                  {"dwgSourceVersion": "int64"}, "dwgSourceVersion")
            check(record, "modelerPayloadStatusName",
                  {"modelerPayloadStatus": "string"}, "modelerPayloadStatus")
            check(record, "modelerDwgAcisPayload", {
                "hasDwgInlineAcisPayload": "bool",
                "dwgInlineAcisByteCount": "uint64",
            }, "dwgInlineAcisPayload")
            check(record, "modelerDxfPayloadChunks",
                  {"dxfPayloadChunk.count": "uint64"}, "dxfPayloadChunks")

            range_count_field = next(
                (field for field in record["fields"]
                 if field["name"] == "payloadRangeCount"),
                None,
            )
            if (range_count_field is None
                    or range_count_field["type"] != "uint64"
                    or not isinstance(range_count_field["value"], str)
                    or not range_count_field["value"].isdigit()):
                raise QualifiedDifferentialError(
                    f"record {record['id']} has an invalid payloadRangeCount"
                )
            range_count = int(range_count_field["value"])
            if range_count > len(record["fields"]):
                raise QualifiedDifferentialError(
                    f"record {record['id']} payloadRangeCount exceeds its field denominator"
                )
            expected_offset_names = {
                f"payloadRange.{index}.bitOffset"
                for index in range(range_count)
            }
            actual_offset_names = {
                field["name"] for field in record["fields"]
                if re.fullmatch(r"payloadRange\.\d+\.bitOffset", field["name"])
            }
            if (capabilities["payloadRangeBitOffset"]
                    and actual_offset_names != expected_offset_names):
                raise QualifiedDifferentialError(
                    f"record {record['id']} has out-of-range payload bit-offset fields"
                )
            if (not capabilities["payloadRangeBitOffset"]
                    and actual_offset_names):
                raise QualifiedDifferentialError(
                    f"record {record['id']} fabricated unavailable payload bit-offset fields"
                )
            expected_offset_sources = {
                f"/records/{_pointer_token(record['id'])}/"
                f"{_pointer_token(name)}"
                for name in expected_offset_names
            }
            offset_exclusion_rows = [
                row for row in unsupported
                if row["recordId"] == record["id"]
                and re.fullmatch(
                    rf"/records/{re.escape(_pointer_token(record['id']))}/"
                    r"payloadRange\.\d+\.bitOffset",
                    row["source"],
                )
            ]
            actual_offset_sources = {
                row["source"] for row in offset_exclusion_rows
            }
            if (capabilities["payloadRangeBitOffset"]
                    and offset_exclusion_rows):
                raise QualifiedDifferentialError(
                    f"record {record['id']} excludes available payload bit offsets"
                )
            if (not capabilities["payloadRangeBitOffset"]
                    and (len(offset_exclusion_rows) != range_count
                         or actual_offset_sources != expected_offset_sources)):
                raise QualifiedDifferentialError(
                    f"record {record['id']} has out-of-range payload bit-offset exclusions"
                )
            for index in range(range_count):
                name = f"payloadRange.{index}.bitOffset"
                check(record, "payloadRangeBitOffset",
                      {name: "uint64"}, name)
        elif record["recordClass"] in SURFACE_RECORD_CLASSES:
            check(record, "surfaceDxfPayloadChunks",
                  {"dxfAcisChunk.count": "uint64"}, "dxfAcisChunks")


def _validate_adapter_support_inputs(value: Any, path: str) -> list[dict[str, str]]:
    rows = _list(value, path)
    logical_paths: list[str] = []
    snapshot_paths: list[str] = []
    for index, item in enumerate(rows):
        row_path = f"{path}[{index}]"
        row = _exact_keys(item, ADAPTER_SUPPORT_INPUT_KEYS, row_path)
        logical_paths.append(_path_hint(row["path"], f"{row_path}.path"))
        snapshot_paths.append(
            _path_hint(row["snapshotPath"], f"{row_path}.snapshotPath")
        )
        _digest(row["digest"], f"{row_path}.digest")
    _unique(logical_paths, f"{path} logical paths")
    _unique(snapshot_paths, f"{path} snapshot paths")
    return rows


def _without_adapter_support_inputs(value: Any, path: str) -> Any:
    """Validate additive modern support-input evidence without loosening old schemas."""
    if not isinstance(value, dict) or "adapterSupportInputs" not in value:
        return value
    _validate_adapter_support_inputs(
        value["adapterSupportInputs"], f"{path}.adapterSupportInputs"
    )
    base = {key: item for key, item in value.items()
            if key != "adapterSupportInputs"}
    if frozenset(base) not in MODERN_CONFIG_INPUT_KEYSETS:
        raise QualifiedDifferentialError(
            f"{path} adapterSupportInputs is unsupported for this config identity shape"
        )
    return base


def _list(value: Any, path: str) -> list[Any]:
    if not isinstance(value, list):
        raise QualifiedDifferentialError(f"{path} must be an array")
    return value


def _string(value: Any, path: str, *, allow_empty: bool = False) -> str:
    if not isinstance(value, str) or (not allow_empty and not value):
        raise QualifiedDifferentialError(f"{path} must be a non-empty string")
    if "\x00" in value:
        raise QualifiedDifferentialError(f"{path} contains NUL")
    return value


def _optional_string(value: Any, path: str) -> str | None:
    if value is None:
        return None
    return _string(value, path)


def _identifier(value: Any, path: str) -> str:
    text = _string(value, path)
    if not ID_RE.fullmatch(text):
        raise QualifiedDifferentialError(f"{path} is not a stable identifier")
    return text


def _digest(value: Any, path: str) -> str:
    text = _string(value, path)
    if not SHA256_RE.fullmatch(text):
        raise QualifiedDifferentialError(f"{path} must be a lowercase SHA-256")
    return text


def _optional_digest(value: Any, path: str) -> str | None:
    if value is None:
        return None
    return _digest(value, path)


def _commit(value: Any, path: str) -> str:
    text = _string(value, path)
    if not COMMIT_RE.fullmatch(text):
        raise QualifiedDifferentialError(f"{path} must be a full lowercase commit")
    return text


def _integer(value: Any, path: str, *, minimum: int | None = None) -> int:
    if isinstance(value, bool) or not isinstance(value, int):
        raise QualifiedDifferentialError(f"{path} must be an integer")
    if minimum is not None and value < minimum:
        raise QualifiedDifferentialError(f"{path} must be >= {minimum}")
    return value


def _boolean(value: Any, path: str) -> bool:
    if not isinstance(value, bool):
        raise QualifiedDifferentialError(f"{path} must be boolean")
    return value


def _finite_number(value: Any, path: str) -> int | float:
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        raise QualifiedDifferentialError(f"{path} must be numeric")
    if isinstance(value, float) and not math.isfinite(value):
        raise QualifiedDifferentialError(f"{path} must be finite")
    return value


def _nullable_identifier(value: Any, path: str) -> str | None:
    if value is None:
        return None
    return _identifier(value, path)


def _handle(value: Any, path: str) -> str:
    text = _string(value, path)
    if not HANDLE_RE.fullmatch(text):
        raise QualifiedDifferentialError(f"{path} must be canonical lowercase hex")
    return text


def _optional_handle(value: Any, path: str) -> str | None:
    if value is None:
        return None
    return _handle(value, path)


def _path_hint(value: Any, path: str) -> str:
    text = _string(value, path)
    if "\\" in text or re.match(r"^[A-Za-z]:", text):
        raise QualifiedDifferentialError(f"{path} must use logical POSIX form")
    logical = PurePosixPath(text)
    if logical.is_absolute() or any(part in {"", ".", ".."} for part in logical.parts):
        raise QualifiedDifferentialError(f"{path} must be a normalized relative hint")
    if logical.as_posix() != text:
        raise QualifiedDifferentialError(f"{path} is not normalized")
    return text


SEMANTIC_PATH_ROOTS = {
    "input", "output", "status", "callbacks", "records", "graphNodes", "graphEdges",
    "opaqueCarriers", "unsupportedContent", "diagnostics",
}


def _semantic_path(value: Any, path: str) -> str:
    text = _string(value, path)
    if "\\" in text or not text.startswith("/"):
        raise QualifiedDifferentialError(f"{path} must be a semantic JSON pointer")
    root = text[1:].split("/", 1)[0]
    if root not in SEMANTIC_PATH_ROOTS or ".." in text.split("/"):
        raise QualifiedDifferentialError(f"{path} has a non-semantic path root")
    return text


def _unique(values: Iterable[str], path: str) -> set[str]:
    items = list(values)
    if len(items) != len(set(items)):
        raise QualifiedDifferentialError(f"{path} must be unique")
    return set(items)


def _validate_input(value: Any) -> dict[str, Any]:
    row = _exact_keys(value, INPUT_KEYS, "input")
    _identifier(row["id"], "input.id")
    _path_hint(row["pathHint"], "input.pathHint")
    if row["originKind"] not in ORIGIN_KINDS:
        raise QualifiedDifferentialError("input.originKind is not admitted")
    _digest(row["sha256"], "input.sha256")
    _integer(row["byteSize"], "input.byteSize", minimum=0)
    if row["detectedFormat"] not in {
        "dwg", "dxf-ascii", "dxf-binary", "recipe-json", "unknown"
    }:
        raise QualifiedDifferentialError("input.detectedFormat is invalid")
    version = _string(row["detectedVersion"], "input.detectedVersion")
    if version != "unknown" and not re.fullmatch(r"AC\d{4}", version):
        raise QualifiedDifferentialError("input.detectedVersion is invalid")
    repository = _optional_string(row["repository"], "input.repository")
    commit = row["commit"]
    source_path = row["sourcePath"]
    source_blob = row["sourceBlob"]
    registry_digest = row["registryDigest"]
    generator = row["generatorDigest"]
    if row["originKind"] == "lockedRepositoryBlob":
        if repository is None:
            raise QualifiedDifferentialError(
                "lockedRepositoryBlob input requires repository"
            )
        _commit(commit, "input.commit")
        _path_hint(source_path, "input.sourcePath")
        _commit(source_blob, "input.sourceBlob")
        _digest(registry_digest, "input.registryDigest")
        if generator is not None:
            raise QualifiedDifferentialError(
                "lockedRepositoryBlob input cannot claim a generator digest"
            )
    else:
        if any(value is not None for value in (
            commit, repository, source_path, source_blob, registry_digest
        )):
            raise QualifiedDifferentialError(
                "localFromScratch input cannot claim repository provenance"
            )
        _digest(generator, "input.generatorDigest")
    return row


def _validate_adapter(value: Any) -> dict[str, Any]:
    row = _exact_keys(value, ADAPTER_KEYS, "adapter")
    _identifier(row["name"], "adapter.name")
    if row["side"] not in SIDES:
        raise QualifiedDifferentialError("adapter.side is invalid")
    if row["name"] != f"libdxfrw_semantic_adapter_{row['side']}":
        raise QualifiedDifferentialError("adapter.name does not identify its side")
    _string(row["package"], "adapter.package")
    _commit(row["commit"], "adapter.commit")
    for key in (
        "sourceDigest",
        "configDigest",
        "staticLibraryDigest",
        "binaryDigest",
        "linkedClosureDigest",
    ):
        _digest(row[key], f"adapter.{key}")
    return row


def _validate_invocation(value: Any) -> dict[str, Any]:
    row = _exact_keys(value, INVOCATION_KEYS, "invocation")
    if row["facade"] not in FACADES:
        raise QualifiedDifferentialError("invocation.facade is invalid")
    if row["direction"] not in DIRECTIONS:
        raise QualifiedDifferentialError("invocation.direction is invalid")
    options = _exact_keys(
        row["options"], INVOCATION_OPTION_KEYS, "invocation.options"
    )
    _boolean(options["applyExtrusion"], "invocation.options.applyExtrusion")
    if options["compatibilityProfile"] not in {
        "default", "standalone-safe", "librecad-master-legacy"
    }:
        raise QualifiedDifferentialError(
            "invocation.options.compatibilityProfile is invalid"
        )
    _digest(row["optionsDigest"], "invocation.optionsDigest")
    if row["optionsDigest"] != hashlib.sha256(
        _canonical_bytes(options)
    ).hexdigest():
        raise QualifiedDifferentialError("invocation.optionsDigest is stale")
    return row


def _validate_status(value: Any) -> dict[str, Any]:
    row = _exact_keys(value, STATUS_KEYS, "status")
    if row["outcome"] not in STATUS_OUTCOMES:
        raise QualifiedDifferentialError("status.outcome is invalid")
    succeeded = _boolean(row["operationSucceeded"], "status.operationSucceeded")
    exit_code = _integer(row["exitCode"], "status.exitCode")
    failure = _exact_keys(
        row["firstFailure"], FIRST_FAILURE_KEYS, "status.firstFailure"
    )
    if row["outcome"] == "success":
        if not succeeded:
            raise QualifiedDifferentialError(
                "successful status requires operationSucceeded=true"
            )
        if exit_code != 0 or any(failure[key] is not None for key in failure):
            raise QualifiedDifferentialError(
                "completed status must have exitCode 0 and an empty firstFailure"
            )
    else:
        if succeeded:
            raise QualifiedDifferentialError(
                "failed status requires operationSucceeded=false"
            )
        if exit_code == 0:
            raise QualifiedDifferentialError("failed status needs nonzero exitCode")
        _string(failure["stage"], "status.firstFailure.stage")
        _integer(failure["code"], "status.firstFailure.code", minimum=0)
        _semantic_path(failure["path"], "status.firstFailure.path")
        _identifier(failure["diagnosticId"], "status.firstFailure.diagnosticId")
    return row


def _validate_typed_value(type_name: str, value: Any, path: str) -> None:
    if type_name == "bool":
        _boolean(value, path)
    elif type_name == "int64":
        integer = _integer(value, path)
        if integer < -(2 ** 63) or integer >= 2 ** 63:
            raise QualifiedDifferentialError(f"{path} is outside int64")
    elif type_name == "uint64":
        integer = _integer(value, path, minimum=0)
        if integer >= 2 ** 64:
            raise QualifiedDifferentialError(f"{path} is outside uint64")
    elif type_name == "double":
        _finite_number(value, path)
    elif type_name == "string":
        _string(value, path, allow_empty=True)
    elif type_name == "handle":
        _handle(value, path)
    elif type_name == "point3d":
        keys = {"x", "y", "z"}
        point = _exact_keys(value, keys, path)
        for key in sorted(keys):
            _finite_number(point[key], f"{path}.{key}")
    else:  # pragma: no cover - guarded by caller, kept fail-closed
        raise QualifiedDifferentialError(f"{path} has an unknown field type")


OBSERVATION_ENVELOPE_KEYS = {
    "phase", "availability", "reason", "sourceVersion", "scalarCounters",
    "namedSummaries", "integrity", "frames",
}
OBSERVATION_COUNTER_KEYS = {
    "entityParseFailures", "objectParseFailures", "classesCrcMismatch",
    "reader21CrcMismatch", "reader18HeaderCrcMismatch",
    "dataStorageLinkFailures", "dataStorageOrphanRecords",
    "decodedProxyPrimitives",
}
NAMED_SUMMARY_KEYS = {
    "rows", "retainedRowCount", "sourceNameCount", "totalInstanceCount",
    "omittedNameCount", "omittedInstanceCount", "truncatedRetainedNameCount",
    "omittedNameBytes", "aggregateSaturated",
}
BYTE_PREFIX_KEYS = {"bytesHex", "prefixLength", "sourceLength", "truncated"}
NAMED_ROW_KEYS = BYTE_PREFIX_KEYS | {"instances"}
INTEGRITY_KEYS = {
    "rows", "retainedRowCount", "sourceRecordedCount", "extractionOmittedCount",
    "reportedUpstreamDroppedLowerBound", "truncatedRetainedNameCount",
    "omittedNameBytes", "aggregateSaturated",
}
INTEGRITY_ROW_KEYS = {
    "versionCode", "severityCode", "offsetSpaceCode", "phaseCode", "kindCode",
    "logicalSectionId", "sectionDescriptorId", "sectionName", "pageId",
    "fileOffset", "logicalHandle", "expected", "observed", "hasPageId",
    "hasFileOffset", "hasLogicalHandle", "hasExpected", "hasObserved",
}
FRAME_KEYS = {
    "status", "reason", "sourceFrameCount", "dispositions",
    "unrecognizedDispositionCount",
}
FRAME_DISPOSITION_KEYS = {
    "pending", "deferred", "staged", "published", "quarantined", "failed",
    "unresolved",
}
OBSERVATIONS_KEYS = {"inputRead", "outputReadback"}


def _uint64(value: Any, path: str) -> int:
    integer = _integer(value, path, minimum=0)
    if integer >= 2 ** 64:
        raise QualifiedDifferentialError(f"{path} is outside uint64")
    return integer


def _int32(value: Any, path: str) -> int:
    integer = _integer(value, path)
    if integer < -(2 ** 31) or integer >= 2 ** 31:
        raise QualifiedDifferentialError(f"{path} is outside int32")
    return integer


def _validate_byte_prefix(value: Any, path: str) -> dict[str, Any]:
    row = _exact_keys(value, BYTE_PREFIX_KEYS, path)
    encoded = _string(row["bytesHex"], f"{path}.bytesHex", allow_empty=True)
    if not re.fullmatch(r"(?:[0-9a-f]{2}){0,64}", encoded):
        raise QualifiedDifferentialError(f"{path}.bytesHex is not a bounded lowercase byte prefix")
    prefix_length = _uint64(row["prefixLength"], f"{path}.prefixLength")
    source_length = _uint64(row["sourceLength"], f"{path}.sourceLength")
    truncated = _boolean(row["truncated"], f"{path}.truncated")
    if prefix_length != len(encoded) // 2 or source_length < prefix_length:
        raise QualifiedDifferentialError(f"{path} byte-prefix lengths are inconsistent")
    if truncated != (source_length != prefix_length):
        raise QualifiedDifferentialError(f"{path}.truncated contradicts sourceLength")
    return row


def _validate_named_summary(value: Any, path: str) -> dict[str, Any]:
    row = _exact_keys(value, NAMED_SUMMARY_KEYS, path)
    named_rows = _list(row["rows"], f"{path}.rows")
    if len(named_rows) > 16:
        raise QualifiedDifferentialError(f"{path}.rows exceeds the observation bound")
    for index, item in enumerate(named_rows):
        named = _exact_keys(item, NAMED_ROW_KEYS, f"{path}.rows[{index}]")
        _validate_byte_prefix(
            {key: named[key] for key in BYTE_PREFIX_KEYS},
            f"{path}.rows[{index}]",
        )
        _uint64(named["instances"], f"{path}.rows[{index}].instances")
    for key in (
        "retainedRowCount", "sourceNameCount", "totalInstanceCount",
        "omittedNameCount", "omittedInstanceCount",
        "truncatedRetainedNameCount", "omittedNameBytes",
    ):
        _uint64(row[key], f"{path}.{key}")
    _boolean(row["aggregateSaturated"], f"{path}.aggregateSaturated")
    if row["retainedRowCount"] != len(named_rows):
        raise QualifiedDifferentialError(f"{path}.retainedRowCount disagrees with rows")
    if row["sourceNameCount"] < row["retainedRowCount"]:
        raise QualifiedDifferentialError(f"{path}.sourceNameCount is below retained rows")
    if not row["aggregateSaturated"]:
        if row["sourceNameCount"] != row["retainedRowCount"] + row["omittedNameCount"]:
            raise QualifiedDifferentialError(f"{path} source/omitted name counts disagree")
        retained_instances = sum(item["instances"] for item in named_rows)
        if row["totalInstanceCount"] != retained_instances + row["omittedInstanceCount"]:
            raise QualifiedDifferentialError(f"{path} retained/omitted instance counts disagree")
        if row["truncatedRetainedNameCount"] != sum(
            1 for item in named_rows if item["truncated"]
        ):
            raise QualifiedDifferentialError(f"{path}.truncatedRetainedNameCount disagrees with rows")
        retained_suffix_bytes = sum(
            item["sourceLength"] - item["prefixLength"]
            for item in named_rows if item["truncated"]
        )
        if row["omittedNameBytes"] < retained_suffix_bytes:
            raise QualifiedDifferentialError(f"{path}.omittedNameBytes omits retained suffix bytes")
    return row


def _validate_integrity_row(value: Any, path: str) -> dict[str, Any]:
    row = _exact_keys(value, INTEGRITY_ROW_KEYS, path)
    for key in ("versionCode", "severityCode", "offsetSpaceCode", "phaseCode", "kindCode"):
        _integer(row[key], f"{path}.{key}", minimum=0)
        if row[key] > 255:
            raise QualifiedDifferentialError(f"{path}.{key} is outside uint8")
    _int32(row["logicalSectionId"], f"{path}.logicalSectionId")
    _int32(row["sectionDescriptorId"], f"{path}.sectionDescriptorId")
    _validate_byte_prefix(row["sectionName"], f"{path}.sectionName")
    for key in ("pageId", "fileOffset", "expected", "observed"):
        _uint64(row[key], f"{path}.{key}")
    _integer(row["logicalHandle"], f"{path}.logicalHandle", minimum=0)
    if row["logicalHandle"] >= 2 ** 32:
        raise QualifiedDifferentialError(f"{path}.logicalHandle is outside uint32")
    for key in (
        "hasPageId", "hasFileOffset", "hasLogicalHandle", "hasExpected",
        "hasObserved",
    ):
        _boolean(row[key], f"{path}.{key}")
    return row


def _validate_integrity(value: Any, path: str) -> dict[str, Any]:
    row = _exact_keys(value, INTEGRITY_KEYS, path)
    integrity_rows = _list(row["rows"], f"{path}.rows")
    if len(integrity_rows) > 16:
        raise QualifiedDifferentialError(f"{path}.rows exceeds the observation bound")
    for index, item in enumerate(integrity_rows):
        _validate_integrity_row(item, f"{path}.rows[{index}]")
    for key in (
        "retainedRowCount", "sourceRecordedCount", "extractionOmittedCount",
        "reportedUpstreamDroppedLowerBound", "truncatedRetainedNameCount",
        "omittedNameBytes",
    ):
        _uint64(row[key], f"{path}.{key}")
    _boolean(row["aggregateSaturated"], f"{path}.aggregateSaturated")
    if row["retainedRowCount"] != len(integrity_rows):
        raise QualifiedDifferentialError(f"{path}.retainedRowCount disagrees with rows")
    if row["sourceRecordedCount"] < row["retainedRowCount"]:
        raise QualifiedDifferentialError(f"{path}.sourceRecordedCount is below retained rows")
    if not row["aggregateSaturated"]:
        if row["sourceRecordedCount"] != row["retainedRowCount"] + row["extractionOmittedCount"]:
            raise QualifiedDifferentialError(f"{path} source/omitted row counts disagree")
        if row["truncatedRetainedNameCount"] != sum(
            1 for item in integrity_rows if item["sectionName"]["truncated"]
        ):
            raise QualifiedDifferentialError(f"{path}.truncatedRetainedNameCount disagrees with rows")
        retained_suffix_bytes = sum(
            item["sectionName"]["sourceLength"]
            - item["sectionName"]["prefixLength"]
            for item in integrity_rows if item["sectionName"]["truncated"]
        )
        if row["omittedNameBytes"] < retained_suffix_bytes:
            raise QualifiedDifferentialError(f"{path}.omittedNameBytes omits retained suffix bytes")
    return row


def _validate_frames(value: Any, path: str) -> dict[str, Any]:
    row = _exact_keys(value, FRAME_KEYS, path)
    statuses = {"notAvailable", "inProgress", "finalizedComplete", "finalizedPartial"}
    if row["status"] not in statuses:
        raise QualifiedDifferentialError(f"{path}.status is invalid")
    _nullable_identifier(row["reason"], f"{path}.reason")
    dispositions = _exact_keys(row["dispositions"], FRAME_DISPOSITION_KEYS,
                               f"{path}.dispositions")
    if row["status"] == "notAvailable":
        if row["reason"] is None or row["sourceFrameCount"] is not None:
            raise QualifiedDifferentialError(f"{path} unknown frame totals need a reason and null count")
        if any(item is not None for item in dispositions.values()) or row["unrecognizedDispositionCount"] is not None:
            raise QualifiedDifferentialError(f"{path} unknown frame dispositions must be null")
    else:
        if row["reason"] is not None:
            raise QualifiedDifferentialError(f"{path}.reason is only valid for unavailable totals")
        _uint64(row["sourceFrameCount"], f"{path}.sourceFrameCount")
        for key, item in dispositions.items():
            _uint64(item, f"{path}.dispositions.{key}")
        _uint64(row["unrecognizedDispositionCount"], f"{path}.unrecognizedDispositionCount")
        if sum(dispositions.values()) + row["unrecognizedDispositionCount"] != row["sourceFrameCount"]:
            raise QualifiedDifferentialError(f"{path} dispositions disagree with sourceFrameCount")
    return row


def _validate_observation_envelope(value: Any, path: str) -> dict[str, Any]:
    row = _exact_keys(value, OBSERVATION_ENVELOPE_KEYS, path)
    if (not isinstance(row["phase"], str)
            or row["phase"] not in {"inputRead", "outputReadback"}):
        raise QualifiedDifferentialError(f"{path}.phase is invalid")
    availability = row["availability"]
    if (not isinstance(availability, str)
            or availability not in {"available", "unavailable", "notApplicable", "notRun"}):
        raise QualifiedDifferentialError(f"{path}.availability is invalid")
    reason = _nullable_identifier(row["reason"], f"{path}.reason")
    source_version = _optional_string(row["sourceVersion"], f"{path}.sourceVersion")
    if source_version is not None and source_version != "unknown" and not re.fullmatch(r"AC\d{4}", source_version):
        raise QualifiedDifferentialError(f"{path}.sourceVersion is invalid")
    payload_keys = ("scalarCounters", "namedSummaries", "integrity", "frames")
    if availability != "available":
        if reason is None or any(row[key] is not None for key in payload_keys):
            raise QualifiedDifferentialError(
                f"{path} non-available observations require a reason and null totals"
            )
        if availability == "notApplicable" and source_version is not None:
            raise QualifiedDifferentialError(
                f"{path} notApplicable observations cannot claim a source version"
            )
        if availability == "notRun" and source_version is not None:
            raise QualifiedDifferentialError(
                f"{path} notRun observations cannot claim a source version"
            )
        return row
    if reason is not None or source_version is None:
        raise QualifiedDifferentialError(f"{path} available observations require a source version and no reason")
    counters = _exact_keys(row["scalarCounters"], OBSERVATION_COUNTER_KEYS,
                           f"{path}.scalarCounters")
    for key, item in counters.items():
        _uint64(item, f"{path}.scalarCounters.{key}")
    named = _exact_keys(row["namedSummaries"], {
        "skippedCustomClasses", "skippedUnsupportedObjects",
    }, f"{path}.namedSummaries")
    for key, item in named.items():
        _validate_named_summary(item, f"{path}.namedSummaries.{key}")
    _validate_integrity(row["integrity"], f"{path}.integrity")
    frames = _validate_frames(row["frames"], f"{path}.frames")
    return row


def _validate_observations(value: Any, document: dict[str, Any]) -> dict[str, Any]:
    observations = _exact_keys(value, OBSERVATIONS_KEYS, "observations")
    input_read = _validate_observation_envelope(
        observations["inputRead"], "observations.inputRead"
    )
    output_readback = _validate_observation_envelope(
        observations["outputReadback"], "observations.outputReadback"
    )
    if input_read["phase"] != "inputRead":
        raise QualifiedDifferentialError(
            "observations.inputRead phase does not match its role"
        )
    if output_readback["phase"] != "outputReadback":
        raise QualifiedDifferentialError(
            "observations.outputReadback phase does not match its role"
        )
    invocation = document["invocation"]
    input_version = document["input"]["detectedVersion"]
    if invocation["facade"] == "dxfRW":
        if input_read["availability"] != "notApplicable" or output_readback["availability"] != "notApplicable":
            raise QualifiedDifferentialError("DXF invocation cannot claim DWG read observations")
    elif invocation["direction"] == "read":
        if input_read["phase"] != "inputRead" or input_read["availability"] not in {"available", "unavailable"}:
            raise QualifiedDifferentialError("DWG read requires inputRead observations or an explicit unavailable result")
        if input_read["sourceVersion"] != input_version:
            raise QualifiedDifferentialError("inputRead sourceVersion differs from the input signature")
        if output_readback["availability"] != "notApplicable":
            raise QualifiedDifferentialError("read-only invocation cannot claim outputReadback observations")
    else:
        if input_read["availability"] != "notApplicable":
            raise QualifiedDifferentialError("DWG write recipe cannot claim input-source read observations")
        failure_stage = document["status"]["firstFailure"]["stage"]
        if failure_stage in {"write", "writeResult"}:
            if output_readback["availability"] != "notRun":
                raise QualifiedDifferentialError("pre-readback DWG write failure must say outputReadback was notRun")
        else:
            if output_readback["phase"] != "outputReadback" or output_readback["availability"] not in {"available", "unavailable"}:
                raise QualifiedDifferentialError("DWG write requires outputReadback observations or an explicit unavailable result")
    return observations


def _validate_write_result_readback_version(
    invocation: dict[str, Any], status: dict[str, Any],
    observations: dict[str, Any], records: list[dict[str, Any]],
) -> None:
    if invocation["facade"] != "dwgRW" or invocation["direction"] != "write":
        return
    failure_stage = status["firstFailure"]["stage"]
    if failure_stage in {"write", "writeResult"}:
        return
    write_results = [
        record for record in records
        if record["entity"] == "WRITE_RESULT"
        or record["recordClass"] == "WRITE_RESULT"
    ]
    if len(write_results) != 1:
        raise QualifiedDifferentialError(
            "DWG output readback requires exactly one WRITE_RESULT record"
        )
    version_fields = [
        field for field in write_results[0]["fields"]
        if field["name"] == "version"
    ]
    if (len(version_fields) != 1 or version_fields[0]["type"] != "string"
            or version_fields[0]["value"]
            != observations["outputReadback"]["sourceVersion"]):
        raise QualifiedDifferentialError(
            "outputReadback sourceVersion differs from WRITE_RESULT version"
        )


def _validate_modeler_history_reference(
    record: dict[str, Any],
    outgoing: list[dict[str, Any]],
    node_by_id: dict[str, dict[str, Any]],
    captured_record_handles: set[str],
    path: str,
) -> None:
    fields = {field["name"]: field for field in record["fields"]}
    history_field = fields.get("historyHandle")
    if history_field is None or history_field["type"] != "handle":
        raise QualifiedDifferentialError(
            f"{path} MODELER_GEOMETRY lacks a typed historyHandle field"
        )
    history_handle = _handle(history_field["value"], f"{path}.historyHandle")
    history_edges = [
        edge for edge in outgoing if edge["kind"] == "historyReference"
    ]
    if history_handle == "0":
        if history_edges:
            raise QualifiedDifferentialError(
                f"{path} has a history edge for a null historyHandle"
            )
        return
    if len(history_edges) != 1:
        raise QualifiedDifferentialError(
            f"{path} historyHandle lacks exactly one historyReference edge"
        )
    target = node_by_id.get(history_edges[0]["to"])
    if (target is None or target["kind"] != "handle"
            or target["handle"] != history_handle):
        raise QualifiedDifferentialError(
            f"{path} historyReference edge contradicts historyHandle"
        )
    expected_disposition = (
        "resolved" if history_handle in captured_record_handles else "unresolved"
    )
    if history_edges[0]["disposition"] != expected_disposition:
        raise QualifiedDifferentialError(
            f"{path} historyReference disposition contradicts captured records"
        )


def _validate_owner_reference(
    record: dict[str, Any],
    owner_edges: list[dict[str, Any]],
    node_by_id: dict[str, dict[str, Any]],
    captured_record_handles: set[str],
    path: str,
) -> None:
    owner_handle = record["ownerHandle"]
    if owner_handle is None or owner_handle == "0":
        if owner_edges:
            raise QualifiedDifferentialError(
                f"{path} has an owner edge for a null ownerHandle"
            )
        return
    if len(owner_edges) != 1:
        raise QualifiedDifferentialError(
            f"{path} ownerHandle lacks exactly one owner edge"
        )
    target = node_by_id.get(owner_edges[0]["to"])
    if (target is None or target["kind"] not in {"handle", "block"}
            or target["handle"] != owner_handle):
        raise QualifiedDifferentialError(
            f"{path} owner edge contradicts ownerHandle"
        )
    if owner_handle in captured_record_handles:
        expected_disposition = "resolved"
    else:
        expected_disposition = "unresolved"
    if owner_edges[0]["disposition"] != expected_disposition:
        raise QualifiedDifferentialError(
            f"{path} owner disposition contradicts captured records"
        )


def validate_result(value: Any, expected_adapter: dict[str, Any] | None = None) -> dict[str, Any]:
    """Validate one adapter result and return it unchanged."""

    if not isinstance(value, dict):
        raise QualifiedDifferentialError("result must be an object")
    schema = value.get("schema")
    if type(schema) is not int or schema not in {
        ADAPTER_RESULT_SCHEMA_V2, ADAPTER_RESULT_SCHEMA_V3,
    }:
        raise QualifiedDifferentialError("unexpected adapter result schema or kind")
    keys = ROOT_KEYS_V2 if schema == ADAPTER_RESULT_SCHEMA_V2 else ROOT_KEYS_V3
    document = _exact_keys(value, keys, "result")
    if document["kind"] != RESULT_KIND:
        raise QualifiedDifferentialError("unexpected adapter result schema or kind")
    _validate_input(document["input"])
    adapter = _validate_adapter(document["adapter"])
    if expected_adapter is not None and adapter != expected_adapter:
        raise QualifiedDifferentialError(
            "adapter result identity does not match its build receipt"
        )
    _validate_invocation(document["invocation"])
    status = _validate_status(document["status"])
    if schema == ADAPTER_RESULT_SCHEMA_V3:
        _validate_observations(document["observations"], document)
    if document["input"]["detectedFormat"] == "unknown" and status["outcome"] != "failure":
        raise QualifiedDifferentialError(
            "unknown input format is allowed only for structured failure"
        )

    callbacks = _list(document["callbacks"], "callbacks")
    callback_by_ordinal: dict[int, dict[str, Any]] = {}
    callback_ids: list[str] = []
    for index, item in enumerate(callbacks):
        row = _exact_keys(item, CALLBACK_KEYS, f"callbacks[{index}]")
        callback_ids.append(_identifier(row["id"], f"callbacks[{index}].id"))
        ordinal = _integer(row["ordinal"], f"callbacks[{index}].ordinal", minimum=0)
        if ordinal != index:
            raise QualifiedDifferentialError(
                "callback ordinals must be contiguous and preserve array order"
            )
        _identifier(row["kind"], f"callbacks[{index}].kind")
        _nullable_identifier(row["recordId"], f"callbacks[{index}].recordId")
        _nullable_identifier(
            row["blockContextId"], f"callbacks[{index}].blockContextId"
        )
        carrier_ids_value = _list(
            row["carrierIds"], f"callbacks[{index}].carrierIds"
        )
        _unique(
            (_identifier(carrier_id, f"callbacks[{index}].carrierIds")
             for carrier_id in carrier_ids_value),
            f"callbacks[{index}].carrierIds",
        )
        callback_by_ordinal[ordinal] = row
    _unique(callback_ids, "callback IDs")

    records = _list(document["records"], "records")
    record_ids = _unique(
        ((_identifier(item.get("id"), f"records[{i}].id")
          if isinstance(item, dict) else _identifier(None, f"records[{i}].id"))
         for i, item in enumerate(records)),
        "record IDs",
    )
    field_count = 0
    for index, item in enumerate(records):
        row = _exact_keys(item, RECORD_KEYS, f"records[{index}]")
        callback_ordinal = _integer(
            row["callbackOrdinal"], f"records[{index}].callbackOrdinal", minimum=0
        )
        if callback_ordinal not in callback_by_ordinal:
            raise QualifiedDifferentialError(
                f"records[{index}].callbackOrdinal is dangling"
            )
        if row["kind"] not in RECORD_KINDS:
            raise QualifiedDifferentialError(f"records[{index}].kind is invalid")
        _string(row["recordClass"], f"records[{index}].recordClass")
        _string(row["entity"], f"records[{index}].entity")
        _optional_handle(row["sourceHandle"], f"records[{index}].sourceHandle")
        _optional_handle(row["ownerHandle"], f"records[{index}].ownerHandle")
        _nullable_identifier(row["blockId"], f"records[{index}].blockId")
        fields = _list(row["fields"], f"records[{index}].fields")
        names: list[str] = []
        for field_index, item_field in enumerate(fields):
            field = _exact_keys(
                item_field, FIELD_KEYS,
                f"records[{index}].fields[{field_index}]",
            )
            name = _identifier(
                field["name"], f"records[{index}].fields[{field_index}].name"
            )
            names.append(name)
            if field["type"] not in FIELD_TYPES:
                raise QualifiedDifferentialError(
                    f"records[{index}].fields[{field_index}].type is invalid"
                )
            _validate_typed_value(
                field["type"], field["value"],
                f"records[{index}].fields[{field_index}].value",
            )
        _unique(names, f"records[{index}].fields names")
        field_count += len(fields)

    graph_nodes = _list(document["graphNodes"], "graphNodes")
    graph_node_ids = _unique(
        ((_identifier(item.get("id"), f"graphNodes[{i}].id")
          if isinstance(item, dict) else _identifier(None, f"graphNodes[{i}].id"))
         for i, item in enumerate(graph_nodes)),
        "graph node IDs",
    )
    for index, item in enumerate(graph_nodes):
        row = _exact_keys(item, GRAPH_NODE_KEYS, f"graphNodes[{index}]")
        if row["kind"] not in GRAPH_NODE_KINDS:
            raise QualifiedDifferentialError(f"graphNodes[{index}].kind is invalid")
        record_id = _nullable_identifier(
            row["recordId"], f"graphNodes[{index}].recordId"
        )
        if record_id is not None and record_id not in record_ids:
            raise QualifiedDifferentialError(
                f"graphNodes[{index}].recordId is dangling"
            )
        _optional_handle(row["handle"], f"graphNodes[{index}].handle")

    graph_edges = _list(document["graphEdges"], "graphEdges")
    _unique(
        ((_identifier(item.get("id"), f"graphEdges[{i}].id")
          if isinstance(item, dict) else _identifier(None, f"graphEdges[{i}].id"))
         for i, item in enumerate(graph_edges)),
        "graph edge IDs",
    )
    for index, item in enumerate(graph_edges):
        row = _exact_keys(item, GRAPH_EDGE_KEYS, f"graphEdges[{index}]")
        if row["kind"] not in GRAPH_EDGE_KINDS:
            raise QualifiedDifferentialError(f"graphEdges[{index}].kind is invalid")
        if row["disposition"] not in GRAPH_EDGE_DISPOSITIONS:
            raise QualifiedDifferentialError(
                f"graphEdges[{index}].disposition is invalid"
            )
        if (row["kind"] not in {"historyReference", "owner"}
                and row["disposition"] != "observed"):
            raise QualifiedDifferentialError(
                f"graphEdges[{index}] has a resolution disposition for an unrelated edge"
            )
        for key in ("from", "to"):
            endpoint = _identifier(row[key], f"graphEdges[{index}].{key}")
            if endpoint not in graph_node_ids:
                raise QualifiedDifferentialError(
                    f"graphEdges[{index}].{key} is dangling"
                )

    opaque = _list(document["opaqueCarriers"], "opaqueCarriers")
    carrier_ids = _unique(
        ((_identifier(item.get("id"), f"opaqueCarriers[{i}].id")
          if isinstance(item, dict) else _identifier(None, f"opaqueCarriers[{i}].id"))
         for i, item in enumerate(opaque)),
        "opaque carrier IDs",
    )
    for index, item in enumerate(opaque):
        row = _exact_keys(item, OPAQUE_KEYS, f"opaqueCarriers[{index}]")
        _semantic_path(row["source"], f"opaqueCarriers[{index}].source")
        _integer(row["byteSize"], f"opaqueCarriers[{index}].byteSize", minimum=0)
        _digest(row["sha256"], f"opaqueCarriers[{index}].sha256")
        if row["disposition"] not in OPAQUE_DISPOSITIONS:
            raise QualifiedDifferentialError(
                f"opaqueCarriers[{index}].disposition is invalid"
            )
        record_id = _nullable_identifier(
            row["recordId"], f"opaqueCarriers[{index}].recordId"
        )
        if record_id is not None and record_id not in record_ids:
            raise QualifiedDifferentialError(
                f"opaqueCarriers[{index}].recordId is dangling"
            )
        _optional_handle(
            row["sourceHandle"], f"opaqueCarriers[{index}].sourceHandle"
        )

    unsupported = _list(document["unsupportedContent"], "unsupportedContent")
    _unique(
        ((_identifier(item.get("id"), f"unsupportedContent[{i}].id")
          if isinstance(item, dict) else _identifier(None, f"unsupportedContent[{i}].id"))
         for i, item in enumerate(unsupported)),
        "unsupported content IDs",
    )
    for index, item in enumerate(unsupported):
        row = _exact_keys(
            item, UNSUPPORTED_KEYS, f"unsupportedContent[{index}]"
        )
        _identifier(row["kind"], f"unsupportedContent[{index}].kind")
        _semantic_path(row["source"], f"unsupportedContent[{index}].source")
        record_id = _nullable_identifier(
            row["recordId"], f"unsupportedContent[{index}].recordId"
        )
        if record_id is not None and record_id not in record_ids:
            raise QualifiedDifferentialError(
                f"unsupportedContent[{index}].recordId is dangling"
            )
        carrier_id = _nullable_identifier(
            row["carrierId"], f"unsupportedContent[{index}].carrierId"
        )
        if carrier_id is not None and carrier_id not in carrier_ids:
            raise QualifiedDifferentialError(
                f"unsupportedContent[{index}].carrierId is dangling"
            )
        _nullable_identifier(
            row["diagnosticId"], f"unsupportedContent[{index}].diagnosticId"
        )
        if row["disposition"] not in UNSUPPORTED_DISPOSITIONS:
            raise QualifiedDifferentialError(
                f"unsupportedContent[{index}].disposition is invalid"
            )
        _string(row["reason"], f"unsupportedContent[{index}].reason")
        if row["disposition"] in {"preserved", "preservedCanonical"} and carrier_id is None:
            raise QualifiedDifferentialError(
                "preserved unsupported content requires an opaque carrier"
            )

    diagnostics = _list(document["diagnostics"], "diagnostics")
    diagnostic_ids = _unique(
        ((_identifier(item.get("id"), f"diagnostics[{i}].id")
          if isinstance(item, dict) else _identifier(None, f"diagnostics[{i}].id"))
         for i, item in enumerate(diagnostics)),
        "diagnostic IDs",
    )
    for index, item in enumerate(diagnostics):
        row = _exact_keys(item, DIAGNOSTIC_KEYS, f"diagnostics[{index}]")
        ordinal = _integer(
            row["ordinal"], f"diagnostics[{index}].ordinal", minimum=0
        )
        if ordinal != index:
            raise QualifiedDifferentialError(
                "diagnostic ordinals must be contiguous and preserve array order"
            )
        if row["severity"] not in DIAGNOSTIC_SEVERITIES:
            raise QualifiedDifferentialError(
                f"diagnostics[{index}].severity is invalid"
            )
        _string(row["stage"], f"diagnostics[{index}].stage")
        if row["stage"] not in DIAGNOSTIC_STAGES:
            raise QualifiedDifferentialError(
                f"diagnostics[{index}].stage is invalid"
            )
        _string(row["code"], f"diagnostics[{index}].code")
        _semantic_path(row["path"], f"diagnostics[{index}].path")
        _digest(row["messageDigest"], f"diagnostics[{index}].messageDigest")

    # Callback references are checked only after all record and graph IDs exist.
    for index, row in enumerate(callbacks):
        if row["recordId"] is not None and row["recordId"] not in record_ids:
            raise QualifiedDifferentialError(
                f"callbacks[{index}].recordId is dangling"
            )
        if row["recordId"] is not None:
            record = next(item for item in records if item["id"] == row["recordId"])
            if record["callbackOrdinal"] != row["ordinal"]:
                raise QualifiedDifferentialError(
                    f"callbacks[{index}] and record callback references are not reciprocal"
                )
            if row["blockContextId"] != record["blockId"]:
                raise QualifiedDifferentialError(
                    f"callbacks[{index}] block context contradicts its record"
                )
        if (row["blockContextId"] is not None
                and row["blockContextId"] not in graph_node_ids):
            raise QualifiedDifferentialError(
                f"callbacks[{index}].blockContextId is dangling"
            )
        if row["blockContextId"] is not None:
            block_node = next(
                node for node in graph_nodes if node["id"] == row["blockContextId"]
            )
            if block_node["kind"] != "block":
                raise QualifiedDifferentialError(
                    f"callbacks[{index}].blockContextId is not a block node"
                )
        for carrier_id in row["carrierIds"]:
            if carrier_id not in carrier_ids:
                raise QualifiedDifferentialError(
                    f"callbacks[{index}].carrierIds contains dangling {carrier_id}"
                )
            carrier = next(item for item in opaque if item["id"] == carrier_id)
            if carrier["recordId"] != row["recordId"]:
                raise QualifiedDifferentialError(
                    f"callbacks[{index}] carrier record reference is inconsistent"
                )
    for index, row in enumerate(records):
        if row["blockId"] is not None and row["blockId"] not in graph_node_ids:
            raise QualifiedDifferentialError(f"records[{index}].blockId is dangling")
        callback = callback_by_ordinal[row["callbackOrdinal"]]
        if callback["recordId"] != row["id"]:
            raise QualifiedDifferentialError(
                f"records[{index}] and callback record references are not reciprocal"
            )

    # Every record has exactly one graph node, and no graph record reference
    # may alias another node.  This makes graph cardinality independently
    # checkable rather than inferred from callback order.
    graph_record_counts = Counter(
        row["recordId"] for row in graph_nodes if row["recordId"] is not None
    )
    if set(graph_record_counts) != record_ids or any(
        count != 1 for count in graph_record_counts.values()
    ):
        raise QualifiedDifferentialError(
            "record IDs and graph-node record mappings are not one-to-one"
        )
    record_by_id = {row["id"]: row for row in records}
    captured_record_handles = {
        row["sourceHandle"] for row in records
        if row["sourceHandle"] is not None
    }
    node_by_id = {row["id"]: row for row in graph_nodes}
    record_node_by_id = {
        row["recordId"]: row
        for row in graph_nodes
        if row["recordId"] is not None
    }
    for node in graph_nodes:
        if node["kind"] == "handle":
            if node["recordId"] is not None:
                raise QualifiedDifferentialError(
                    "handle graph nodes cannot claim a record reference"
                )
            if node["handle"] is None:
                raise QualifiedDifferentialError(
                    "handle graph nodes require a handle"
                )
            if node["id"] != f"node:handle:{node['handle']}":
                raise QualifiedDifferentialError(
                    "handle graph node identity is not canonical"
                )
    handle_values = [
        node["handle"] for node in graph_nodes if node["kind"] == "handle"
    ]
    if len(handle_values) != len(set(handle_values)):
        raise QualifiedDifferentialError("handle graph-node identities are not unique")
    referenced_nodes = {
        node["id"] for node in graph_nodes if node["recordId"] is not None
    }
    referenced_nodes.update(
        callback["blockContextId"]
        for callback in callbacks
        if callback["blockContextId"] is not None
    )
    referenced_nodes.update(
        endpoint for edge in graph_edges for endpoint in (edge["from"], edge["to"])
    )
    orphaned_nodes = sorted(graph_node_ids - referenced_nodes)
    if orphaned_nodes:
        raise QualifiedDifferentialError(
            f"graph contains orphaned nodes: {orphaned_nodes}"
        )
    for edge in graph_edges:
        source_node = node_by_id[edge["from"]]
        target_node = node_by_id[edge["to"]]
        if source_node["kind"] not in {"record", "block"} or source_node["recordId"] is None:
            raise QualifiedDifferentialError(
                f"{edge['kind']} edge must originate at a record node"
            )
        if edge["kind"] in {"handleReference", "historyReference", "owner"}:
            if target_node["kind"] not in {"handle", "block"} or not target_node["handle"]:
                raise QualifiedDifferentialError(
                    f"{edge['kind']} edge must target a handled node"
                )
            if edge["kind"] == "historyReference" and target_node["kind"] != "handle":
                raise QualifiedDifferentialError(
                    "historyReference edge must target a handle node"
                )
        elif target_node["kind"] != "block":
            raise QualifiedDifferentialError(
                "blockMembership edge must target a block node"
            )
    for record_id, record in record_by_id.items():
        node = record_node_by_id[record_id]
        expected_node_kind = "block" if record["kind"] == "block" else "record"
        if node["kind"] != expected_node_kind:
            raise QualifiedDifferentialError(
                f"record {record_id} has the wrong graph-node kind"
            )
        if node["handle"] != record["sourceHandle"]:
            raise QualifiedDifferentialError(
                f"record {record_id} sourceHandle contradicts its graph node"
            )
        field_map = {field["name"]: field for field in record["fields"]}
        for name in ("recordClass", "entity"):
            if (
                name not in field_map
                or field_map[name]["type"] != "string"
                or field_map[name]["value"] != record[name]
            ):
                raise QualifiedDifferentialError(
                    f"record {record_id} {name} field contradicts record identity"
                )
        for name, record_value in (
            ("handle", record["sourceHandle"]),
            ("ownerHandle", record["ownerHandle"]),
        ):
            if name in field_map and (
                field_map[name]["type"] != "handle"
                or field_map[name]["value"] != record_value
            ):
                raise QualifiedDifferentialError(
                    f"record {record_id} {name} field contradicts record identity"
                )
        outgoing = [edge for edge in graph_edges if edge["from"] == node["id"]]
        handle_edges = [edge for edge in outgoing if edge["kind"] == "handleReference"]
        owner_edges = [edge for edge in outgoing if edge["kind"] == "owner"]
        block_edges = [edge for edge in outgoing if edge["kind"] == "blockMembership"]
        if record["recordClass"] == "MODELER_GEOMETRY":
            _validate_modeler_history_reference(
                record, outgoing, node_by_id, captured_record_handles,
                f"record {record_id}"
            )
        elif any(edge["kind"] == "historyReference" for edge in outgoing):
            raise QualifiedDifferentialError(
                f"record {record_id} has historyReference on a non-modeler record"
            )
        # handleReference is also used for semantic references carried by
        # fields (for example DICTIONARY entry handles and BLOCK_RECORD's
        # layout handle).  Validate the record identity edge by its target
        # handle rather than assuming it is the only outgoing reference.
        source_handle_edges = [
            edge for edge in handle_edges
            if node_by_id[edge["to"]]["handle"] == record["sourceHandle"]
        ] if record["sourceHandle"] is not None else []
        if record["sourceHandle"] is not None and len(source_handle_edges) != 1:
            raise QualifiedDifferentialError(
                f"record {record_id} sourceHandle lacks exactly one identity edge"
            )
        _validate_owner_reference(
            record, owner_edges, node_by_id, captured_record_handles,
            f"record {record_id}",
        )
        if record["blockId"] is None:
            if block_edges:
                raise QualifiedDifferentialError(
                    f"record {record_id} has block membership without blockId"
                )
        elif (
            node_by_id[record["blockId"]]["kind"] != "block"
            or len(block_edges) != 1
            or block_edges[0]["to"] != record["blockId"]
        ):
            raise QualifiedDifferentialError(
                f"record {record_id} blockId lacks one reciprocal membership edge"
            )

    callback_carrier_refs = Counter(
        carrier_id for callback in callbacks for carrier_id in callback["carrierIds"]
    )
    for carrier in opaque:
        carrier_id = carrier["id"]
        if callback_carrier_refs[carrier_id] != 1:
            raise QualifiedDifferentialError(
                f"opaque carrier {carrier_id} must be attached to exactly one callback"
            )
        record_id = carrier["recordId"]
        if record_id is not None:
            record = record_by_id[record_id]
            if carrier["sourceHandle"] != record["sourceHandle"]:
                raise QualifiedDifferentialError(
                    f"opaque carrier {carrier_id} sourceHandle contradicts its record"
                )
            expected_prefix = f"/records/{_pointer_token(record_id)}/"
            if not carrier["source"].startswith(expected_prefix):
                raise QualifiedDifferentialError(
                    f"opaque carrier {carrier_id} source path contradicts its record"
                )
        elif carrier["sourceHandle"] is not None:
            raise QualifiedDifferentialError(
                f"recordless opaque carrier {carrier_id} cannot claim sourceHandle"
            )
        elif carrier["source"].startswith("/records/"):
            raise QualifiedDifferentialError(
                f"recordless opaque carrier {carrier_id} cannot use a record path"
            )
    for index, row in enumerate(unsupported):
        diagnostic_id = row["diagnosticId"]
        if diagnostic_id is not None and diagnostic_id not in diagnostic_ids:
            raise QualifiedDifferentialError(
                f"unsupportedContent[{index}].diagnosticId is dangling"
            )
        if row["carrierId"] is not None and row["recordId"] is not None:
            carrier = next(item for item in opaque if item["id"] == row["carrierId"])
            if carrier["recordId"] != row["recordId"]:
                raise QualifiedDifferentialError(
                    f"unsupportedContent[{index}] carrier record reference is inconsistent"
                )
        elif row["carrierId"] is not None:
            carrier = next(item for item in opaque if item["id"] == row["carrierId"])
            if carrier["recordId"] is not None:
                raise QualifiedDifferentialError(
                    f"unsupportedContent[{index}] omits its carrier record reference"
                )
        if row["disposition"] in {"preserved", "preservedCanonical"}:
            carrier = next(item for item in opaque if item["id"] == row["carrierId"])
            if carrier["disposition"] != row["disposition"]:
                raise QualifiedDifferentialError(
                    f"unsupportedContent[{index}] preservation disposition "
                    "contradicts its carrier"
                )
        if row["recordId"] is not None:
            expected_prefix = f"/records/{_pointer_token(row['recordId'])}/"
            if not row["source"].startswith(expected_prefix):
                raise QualifiedDifferentialError(
                    f"unsupportedContent[{index}] source path contradicts its record"
                )
        elif row["source"].startswith("/records/"):
            raise QualifiedDifferentialError(
                f"recordless unsupportedContent[{index}] cannot use a record path"
            )
    if schema == ADAPTER_RESULT_SCHEMA_V3:
        _validate_adapter_field_capabilities(
            records, unsupported, adapter["side"]
        )
    failure = document["status"]["firstFailure"]
    if document["status"]["outcome"] == "failure":
        if failure["diagnosticId"] not in diagnostic_ids:
            raise QualifiedDifferentialError(
                "status.firstFailure.diagnosticId is dangling"
            )
        diagnostic = next(
            row for row in diagnostics if row["id"] == failure["diagnosticId"]
        )
        first_error = next(
            (row for row in diagnostics if row["severity"] == "error"), None
        )
        if first_error is None or first_error["id"] != failure["diagnosticId"]:
            raise QualifiedDifferentialError(
                "status.firstFailure must reference the earliest error diagnostic"
            )
        if diagnostic["stage"] != failure["stage"] or diagnostic["path"] != failure["path"]:
            raise QualifiedDifferentialError(
                "first failure does not match its diagnostic stage/path"
            )
        if diagnostic["severity"] != "error":
            raise QualifiedDifferentialError("first failure diagnostic must be an error")
        expected_callback_code = (
            failure["stage"] == "callback"
            and failure["code"] == 1
            and diagnostic["code"] == "null-callback-payload-1"
        )
        operation_prefix = {
            "write": "write",
            "writeResult": "write-result",
            "readback": "readback",
        }.get(failure["stage"], "read")
        expected_operation_code = f"{operation_prefix}-error-{failure['code']}"
        if diagnostic["code"] != expected_operation_code and not expected_callback_code:
            raise QualifiedDifferentialError(
                "first failure code does not match its diagnostic code"
            )
        expected_path = _expected_failure_path(failure["stage"])
        if failure["path"] != expected_path:
            raise QualifiedDifferentialError(
                "first failure path does not match its operation"
            )
    elif any(row["severity"] == "error" for row in diagnostics):
        raise QualifiedDifferentialError(
            "successful status cannot contain an error diagnostic"
        )

    cardinality = _exact_keys(document["cardinality"], CARDINALITY_KEYS, "cardinality")
    actual_counts = {
        "callbackCount": len(callbacks),
        "recordCount": len(records),
        "fieldCount": field_count,
        "graphNodeCount": len(graph_nodes),
        "graphEdgeCount": len(graph_edges),
        "opaqueCarrierCount": len(opaque),
        "unsupportedContentCount": len(unsupported),
        "diagnosticCount": len(diagnostics),
    }
    for key, actual in actual_counts.items():
        declared = _integer(cardinality[key], f"cardinality.{key}", minimum=0)
        if declared != actual:
            raise QualifiedDifferentialError(
                f"cardinality.{key}={declared} but observed {actual}"
            )
    if schema == ADAPTER_RESULT_SCHEMA_V3:
        _validate_write_result_readback_version(
            document["invocation"], document["status"],
            document["observations"], records,
        )
    return document


RECEIPT_KEYS = {
    "schema", "kind", "adapter", "targetLock", "artifacts", "linkClosure", "build"
}
TARGET_LOCK_RECEIPT_KEYS = {
    "path",
    "digest",
    "targetCommit",
    "snapshotRevision",
    "archiveDigest",
    "manifestDigest",
    "manifestEntries",
}
ARTIFACTS_KEYS = {"executable", "staticLibrary"}
ARTIFACT_KEYS = {"path", "size", "digest"}
LINK_CLOSURE_KEYS = {"algorithm", "digest", "artifacts"}
LINK_ARTIFACT_KEYS = {"role", "path", "size", "digest"}
BUILD_KEYS = {
    "configuration",
    "cmakeVersion",
    "generator",
    "compilerId",
    "compilerVersion",
    "wrapperDigest",
    "buildConfigDigest",
    "configIdentity",
    "buildParityIdentity",
    "buildParityDigest",
}
CONFIG_IDENTITY_KEYS = {
    "schema", "side", "configuration", "cmake", "inputs", "definitions",
    "warningPolicy", "targets"
}
CONFIG_CMAKE_KEYS = {
    "cmakeVersion", "configuredCmakeVersion", "cmakeExecutable", "generator",
    "multiConfig", "generatorPlatform", "generatorToolset", "generatorInstance",
    "configurationTypes", "compilerPath", "compilerId", "compilerVersion",
    "compilerFrontendVariant", "compilerSimulateId", "compilerSimulateVersion",
    "compilerTarget", "compilerArchitectureId", "compilerAbi",
    "compilerByteOrder", "sizeofDataPointer", "compilerAppleSysroot",
    "compilerLinkerPath", "compilerLinkerId", "compilerLinkerVersion",
    "compilerLinkerFrontendVariant", "archiverPath", "ranlibPath", "systemName",
    "systemVersion", "systemProcessor", "crossCompiling", "cxxStandard",
    "cxxExtensions", "cacheBuildType", "cxxFlags", "configurationFlags",
    "exeLinkerFlags", "configurationExeLinkerFlags", "staticLinkerFlags",
    "configurationStaticLinkerFlags", "toolchainFile", "sysroot",
    "sysrootCompile", "sysrootLink", "compilerLauncher", "linkerLauncher",
    "osxArchitectures", "osxSysroot", "osxDeploymentTarget",
    "msvcRuntimeLibrary", "windowsTargetPlatformVersion",
    "positionIndependentCode", "interproceduralOptimization",
    "configurationInterproceduralOptimization", "toolBinaries",
    "compilerBuiltinSpecs",
}
LEGACY_CONFIG_CMAKE_KEYS = CONFIG_CMAKE_KEYS - {
    "toolBinaries", "compilerBuiltinSpecs",
}
PRE_BUILTIN_SPECS_CONFIG_CMAKE_KEYS = CONFIG_CMAKE_KEYS - {
    "compilerBuiltinSpecs",
}
TOOL_BINARY_KEYS = {
    "cmake", "buildProgram", "compiler", "compilerLinker", "archiver",
    "ranlib",
}
LEGACY_TOOL_BINARY_KEYS = set(TOOL_BINARY_KEYS)
TOOL_BINARY_KEYS = TOOL_BINARY_KEYS | {
    "compilerFrontend", "compilerAssembler", "compilerCollect2",
    "compilerLtoWrapper", "compilerSpecs",
}
TOOL_BINARY_IDENTITY_KEYS = {"path", "digest"}
COMPILER_BUILTIN_SPECS_V1_KEYS = {
    "schema", "status", "method", "compilerId", "byteCount", "digest",
    "reason",
}
COMPILER_BUILTIN_SPECS_V2_KEYS = COMPILER_BUILTIN_SPECS_V1_KEYS | {
    "includeClosure",
}
GCC_SPECS_INCLUDE_CLOSURE_KEYS = {
    "schema", "status", "builtinDirectiveCount", "externalDirectiveCount",
    "reason",
}
COMPILER_BUILTIN_SPECS_METHOD = "gcc-dumpspecs-stdout-v1"
MAX_GCC_BUILTIN_SPECS_BYTES = 16 * 1024 * 1024
LEGACY_CONFIG_INPUT_KEYS = {
    "adapterSourceDigest", "semanticManifestPath", "semanticManifestDigest",
    "packageCommit", "packageArchiveDigest", "configFiles",
    "verifiedLinkCommandDigest", "linkClosureEnforced",
    "linkVerificationMethod", "linkedLibraryTarget",
    "verifiedCompileCommandDigest", "compileContractEnforced",
    "verifiedInterfaceContractDigest", "interfaceContractEnforced",
    "interfaceMethodCount",
}
DIRECT_ONLY_CONFIG_INPUT_KEYS = LEGACY_CONFIG_INPUT_KEYS | {
    "compileContractMode", "compileOptionAdmissionEnforced",
}
CONTROLLED_CONTEXT_CONFIG_INPUT_KEYS = DIRECT_ONLY_CONFIG_INPUT_KEYS | {
    "driverContext",
}
DIRECT_PROFILE_CONFIG_INPUT_KEYS = DIRECT_ONLY_CONFIG_INPUT_KEYS | {
    "interfaceProfile",
}
CONTROLLED_PROFILE_CONFIG_INPUT_KEYS = CONTROLLED_CONTEXT_CONFIG_INPUT_KEYS | {
    "interfaceProfile",
}
DIRECT_CONTRACT_PROFILE_CONFIG_INPUT_KEYS = DIRECT_PROFILE_CONFIG_INPUT_KEYS | {
    "compileContractProfile",
}
CONTROLLED_CONTRACT_PROFILE_CONFIG_INPUT_KEYS = CONTROLLED_PROFILE_CONFIG_INPUT_KEYS | {
    "compileContractProfile",
}
DIRECT_CAPABILITY_PROFILE_CONFIG_INPUT_KEYS = DIRECT_PROFILE_CONFIG_INPUT_KEYS | {
    "capabilityProfile",
}
CONTROLLED_CAPABILITY_PROFILE_CONFIG_INPUT_KEYS = CONTROLLED_PROFILE_CONFIG_INPUT_KEYS | {
    "capabilityProfile",
}
DIRECT_CONTRACT_CAPABILITY_PROFILE_CONFIG_INPUT_KEYS = (
    DIRECT_CONTRACT_PROFILE_CONFIG_INPUT_KEYS | {"capabilityProfile"}
)
CONTROLLED_CONTRACT_CAPABILITY_PROFILE_CONFIG_INPUT_KEYS = (
    CONTROLLED_CONTRACT_PROFILE_CONFIG_INPUT_KEYS | {"capabilityProfile"}
)
MODERN_CONFIG_INPUT_KEYSETS = {
    frozenset(DIRECT_ONLY_CONFIG_INPUT_KEYS),
    frozenset(CONTROLLED_CONTEXT_CONFIG_INPUT_KEYS),
    frozenset(DIRECT_PROFILE_CONFIG_INPUT_KEYS),
    frozenset(CONTROLLED_PROFILE_CONFIG_INPUT_KEYS),
    frozenset(DIRECT_CONTRACT_PROFILE_CONFIG_INPUT_KEYS),
    frozenset(CONTROLLED_CONTRACT_PROFILE_CONFIG_INPUT_KEYS),
    frozenset(DIRECT_CAPABILITY_PROFILE_CONFIG_INPUT_KEYS),
    frozenset(CONTROLLED_CAPABILITY_PROFILE_CONFIG_INPUT_KEYS),
    frozenset(DIRECT_CONTRACT_CAPABILITY_PROFILE_CONFIG_INPUT_KEYS),
    frozenset(CONTROLLED_CONTRACT_CAPABILITY_PROFILE_CONFIG_INPUT_KEYS),
}
ADAPTER_SUPPORT_INPUT_KEYS = {"path", "snapshotPath", "digest"}
COMPILE_CONTRACT_PROFILE_KEYS = {
    "schema", "name", "mode", "disposition", "checks", "digest",
}
COMPILE_CONTRACT_CHECK_KEYS = {
    "directOptions", "driverIdentity", "freshBuild", "ambientOverrides",
    "clangDefaultConfig", "implicitConfiguration", "effectiveOptionPolicy",
}
LEGACY_CONTROLLED_DRIVER_CONTEXT_KEYS = {
    "schema", "mode", "environmentDigest", "compilerPath",
    "compilerPathDigest", "compilerBinaryDigest", "digest",
}
CONTROLLED_DRIVER_CONTEXT_KEYS = LEGACY_CONTROLLED_DRIVER_CONTEXT_KEYS | {
    "cmakePath", "cmakePathDigest", "cmakeBinaryDigest",
}
EXPECTED_INTERFACE_METHOD_COUNT = 170
EXPECTED_COMMON_INTERFACE_DIGEST = (
    "894c58db480ef3f5c7e25a9066ca9a178ecf5e41137a9e709a1144f340faba7a"
)
EXPECTED_INTERFACE_PROFILE_NAME = "common170-standalone176-callbacks-v2"
LEGACY_INTERFACE_PROFILE_NAME = "common170-standalone173-callbacks-v1"
PREVIOUS_INTERFACE_PROFILE_NAME = "common170-standalone174-callbacks-v1"
LEGACY_STANDALONE_INTERFACE_EXTRAS = [
    "addBlockRecord",
    "addNamedObjectDictionary",
    "finalizeDxfWrite",
]
EXPECTED_STANDALONE_INTERFACE_EXTRAS = [
    "addBlockEnd",
    "addBlockRecord",
    "addDwgTemplateDescription",
    "addNamedObjectDictionary",
    "addProxySubentityMarker",
    "finalizeDxfWrite",
]
STANDALONE_INTERFACE_PROFILE_INVENTORIES = {
    LEGACY_INTERFACE_PROFILE_NAME: {
        "methodCount": 173,
        "extras": LEGACY_STANDALONE_INTERFACE_EXTRAS,
    },
    PREVIOUS_INTERFACE_PROFILE_NAME: {
        "methodCount": 174,
        "extras": [
            "addBlockEnd",
            *LEGACY_STANDALONE_INTERFACE_EXTRAS,
        ],
    },
    EXPECTED_INTERFACE_PROFILE_NAME: {
        "methodCount": 176,
        "extras": EXPECTED_STANDALONE_INTERFACE_EXTRAS,
    },
}
INTERFACE_PROFILE_KEYS = {
    "schema", "name", "side", "commonMethodCount", "commonMethodDigest",
    "interfaceMethodCount", "interfaceMethodDigest",
    "defaultedInterfaceExtras", "adapterSupplementalMethods",
    "unobservedDefaultedExtras",
}
EXPECTED_CAPABILITY_PROFILE_NAME = "semantic-adapter-capabilities-v1"
CAPABILITY_PROFILE_KEYS = {"schema", "name", "side", "capabilities"}
CAPABILITY_KEYS = {
    "dwgReadObservations",
    "modelerDwgSourceVersion",
    "modelerDwgAcisPayload",
    "modelerDxfPayloadChunks",
    "modelerPayloadStatusName",
    "payloadRangeBitOffset",
    "surfaceDxfPayloadChunks",
}
EXPECTED_CAPABILITIES_BY_SIDE = {
    "target": {key: False for key in CAPABILITY_KEYS},
    "standalone": {key: True for key in CAPABILITY_KEYS},
}
CONFIG_FILE_KEYS = {"path", "digest"}
CONFIG_DEFINITION_KEYS = {
    "sideMacro", "adapterName", "adapterCommit", "embeddedConfigDigest"
}
CONFIG_WARNING_KEYS = {"library", "adapter"}
CONFIG_TARGET_KEYS = {"executable", "staticLibrary"}
BUILD_PARITY_BASE_KEYS = {
    "schema", "configuration", "cmake", "adapterCompile", "libraryCompile",
    "adapterLink",
}
LEGACY_BUILD_PARITY_KEYS = set(BUILD_PARITY_BASE_KEYS)
COMPILER_DEPENDENCY_BUILD_PARITY_KEYS = BUILD_PARITY_BASE_KEYS | {
    "compilerDependencies",
}
COMPILER_RESOURCE_BUILD_PARITY_KEYS = COMPILER_DEPENDENCY_BUILD_PARITY_KEYS | {
    "compilerResources",
}
APPLE_SDK_BUILD_PARITY_KEYS = COMPILER_RESOURCE_BUILD_PARITY_KEYS | {"appleSdk"}
APPLE_RUNTIME_BUILD_PARITY_KEYS = APPLE_SDK_BUILD_PARITY_KEYS | {"appleRuntime"}
BUILD_PARITY_KEYS = APPLE_RUNTIME_BUILD_PARITY_KEYS | {"appleLinkInputs"}
COMPILER_DEPENDENCY_KEYS = {
    "schema", "status", "method", "translationUnitCount", "externalInputs",
    "externalInputsDigest", "reason",
}
COMPILER_DEPENDENCY_FILE_KEYS = {"path", "size", "digest"}
COMPILER_RESOURCE_KEYS = {
    "schema", "status", "method", "compilerId", "path", "entryCount",
    "fileCount", "totalBytes", "treeDigest", "reason",
}
APPLE_SDK_KEYS = {
    "schema", "status", "method", "systemName", "compilerId", "roots",
    "rootsDigest", "reason",
}
APPLE_SDK_ROOT_KEYS = {
    "selectedPaths", "resolvedPath", "metadataFiles", "metadataDigest",
}
APPLE_SDK_SELECTION_KEYS = {"source", "path"}
APPLE_SDK_METADATA_FILE_KEYS = {"path", "size", "digest"}
APPLE_RUNTIME_KEYS = {
    "schema", "status", "method", "systemName", "compilerId",
    "executableDigest", "tool", "nodes", "edges", "nodesDigest",
    "edgesDigest", "unresolvedImageCount", "unmatchedSdkStubCount",
    "maximumDepth", "reason",
}
APPLE_RUNTIME_V2_KEYS = APPLE_RUNTIME_KEYS | {
    "linkerSelectedStubs", "linkerSelectedStubsDigest",
    "unselectedDirectInstallNameCount",
}
APPLE_RUNTIME_TOOL_KEYS = {"path", "digest"}
APPLE_RUNTIME_NODE_KEYS = {
    "installName", "directLinkInput", "images", "sdkStubs",
}
APPLE_RUNTIME_IMAGE_KEYS = {"architecture", "uuid"}
APPLE_RUNTIME_STUB_KEYS = {
    "sdkRoot", "path", "resolvedPath", "size", "digest", "installName",
    "currentVersion", "compatibilityVersion",
}
APPLE_RUNTIME_SELECTED_STUB_KEYS = {
    "installName", "sdkRoot", "path", "size", "digest",
}
APPLE_RUNTIME_EDGE_KEYS = {"source", "target", "architecture", "attribute"}
APPLE_LINK_INPUTS_KEYS = {
    "schema", "status", "method", "systemName", "compilerId",
    "architecture", "executableDigest", "mapSize", "mapDigest",
    "objectCount", "synthesizedObjectCount", "buildObjects",
    "buildObjectsDigest", "linkedArchive", "archiveMembers",
    "archiveMembersDigest", "externalInputs", "externalInputsDigest",
    "unclassifiedCount", "unclassifiedDigest", "reason",
}
APPLE_LINK_ARCHIVE_KEYS = {"path", "size", "digest"}
APPLE_LINK_BUILD_OBJECT_KEYS = {"path", "size", "digest"}
APPLE_LINK_EXTERNAL_INPUT_KEYS = {"sdkRoot", "path", "size", "digest"}
COMPILE_PROFILE_KEYS = {
    "language", "languageStandard", "flags", "defines", "includes", "sysroot"
}
INCLUDE_PROFILE_KEYS = {"path", "isSystem"}
LINK_PROFILE_KEYS = {"language", "fragments", "sysroot"}
LINK_FRAGMENT_KEYS = {"role", "tokens"}


def _closure_digest(rows: list[dict[str, Any]]) -> str:
    tuples: list[bytes] = []
    for row in rows:
        tuples.append(
            (f"{row['role']}\0{row['path']}\0{row['size']}\0{row['digest']}\n")
            .encode("utf-8")
        )
    return hashlib.sha256(b"".join(sorted(tuples))).hexdigest()


def _secure_receipt_artifact_path(receipt_path: Path, relative: str,
                                  label: str) -> Path:
    _path_hint(relative, label)
    root = receipt_path.parent.resolve()
    lexical = receipt_path.parent / PurePosixPath(relative)
    current = receipt_path.parent
    for part in PurePosixPath(relative).parts:
        current = current / part
        if current.is_symlink():
            raise QualifiedDifferentialError(f"{label} traverses a symlink")
    try:
        resolved = lexical.resolve(strict=True)
    except OSError as exc:
        raise QualifiedDifferentialError(f"{label} is absent: {lexical}") from exc
    try:
        resolved.relative_to(root)
    except ValueError as exc:
        raise QualifiedDifferentialError(f"{label} escapes its receipt directory") from exc
    try:
        mode = resolved.stat().st_mode
    except OSError as exc:
        raise QualifiedDifferentialError(f"cannot inspect {label}: {exc}") from exc
    if not stat.S_ISREG(mode):
        raise QualifiedDifferentialError(f"{label} is not a regular file")
    return resolved


def _string_array(value: Any, path: str, *, require_unique: bool = False) -> list[str]:
    rows = _list(value, path)
    parsed = [_string(row, f"{path}[{index}]", allow_empty=False)
              for index, row in enumerate(rows)]
    if require_unique and len(parsed) != len(set(parsed)):
        raise QualifiedDifferentialError(f"{path} must contain unique strings")
    return parsed


def _validate_compiler_builtin_specs_profile(
    value: Any, path: str, compiler_id: str
) -> dict[str, Any]:
    if not isinstance(value, dict):
        raise QualifiedDifferentialError(f"{path} must be an object")
    schema = value.get("schema")
    if type(schema) is not int or schema not in (1, 2):
        raise QualifiedDifferentialError(f"{path}.schema is unsupported")
    profile = _exact_keys(
        value,
        (COMPILER_BUILTIN_SPECS_V1_KEYS if schema == 1
         else COMPILER_BUILTIN_SPECS_V2_KEYS),
        path,
    )
    status = _string(profile["status"], f"{path}.status", allow_empty=False)
    method = _string(profile["method"], f"{path}.method", allow_empty=True)
    reported_compiler = _string(
        profile["compilerId"], f"{path}.compilerId", allow_empty=False
    )
    byte_count = _integer(profile["byteCount"], f"{path}.byteCount", minimum=0)
    digest = _string(profile["digest"], f"{path}.digest", allow_empty=True)
    reason = _string(profile["reason"], f"{path}.reason", allow_empty=True)
    if reported_compiler != compiler_id:
        raise QualifiedDifferentialError(
            f"{path}.compilerId contradicts CMake compiler identity"
        )
    if status == "captured":
        if (compiler_id != "GNU" or method != COMPILER_BUILTIN_SPECS_METHOD
                or byte_count == 0 or byte_count > MAX_GCC_BUILTIN_SPECS_BYTES
                or reason):
            raise QualifiedDifferentialError(
                f"{path} captured built-in specs profile is inconsistent"
            )
        _digest(digest, f"{path}.digest")
    elif status == "unavailable":
        allowed_reasons = {
            "compiler-family-not-qualified",
            "compiler-path-unavailable",
            "gcc-built-in-specs-query-failed",
            "gcc-built-in-specs-size-limit",
            "gcc-built-in-specs-empty",
            "existing-build-not-scanned-before-compilation",
        }
        if (method or byte_count or digest or reason not in allowed_reasons
                or (compiler_id != "GNU"
                    and reason != "compiler-family-not-qualified")):
            raise QualifiedDifferentialError(
                f"{path} unavailable built-in specs profile is inconsistent"
            )
    else:
        raise QualifiedDifferentialError(f"{path}.status is unsupported")
    if schema == 2:
        closure = _exact_keys(
            profile["includeClosure"], GCC_SPECS_INCLUDE_CLOSURE_KEYS,
            f"{path}.includeClosure",
        )
        if type(closure["schema"]) is not int or closure["schema"] != 1:
            raise QualifiedDifferentialError(
                f"{path}.includeClosure.schema must be 1"
            )
        closure_status = _string(
            closure["status"], f"{path}.includeClosure.status", allow_empty=False
        )
        builtin_count = _integer(
            closure["builtinDirectiveCount"],
            f"{path}.includeClosure.builtinDirectiveCount", minimum=0,
        )
        external_count = _integer(
            closure["externalDirectiveCount"],
            f"{path}.includeClosure.externalDirectiveCount", minimum=0,
        )
        closure_reason = _string(
            closure["reason"], f"{path}.includeClosure.reason", allow_empty=True
        )
        if closure_status == "no-includes-detected":
            if (status != "captured" or compiler_id != "GNU"
                    or builtin_count or external_count or closure_reason):
                raise QualifiedDifferentialError(
                    f"{path}.includeClosure no-include result is inconsistent"
                )
        elif closure_status == "partial":
            if (status != "captured" or compiler_id != "GNU"
                    or not (builtin_count or external_count)
                    or closure_reason
                    != "spec-includes-not-resolved-or-content-bound"):
                raise QualifiedDifferentialError(
                    f"{path}.includeClosure partial result is inconsistent"
                )
        elif closure_status == "unavailable":
            allowed_closure_reasons = {
                "compiler-family-not-qualified",
                "compiler-path-unavailable",
                "gcc-built-in-specs-query-failed",
                "gcc-built-in-specs-size-limit",
                "gcc-built-in-specs-empty",
                "existing-build-not-scanned-before-compilation",
                "external-specs-file-identity-unavailable",
                "external-specs-file-unreadable",
                "external-specs-file-size-limit",
            }
            if (closure_reason not in allowed_closure_reasons
                    or (status == "unavailable"
                        and (closure_reason != reason or builtin_count
                             or external_count))):
                raise QualifiedDifferentialError(
                    f"{path}.includeClosure unavailable result is inconsistent"
                )
        else:
            raise QualifiedDifferentialError(
                f"{path}.includeClosure.status is unsupported"
            )
    return profile


def _validate_cmake_identity(value: Any, path: str,
                             configuration: str) -> dict[str, Any]:
    if isinstance(value, dict) and set(value) == LEGACY_CONFIG_CMAKE_KEYS:
        # Preserve already-issued receipts while requiring tool binary
        # identities from newly generated builder output.
        cmake = _exact_keys(value, LEGACY_CONFIG_CMAKE_KEYS, path)
    elif (isinstance(value, dict)
          and set(value) == PRE_BUILTIN_SPECS_CONFIG_CMAKE_KEYS):
        cmake = _exact_keys(value, PRE_BUILTIN_SPECS_CONFIG_CMAKE_KEYS, path)
    else:
        cmake = _exact_keys(value, CONFIG_CMAKE_KEYS, path)
    required_strings = {
        "cmakeVersion", "configuredCmakeVersion", "cmakeExecutable", "generator",
        "compilerPath", "compilerId", "compilerVersion", "sizeofDataPointer",
        "systemName",
    }
    array_keys = {"configurationTypes", "osxArchitectures"}
    bool_keys = {"multiConfig", "crossCompiling", "cxxExtensions"}
    for key in sorted(
        CONFIG_CMAKE_KEYS - array_keys - bool_keys - {
            "cxxStandard", "toolBinaries", "compilerBuiltinSpecs",
        }
    ):
        _string(cmake[key], f"{path}.{key}", allow_empty=key not in required_strings)
    if not cmake["sizeofDataPointer"].isdigit() or int(cmake["sizeofDataPointer"]) <= 0:
        raise QualifiedDifferentialError(f"{path}.sizeofDataPointer is invalid")
    configuration_types = _string_array(
        cmake["configurationTypes"], f"{path}.configurationTypes",
        require_unique=True,
    )
    _string_array(
        cmake["osxArchitectures"], f"{path}.osxArchitectures",
        require_unique=True,
    )
    if "toolBinaries" in cmake:
        binary_rows = cmake["toolBinaries"]
        if (isinstance(binary_rows, dict)
                and set(binary_rows) == LEGACY_TOOL_BINARY_KEYS):
            tools = _exact_keys(binary_rows, LEGACY_TOOL_BINARY_KEYS,
                                f"{path}.toolBinaries")
        else:
            tools = _exact_keys(binary_rows, TOOL_BINARY_KEYS,
                                f"{path}.toolBinaries")
        for role, identity in tools.items():
            row = _exact_keys(identity, TOOL_BINARY_IDENTITY_KEYS,
                              f"{path}.toolBinaries.{role}")
            _string(row["path"], f"{path}.toolBinaries.{role}.path",
                    allow_empty=True)
            _string(row["digest"], f"{path}.toolBinaries.{role}.digest",
                    allow_empty=True)
            if bool(row["path"]) != bool(row["digest"]):
                raise QualifiedDifferentialError(
                    f"{path}.toolBinaries.{role} path/digest presence differs"
                )
            if row["digest"]:
                _digest(row["digest"], f"{path}.toolBinaries.{role}.digest")
        for role in ("cmake", "buildProgram", "compiler"):
            if not tools[role]["path"] or not tools[role]["digest"]:
                raise QualifiedDifferentialError(
                    f"{path}.toolBinaries.{role} identity is required"
                )
        expected_tool_paths = {
            "cmake": cmake["cmakeExecutable"],
            "compiler": cmake["compilerPath"],
            "compilerLinker": cmake["compilerLinkerPath"],
            "archiver": cmake["archiverPath"],
            "ranlib": cmake["ranlibPath"],
        }
        for role, expected in expected_tool_paths.items():
            if tools[role]["path"] != expected:
                raise QualifiedDifferentialError(
                    f"{path}.toolBinaries.{role} path contradicts CMake metadata"
                )
    if "compilerBuiltinSpecs" in cmake:
        _validate_compiler_builtin_specs_profile(
            cmake["compilerBuiltinSpecs"],
            f"{path}.compilerBuiltinSpecs",
            cmake["compilerId"],
        )
    if not isinstance(cmake["multiConfig"], bool):
        raise QualifiedDifferentialError(f"{path}.multiConfig must be boolean")
    if not isinstance(cmake["crossCompiling"], bool):
        raise QualifiedDifferentialError(f"{path}.crossCompiling must be boolean")
    if cmake["cxxExtensions"] is not False:
        raise QualifiedDifferentialError(f"{path}.cxxExtensions must be false")
    if _integer(cmake["cxxStandard"], f"{path}.cxxStandard") != 17:
        raise QualifiedDifferentialError("semantic adapter must use strict C++17")
    if cmake["multiConfig"]:
        if not configuration_types or configuration not in configuration_types:
            raise QualifiedDifferentialError(
                f"{path}.configurationTypes omits the selected configuration"
            )
        if cmake["cacheBuildType"] not in {"", configuration}:
            raise QualifiedDifferentialError(
                f"{path}.cacheBuildType contradicts the selected configuration"
            )
    else:
        if configuration_types:
            raise QualifiedDifferentialError(
                f"{path}.configurationTypes must be empty for a single-config build"
            )
        if cmake["cacheBuildType"] != configuration:
            raise QualifiedDifferentialError(
                f"{path}.cacheBuildType contradicts the selected configuration"
            )
    return cmake


def _validate_compile_profile(value: Any, path: str) -> dict[str, Any]:
    profile = _exact_keys(value, COMPILE_PROFILE_KEYS, path)
    if profile["language"] != "CXX" or profile["languageStandard"] != "17":
        raise QualifiedDifferentialError(f"{path} is not a C++17 profile")
    _string_array(profile["flags"], f"{path}.flags")
    defines = _string_array(
        profile["defines"], f"{path}.defines", require_unique=True
    )
    if defines != sorted(defines):
        raise QualifiedDifferentialError(f"{path}.defines must be sorted")
    includes = _list(profile["includes"], f"{path}.includes")
    for index, item in enumerate(includes):
        row = _exact_keys(item, INCLUDE_PROFILE_KEYS, f"{path}.includes[{index}]")
        _string(row["path"], f"{path}.includes[{index}].path")
        if not isinstance(row["isSystem"], bool):
            raise QualifiedDifferentialError(
                f"{path}.includes[{index}].isSystem must be boolean"
            )
    _string(profile["sysroot"], f"{path}.sysroot", allow_empty=True)
    return profile


def _validate_link_profile(value: Any, path: str) -> dict[str, Any]:
    profile = _exact_keys(value, LINK_PROFILE_KEYS, path)
    if profile["language"] != "CXX":
        raise QualifiedDifferentialError(f"{path} is not a C++ link profile")
    fragments = _list(profile["fragments"], f"{path}.fragments")
    for index, item in enumerate(fragments):
        row = _exact_keys(item, LINK_FRAGMENT_KEYS, f"{path}.fragments[{index}]")
        _string(row["role"], f"{path}.fragments[{index}].role")
        tokens = _string_array(
            row["tokens"], f"{path}.fragments[{index}].tokens"
        )
        if not tokens:
            raise QualifiedDifferentialError(
                f"{path}.fragments[{index}].tokens must not be empty"
            )
    _string(profile["sysroot"], f"{path}.sysroot", allow_empty=True)
    return profile


def _validate_compiler_resources(value: Any, path: str) -> dict[str, Any]:
    profile = _exact_keys(value, COMPILER_RESOURCE_KEYS, path)
    if isinstance(profile["schema"], bool) or profile["schema"] != 1:
        raise QualifiedDifferentialError(f"{path}.schema is unsupported")
    compiler_id = _string(profile["compilerId"], f"{path}.compilerId")
    status = profile["status"]
    if status not in {"captured", "unavailable"}:
        raise QualifiedDifferentialError(f"{path}.status is unsupported")
    method = _string(profile["method"], f"{path}.method", allow_empty=True)
    resource_path = _string(profile["path"], f"{path}.path", allow_empty=True)
    entry_count = _integer(profile["entryCount"], f"{path}.entryCount", minimum=0)
    file_count = _integer(profile["fileCount"], f"{path}.fileCount", minimum=0)
    total_bytes = _integer(profile["totalBytes"], f"{path}.totalBytes", minimum=0)
    tree_digest = _digest(profile["treeDigest"], f"{path}.treeDigest")
    reason = _string(profile["reason"], f"{path}.reason", allow_empty=True)
    if status == "captured":
        if (compiler_id not in {"Clang", "AppleClang"}
                or method != "clang-resource-tree-v1"
                or not Path(resource_path).is_absolute()
                or entry_count < 1 or file_count < 1 or total_bytes < 1
                or reason):
            raise QualifiedDifferentialError(
                f"{path} captured resource profile is inconsistent"
            )
    else:
        allowed_reasons = {
            "compiler-family-not-qualified", "compiler-path-unavailable",
            "clang-resource-query-failed", "clang-resource-query-malformed",
            "clang-resource-path-not-absolute", "clang-resource-capture-budget",
            "clang-resource-tree-unavailable",
        }
        expected_empty_digest = hashlib.sha256(_canonical_bytes([]) + b"\n").hexdigest()
        if (method or resource_path or entry_count or file_count or total_bytes
                or tree_digest != expected_empty_digest or reason not in allowed_reasons):
            raise QualifiedDifferentialError(
                f"{path} unavailable resource profile is inconsistent"
            )
    return profile


def _validate_apple_sdk_profile(value: Any, path: str) -> dict[str, Any]:
    profile = _exact_keys(value, APPLE_SDK_KEYS, path)
    if isinstance(profile["schema"], bool) or profile["schema"] != 1:
        raise QualifiedDifferentialError(f"{path}.schema is unsupported")
    status = profile["status"]
    if status not in {"captured", "unavailable"}:
        raise QualifiedDifferentialError(f"{path}.status is unsupported")
    method = _string(profile["method"], f"{path}.method", allow_empty=True)
    system_name = _string(profile["systemName"], f"{path}.systemName")
    compiler_id = _string(profile["compilerId"], f"{path}.compilerId")
    roots = _list(profile["roots"], f"{path}.roots")
    roots_digest = _digest(profile["rootsDigest"], f"{path}.rootsDigest")
    reason = _string(profile["reason"], f"{path}.reason", allow_empty=True)
    if status == "captured":
        if (system_name != "Darwin"
                or compiler_id not in {"AppleClang", "Clang"}
                or method != "apple-sdk-settings-v1"
                or not roots or reason):
            raise QualifiedDifferentialError(
                f"{path} captured Apple SDK profile is inconsistent"
            )
        normalized_roots: list[dict[str, Any]] = []
        resolved_paths: list[str] = []
        for index, item in enumerate(roots):
            root_path = f"{path}.roots[{index}]"
            root = _exact_keys(item, APPLE_SDK_ROOT_KEYS, root_path)
            selected_paths = _list(
                root["selectedPaths"], f"{root_path}.selectedPaths"
            )
            if not selected_paths:
                raise QualifiedDifferentialError(
                    f"{root_path}.selectedPaths must not be empty"
                )
            normalized_selections: list[dict[str, str]] = []
            for selection_index, selection_value in enumerate(selected_paths):
                selection_path = (
                    f"{root_path}.selectedPaths[{selection_index}]"
                )
                selection = _exact_keys(
                    selection_value, APPLE_SDK_SELECTION_KEYS, selection_path
                )
                source = _string(selection["source"], f"{selection_path}.source")
                selected = _string(selection["path"], f"{selection_path}.path")
                if source not in {
                    "CMAKE_SYSROOT", "CMAKE_SYSROOT_COMPILE",
                    "CMAKE_SYSROOT_LINK", "CMAKE_OSX_SYSROOT",
                    "CMAKE_CXX_COMPILER_APPLE_SYSROOT",
                } or not Path(selected).is_absolute():
                    raise QualifiedDifferentialError(
                        f"{selection_path} is not a supported absolute sysroot"
                    )
                normalized_selections.append({"source": source, "path": selected})
            if normalized_selections != sorted(
                    normalized_selections,
                    key=lambda row: (row["source"], row["path"])):
                raise QualifiedDifferentialError(
                    f"{root_path}.selectedPaths must be sorted"
                )
            resolved = _string(root["resolvedPath"], f"{root_path}.resolvedPath")
            if not Path(resolved).is_absolute():
                raise QualifiedDifferentialError(
                    f"{root_path}.resolvedPath must be absolute"
                )
            resolved_paths.append(resolved)
            metadata_files = _list(
                root["metadataFiles"], f"{root_path}.metadataFiles"
            )
            metadata_rows: list[dict[str, Any]] = []
            for metadata_index, metadata_value in enumerate(metadata_files):
                metadata_path = f"{root_path}.metadataFiles[{metadata_index}]"
                metadata = _exact_keys(
                    metadata_value, APPLE_SDK_METADATA_FILE_KEYS, metadata_path
                )
                name = _string(metadata["path"], f"{metadata_path}.path")
                size = _integer(metadata["size"], f"{metadata_path}.size", minimum=0)
                digest = _digest(metadata["digest"], f"{metadata_path}.digest")
                if name not in {"SDKSettings.json", "SDKSettings.plist"}:
                    raise QualifiedDifferentialError(
                        f"{metadata_path}.path is not a recognized SDK manifest"
                    )
                if size > 32 * 1024 * 1024:
                    raise QualifiedDifferentialError(
                        f"{metadata_path}.size exceeds the SDK manifest budget"
                    )
                metadata_rows.append({"path": name, "size": size, "digest": digest})
            metadata_names = [row["path"] for row in metadata_rows]
            if (metadata_names != sorted(set(metadata_names))
                    or "SDKSettings.json" not in metadata_names):
                raise QualifiedDifferentialError(
                    f"{root_path}.metadataFiles must include sorted unique SDKSettings.json"
                )
            metadata_digest = _digest(
                root["metadataDigest"], f"{root_path}.metadataDigest"
            )
            expected_metadata_digest = hashlib.sha256(
                _canonical_bytes(metadata_rows) + b"\n"
            ).hexdigest()
            if metadata_digest != expected_metadata_digest:
                raise QualifiedDifferentialError(
                    f"{root_path}.metadataDigest does not bind its manifests"
                )
            normalized_roots.append({
                "selectedPaths": normalized_selections,
                "resolvedPath": resolved,
                "metadataFiles": metadata_rows,
                "metadataDigest": metadata_digest,
            })
        if resolved_paths != sorted(set(resolved_paths)):
            raise QualifiedDifferentialError(
                f"{path}.roots must be sorted by unique resolvedPath"
            )
        expected_roots_digest = hashlib.sha256(
            _canonical_bytes(normalized_roots) + b"\n"
        ).hexdigest()
        if roots_digest != expected_roots_digest:
            raise QualifiedDifferentialError(
                f"{path}.rootsDigest does not bind its SDK roots"
            )
    else:
        allowed_reasons = {
            "platform-not-qualified", "apple-sdk-root-unavailable",
            "apple-sdk-path-unavailable", "apple-sdk-settings-unavailable",
            "apple-sdk-metadata-budget",
        }
        expected_empty_digest = hashlib.sha256(
            _canonical_bytes([]) + b"\n"
        ).hexdigest()
        if (method or roots or roots_digest != expected_empty_digest
                or reason not in allowed_reasons):
            raise QualifiedDifferentialError(
                f"{path} unavailable Apple SDK profile is inconsistent"
            )
        if reason == "platform-not-qualified" and (
                system_name == "Darwin"
                and compiler_id in {"AppleClang", "Clang"}):
            raise QualifiedDifferentialError(
                f"{path} marks a supported Apple toolchain not-applicable"
            )
    return profile


def _validate_apple_runtime_profile(value: Any, path: str) -> dict[str, Any]:
    if not isinstance(value, dict):
        raise QualifiedDifferentialError(f"{path} must be an object")
    schema = value.get("schema")
    if isinstance(schema, bool) or schema not in {1, 2}:
        raise QualifiedDifferentialError(f"{path}.schema is unsupported")
    profile = _exact_keys(
        value, APPLE_RUNTIME_KEYS if schema == 1 else APPLE_RUNTIME_V2_KEYS, path
    )
    status = profile["status"]
    if status not in {"partial", "unavailable"}:
        raise QualifiedDifferentialError(
            f"{path}.status must remain partial or unavailable"
        )
    method = _string(profile["method"], f"{path}.method", allow_empty=True)
    system_name = _string(profile["systemName"], f"{path}.systemName")
    compiler_id = _string(profile["compilerId"], f"{path}.compilerId")
    executable_digest = _string(
        profile["executableDigest"], f"{path}.executableDigest", allow_empty=True
    )
    tool = _exact_keys(profile["tool"], APPLE_RUNTIME_TOOL_KEYS, f"{path}.tool")
    tool_path = _string(tool["path"], f"{path}.tool.path", allow_empty=True)
    tool_digest = _string(
        tool["digest"], f"{path}.tool.digest", allow_empty=True
    )
    nodes = _list(profile["nodes"], f"{path}.nodes")
    edges = _list(profile["edges"], f"{path}.edges")
    nodes_digest = _digest(profile["nodesDigest"], f"{path}.nodesDigest")
    edges_digest = _digest(profile["edgesDigest"], f"{path}.edgesDigest")
    unresolved_count = _integer(
        profile["unresolvedImageCount"],
        f"{path}.unresolvedImageCount", minimum=0,
    )
    unmatched_stub_count = _integer(
        profile["unmatchedSdkStubCount"],
        f"{path}.unmatchedSdkStubCount", minimum=0,
    )
    maximum_depth = _integer(
        profile["maximumDepth"], f"{path}.maximumDepth", minimum=0
    )
    reason = _string(profile["reason"], f"{path}.reason", allow_empty=True)
    selected_stub_rows: list[dict[str, Any]] = []
    selected_stub_digest = ""
    unselected_direct_count = 0
    if schema == 2:
        selected_values = _list(
            profile["linkerSelectedStubs"], f"{path}.linkerSelectedStubs"
        )
        selected_stub_digest = _digest(
            profile["linkerSelectedStubsDigest"],
            f"{path}.linkerSelectedStubsDigest",
        )
        unselected_direct_count = _integer(
            profile["unselectedDirectInstallNameCount"],
            f"{path}.unselectedDirectInstallNameCount", minimum=0,
        )
        for index, item in enumerate(selected_values):
            row_path = f"{path}.linkerSelectedStubs[{index}]"
            row = _exact_keys(item, APPLE_RUNTIME_SELECTED_STUB_KEYS, row_path)
            install_name = _string(row["installName"], f"{row_path}.installName")
            sdk_root = _string(row["sdkRoot"], f"{row_path}.sdkRoot")
            input_path = _string(row["path"], f"{row_path}.path")
            size = _integer(row["size"], f"{row_path}.size", minimum=1)
            digest = _digest(row["digest"], f"{row_path}.digest")
            pure_input_path = PurePosixPath(input_path)
            if (not install_name.startswith(("/", "@"))
                    or not PurePosixPath(sdk_root).is_absolute()
                    or pure_input_path.is_absolute()
                    or ".." in pure_input_path.parts
                    or pure_input_path.as_posix() != input_path):
                raise QualifiedDifferentialError(
                    f"{row_path} is not a normalized SDK-selected input"
                )
            selected_stub_rows.append({
                "installName": install_name,
                "sdkRoot": sdk_root,
                "path": input_path,
                "size": size,
                "digest": digest,
            })
        selected_keys = [(
            row["installName"], row["sdkRoot"], row["path"]
        ) for row in selected_stub_rows]
        if selected_keys != sorted(set(selected_keys)):
            raise QualifiedDifferentialError(
                f"{path}.linkerSelectedStubs must be sorted and unique"
            )
        expected_selected_digest = hashlib.sha256(
            _canonical_bytes(selected_stub_rows) + b"\n"
        ).hexdigest()
        if selected_stub_digest != expected_selected_digest:
            raise QualifiedDifferentialError(
                f"{path}.linkerSelectedStubsDigest is stale"
            )
    expected_empty_digest = hashlib.sha256(
        _canonical_bytes([]) + b"\n"
    ).hexdigest()
    if status == "unavailable":
        allowed_reasons = {
            "platform-not-qualified", "apple-sdk-profile-unavailable",
            "dyld-info-unavailable", "macho-inspection-failed",
            "macho-load-command-malformed",
        }
        if (schema != 1 or method or executable_digest or tool_path or tool_digest or nodes or edges
                or nodes_digest != expected_empty_digest
                or edges_digest != expected_empty_digest
                or unresolved_count or unmatched_stub_count or maximum_depth
                or reason not in allowed_reasons):
            raise QualifiedDifferentialError(
                f"{path} unavailable Apple runtime profile is inconsistent"
            )
        if reason == "platform-not-qualified" and (
                system_name == "Darwin"
                and compiler_id in {"AppleClang", "Clang"}):
            raise QualifiedDifferentialError(
                f"{path} marks a supported Apple runtime not-applicable"
            )
        return profile

    allowed_reasons = {"link-stub-selection-not-proven", "runtime-closure-limit"}
    if schema == 2:
        allowed_reasons.add("runtime-byte-identity-not-proven")
    if (system_name != "Darwin"
            or compiler_id not in {"AppleClang", "Clang"}
            or method != "macho-dyld-uuid-closure-v1"
            or reason not in allowed_reasons
            or maximum_depth > 32
            or len(nodes) > 4096 or len(edges) > 100_000):
        raise QualifiedDifferentialError(
            f"{path} partial Apple runtime profile is inconsistent"
        )
    _digest(executable_digest, f"{path}.executableDigest")
    if (not tool_path or not PurePosixPath(tool_path).is_absolute()
            or not re.fullmatch(r"[0-9a-f]{64}", tool_digest)):
        raise QualifiedDifferentialError(f"{path}.tool identity is malformed")

    node_rows: list[dict[str, Any]] = []
    install_names: list[str] = []
    for index, item in enumerate(nodes):
        node_path = f"{path}.nodes[{index}]"
        node = _exact_keys(item, APPLE_RUNTIME_NODE_KEYS, node_path)
        install_name = _string(node["installName"], f"{node_path}.installName")
        direct_link_input = node["directLinkInput"]
        if not isinstance(direct_link_input, bool):
            raise QualifiedDifferentialError(
                f"{node_path}.directLinkInput must be boolean"
            )
        if (not install_name.startswith(("/", "@")) or "\n" in install_name
                or "\x00" in install_name):
            raise QualifiedDifferentialError(
                f"{node_path}.installName is malformed"
            )
        install_names.append(install_name)
        images = _list(node["images"], f"{node_path}.images")
        normalized_images: list[dict[str, str]] = []
        for image_index, image_value in enumerate(images):
            image_path = f"{node_path}.images[{image_index}]"
            image = _exact_keys(image_value, APPLE_RUNTIME_IMAGE_KEYS, image_path)
            architecture = _string(image["architecture"], f"{image_path}.architecture")
            uuid = _string(image["uuid"], f"{image_path}.uuid")
            if not re.fullmatch(
                    r"[0-9A-Fa-f]{8}(?:-[0-9A-Fa-f]{4}){3}-[0-9A-Fa-f]{12}",
                    uuid):
                raise QualifiedDifferentialError(f"{image_path}.uuid is malformed")
            normalized_images.append({
                "architecture": architecture, "uuid": uuid.upper(),
            })
        image_keys = [(row["architecture"], row["uuid"]) for row in normalized_images]
        if image_keys != sorted(set(image_keys)):
            raise QualifiedDifferentialError(
                f"{node_path}.images must be sorted and unique"
            )
        stub_values = _list(node["sdkStubs"], f"{node_path}.sdkStubs")
        normalized_stubs: list[dict[str, Any]] = []
        for stub_index, stub_value in enumerate(stub_values):
            stub_path = f"{node_path}.sdkStubs[{stub_index}]"
            stub = _exact_keys(stub_value, APPLE_RUNTIME_STUB_KEYS, stub_path)
            sdk_root = _string(stub["sdkRoot"], f"{stub_path}.sdkRoot")
            path_value = _string(stub["path"], f"{stub_path}.path")
            resolved_path = _string(stub["resolvedPath"], f"{stub_path}.resolvedPath")
            size = _integer(stub["size"], f"{stub_path}.size", minimum=1)
            digest = _digest(stub["digest"], f"{stub_path}.digest")
            stub_install_name = _string(
                stub["installName"], f"{stub_path}.installName"
            )
            current_version = _string(
                stub["currentVersion"], f"{stub_path}.currentVersion", allow_empty=True
            )
            compatibility_version = _string(
                stub["compatibilityVersion"],
                f"{stub_path}.compatibilityVersion", allow_empty=True,
            )
            if (not PurePosixPath(sdk_root).is_absolute()
                    or not PurePosixPath(path_value).is_absolute()
                    or not PurePosixPath(resolved_path).is_absolute()
                    or size > 64 * 1024 * 1024
                    or stub_install_name != install_name):
                raise QualifiedDifferentialError(
                    f"{stub_path} is not a bounded matching SDK stub"
                )
            normalized_stubs.append({
                "sdkRoot": sdk_root,
                "path": path_value,
                "resolvedPath": resolved_path,
                "size": size,
                "digest": digest,
                "installName": stub_install_name,
                "currentVersion": current_version,
                "compatibilityVersion": compatibility_version,
            })
        stub_keys = [(row["sdkRoot"], row["path"]) for row in normalized_stubs]
        if stub_keys != sorted(set(stub_keys)):
            raise QualifiedDifferentialError(
                f"{node_path}.sdkStubs must be sorted and unique"
            )
        node_rows.append({
            "installName": install_name,
            "directLinkInput": direct_link_input,
            "images": normalized_images,
            "sdkStubs": normalized_stubs,
        })
    if install_names != sorted(set(install_names)):
        raise QualifiedDifferentialError(
            f"{path}.nodes must be sorted and unique by installName"
        )

    edge_rows: list[dict[str, str]] = []
    for index, item in enumerate(edges):
        edge_path = f"{path}.edges[{index}]"
        edge = _exact_keys(item, APPLE_RUNTIME_EDGE_KEYS, edge_path)
        source = _string(edge["source"], f"{edge_path}.source")
        target = _string(edge["target"], f"{edge_path}.target")
        architecture = _string(edge["architecture"], f"{edge_path}.architecture")
        attribute = _string(edge["attribute"], f"{edge_path}.attribute")
        if (source != "adapter-executable" and not source.startswith(("/", "@"))
                or not target.startswith(("/", "@"))
                or not re.fullmatch(r"[A-Za-z][A-Za-z-]*", attribute)):
            raise QualifiedDifferentialError(f"{edge_path} is malformed")
        edge_rows.append({
            "source": source, "target": target,
            "architecture": architecture, "attribute": attribute,
        })
    edge_keys = [(
        row["source"], row["target"], row["architecture"], row["attribute"]
    ) for row in edge_rows]
    if edge_keys != sorted(set(edge_keys)):
        raise QualifiedDifferentialError(f"{path}.edges must be sorted and unique")
    direct_targets = {
        row["target"] for row in edge_rows
        if row["source"] == "adapter-executable"
    }
    direct_nodes = {
        node["installName"] for node in node_rows if node["directLinkInput"]
    }
    if not direct_targets.issubset(direct_nodes):
        raise QualifiedDifferentialError(
            f"{path} omits a direct adapter runtime dependency node"
        )
    if direct_nodes != direct_targets:
        raise QualifiedDifferentialError(
            f"{path} directLinkInput flags disagree with the executable edges"
        )
    expected_nodes_digest = hashlib.sha256(
        _canonical_bytes(node_rows) + b"\n"
    ).hexdigest()
    expected_edges_digest = hashlib.sha256(
        _canonical_bytes(edge_rows) + b"\n"
    ).hexdigest()
    if nodes_digest != expected_nodes_digest or edges_digest != expected_edges_digest:
        raise QualifiedDifferentialError(
            f"{path} runtime graph digest does not bind its content"
        )
    if unresolved_count != sum(1 for node in node_rows if not node["images"]):
        raise QualifiedDifferentialError(
            f"{path}.unresolvedImageCount is stale"
        )
    if unmatched_stub_count != sum(
            1 for node in node_rows
            if node["directLinkInput"] and not node["sdkStubs"]):
        raise QualifiedDifferentialError(
            f"{path}.unmatchedSdkStubCount is stale"
        )
    if reason == "link-stub-selection-not-proven" and unresolved_count and not edges:
        raise QualifiedDifferentialError(
            f"{path} has unresolved runtime images without dependency edges"
        )
    return profile


def _validate_apple_runtime_linker_selection(
    runtime_profile: dict[str, Any],
    link_inputs_profile: dict[str, Any],
    path: str,
) -> None:
    """Cross-check selected runtime stubs against exact linker-map inputs."""
    if runtime_profile.get("schema") != 2:
        return
    external_inputs = link_inputs_profile.get("externalInputs")
    if not isinstance(external_inputs, list):
        raise QualifiedDifferentialError(
            f"{path}.appleLinkInputs.externalInputs is malformed"
        )
    selected_input_keys = {
        (row["sdkRoot"], row["path"], row["size"], row["digest"])
        for row in external_inputs
    }
    nodes = runtime_profile["nodes"]
    direct_names: set[str] = set()
    selected_names: set[str] = set()
    expected_rows: list[dict[str, Any]] = []
    for node in nodes:
        if not node["directLinkInput"]:
            continue
        install_name = node["installName"]
        direct_names.add(install_name)
        for candidate in node["sdkStubs"]:
            sdk_root = candidate["sdkRoot"]
            try:
                relative_path = Path(candidate["resolvedPath"]).relative_to(
                    Path(sdk_root)
                ).as_posix()
            except ValueError:
                raise QualifiedDifferentialError(
                    f"{path}.appleRuntime candidate resolves outside its SDK root"
                ) from None
            identity = (
                sdk_root, relative_path, candidate["size"], candidate["digest"]
            )
            if identity not in selected_input_keys:
                continue
            expected_rows.append({
                "installName": install_name,
                "sdkRoot": sdk_root,
                "path": relative_path,
                "size": candidate["size"],
                "digest": candidate["digest"],
            })
            selected_names.add(install_name)
    expected_rows.sort(key=lambda row: (
        row["installName"], row["sdkRoot"], row["path"]
    ))
    if runtime_profile["linkerSelectedStubs"] != expected_rows:
        raise QualifiedDifferentialError(
            f"{path}.appleRuntime linker-selected stubs do not match runtime candidates and linker inputs"
        )
    expected_unselected = len(direct_names - selected_names)
    if runtime_profile["unselectedDirectInstallNameCount"] != expected_unselected:
        raise QualifiedDifferentialError(
            f"{path}.appleRuntime unselected direct install-name count is stale"
        )
    if runtime_profile["reason"] != "runtime-closure-limit":
        expected_reason = (
            "runtime-byte-identity-not-proven"
            if (link_inputs_profile.get("status") == "captured"
                and selected_names == direct_names)
            else "link-stub-selection-not-proven"
        )
        if runtime_profile["reason"] != expected_reason:
            raise QualifiedDifferentialError(
                f"{path}.appleRuntime reason disagrees with selected linker stubs"
            )


def _validate_apple_link_inputs_profile(value: Any, path: str) -> dict[str, Any]:
    profile = _exact_keys(value, APPLE_LINK_INPUTS_KEYS, path)
    if isinstance(profile["schema"], bool) or profile["schema"] != 1:
        raise QualifiedDifferentialError(f"{path}.schema is unsupported")
    status = profile["status"]
    if status not in {"captured", "partial", "unavailable"}:
        raise QualifiedDifferentialError(f"{path}.status is unsupported")
    method = _string(profile["method"], f"{path}.method", allow_empty=True)
    system_name = _string(profile["systemName"], f"{path}.systemName")
    compiler_id = _string(profile["compilerId"], f"{path}.compilerId")
    architecture = _string(
        profile["architecture"], f"{path}.architecture", allow_empty=True
    )
    executable_digest = _string(
        profile["executableDigest"], f"{path}.executableDigest", allow_empty=True
    )
    map_size = _integer(profile["mapSize"], f"{path}.mapSize", minimum=0)
    map_digest = _string(profile["mapDigest"], f"{path}.mapDigest", allow_empty=True)
    object_count = _integer(profile["objectCount"], f"{path}.objectCount", minimum=0)
    synthesized_count = _integer(
        profile["synthesizedObjectCount"],
        f"{path}.synthesizedObjectCount", minimum=0,
    )
    build_objects = _list(profile["buildObjects"], f"{path}.buildObjects")
    build_objects_digest = _digest(
        profile["buildObjectsDigest"], f"{path}.buildObjectsDigest"
    )
    archive = _exact_keys(
        profile["linkedArchive"], APPLE_LINK_ARCHIVE_KEYS, f"{path}.linkedArchive"
    )
    archive_path = _string(archive["path"], f"{path}.linkedArchive.path", allow_empty=True)
    archive_size = _integer(archive["size"], f"{path}.linkedArchive.size", minimum=0)
    archive_digest = _string(
        archive["digest"], f"{path}.linkedArchive.digest", allow_empty=True
    )
    archive_members = _list(profile["archiveMembers"], f"{path}.archiveMembers")
    archive_members_digest = _digest(
        profile["archiveMembersDigest"], f"{path}.archiveMembersDigest"
    )
    external_inputs = _list(profile["externalInputs"], f"{path}.externalInputs")
    external_inputs_digest = _digest(
        profile["externalInputsDigest"], f"{path}.externalInputsDigest"
    )
    unclassified_count = _integer(
        profile["unclassifiedCount"], f"{path}.unclassifiedCount", minimum=0
    )
    unclassified_digest = _digest(
        profile["unclassifiedDigest"], f"{path}.unclassifiedDigest"
    )
    reason = _string(profile["reason"], f"{path}.reason", allow_empty=True)

    empty_digest = hashlib.sha256(_canonical_bytes([]) + b"\n").hexdigest()
    if status == "unavailable":
        allowed_reasons = {
            "platform-not-qualified", "apple-sdk-profile-unavailable",
        }
        if (method or architecture or executable_digest or map_size or map_digest
                or object_count or synthesized_count or build_objects
                or build_objects_digest != empty_digest or archive_path
                or archive_size or archive_digest or archive_members
                or archive_members_digest != empty_digest or external_inputs
                or external_inputs_digest != empty_digest or unclassified_count
                or unclassified_digest != empty_digest or reason not in allowed_reasons):
            raise QualifiedDifferentialError(
                f"{path} unavailable Apple linker input profile is inconsistent"
            )
        if (reason == "platform-not-qualified" and system_name == "Darwin"
                and compiler_id in {"AppleClang", "Clang"}):
            raise QualifiedDifferentialError(
                f"{path} marks a supported Apple linker not-applicable"
            )
        return profile

    if (system_name != "Darwin"
            or compiler_id not in {"AppleClang", "Clang"}
            or method != "apple-ld-map-v1"
            or not re.fullmatch(r"[A-Za-z0-9_+-]{1,32}", architecture)
            or not re.fullmatch(r"[0-9a-f]{64}", executable_digest)
            or map_size < 1 or map_size > 64 * 1024 * 1024
            or not re.fullmatch(r"[0-9a-f]{64}", map_digest)
            or not 1 <= object_count <= 16_384
            or synthesized_count > object_count
            or not 0 < archive_size <= 512 * 1024 * 1024
            or not re.fullmatch(r"[0-9a-f]{64}", archive_digest)
            or not archive_path.startswith("<build>/")
            or ".." in PurePosixPath(archive_path).parts):
        raise QualifiedDifferentialError(
            f"{path} captured Apple linker input profile is inconsistent"
        )
    _digest(build_objects_digest, f"{path}.buildObjectsDigest")
    _digest(archive_members_digest, f"{path}.archiveMembersDigest")
    if len(build_objects) > object_count or len(archive_members) > object_count:
        raise QualifiedDifferentialError(f"{path} input counts exceed map object count")

    normalized_build_objects: list[dict[str, Any]] = []
    previous_build_path = ""
    for index, value_item in enumerate(build_objects):
        row_path = f"{path}.buildObjects[{index}]"
        row = _exact_keys(value_item, APPLE_LINK_BUILD_OBJECT_KEYS, row_path)
        object_path = _string(row["path"], f"{row_path}.path")
        object_size = _integer(row["size"], f"{row_path}.size", minimum=1)
        digest = _digest(row["digest"], f"{row_path}.digest")
        parts = PurePosixPath(object_path).parts
        if (PurePosixPath(object_path).is_absolute() or ".." in parts
                or object_path <= previous_build_path):
            raise QualifiedDifferentialError(
                f"{row_path}.path must be sorted, relative, and traversal-free"
            )
        previous_build_path = object_path
        normalized_build_objects.append({
            "path": object_path, "size": object_size, "digest": digest,
        })
    if hashlib.sha256(_canonical_bytes(normalized_build_objects) + b"\n").hexdigest() != build_objects_digest:
        raise QualifiedDifferentialError(f"{path}.buildObjectsDigest is stale")

    normalized_members: list[str] = []
    previous_member = ""
    for index, member_value in enumerate(archive_members):
        member = _string(member_value, f"{path}.archiveMembers[{index}]")
        if ("/" in member or "\\" in member or member <= previous_member):
            raise QualifiedDifferentialError(
                f"{path}.archiveMembers must be sorted unique basenames"
            )
        previous_member = member
        normalized_members.append(member)
    if not normalized_members or hashlib.sha256(
            _canonical_bytes(normalized_members) + b"\n").hexdigest() != archive_members_digest:
        raise QualifiedDifferentialError(f"{path}.archiveMembersDigest is stale")

    normalized_external: list[dict[str, Any]] = []
    previous_external_key = ("", "")
    for index, value_item in enumerate(external_inputs):
        row_path = f"{path}.externalInputs[{index}]"
        row = _exact_keys(value_item, APPLE_LINK_EXTERNAL_INPUT_KEYS, row_path)
        sdk_root = _string(row["sdkRoot"], f"{row_path}.sdkRoot")
        input_path = _string(row["path"], f"{row_path}.path")
        size = _integer(row["size"], f"{row_path}.size", minimum=1)
        digest = _digest(row["digest"], f"{row_path}.digest")
        pure_path = PurePosixPath(input_path)
        key = (sdk_root, input_path)
        if (not PurePosixPath(sdk_root).is_absolute() or pure_path.is_absolute()
                or ".." in pure_path.parts or key <= previous_external_key):
            raise QualifiedDifferentialError(
                f"{row_path} is not a sorted SDK-relative input identity"
            )
        previous_external_key = key
        normalized_external.append({
            "sdkRoot": sdk_root, "path": input_path,
            "size": size, "digest": digest,
        })
    if hashlib.sha256(_canonical_bytes(normalized_external) + b"\n").hexdigest() != external_inputs_digest:
        raise QualifiedDifferentialError(f"{path}.externalInputsDigest is stale")

    if status == "captured":
        if (unclassified_count or reason
                or unclassified_digest != empty_digest
                or synthesized_count + len(normalized_build_objects)
                   + len(normalized_members) + len(normalized_external)
                   != object_count):
            raise QualifiedDifferentialError(
                f"{path} captured profile does not classify every map row"
            )
    elif (unclassified_count < 1 or reason != "unclassified-link-inputs"
          or unclassified_digest == empty_digest
          or synthesized_count + len(normalized_build_objects)
             + len(normalized_members) + len(normalized_external)
             + unclassified_count != object_count):
        raise QualifiedDifferentialError(
            f"{path} partial profile has inconsistent unclassified rows"
        )
    return profile


def _validate_compiler_dependencies(value: Any, path: str) -> dict[str, Any]:
    profile = _exact_keys(value, COMPILER_DEPENDENCY_KEYS, path)
    if isinstance(profile["schema"], bool) or profile["schema"] != 1:
        raise QualifiedDifferentialError(f"{path}.schema is unsupported")
    status = profile["status"]
    if status not in {"captured", "unavailable"}:
        raise QualifiedDifferentialError(f"{path}.status is unsupported")
    _string(profile["method"], f"{path}.method", allow_empty=True)
    _string(profile["reason"], f"{path}.reason", allow_empty=True)
    units = _integer(profile["translationUnitCount"],
                     f"{path}.translationUnitCount", minimum=0)
    inputs = _list(profile["externalInputs"], f"{path}.externalInputs")
    _digest(profile["externalInputsDigest"],
            f"{path}.externalInputsDigest")
    canonical_inputs: list[dict[str, Any]] = []
    previous_path = ""
    for index, item in enumerate(inputs):
        row = _exact_keys(item, COMPILER_DEPENDENCY_FILE_KEYS,
                          f"{path}.externalInputs[{index}]")
        dependency_path = _string(
            row["path"], f"{path}.externalInputs[{index}].path"
        )
        if not Path(dependency_path).is_absolute():
            raise QualifiedDifferentialError(
                f"{path}.externalInputs[{index}].path must be absolute"
            )
        size = _integer(row["size"],
                        f"{path}.externalInputs[{index}].size", minimum=0)
        digest = _digest(row["digest"],
                         f"{path}.externalInputs[{index}].digest")
        if dependency_path <= previous_path:
            raise QualifiedDifferentialError(
                f"{path}.externalInputs must be sorted and unique by path"
            )
        previous_path = dependency_path
        canonical_inputs.append({
            "path": dependency_path, "size": size, "digest": digest,
        })
    expected_closure_digest = hashlib.sha256(
        _canonical_bytes(canonical_inputs) + b"\n"
    ).hexdigest()
    if profile["externalInputsDigest"] != expected_closure_digest:
        raise QualifiedDifferentialError(
            f"{path}.externalInputsDigest is stale"
        )
    if status == "captured":
        if (profile["method"] != "gnu-clang-m-dependency-scan-v1"
                or not units or profile["reason"]):
            raise QualifiedDifferentialError(
                f"{path} captured dependency profile is inconsistent"
            )
    else:
        if (profile["method"] or profile["reason"] not in {
                "windows-depfile-grammar-not-qualified",
                "multi-config-compile-database-not-qualified",
                "compiler-family-not-qualified",
                "compile-database-not-generated",
                "existing-build-not-scanned-before-compilation",
        } or units or inputs):
            raise QualifiedDifferentialError(
                f"{path} unavailable dependency profile is inconsistent"
            )
    return profile


def _validate_build_parity_identity(value: Any, path: str,
                                    configuration: str) -> dict[str, Any]:
    if isinstance(value, dict) and set(value) == LEGACY_BUILD_PARITY_KEYS:
        identity = _exact_keys(value, LEGACY_BUILD_PARITY_KEYS, path)
    elif (isinstance(value, dict)
          and set(value) == COMPILER_DEPENDENCY_BUILD_PARITY_KEYS):
        identity = _exact_keys(
            value, COMPILER_DEPENDENCY_BUILD_PARITY_KEYS, path
        )
    elif (isinstance(value, dict)
          and set(value) == COMPILER_RESOURCE_BUILD_PARITY_KEYS):
        identity = _exact_keys(
            value, COMPILER_RESOURCE_BUILD_PARITY_KEYS, path
        )
    elif (isinstance(value, dict)
          and set(value) == APPLE_SDK_BUILD_PARITY_KEYS):
        identity = _exact_keys(value, APPLE_SDK_BUILD_PARITY_KEYS, path)
    elif (isinstance(value, dict)
          and set(value) == APPLE_RUNTIME_BUILD_PARITY_KEYS):
        identity = _exact_keys(value, APPLE_RUNTIME_BUILD_PARITY_KEYS, path)
    else:
        identity = _exact_keys(value, BUILD_PARITY_KEYS, path)
    if identity["schema"] != 1 or identity["configuration"] != configuration:
        raise QualifiedDifferentialError(f"{path} schema/configuration is stale")
    _validate_cmake_identity(identity["cmake"], f"{path}.cmake", configuration)
    _validate_compile_profile(identity["adapterCompile"], f"{path}.adapterCompile")
    _validate_compile_profile(identity["libraryCompile"], f"{path}.libraryCompile")
    _validate_link_profile(identity["adapterLink"], f"{path}.adapterLink")
    if "compilerDependencies" in identity:
        _validate_compiler_dependencies(
            identity["compilerDependencies"],
            f"{path}.compilerDependencies",
        )
    if "compilerResources" in identity:
        _validate_compiler_resources(
            identity["compilerResources"], f"{path}.compilerResources"
        )
    if "appleSdk" in identity:
        _validate_apple_sdk_profile(identity["appleSdk"], f"{path}.appleSdk")
    runtime_profile = None
    if "appleRuntime" in identity:
        runtime_profile = _validate_apple_runtime_profile(
            identity["appleRuntime"], f"{path}.appleRuntime"
        )
    link_inputs = None
    if "appleLinkInputs" in identity:
        link_inputs = _validate_apple_link_inputs_profile(
            identity["appleLinkInputs"], f"{path}.appleLinkInputs"
        )
        if link_inputs["status"] != "unavailable":
            if (link_inputs["systemName"] != identity["cmake"]["systemName"]
                    or link_inputs["compilerId"] != identity["cmake"]["compilerId"]):
                raise QualifiedDifferentialError(
                    f"{path}.appleLinkInputs toolchain identity differs from CMake"
                )
            sdk_profile = identity.get("appleSdk")
            if not isinstance(sdk_profile, dict):
                raise QualifiedDifferentialError(
                    f"{path}.appleLinkInputs has no Apple SDK profile"
                )
            sdk_roots = {
                row["resolvedPath"] for row in sdk_profile.get("roots", [])
                if isinstance(row, dict) and isinstance(row.get("resolvedPath"), str)
            }
            if any(row["sdkRoot"] not in sdk_roots
                   for row in link_inputs["externalInputs"]):
                raise QualifiedDifferentialError(
                    f"{path}.appleLinkInputs names an SDK root outside its profile"
                )
    if isinstance(runtime_profile, dict) and runtime_profile.get("schema") == 2:
        if not isinstance(link_inputs, dict):
            raise QualifiedDifferentialError(
                f"{path}.appleRuntime schema 2 requires Apple linker-input evidence"
            )
        _validate_apple_runtime_linker_selection(
            runtime_profile, link_inputs, path
        )
    return identity


def validate_receipt(value: Any, *, receipt_path: Path | None = None,
                     check_artifacts: bool = False) -> dict[str, Any]:
    """Validate a build receipt and, optionally, its on-disk artifacts."""

    receipt = _exact_keys(value, RECEIPT_KEYS, "receipt")
    if receipt["schema"] != RECEIPT_SCHEMA or receipt["kind"] != RECEIPT_KIND:
        raise QualifiedDifferentialError("unexpected build receipt schema or kind")
    adapter = _validate_adapter(receipt["adapter"])
    if adapter["side"] == "target":
        lock = _exact_keys(receipt["targetLock"], TARGET_LOCK_RECEIPT_KEYS,
                           "receipt.targetLock")
        _path_hint(lock["path"], "receipt.targetLock.path")
        _digest(lock["digest"], "receipt.targetLock.digest")
        _commit(lock["targetCommit"], "receipt.targetLock.targetCommit")
        _commit(lock["snapshotRevision"], "receipt.targetLock.snapshotRevision")
        _digest(lock["archiveDigest"], "receipt.targetLock.archiveDigest")
        _digest(lock["manifestDigest"], "receipt.targetLock.manifestDigest")
        _integer(lock["manifestEntries"], "receipt.targetLock.manifestEntries", minimum=1)
    elif receipt["targetLock"] is not None:
        raise QualifiedDifferentialError(
            "standalone build receipt targetLock must be null"
        )

    artifacts = _exact_keys(receipt["artifacts"], ARTIFACTS_KEYS,
                            "receipt.artifacts")
    parsed_artifacts: dict[str, dict[str, Any]] = {}
    for key in sorted(ARTIFACTS_KEYS):
        artifact = _exact_keys(artifacts[key], ARTIFACT_KEYS,
                               f"receipt.artifacts.{key}")
        _path_hint(artifact["path"], f"receipt.artifacts.{key}.path")
        _integer(artifact["size"], f"receipt.artifacts.{key}.size", minimum=1)
        _digest(artifact["digest"], f"receipt.artifacts.{key}.digest")
        parsed_artifacts[key] = artifact
    if parsed_artifacts["executable"]["digest"] != adapter["binaryDigest"]:
        raise QualifiedDifferentialError(
            "receipt executable digest does not match adapter.binaryDigest"
        )
    if (parsed_artifacts["staticLibrary"]["digest"]
            != adapter["staticLibraryDigest"]):
        raise QualifiedDifferentialError(
            "receipt static library digest does not match adapter.staticLibraryDigest"
        )

    closure = _exact_keys(receipt["linkClosure"], LINK_CLOSURE_KEYS,
                          "receipt.linkClosure")
    if closure["algorithm"] != "sha256-nul-tuples-v1":
        raise QualifiedDifferentialError("unsupported link-closure algorithm")
    _digest(closure["digest"], "receipt.linkClosure.digest")
    closure_rows = _list(closure["artifacts"], "receipt.linkClosure.artifacts")
    if not closure_rows:
        raise QualifiedDifferentialError("link closure must not be empty")
    closure_keys: list[tuple[str, str]] = []
    for index, item in enumerate(closure_rows):
        row = _exact_keys(item, LINK_ARTIFACT_KEYS,
                          f"receipt.linkClosure.artifacts[{index}]")
        role = _identifier(row["role"],
                           f"receipt.linkClosure.artifacts[{index}].role")
        path = _path_hint(row["path"],
                          f"receipt.linkClosure.artifacts[{index}].path")
        _integer(row["size"],
                 f"receipt.linkClosure.artifacts[{index}].size", minimum=1)
        _digest(row["digest"],
                f"receipt.linkClosure.artifacts[{index}].digest")
        closure_keys.append((role, path))
    _unique((f"{role}\0{path}" for role, path in closure_keys),
            "receipt.linkClosure artifacts")
    if closure["digest"] != _closure_digest(closure_rows):
        raise QualifiedDifferentialError("link-closure digest is stale")
    if closure["digest"] != adapter["linkedClosureDigest"]:
        raise QualifiedDifferentialError(
            "link-closure digest does not match adapter identity"
        )

    build = _exact_keys(receipt["build"], BUILD_KEYS, "receipt.build")
    for key in (
        "configuration", "cmakeVersion", "generator", "compilerId",
        "compilerVersion",
    ):
        _string(build[key], f"receipt.build.{key}")
    _digest(build["wrapperDigest"], "receipt.build.wrapperDigest")
    _digest(build["buildConfigDigest"], "receipt.build.buildConfigDigest")
    _digest(build["buildParityDigest"], "receipt.build.buildParityDigest")
    identity = _exact_keys(
        build["configIdentity"], CONFIG_IDENTITY_KEYS,
        "receipt.build.configIdentity",
    )
    if identity["schema"] != 1 or identity["side"] != adapter["side"]:
        raise QualifiedDifferentialError("build config identity schema/side is stale")
    if identity["configuration"] != build["configuration"]:
        raise QualifiedDifferentialError("build configuration identity is stale")
    cmake = _validate_cmake_identity(
        identity["cmake"], "receipt.build.configIdentity.cmake",
        build["configuration"],
    )
    for key in ("cmakeVersion", "generator", "compilerId", "compilerVersion"):
        if cmake[key] != build[key]:
            raise QualifiedDifferentialError(f"build {key} identity is stale")
    raw_input_value = identity["inputs"]
    input_value = _without_adapter_support_inputs(
        raw_input_value, "receipt.build.configIdentity.inputs"
    )
    if isinstance(input_value, dict) and set(input_value) == LEGACY_CONFIG_INPUT_KEYS:
        # Frozen schema-1 receipts retain their original replay semantics.
        inputs = _exact_keys(input_value, LEGACY_CONFIG_INPUT_KEYS,
                             "receipt.build.configIdentity.inputs")
    elif (isinstance(input_value, dict)
          and set(input_value)
              == CONTROLLED_CONTRACT_CAPABILITY_PROFILE_CONFIG_INPUT_KEYS):
        inputs = _exact_keys(
            input_value,
            CONTROLLED_CONTRACT_CAPABILITY_PROFILE_CONFIG_INPUT_KEYS,
            "receipt.build.configIdentity.inputs",
        )
        _validate_controlled_driver_context(
            inputs["driverContext"],
            "receipt.build.configIdentity.inputs.driverContext",
        )
        _validate_interface_profile(
            inputs["interfaceProfile"], adapter["side"],
            "receipt.build.configIdentity.inputs.interfaceProfile",
        )
        _validate_semantic_capability_profile(
            inputs["capabilityProfile"], adapter["side"],
            "receipt.build.configIdentity.inputs.capabilityProfile",
        )
    elif (isinstance(input_value, dict)
          and set(input_value)
              == DIRECT_CONTRACT_CAPABILITY_PROFILE_CONFIG_INPUT_KEYS):
        inputs = _exact_keys(
            input_value,
            DIRECT_CONTRACT_CAPABILITY_PROFILE_CONFIG_INPUT_KEYS,
            "receipt.build.configIdentity.inputs",
        )
        _validate_interface_profile(
            inputs["interfaceProfile"], adapter["side"],
            "receipt.build.configIdentity.inputs.interfaceProfile",
        )
        _validate_semantic_capability_profile(
            inputs["capabilityProfile"], adapter["side"],
            "receipt.build.configIdentity.inputs.capabilityProfile",
        )
    elif (isinstance(input_value, dict)
          and set(input_value) == CONTROLLED_CAPABILITY_PROFILE_CONFIG_INPUT_KEYS):
        inputs = _exact_keys(
            input_value, CONTROLLED_CAPABILITY_PROFILE_CONFIG_INPUT_KEYS,
            "receipt.build.configIdentity.inputs",
        )
        _validate_controlled_driver_context(
            inputs["driverContext"],
            "receipt.build.configIdentity.inputs.driverContext",
        )
        _validate_interface_profile(
            inputs["interfaceProfile"], adapter["side"],
            "receipt.build.configIdentity.inputs.interfaceProfile",
        )
        _validate_semantic_capability_profile(
            inputs["capabilityProfile"], adapter["side"],
            "receipt.build.configIdentity.inputs.capabilityProfile",
        )
    elif (isinstance(input_value, dict)
          and set(input_value) == DIRECT_CAPABILITY_PROFILE_CONFIG_INPUT_KEYS):
        inputs = _exact_keys(
            input_value, DIRECT_CAPABILITY_PROFILE_CONFIG_INPUT_KEYS,
            "receipt.build.configIdentity.inputs",
        )
        _validate_interface_profile(
            inputs["interfaceProfile"], adapter["side"],
            "receipt.build.configIdentity.inputs.interfaceProfile",
        )
        _validate_semantic_capability_profile(
            inputs["capabilityProfile"], adapter["side"],
            "receipt.build.configIdentity.inputs.capabilityProfile",
        )
    elif (isinstance(input_value, dict)
          and set(input_value) == CONTROLLED_CONTRACT_PROFILE_CONFIG_INPUT_KEYS):
        inputs = _exact_keys(
            input_value, CONTROLLED_CONTRACT_PROFILE_CONFIG_INPUT_KEYS,
            "receipt.build.configIdentity.inputs",
        )
        _validate_controlled_driver_context(
            inputs["driverContext"],
            "receipt.build.configIdentity.inputs.driverContext",
        )
        _validate_interface_profile(
            inputs["interfaceProfile"], adapter["side"],
            "receipt.build.configIdentity.inputs.interfaceProfile",
        )
    elif (isinstance(input_value, dict)
          and set(input_value) == DIRECT_CONTRACT_PROFILE_CONFIG_INPUT_KEYS):
        inputs = _exact_keys(
            input_value, DIRECT_CONTRACT_PROFILE_CONFIG_INPUT_KEYS,
            "receipt.build.configIdentity.inputs",
        )
        _validate_interface_profile(
            inputs["interfaceProfile"], adapter["side"],
            "receipt.build.configIdentity.inputs.interfaceProfile",
        )
    elif (isinstance(input_value, dict)
          and set(input_value) == CONTROLLED_PROFILE_CONFIG_INPUT_KEYS):
        inputs = _exact_keys(
            input_value, CONTROLLED_PROFILE_CONFIG_INPUT_KEYS,
            "receipt.build.configIdentity.inputs",
        )
        _validate_controlled_driver_context(
            inputs["driverContext"],
            "receipt.build.configIdentity.inputs.driverContext",
        )
        _validate_interface_profile(
            inputs["interfaceProfile"], adapter["side"],
            "receipt.build.configIdentity.inputs.interfaceProfile",
        )
    elif (isinstance(input_value, dict)
          and set(input_value) == CONTROLLED_CONTEXT_CONFIG_INPUT_KEYS):
        inputs = _exact_keys(
            input_value, CONTROLLED_CONTEXT_CONFIG_INPUT_KEYS,
            "receipt.build.configIdentity.inputs",
        )
        _validate_controlled_driver_context(
            inputs["driverContext"],
            "receipt.build.configIdentity.inputs.driverContext",
        )
    elif (isinstance(input_value, dict)
          and set(input_value) == DIRECT_PROFILE_CONFIG_INPUT_KEYS):
        inputs = _exact_keys(
            input_value, DIRECT_PROFILE_CONFIG_INPUT_KEYS,
            "receipt.build.configIdentity.inputs",
        )
        _validate_interface_profile(
            inputs["interfaceProfile"], adapter["side"],
            "receipt.build.configIdentity.inputs.interfaceProfile",
        )
    else:
        inputs = _exact_keys(input_value, DIRECT_ONLY_CONFIG_INPUT_KEYS,
                             "receipt.build.configIdentity.inputs")
    _digest(inputs["adapterSourceDigest"],
            "receipt.build.configIdentity.inputs.adapterSourceDigest")
    _path_hint(inputs["semanticManifestPath"],
               "receipt.build.configIdentity.inputs.semanticManifestPath")
    _digest(inputs["semanticManifestDigest"],
            "receipt.build.configIdentity.inputs.semanticManifestDigest")
    _commit(inputs["packageCommit"],
            "receipt.build.configIdentity.inputs.packageCommit")
    if adapter["side"] == "target":
        _digest(inputs["packageArchiveDigest"],
                "receipt.build.configIdentity.inputs.packageArchiveDigest")
    elif inputs["packageArchiveDigest"] is not None:
        raise QualifiedDifferentialError(
            "standalone config identity packageArchiveDigest must be null"
        )
    config_files = _list(inputs["configFiles"],
                         "receipt.build.configIdentity.inputs.configFiles")
    if not config_files:
        raise QualifiedDifferentialError("build configFiles must not be empty")
    config_paths: list[str] = []
    for index, item in enumerate(config_files):
        config_file = _exact_keys(
            item, CONFIG_FILE_KEYS,
            f"receipt.build.configIdentity.inputs.configFiles[{index}]",
        )
        config_paths.append(_path_hint(
            config_file["path"],
            f"receipt.build.configIdentity.inputs.configFiles[{index}].path",
        ))
        _digest(config_file["digest"],
                f"receipt.build.configIdentity.inputs.configFiles[{index}].digest")
    if config_paths != sorted(config_paths) or len(config_paths) != len(set(config_paths)):
        raise QualifiedDifferentialError("build configFiles must be sorted and unique")
    _digest(inputs["verifiedLinkCommandDigest"],
            "receipt.build.configIdentity.inputs.verifiedLinkCommandDigest")
    if inputs["linkClosureEnforced"] is not True:
        raise QualifiedDifferentialError("build link closure was not enforced")
    _digest(inputs["verifiedCompileCommandDigest"],
            "receipt.build.configIdentity.inputs.verifiedCompileCommandDigest")
    if "compileContractProfile" in inputs:
        _validate_compile_contract_profile(
            inputs["compileContractProfile"],
            driver_context_present="driverContext" in inputs,
            compiler_id=cmake["compilerId"],
            path="receipt.build.configIdentity.inputs.compileContractProfile",
        )
    _validate_compile_contract_disposition(inputs)
    _digest(
        inputs["verifiedInterfaceContractDigest"],
        "receipt.build.configIdentity.inputs.verifiedInterfaceContractDigest",
    )
    if ("interfaceProfile" in inputs
            and inputs["verifiedInterfaceContractDigest"]
                != inputs["interfaceProfile"]["commonMethodDigest"]):
        raise QualifiedDifferentialError(
            "build interface contract digest does not match named common profile"
        )
    if inputs["interfaceContractEnforced"] is not True:
        raise QualifiedDifferentialError("adapter interface contract was not enforced")
    if _integer(
        inputs["interfaceMethodCount"],
        "receipt.build.configIdentity.inputs.interfaceMethodCount",
        minimum=1,
    ) != EXPECTED_INTERFACE_METHOD_COUNT:
        raise QualifiedDifferentialError(
            "adapter interface callback inventory count is stale"
        )
    if inputs["linkVerificationMethod"] not in {"link.txt", "cmake-file-api-v2"}:
        raise QualifiedDifferentialError("build link verification method is invalid")
    expected_library_target = (
        "libdxfrw_semantic_target" if adapter["side"] == "target" else "dxfrw"
    )
    if inputs["linkedLibraryTarget"] != expected_library_target:
        raise QualifiedDifferentialError("build linked-library target is stale")
    definitions = _exact_keys(
        identity["definitions"], CONFIG_DEFINITION_KEYS,
        "receipt.build.configIdentity.definitions",
    )
    expected_side_macro = (
        "LIBDXFRW_SEMANTIC_SIDE_TARGET=1" if adapter["side"] == "target"
        else "LIBDXFRW_SEMANTIC_SIDE_STANDALONE=1"
    )
    if definitions["sideMacro"] != expected_side_macro:
        raise QualifiedDifferentialError("build sideMacro identity is stale")
    _identifier(definitions["adapterName"],
                "receipt.build.configIdentity.definitions.adapterName")
    _commit(definitions["adapterCommit"],
            "receipt.build.configIdentity.definitions.adapterCommit")
    if definitions["adapterName"] != adapter["name"]:
        raise QualifiedDifferentialError("build adapterName identity is stale")
    if definitions["adapterCommit"] != adapter["commit"]:
        raise QualifiedDifferentialError("build adapterCommit identity is stale")
    if adapter["side"] == "target":
        _digest(definitions["embeddedConfigDigest"],
                "receipt.build.configIdentity.definitions.embeddedConfigDigest")
        if definitions["embeddedConfigDigest"] != adapter["configDigest"]:
            raise QualifiedDifferentialError("embedded config digest is stale")
    elif definitions["embeddedConfigDigest"] is not None:
        raise QualifiedDifferentialError(
            "standalone embeddedConfigDigest must be null"
        )
    warning = _exact_keys(
        identity["warningPolicy"], CONFIG_WARNING_KEYS,
        "receipt.build.configIdentity.warningPolicy",
    )
    _string(warning["library"],
            "receipt.build.configIdentity.warningPolicy.library")
    _string(warning["adapter"],
            "receipt.build.configIdentity.warningPolicy.adapter")
    targets = _exact_keys(identity["targets"], CONFIG_TARGET_KEYS,
                          "receipt.build.configIdentity.targets")
    _identifier(targets["executable"],
                "receipt.build.configIdentity.targets.executable")
    _identifier(targets["staticLibrary"],
                "receipt.build.configIdentity.targets.staticLibrary")
    if targets["executable"] != adapter["name"]:
        raise QualifiedDifferentialError("build executable target identity is stale")
    if inputs["adapterSourceDigest"] != adapter["sourceDigest"]:
        raise QualifiedDifferentialError("build adapter-source digest is stale")
    if inputs["semanticManifestDigest"] != adapter["configDigest"]:
        raise QualifiedDifferentialError("build semantic-manifest digest is stale")
    if inputs["packageCommit"] != adapter["commit"]:
        raise QualifiedDifferentialError("build package commit is stale")
    if adapter["side"] == "target":
        if lock["targetCommit"] != adapter["commit"]:
            raise QualifiedDifferentialError("target lock/adapter commits differ")
        if inputs["packageArchiveDigest"] != lock["archiveDigest"]:
            raise QualifiedDifferentialError(
                "target build archive digest does not match target lock"
            )
    expected_build_digest = hashlib.sha256(
        _canonical_bytes(identity) + b"\n"
    ).hexdigest()
    if build["buildConfigDigest"] != expected_build_digest:
        raise QualifiedDifferentialError("buildConfigDigest is stale")
    if build["wrapperDigest"] != config_files[0]["digest"]:
        raise QualifiedDifferentialError("wrapperDigest is not configFiles[0]")
    parity_identity = _validate_build_parity_identity(
        build["buildParityIdentity"], "receipt.build.buildParityIdentity",
        build["configuration"],
    )
    runtime_profile = parity_identity.get("appleRuntime")
    if (isinstance(runtime_profile, dict)
            and runtime_profile.get("status") == "partial"
            and runtime_profile.get("executableDigest") != adapter["binaryDigest"]):
        raise QualifiedDifferentialError(
            "Apple runtime profile is not bound to the adapter executable"
        )
    link_inputs_profile = parity_identity.get("appleLinkInputs")
    if (isinstance(link_inputs_profile, dict)
            and link_inputs_profile.get("status") != "unavailable"
            and link_inputs_profile.get("executableDigest") != adapter["binaryDigest"]):
        raise QualifiedDifferentialError(
            "Apple linker input profile is not bound to the adapter executable"
        )
    if parity_identity["cmake"] != cmake:
        raise QualifiedDifferentialError(
            "build parity/config CMake identities differ"
        )
    expected_parity_digest = hashlib.sha256(
        _canonical_bytes(parity_identity) + b"\n"
    ).hexdigest()
    if build["buildParityDigest"] != expected_parity_digest:
        raise QualifiedDifferentialError("buildParityDigest is stale")

    if check_artifacts:
        if receipt_path is None:
            raise QualifiedDifferentialError(
                "receipt path is required for artifact validation"
            )
        for key, artifact in parsed_artifacts.items():
            artifact_path = _secure_receipt_artifact_path(
                receipt_path, artifact["path"], f"receipt {key} artifact"
            )
            if artifact_path.stat().st_size != artifact["size"]:
                raise QualifiedDifferentialError(
                    f"receipt {key} artifact size is stale"
                )
            if _sha256_file(artifact_path) != artifact["digest"]:
                raise QualifiedDifferentialError(
                    f"receipt {key} artifact digest is stale"
                )
        for index, artifact in enumerate(closure_rows):
            artifact_path = _secure_receipt_artifact_path(
                receipt_path, artifact["path"],
                f"receipt link-closure artifact {index}",
            )
            if artifact_path.stat().st_size != artifact["size"]:
                raise QualifiedDifferentialError(
                    f"receipt link-closure artifact {index} size is stale"
                )
            if _sha256_file(artifact_path) != artifact["digest"]:
                raise QualifiedDifferentialError(
                    f"receipt link-closure artifact {index} digest is stale"
                )
    return receipt


def _validate_receipt_pair(target: dict[str, Any],
                           standalone: dict[str, Any]) -> None:
    target_build = target["build"]
    standalone_build = standalone["build"]
    target_parity = target_build["buildParityIdentity"]
    standalone_parity = standalone_build["buildParityIdentity"]
    target_dependencies = target_parity.get("compilerDependencies")
    standalone_dependencies = standalone_parity.get("compilerDependencies")
    target_link_inputs = target_parity.get("appleLinkInputs")
    standalone_link_inputs = standalone_parity.get("appleLinkInputs")
    target_core = {
        key: value for key, value in target_parity.items()
        if key not in {"compilerDependencies", "appleLinkInputs"}
    }
    standalone_core = {
        key: value for key, value in standalone_parity.items()
        if key not in {"compilerDependencies", "appleLinkInputs"}
    }
    for core in (target_core, standalone_core):
        runtime_profile = core.get("appleRuntime")
        if isinstance(runtime_profile, dict):
            core["appleRuntime"] = {
                key: value for key, value in runtime_profile.items()
                if key != "executableDigest"
            }
    if target_core != standalone_core:
        raise QualifiedDifferentialError(
            "target/standalone effective build profiles differ"
        )
    if (target_link_inputs is None) != (standalone_link_inputs is None):
        raise QualifiedDifferentialError(
            "target/standalone Apple linker-input evidence is asymmetric"
        )
    if isinstance(target_link_inputs, dict):
        assert isinstance(standalone_link_inputs, dict)
        for key in ("status", "method", "systemName", "compilerId"):
            if target_link_inputs[key] != standalone_link_inputs[key]:
                raise QualifiedDifferentialError(
                    f"target/standalone Apple linker-input {key} differs"
                )
        if target_link_inputs["status"] == "unavailable":
            if target_link_inputs != standalone_link_inputs:
                raise QualifiedDifferentialError(
                    "target/standalone unavailable Apple linker-input reasons differ"
                )
        elif target_link_inputs["status"] != "captured":
            raise QualifiedDifferentialError(
                "partial Apple linker-input profiles cannot qualify a receipt pair"
            )
        else:
            target_inputs = {
                (row["sdkRoot"], row["path"]): row
                for row in target_link_inputs["externalInputs"]
            }
            standalone_inputs = {
                (row["sdkRoot"], row["path"]): row
                for row in standalone_link_inputs["externalInputs"]
            }
            for key in sorted(target_inputs.keys() & standalone_inputs.keys()):
                if target_inputs[key] != standalone_inputs[key]:
                    raise QualifiedDifferentialError(
                        "target/standalone selected SDK link input differs: "
                        + key[1]
                    )
    if (target_dependencies is None) != (standalone_dependencies is None):
        raise QualifiedDifferentialError(
            "target/standalone compiler-dependency evidence is asymmetric"
        )
    if target_dependencies is None:
        if target_build["buildParityDigest"] != standalone_build["buildParityDigest"]:
            raise QualifiedDifferentialError(
                "target/standalone build-parity digests differ"
            )
    else:
        assert isinstance(target_dependencies, dict)
        assert isinstance(standalone_dependencies, dict)
        for key in ("status", "method"):
            if target_dependencies[key] != standalone_dependencies[key]:
                raise QualifiedDifferentialError(
                    f"target/standalone compiler dependency {key} differs"
                )
        if target_dependencies["status"] == "unavailable":
            if target_dependencies != standalone_dependencies:
                raise QualifiedDifferentialError(
                    "target/standalone unavailable compiler dependency reasons differ"
                )
        else:
            # The pinned and standalone source versions may include different
            # system headers. Compare content wherever both builds actually
            # depended on the same path; a set difference alone is not a
            # toolchain mismatch.
            target_inputs = {
                row["path"]: row for row in target_dependencies["externalInputs"]
            }
            standalone_inputs = {
                row["path"]: row for row in standalone_dependencies["externalInputs"]
            }
            for path in sorted(target_inputs.keys() & standalone_inputs.keys()):
                if target_inputs[path] != standalone_inputs[path]:
                    raise QualifiedDifferentialError(
                        "target/standalone compiler dependency content differs: "
                        + path
                    )
    target_inputs = target_build["configIdentity"]["inputs"]
    standalone_inputs = standalone_build["configIdentity"]["inputs"]
    target_support = target_inputs.get("adapterSupportInputs")
    standalone_support = standalone_inputs.get("adapterSupportInputs")
    if (target_support is None) != (standalone_support is None):
        raise QualifiedDifferentialError(
            "target/standalone adapter support-input evidence is asymmetric"
        )
    if target_support is not None and standalone_support is not None:
        target_support_by_path = {
            row["path"]: row["digest"] for row in target_support
        }
        standalone_support_by_path = {
            row["path"]: row["digest"] for row in standalone_support
        }
        if target_support_by_path != standalone_support_by_path:
            raise QualifiedDifferentialError(
                "target/standalone adapter support-input content differs"
            )
    compile_modes = {
        target_inputs.get("compileContractMode"),
        standalone_inputs.get("compileContractMode"),
    }
    if "direct-options-only" in compile_modes:
        raise QualifiedDifferentialError(
            "direct-option admission does not attest the effective compiler driver; "
            "this receipt pair is not eligible"
        )
    if len(compile_modes) != 1:
        raise QualifiedDifferentialError(
            "target/standalone compile-contract dispositions differ"
        )
    compile_mode = next(iter(compile_modes))
    if compile_mode == "controlled-driver-profile-v1":
        target_contract = target_inputs.get("compileContractProfile")
        standalone_contract = standalone_inputs.get("compileContractProfile")
        if (not isinstance(target_contract, dict)
                or not isinstance(standalone_contract, dict)
                or target_contract.get("disposition") != "enforced"
                or standalone_contract.get("disposition") != "enforced"):
            raise QualifiedDifferentialError(
                "target/standalone compile-contract profile is advisory or unavailable"
            )
        if target_contract != standalone_contract:
            raise QualifiedDifferentialError(
                "target/standalone compile-contract profiles differ"
            )
    elif compile_mode is not None:
        raise QualifiedDifferentialError(
            "target/standalone compile-contract mode is unsupported"
        )
    target_capabilities = target_inputs.get("capabilityProfile")
    standalone_capabilities = standalone_inputs.get("capabilityProfile")
    if (target_capabilities is None) != (standalone_capabilities is None):
        raise QualifiedDifferentialError(
            "target/standalone semantic capability profiles are asymmetric"
        )
    if compile_mode == "controlled-driver-profile-v1" and (
            target_capabilities is None or standalone_capabilities is None):
        raise QualifiedDifferentialError(
            "controlled receipt pair omits the required semantic capability profile"
        )
    if target_capabilities is not None and standalone_capabilities is not None:
        target_capabilities = _validate_semantic_capability_profile(
            target_capabilities, "target", "pair.target.capabilityProfile"
        )
        standalone_capabilities = _validate_semantic_capability_profile(
            standalone_capabilities, "standalone",
            "pair.standalone.capabilityProfile",
        )
        if (target_capabilities["schema"] != standalone_capabilities["schema"]
                or target_capabilities["name"]
                    != standalone_capabilities["name"]):
            raise QualifiedDifferentialError(
                "target/standalone semantic capability profile identities differ"
            )
    for key in ("verifiedInterfaceContractDigest", "interfaceMethodCount"):
        if target_inputs[key] != standalone_inputs[key]:
            raise QualifiedDifferentialError(
                "target/standalone interface callback inventories differ"
            )
    target_profile = target_inputs.get("interfaceProfile")
    standalone_profile = standalone_inputs.get("interfaceProfile")
    if (target_profile is None) != (standalone_profile is None):
        raise QualifiedDifferentialError(
            "target/standalone named interface profiles differ"
        )
    if target_profile is not None and standalone_profile is not None:
        if (target_profile["name"] != standalone_profile["name"]
                or target_profile["commonMethodDigest"]
                != standalone_profile["commonMethodDigest"]):
            raise QualifiedDifferentialError(
                "target/standalone common callback profiles differ"
            )
        if standalone_profile["unobservedDefaultedExtras"]:
            raise QualifiedDifferentialError(
                "standalone defaulted supplementary callbacks are not captured; "
                "this receipt pair is not eligible"
            )


MANIFEST_KEYS_V2 = {
    "schema",
    "kind",
    "fixturePolicy",
    "resultKind",
    "targetLock",
    "adapterSource",
    "inputRegistry",
    "localRecipes",
    "expectedFailures",
    "runners",
    "comparison",
}
MANIFEST_KEYS_V3 = MANIFEST_KEYS_V2 | {
    "adapterResultSchema", "normalizationSchema",
}
MANIFEST_TARGET_LOCK_KEYS = set(TARGET_LOCK_RECEIPT_KEYS)
ADAPTER_SOURCE_KEYS = {"path", "digestAlgorithm"}
INPUT_REGISTRY_KEYS = {"path", "digest"}
LOCAL_RECIPE_KEYS = {
    "id", "path", "digest", "byteSize", "generatorSource", "format", "version"
}
EXPECTED_FAILURE_KEYS = {
    "id", "inputId", "inputPathHint", "facade", "direction", "firstFailure",
    "outputProbe"
}
EXPECTED_FIRST_FAILURE_KEYS = {"stage", "code", "path"}
OUTPUT_PROBES = {"none", "existing-directory"}
RUNNER_KEYS = {
    "id",
    "side",
    "cmakeTarget",
    "buildCommand",
    "runCommand",
    "inputMode",
    "outputMode",
    "promotesSupport",
}
COMPARISON_KEYS = {"tolerances", "reviewedTargetDebt", "exclusions"}
TOLERANCE_KEYS = {
    "id", "scope", "path", "absolute", "relative", "expectedMatches",
    "evidenceId"
}
DEBT_KEYS = {
    "id", "scope", "path", "targetDigest", "standaloneDigest",
    "expectedMatches", "evidenceId", "reason", "diagnosticContext"
}
EXCLUSION_KEYS = {
    "id", "scope", "path", "reason", "expectedMatches", "evidenceId"
}
RULE_SCOPE_KEYS = {
    "inputId", "inputPathHint", "inputSha256", "facade", "direction",
    "optionsDigest"
}
DEBT_DIAGNOSTIC_CONTEXT_KEYS = {"target", "standalone"}
DEBT_DIAGNOSTIC_KEYS = {"id", "ordinal", "severity", "stage", "code", "path"}

BUILD_TOKENS = {"{sourceRoot}", "{buildDir}", "{configuration}"}
RUN_TOKENS = {
    "{executable}",
    "{input}",
    "{inputPathHint}",
    "{output}",
    "{originKind}",
    "{inputId}",
    "{inputRepository}",
    "{inputCommit}",
    "{inputSourcePath}",
    "{inputSourceBlob}",
    "{inputRegistryDigest}",
    "{generatorDigest}",
    "{facade}",
    "{direction}",
    "{adapterName}",
    "{adapterSide}",
    "{adapterPackage}",
    "{adapterCommit}",
    "{sourceDigest}",
    "{configDigest}",
    "{staticLibraryDigest}",
    "{binaryDigest}",
    "{linkedClosureDigest}",
}


def _validate_command(value: Any, path: str, *, allowed_tokens: set[str],
                      required_tokens: set[str]) -> list[str]:
    command = _list(value, path)
    if not command or any(not isinstance(item, str) or not item for item in command):
        raise QualifiedDifferentialError(
            f"{path} must be a non-empty string array"
        )
    if any("<" in item or ">" in item for item in command):
        raise QualifiedDifferentialError(f"{path} contains a placeholder command")
    observed: list[str] = []
    for item in command:
        for token in re.findall(r"\{[A-Za-z][A-Za-z0-9]*\}", item):
            observed.append(token)
            if token not in allowed_tokens:
                raise QualifiedDifferentialError(
                    f"{path} contains unsupported token {token}"
                )
    for token in required_tokens:
        if observed.count(token) != 1:
            raise QualifiedDifferentialError(
                f"{path} must contain {token} exactly once"
            )
    return command


def _validate_rule_scope(value: Any, path: str) -> dict[str, Any]:
    scope = _exact_keys(value, RULE_SCOPE_KEYS, path)
    _identifier(scope["inputId"], f"{path}.inputId")
    _path_hint(scope["inputPathHint"], f"{path}.inputPathHint")
    _digest(scope["inputSha256"], f"{path}.inputSha256")
    _digest(scope["optionsDigest"], f"{path}.optionsDigest")
    if scope["facade"] not in FACADES:
        raise QualifiedDifferentialError(f"{path}.facade is invalid")
    if scope["direction"] not in DIRECTIONS:
        raise QualifiedDifferentialError(f"{path}.direction is invalid")
    return scope


def _exact_json_pointer(value: Any, path: str) -> str:
    pointer = _string(value, path)
    if not pointer.startswith("/") or pointer == "/":
        raise QualifiedDifferentialError(f"{path} is not a non-root JSON pointer")
    if any(character in pointer for character in "*?[]"):
        raise QualifiedDifferentialError(f"{path} must not contain glob metacharacters")
    tokens = pointer[1:].split("/")
    if any(not token for token in tokens):
        raise QualifiedDifferentialError(f"{path} contains an empty JSON pointer token")
    for token in tokens:
        if re.search(r"~(?![01])", token):
            raise QualifiedDifferentialError(f"{path} has an invalid JSON pointer escape")
        decoded = token.replace("~1", "/").replace("~0", "~")
        normalized = decoded.replace("~", "~0").replace("/", "~1")
        if normalized != token:
            raise QualifiedDifferentialError(f"{path} is not a normalized JSON pointer")
    return pointer


def _validate_debt_diagnostic_context(
    value: Any, rule_path: str, target_digest: str | None,
    standalone_digest: str | None, path: str,
) -> None:
    if value is None:
        if re.fullmatch(r"/diagnostics/[0-9]+/messageDigest", rule_path):
            raise QualifiedDifferentialError(
                f"{path} is required for diagnostic message debt"
            )
        return
    match = re.fullmatch(r"/diagnostics/([0-9]+)/messageDigest", rule_path)
    if match is None:
        raise QualifiedDifferentialError(
            f"{path} is only valid for an exact diagnostic messageDigest path"
        )
    context = _exact_keys(value, DEBT_DIAGNOSTIC_CONTEXT_KEYS, path)
    expected_ordinal = int(match.group(1))
    for side, digest in (
        ("target", target_digest), ("standalone", standalone_digest),
    ):
        if digest is None:
            if context[side] is not None:
                raise QualifiedDifferentialError(
                    f"{path}.{side} must be null when that side is absent"
                )
            continue
        if context[side] is None:
            raise QualifiedDifferentialError(
                f"{path}.{side} is required when that side is present"
            )
        row = _exact_keys(
            context[side], DEBT_DIAGNOSTIC_KEYS, f"{path}.{side}",
        )
        _identifier(row["id"], f"{path}.{side}.id")
        ordinal = _integer(
            row["ordinal"], f"{path}.{side}.ordinal", minimum=0,
        )
        if ordinal != expected_ordinal:
            raise QualifiedDifferentialError(
                f"{path}.{side}.ordinal does not match the diagnostic path"
            )
        if row["severity"] not in DIAGNOSTIC_SEVERITIES:
            raise QualifiedDifferentialError(f"{path}.{side}.severity is invalid")
        if row["stage"] not in DIAGNOSTIC_STAGES:
            raise QualifiedDifferentialError(f"{path}.{side}.stage is invalid")
        _string(row["code"], f"{path}.{side}.code")
        _string(row["path"], f"{path}.{side}.path")


def _validate_rules(comparison: Any) -> dict[str, Any]:
    value = _exact_keys(comparison, COMPARISON_KEYS, "comparison")
    rule_ids: list[str] = []
    rule_paths: list[str] = []
    for index, item in enumerate(_list(value["tolerances"], "comparison.tolerances")):
        row = _exact_keys(item, TOLERANCE_KEYS,
                          f"comparison.tolerances[{index}]")
        rule_ids.append(_identifier(row["id"], f"comparison.tolerances[{index}].id"))
        scope = _validate_rule_scope(
            row["scope"], f"comparison.tolerances[{index}].scope",
        )
        path = _string(row["path"], f"comparison.tolerances[{index}].path")
        if (
            not path.startswith("/records/")
            or "/fields/" not in path
            or "/value" not in path
            or "/@" in path
        ):
            raise QualifiedDifferentialError(
                "numeric tolerance paths are limited to typed record field values"
            )
        rule_paths.append(
            f"{scope['inputId']}\0{scope['inputPathHint']}\0"
            f"{scope['inputSha256']}\0{scope['facade']}\0"
            f"{scope['direction']}\0{scope['optionsDigest']}\0{path}"
        )
        absolute = _finite_number(
            row["absolute"], f"comparison.tolerances[{index}].absolute"
        )
        relative = _finite_number(
            row["relative"], f"comparison.tolerances[{index}].relative"
        )
        if absolute < 0 or relative < 0 or (absolute == 0 and relative == 0):
            raise QualifiedDifferentialError(
                "tolerance must declare a positive finite bound"
            )
        _integer(row["expectedMatches"],
                 f"comparison.tolerances[{index}].expectedMatches", minimum=1)
        _identifier(row["evidenceId"],
                    f"comparison.tolerances[{index}].evidenceId")
    for index, item in enumerate(
        _list(value["reviewedTargetDebt"], "comparison.reviewedTargetDebt")
    ):
        row = _exact_keys(item, DEBT_KEYS,
                          f"comparison.reviewedTargetDebt[{index}]")
        rule_ids.append(_identifier(
            row["id"], f"comparison.reviewedTargetDebt[{index}].id"
        ))
        scope = _validate_rule_scope(
            row["scope"], f"comparison.reviewedTargetDebt[{index}].scope",
        )
        path = _exact_json_pointer(
            row["path"], f"comparison.reviewedTargetDebt[{index}].path",
        )
        rule_paths.append(
            f"{scope['inputId']}\0{scope['inputPathHint']}\0"
            f"{scope['inputSha256']}\0{scope['facade']}\0"
            f"{scope['direction']}\0{scope['optionsDigest']}\0{path}"
        )
        target_digest = _optional_digest(
            row["targetDigest"],
            f"comparison.reviewedTargetDebt[{index}].targetDigest",
        )
        standalone_digest = _optional_digest(
            row["standaloneDigest"],
            f"comparison.reviewedTargetDebt[{index}].standaloneDigest",
        )
        if target_digest is None and standalone_digest is None:
            raise QualifiedDifferentialError(
                "reviewed target debt cannot expect both sides to be absent"
            )
        _string(row["reason"],
                f"comparison.reviewedTargetDebt[{index}].reason")
        _validate_debt_diagnostic_context(
            row["diagnosticContext"], path, target_digest, standalone_digest,
            f"comparison.reviewedTargetDebt[{index}].diagnosticContext",
        )
        _integer(row["expectedMatches"],
                 f"comparison.reviewedTargetDebt[{index}].expectedMatches", minimum=1)
        _identifier(row["evidenceId"],
                    f"comparison.reviewedTargetDebt[{index}].evidenceId")
    for index, item in enumerate(_list(value["exclusions"], "comparison.exclusions")):
        row = _exact_keys(item, EXCLUSION_KEYS,
                          f"comparison.exclusions[{index}]")
        rule_ids.append(_identifier(row["id"], f"comparison.exclusions[{index}].id"))
        scope = _validate_rule_scope(
            row["scope"], f"comparison.exclusions[{index}].scope",
        )
        path = _string(row["path"], f"comparison.exclusions[{index}].path")
        rule_paths.append(
            f"{scope['inputId']}\0{scope['inputPathHint']}\0"
            f"{scope['inputSha256']}\0{scope['facade']}\0"
            f"{scope['direction']}\0{scope['optionsDigest']}\0{path}"
        )
        _string(row["reason"], f"comparison.exclusions[{index}].reason")
        _integer(row["expectedMatches"],
                 f"comparison.exclusions[{index}].expectedMatches", minimum=1)
        _identifier(row["evidenceId"],
                    f"comparison.exclusions[{index}].evidenceId")
    _unique(rule_ids, "comparison rule IDs")
    _unique(rule_paths, "comparison rule invocation/path selectors")
    return value


def _expected_failure_path(stage: str) -> str:
    if stage == "callback":
        return "/callbacks"
    if stage in {"write", "writeResult", "readback"}:
        return "/output"
    return "/input"


def _validate_expected_failure(value: Any, path: str) -> dict[str, Any]:
    row = _exact_keys(value, EXPECTED_FAILURE_KEYS, path)
    _identifier(row["id"], f"{path}.id")
    _identifier(row["inputId"], f"{path}.inputId")
    _path_hint(row["inputPathHint"], f"{path}.inputPathHint")
    if row["facade"] not in FACADES:
        raise QualifiedDifferentialError(f"{path}.facade is invalid")
    if row["direction"] not in DIRECTIONS:
        raise QualifiedDifferentialError(f"{path}.direction is invalid")
    failure = _exact_keys(
        row["firstFailure"], EXPECTED_FIRST_FAILURE_KEYS,
        f"{path}.firstFailure",
    )
    stage = _string(failure["stage"], f"{path}.firstFailure.stage")
    if stage not in DIAGNOSTIC_STAGES:
        raise QualifiedDifferentialError(
            f"{path}.firstFailure.stage is invalid"
        )
    _integer(failure["code"], f"{path}.firstFailure.code", minimum=0)
    failure_path = _semantic_path(
        failure["path"], f"{path}.firstFailure.path"
    )
    if failure_path != _expected_failure_path(stage):
        raise QualifiedDifferentialError(
            f"{path}.firstFailure.path does not match its operation"
        )
    probe = _string(row["outputProbe"], f"{path}.outputProbe")
    if probe not in OUTPUT_PROBES:
        raise QualifiedDifferentialError(f"{path}.outputProbe is invalid")
    if probe != "none" and row["direction"] != "write":
        raise QualifiedDifferentialError(
            f"{path}.outputProbe requires write direction"
        )
    if probe == "existing-directory" and failure != {
        "stage": "write", "code": 2, "path": "/output",
    }:
        raise QualifiedDifferentialError(
            f"{path}.outputProbe requires write/BAD_OPEN/output oracle"
        )
    return row


def _validate_expected_failures(
    value: Any, recipes: list[dict[str, Any]],
) -> list[dict[str, Any]]:
    rows = _list(value, "manifest.expectedFailures")
    expected_failure_ids: list[str] = []
    expected_failure_invocations: list[str] = []
    for index, item in enumerate(rows):
        row = _validate_expected_failure(
            item, f"manifest.expectedFailures[{index}]"
        )
        expected_failure_ids.append(row["id"])
        expected_failure_invocations.append(
            "\0".join((row["inputId"], row["facade"], row["direction"]))
        )
        matching_recipes = [
            recipe for recipe in recipes
            if recipe["id"] == row["inputId"]
            and recipe["path"] == row["inputPathHint"]
        ]
        if row["direction"] == "write" and len(matching_recipes) != 1:
            raise QualifiedDifferentialError(
                "write expected-failure input must name one local recipe"
            )
    if (
        expected_failure_ids != sorted(expected_failure_ids)
        or len(set(expected_failure_ids)) != len(expected_failure_ids)
    ):
        raise QualifiedDifferentialError(
            "expected-failure IDs must be sorted and unique"
        )
    if len(set(expected_failure_invocations)) != len(expected_failure_invocations):
        raise QualifiedDifferentialError(
            "expected-failure invocation tuples must be unique"
        )
    return rows


def validate_manifest(value: Any, *, root: Path | None = None) -> dict[str, Any]:
    if not isinstance(value, dict):
        raise QualifiedDifferentialError("manifest must be an object")
    schema = value.get("schema")
    if type(schema) is not int or schema not in {
        MANIFEST_SCHEMA_V2, MANIFEST_SCHEMA_V3,
    }:
        raise QualifiedDifferentialError("unexpected differential manifest schema or kind")
    manifest = _exact_keys(
        value,
        MANIFEST_KEYS_V2 if schema == MANIFEST_SCHEMA_V2 else MANIFEST_KEYS_V3,
        "manifest",
    )
    if manifest["kind"] != MANIFEST_KIND:
        raise QualifiedDifferentialError("unexpected differential manifest schema or kind")
    if schema == MANIFEST_SCHEMA_V3:
        if manifest["adapterResultSchema"] != ADAPTER_RESULT_SCHEMA_V3:
            raise QualifiedDifferentialError("manifest.adapterResultSchema must select schema 3")
        if manifest["normalizationSchema"] != NORMALIZATION_REPORT_SCHEMA:
            raise QualifiedDifferentialError("manifest.normalizationSchema is unsupported")
    if manifest["fixturePolicy"] != FIXTURE_POLICY:
        raise QualifiedDifferentialError("differential manifest fixture policy is unsafe")
    if manifest["resultKind"] != RESULT_KIND:
        raise QualifiedDifferentialError("differential manifest result kind is stale")
    lock = _exact_keys(manifest["targetLock"], MANIFEST_TARGET_LOCK_KEYS,
                       "manifest.targetLock")
    _path_hint(lock["path"], "manifest.targetLock.path")
    _digest(lock["digest"], "manifest.targetLock.digest")
    _commit(lock["targetCommit"], "manifest.targetLock.targetCommit")
    _commit(lock["snapshotRevision"], "manifest.targetLock.snapshotRevision")
    _digest(lock["archiveDigest"], "manifest.targetLock.archiveDigest")
    _digest(lock["manifestDigest"], "manifest.targetLock.manifestDigest")
    _integer(lock["manifestEntries"], "manifest.targetLock.manifestEntries", minimum=1)
    adapter_source = _exact_keys(manifest["adapterSource"], ADAPTER_SOURCE_KEYS,
                                 "manifest.adapterSource")
    _path_hint(adapter_source["path"], "manifest.adapterSource.path")
    if adapter_source["digestAlgorithm"] != "sha256":
        raise QualifiedDifferentialError("adapter source digest algorithm must be sha256")
    input_registry = _exact_keys(
        manifest["inputRegistry"], INPUT_REGISTRY_KEYS, "manifest.inputRegistry"
    )
    _path_hint(input_registry["path"], "manifest.inputRegistry.path")
    _digest(input_registry["digest"], "manifest.inputRegistry.digest")
    recipes = _list(manifest["localRecipes"], "manifest.localRecipes")
    if not recipes:
        raise QualifiedDifferentialError("manifest.localRecipes must not be empty")
    recipe_ids: list[str] = []
    recipe_paths: list[str] = []
    for index, item in enumerate(recipes):
        recipe = _exact_keys(
            item, LOCAL_RECIPE_KEYS, f"manifest.localRecipes[{index}]"
        )
        recipe_ids.append(_identifier(
            recipe["id"], f"manifest.localRecipes[{index}].id"
        ))
        recipe_paths.append(_path_hint(
            recipe["path"], f"manifest.localRecipes[{index}].path"
        ))
        _digest(recipe["digest"], f"manifest.localRecipes[{index}].digest")
        _integer(
            recipe["byteSize"], f"manifest.localRecipes[{index}].byteSize",
            minimum=1,
        )
        generator_source = _path_hint(
            recipe["generatorSource"],
            f"manifest.localRecipes[{index}].generatorSource",
        )
        if generator_source != adapter_source["path"]:
            raise QualifiedDifferentialError(
                "local recipe generatorSource must be the shared adapter source"
            )
        if recipe["format"] != "recipe-json":
            raise QualifiedDifferentialError(
                "local recipe format must be recipe-json"
            )
        version = _string(
            recipe["version"], f"manifest.localRecipes[{index}].version"
        )
        if not re.fullmatch(r"AC[0-9]{4}", version):
            raise QualifiedDifferentialError("local recipe version is invalid")
    if recipe_ids != sorted(recipe_ids) or len(set(recipe_ids)) != len(recipe_ids):
        raise QualifiedDifferentialError("local recipe IDs must be sorted and unique")
    if len(set(recipe_paths)) != len(recipe_paths):
        raise QualifiedDifferentialError("local recipe paths must be unique")
    expected_failures = _validate_expected_failures(
        manifest["expectedFailures"], recipes
    )
    runners = _list(manifest["runners"], "manifest.runners")
    if len(runners) != 2:
        raise QualifiedDifferentialError("manifest requires exactly two runners")
    ids: list[str] = []
    sides: set[str] = set()
    for index, item in enumerate(runners):
        row = _exact_keys(item, RUNNER_KEYS, f"manifest.runners[{index}]")
        ids.append(_identifier(row["id"], f"manifest.runners[{index}].id"))
        if row["side"] not in SIDES:
            raise QualifiedDifferentialError(f"manifest.runners[{index}].side is invalid")
        sides.add(row["side"])
        target = _identifier(row["cmakeTarget"],
                             f"manifest.runners[{index}].cmakeTarget")
        if target != f"libdxfrw_semantic_adapter_{row['side']}":
            raise QualifiedDifferentialError(
                f"manifest runner {row['side']} names the wrong CMake target"
            )
        _validate_command(
            row["buildCommand"], f"manifest.runners[{index}].buildCommand",
            allowed_tokens=BUILD_TOKENS,
            required_tokens={"{buildDir}"},
        )
        build_target = (
            "libdxfrw_semantic_adapter_standalone_receipt"
            if row["side"] == "standalone"
            else "libdxfrw_semantic_adapter_target"
        )
        expected_build_command = [
            "cmake", "--build", "{buildDir}", "--target", build_target,
            "--config", "{configuration}", "--parallel", "1",
        ]
        if row["buildCommand"] != expected_build_command:
            raise QualifiedDifferentialError(
                f"manifest runner {row['side']} build command is not canonical"
            )
        _validate_command(
            row["runCommand"], f"manifest.runners[{index}].runCommand",
            allowed_tokens=RUN_TOKENS,
            required_tokens={
                "{executable}", "{input}", "{inputPathHint}", "{output}",
                "{facade}", "{direction}"
            },
        )
        if row["inputMode"] != "locked-repository-blob-or-local-recipe":
            raise QualifiedDifferentialError(
                "runner inputMode must name locked blobs/local recipes"
            )
        if row["outputMode"] != "json-stdout":
            raise QualifiedDifferentialError("runner outputMode must be json-stdout")
        if row["promotesSupport"] is not False:
            raise QualifiedDifferentialError("runner cannot promote support")
    if ids != sorted(ids) or len(set(ids)) != 2 or sides != SIDES:
        raise QualifiedDifferentialError(
            "runner IDs must be sorted/unique and cover target plus standalone"
        )
    _validate_rules(manifest["comparison"])

    if root is not None:
        lock_path = root / lock["path"]
        if not lock_path.is_file() or _sha256_file(lock_path) != lock["digest"]:
            raise QualifiedDifferentialError("target lock path/digest is stale")
        registry_path = root / input_registry["path"]
        if (
            not registry_path.is_file()
            or _sha256_file(registry_path) != input_registry["digest"]
        ):
            raise QualifiedDifferentialError("input registry path/digest is stale")
        registry_document = _read_json(registry_path)
        if (
            not isinstance(registry_document, dict)
            or not isinstance(registry_document.get("fixtures"), list)
        ):
            raise QualifiedDifferentialError("fixture registry structure is invalid")
        admitted_input_paths = set(recipe_paths)
        admitted_input_paths.update(
            row["path"] for row in registry_document["fixtures"]
            if isinstance(row, dict) and isinstance(row.get("path"), str)
        )
        for index, row in enumerate(expected_failures):
            if row["inputPathHint"] not in admitted_input_paths:
                raise QualifiedDifferentialError(
                    f"manifest.expectedFailures[{index}] input is not registered"
                )
        for index, recipe in enumerate(recipes):
            recipe_path = root / recipe["path"]
            if not recipe_path.is_file():
                raise QualifiedDifferentialError(
                    f"local recipe {index} path is absent"
                )
            if (
                _sha256_file(recipe_path) != recipe["digest"]
                or recipe_path.stat().st_size != recipe["byteSize"]
            ):
                raise QualifiedDifferentialError(
                    f"local recipe {index} identity is stale"
                )
        lock_document = _read_json(lock_path)
        try:
            if (
                lock_document["libreCAD"]["commit"] != lock["targetCommit"]
                or lock_document["libreCAD"]["snapshotRevision"]
                != lock["snapshotRevision"]
                or lock_document["archive"]["sha256"] != lock["archiveDigest"]
                or lock_document["manifest"]["sha256"] != lock["manifestDigest"]
                or lock_document["manifest"]["entries"] != lock["manifestEntries"]
            ):
                raise QualifiedDifferentialError("target lock values are stale")
        except (KeyError, TypeError) as exc:
            raise QualifiedDifferentialError("target lock structure is invalid") from exc
        source_path = root / adapter_source["path"]
        if not source_path.is_file():
            raise QualifiedDifferentialError("adapter source path is absent")
    return manifest


def _manifest_adapter_result_schema(manifest: dict[str, Any]) -> int:
    if manifest["schema"] == MANIFEST_SCHEMA_V2:
        return ADAPTER_RESULT_SCHEMA_V2
    return manifest["adapterResultSchema"]


_MISSING = object()


def _pointer_token(value: str) -> str:
    return value.replace("~", "~0").replace("/", "~1")


def _semantic_view(document: dict[str, Any]) -> dict[str, Any]:
    """Key unordered semantic collections by their stable IDs/names."""

    records: dict[str, Any] = {}
    for record in document["records"]:
        row = {key: copy.deepcopy(value) for key, value in record.items()
               if key != "fields"}
        row["fields"] = {
            field["name"]: {"type": field["type"], "value": field["value"]}
            for field in record["fields"]
        }
        records[record["id"]] = row
    result = {
        "input": copy.deepcopy(document["input"]),
        "invocation": copy.deepcopy(document["invocation"]),
        "status": copy.deepcopy(document["status"]),
        # Callback and diagnostic sequence order is semantically significant.
        "callbacks": copy.deepcopy(document["callbacks"]),
        "records": records,
        "graphNodes": {row["id"]: copy.deepcopy(row)
                       for row in document["graphNodes"]},
        "graphEdges": {row["id"]: copy.deepcopy(row)
                       for row in document["graphEdges"]},
        "opaqueCarriers": {row["id"]: copy.deepcopy(row)
                           for row in document["opaqueCarriers"]},
        "unsupportedContent": {row["id"]: copy.deepcopy(row)
                               for row in document["unsupportedContent"]},
        "diagnostics": copy.deepcopy(document["diagnostics"]),
        "cardinality": copy.deepcopy(document["cardinality"]),
    }
    if document["schema"] == ADAPTER_RESULT_SCHEMA_V3:
        result["observations"] = copy.deepcopy(document["observations"])
    return result


def _observation_eligibility_blockers(document: dict[str, Any], side: str) -> list[str]:
    if document["schema"] != ADAPTER_RESULT_SCHEMA_V3:
        return []
    blockers: list[str] = []
    for phase, envelope in document["observations"].items():
        prefix = f"{side}.observations.{phase}"
        availability = envelope["availability"]
        if availability == "unavailable":
            blockers.append(prefix + ".unavailable")
            continue
        if availability != "available":
            continue
        counters = envelope["scalarCounters"]
        for key in (
            "entityParseFailures", "objectParseFailures",
            "dataStorageLinkFailures", "dataStorageOrphanRecords",
        ):
            if counters[key] != 0:
                blockers.append(prefix + ".partial:" + key)
        for key in (
            "classesCrcMismatch", "reader21CrcMismatch",
            "reader18HeaderCrcMismatch",
        ):
            if counters[key] != 0:
                blockers.append(prefix + ".crcMismatch:" + key)
        for name, summary in envelope["namedSummaries"].items():
            if summary["totalInstanceCount"] or summary["omittedNameCount"]:
                blockers.append(prefix + ".skipped:" + name)
            if (summary["aggregateSaturated"]
                    or summary["omittedNameBytes"]
                    or summary["truncatedRetainedNameCount"]):
                blockers.append(prefix + ".incompleteNames:" + name)
        integrity = envelope["integrity"]
        if any(row["severityCode"] == 0 for row in integrity["rows"]):
            blockers.append(prefix + ".integrityError")
        if any(integrity[key] for key in (
            "extractionOmittedCount", "reportedUpstreamDroppedLowerBound",
            "truncatedRetainedNameCount", "omittedNameBytes",
        )) or integrity["aggregateSaturated"]:
            blockers.append(prefix + ".incompleteIntegrity")
        frames = envelope["frames"]
        if frames["status"] != "finalizedComplete":
            blockers.append(prefix + ".frameCoverage:" + frames["status"])
        elif any(frames["dispositions"][key] for key in (
            "pending", "deferred", "staged", "quarantined", "failed",
            "unresolved",
        )) or frames["unrecognizedDispositionCount"]:
            blockers.append(prefix + ".nonterminalFrameDisposition")
    return blockers


def _flatten(value: Any, path: str = "") -> dict[str, Any]:
    flattened: dict[str, Any] = {}
    if isinstance(value, dict):
        flattened[path + "/@size"] = len(value)
        for key in sorted(value):
            flattened.update(_flatten(value[key], path + "/" + _pointer_token(key)))
    elif isinstance(value, list):
        flattened[path + "/@length"] = len(value)
        for index, item in enumerate(value):
            flattened.update(_flatten(item, path + f"/{index}"))
    else:
        flattened[path or "/"] = value
    return flattened


def _strict_equal(left: Any, right: Any) -> bool:
    return type(left) is type(right) and left == right


def _numeric_tolerance_paths(document: dict[str, Any]) -> set[str]:
    paths: set[str] = set()
    for record in document["records"]:
        record_token = _pointer_token(record["id"])
        for field in record["fields"]:
            if field["type"] not in {"double", "point2d", "point3d"}:
                continue
            prefix = (
                f"/records/{record_token}/fields/"
                f"{_pointer_token(field['name'])}/value"
            )
            paths.update(
                path
                for path in _flatten(field["value"], prefix)
                if not path.endswith("/@size") and not path.endswith("/@length")
            )
    return paths


def _rule_applies(rule: dict[str, Any], invocation: dict[str, str]) -> bool:
    return rule["scope"] == invocation


def _debt_diagnostic_context_matches(rule: dict[str, Any],
                                     target: dict[str, Any],
                                     standalone: dict[str, Any]) -> bool:
    context = rule["diagnosticContext"]
    if context is None:
        return True
    match = re.fullmatch(r"/diagnostics/([0-9]+)/messageDigest", rule["path"])
    if match is None:  # _validate_rules rejects this shape.
        return False
    index = int(match.group(1))
    keys = DEBT_DIAGNOSTIC_KEYS
    for side, document in (("target", target), ("standalone", standalone)):
        expected = context[side]
        if expected is None:
            if index < len(document["diagnostics"]):
                return False
            continue
        if index >= len(document["diagnostics"]):
            return False
        actual = {key: document["diagnostics"][index][key] for key in keys}
        if actual != expected:
            return False
    return True


def _matching_rules(path: str, comparison: dict[str, Any],
                    invocation: dict[str, str]) -> list[tuple[str, dict[str, Any]]]:
    matches: list[tuple[str, dict[str, Any]]] = []
    for row in comparison["tolerances"]:
        if _rule_applies(row, invocation) and fnmatch.fnmatchcase(path, row["path"]):
            matches.append(("toleranceNormalized", row))
    for row in comparison["reviewedTargetDebt"]:
        if _rule_applies(row, invocation) and fnmatch.fnmatchcase(path, row["path"]):
            matches.append(("reviewedTargetDebt", row))
    for row in comparison["exclusions"]:
        if _rule_applies(row, invocation) and fnmatch.fnmatchcase(path, row["path"]):
            matches.append(("excluded", row))
    if len(matches) > 1:
        raise QualifiedDifferentialError(
            f"comparison path {path} matches multiple normalization rules"
        )
    return matches


def compare_results(target_value: Any, standalone_value: Any,
                    comparison_value: Any, *, manifest_digest: str,
                    target_lock: dict[str, Any],
                    expected_operation_succeeded: bool = True,
                    expected_failure: dict[str, Any] | None = None
                    ) -> dict[str, Any]:
    """Compare two validated results and classify every normalized value."""

    target = validate_result(target_value)
    standalone = validate_result(standalone_value)
    comparison = _validate_rules(comparison_value)
    _digest(manifest_digest, "manifestDigest")

    if target["adapter"]["side"] != "target":
        raise QualifiedDifferentialError("target result has the wrong adapter side")
    if standalone["adapter"]["side"] != "standalone":
        raise QualifiedDifferentialError("standalone result has the wrong adapter side")
    if target["schema"] != standalone["schema"]:
        raise QualifiedDifferentialError(
            "target and standalone adapter result schemas differ"
        )
    if target["input"] != standalone["input"]:
        raise QualifiedDifferentialError(
            "target and standalone results do not bind the same input/provenance"
        )
    if target["invocation"] != standalone["invocation"]:
        raise QualifiedDifferentialError(
            "target and standalone invocation options differ"
        )
    for key in ("sourceDigest", "configDigest"):
        if target["adapter"][key] != standalone["adapter"][key]:
            raise QualifiedDifferentialError(
                f"target and standalone adapter {key} values differ"
            )
    if target["adapter"]["configDigest"] != manifest_digest:
        raise QualifiedDifferentialError(
            "adapter config digest does not bind the differential manifest"
        )
    if target["adapter"]["commit"] != target_lock["targetCommit"]:
        raise QualifiedDifferentialError(
            "target adapter commit does not bind the target lock"
        )
    if expected_operation_succeeded:
        if expected_failure is not None:
            raise QualifiedDifferentialError(
                "successful comparison cannot declare an expected failure"
            )
    elif expected_failure is None:
        raise QualifiedDifferentialError(
            "expected-failure comparison requires a stage/code/path oracle"
        )
    if expected_failure is not None:
        oracle = _validate_expected_failure(
            expected_failure, "expectedFailure"
        )
        expected_invocation = {
            "inputId": target["input"]["id"],
            "inputPathHint": target["input"]["pathHint"],
            "facade": target["invocation"]["facade"],
            "direction": target["invocation"]["direction"],
        }
        for key, actual in expected_invocation.items():
            if oracle[key] != actual:
                raise QualifiedDifferentialError(
                    f"expected-failure {key} does not match the invocation"
                )
        for side, document in (("target", target), ("standalone", standalone)):
            actual = document["status"]["firstFailure"]
            for key in ("stage", "code", "path"):
                expected = oracle["firstFailure"][key]
                if actual[key] != expected:
                    raise QualifiedDifferentialError(
                        f"{side} first failure {key} does not match the "
                        "expected-failure oracle"
                    )
    operation_succeeded = (
        target["status"]["operationSucceeded"]
        and standalone["status"]["operationSucceeded"]
    )
    if expected_operation_succeeded:
        if not operation_succeeded:
            raise QualifiedDifferentialError(
                "qualification expected both adapter operations to succeed"
            )
    elif (
        target["status"]["operationSucceeded"]
        or standalone["status"]["operationSucceeded"]
    ):
        raise QualifiedDifferentialError(
            "expected-failure comparison requires both adapter operations to fail"
        )

    target_flat = _flatten(_semantic_view(target))
    standalone_flat = _flatten(_semantic_view(standalone))
    tolerance_paths = (
        _numeric_tolerance_paths(target) & _numeric_tolerance_paths(standalone)
    )
    invocation_scope = {
        "inputId": target["input"]["id"],
        "inputPathHint": target["input"]["pathHint"],
        "inputSha256": target["input"]["sha256"],
        "facade": target["invocation"]["facade"],
        "direction": target["invocation"]["direction"],
        "optionsDigest": target["invocation"]["optionsDigest"],
    }
    all_paths = sorted(set(target_flat) | set(standalone_flat))
    rows: list[dict[str, Any]] = []
    missing_target = 0
    missing_standalone = 0
    rule_matches: Counter[str] = Counter()
    for path in all_paths:
        left = target_flat.get(path, _MISSING)
        right = standalone_flat.get(path, _MISSING)
        left_present = left is not _MISSING
        right_present = right is not _MISSING
        outcome = "mismatch"
        rule_id: str | None = None
        if not left_present:
            missing_target += 1
        if not right_present:
            missing_standalone += 1
        if left_present and right_present:
            if _strict_equal(left, right):
                outcome = "exact"
            else:
                matches = _matching_rules(path, comparison, invocation_scope)
                if matches:
                    rule_kind, rule = matches[0]
                    rule_id = rule["id"]
                    rule_matches[rule_id] += 1
                    if rule_kind == "toleranceNormalized":
                        if path not in tolerance_paths:
                            raise QualifiedDifferentialError(
                                f"numeric tolerance {rule_id} matched a non-field "
                                f"or non-floating path: {path}"
                            )
                        if (
                            isinstance(left, bool)
                            or isinstance(right, bool)
                            or not isinstance(left, (int, float))
                            or not isinstance(right, (int, float))
                            or not math.isfinite(float(left))
                            or not math.isfinite(float(right))
                        ):
                            raise QualifiedDifferentialError(
                                f"numeric tolerance {rule_id} matched non-finite/non-numeric {path}"
                            )
                        if math.isclose(
                            float(left), float(right), rel_tol=float(rule["relative"]),
                            abs_tol=float(rule["absolute"]),
                        ):
                            outcome = rule_kind
                    elif rule_kind == "reviewedTargetDebt":
                        if (
                            _value_digest(left) == rule["targetDigest"]
                            and _value_digest(right) == rule["standaloneDigest"]
                            and _debt_diagnostic_context_matches(
                                rule, target, standalone,
                            )
                        ):
                            outcome = rule_kind
                    else:
                        outcome = rule_kind
        else:
            # Presence differences remain fail-closed unless exact optional
            # debt digests or an explicit exclusion give the path a reviewed
            # disposition.
            matches = _matching_rules(path, comparison, invocation_scope)
            if matches:
                rule_kind, rule = matches[0]
                rule_id = rule["id"]
                rule_matches[rule_id] += 1
                target_digest = _value_digest(left) if left_present else None
                standalone_digest = (
                    _value_digest(right) if right_present else None
                )
                if rule_kind == "reviewedTargetDebt" and (
                    target_digest == rule["targetDigest"]
                    and standalone_digest == rule["standaloneDigest"]
                    and _debt_diagnostic_context_matches(
                        rule, target, standalone,
                    )
                ):
                    outcome = "reviewedTargetDebt"
                elif rule_kind == "excluded":
                    outcome = "excluded"
        if outcome not in VALUE_OUTCOMES:  # defensive schema assertion
            raise QualifiedDifferentialError("internal comparison outcome is invalid")
        semantic_claim_path = not path.startswith(
            ("/opaqueCarriers/", "/unsupportedContent/", "/diagnostics/")
        )
        row: dict[str, Any] = {
            "path": path,
            "targetPresent": left_present,
            "standalonePresent": right_present,
            "targetDigest": _value_digest(left) if left_present else None,
            "standaloneDigest": _value_digest(right) if right_present else None,
            "outcome": outcome,
            "ruleId": rule_id,
            "claimEligible": (
                expected_operation_succeeded
                and semantic_claim_path
                and outcome in {"exact", "toleranceNormalized"}
            ),
        }
        rows.append(row)

    for category in ("tolerances", "reviewedTargetDebt", "exclusions"):
        for rule in comparison[category]:
            if not _rule_applies(rule, invocation_scope):
                continue
            observed = rule_matches[rule["id"]]
            if observed != rule["expectedMatches"]:
                raise QualifiedDifferentialError(
                    f"comparison rule {rule['id']} expected "
                    f"{rule['expectedMatches']} differing paths but matched {observed}"
                )

    outcome_counts = Counter(row["outcome"] for row in rows)
    complete_counts = {outcome: outcome_counts.get(outcome, 0)
                       for outcome in sorted(VALUE_OUTCOMES)}
    reviewed = complete_counts["reviewedTargetDebt"] + complete_counts["excluded"]
    mismatch = complete_counts["mismatch"]
    excluded_unsupported = sum(
        1
        for row in target["unsupportedContent"]
        if row["disposition"] == "excluded"
    )
    eligibility_blockers: list[str] = []
    if not expected_operation_succeeded:
        eligibility_blockers.append("expectedOperationFailure")
    if excluded_unsupported:
        eligibility_blockers.append(
            f"unsupportedContent.excluded:{excluded_unsupported}"
        )
    eligibility_blockers.extend(
        _observation_eligibility_blockers(target, "target")
    )
    eligibility_blockers.extend(
        _observation_eligibility_blockers(standalone, "standalone")
    )
    if mismatch:
        status = "mismatch"
    elif reviewed or eligibility_blockers:
        status = "reviewedNonPromoting"
    elif complete_counts["toleranceNormalized"]:
        status = "qualifiedNormalized"
    else:
        status = "exact"
    report = {
        "schema": NORMALIZATION_REPORT_SCHEMA,
        "kind": REPORT_KIND,
        "fixturePolicy": FIXTURE_POLICY,
        "manifestDigest": manifest_digest,
        "targetLock": copy.deepcopy(target_lock),
        "input": copy.deepcopy(target["input"]),
        "invocation": copy.deepcopy(target["invocation"]),
        "targetAdapter": copy.deepcopy(target["adapter"]),
        "standaloneAdapter": copy.deepcopy(standalone["adapter"]),
        "adapterResultSchema": target["schema"],
        "deterministicRunsPerSide": 2,
        "expectedFailure": (
            copy.deepcopy(expected_failure)
            if expected_failure is not None else None
        ),
        "status": status,
        "claimEligible": (
            mismatch == 0 and reviewed == 0 and not eligibility_blockers
        ),
        "eligibilityBlockers": eligibility_blockers,
        "promotesSupport": False,
        "outcomeCounts": complete_counts,
        "completeness": {
            "targetValueCount": len(target_flat),
            "standaloneValueCount": len(standalone_flat),
            "comparedPathCount": len(all_paths),
            "missingTargetCount": missing_target,
            "missingStandaloneCount": missing_standalone,
            "targetCardinality": copy.deepcopy(target["cardinality"]),
            "standaloneCardinality": copy.deepcopy(standalone["cardinality"]),
        },
        "comparisons": rows,
    }
    # Report serialization itself must reject non-finite values.
    _canonical_bytes(report)
    return report


def _expand_command(command: list[str], replacements: dict[str, str]) -> list[str]:
    expanded: list[str] = []
    for argument in command:
        rendered = argument
        for token, value in replacements.items():
            rendered = rendered.replace(token, value)
        if re.search(r"\{[A-Za-z][A-Za-z0-9]*\}", rendered):
            raise QualifiedDifferentialError(
                f"runner command has an unresolved token: {rendered}"
            )
        expanded.append(rendered)
    return expanded


def _run_twice(command: list[str], timeout_seconds: float) -> tuple[bytes, int]:
    if timeout_seconds <= 0:
        raise QualifiedDifferentialError("runner timeout must be positive")
    environment = os.environ.copy()
    environment.update({
        "LC_ALL": "C",
        "LANG": "C",
        "TZ": "UTC",
        "SOURCE_DATE_EPOCH": "0",
    })
    outputs: list[bytes] = []
    return_codes: list[int] = []
    for _ in range(2):
        try:
            completed = subprocess.run(
                command, capture_output=True, check=False, timeout=timeout_seconds,
                env=environment,
            )
        except (OSError, subprocess.SubprocessError) as exc:
            raise QualifiedDifferentialError(
                f"adapter command failed to execute: {exc}"
            ) from exc
        if len(completed.stdout) > 64 * 1024 * 1024:
            raise QualifiedDifferentialError("adapter JSON output exceeds 64 MiB")
        outputs.append(completed.stdout)
        return_codes.append(completed.returncode)
    if outputs[0] != outputs[1] or return_codes[0] != return_codes[1]:
        raise QualifiedDifferentialError(
            "adapter output is not byte-identical across two clean executions"
        )
    return outputs[0], return_codes[0]


def _run_adapter_twice(command_template: list[str], replacements: dict[str, str],
                       timeout_seconds: float, *, output_policy: str,
                       output_suffix: str, output_probe: str = "none"
                       ) -> tuple[bytes, int, bytes, dict[str, Any] | None]:
    if output_policy not in {"required", "forbidden", "optional"}:
        raise QualifiedDifferentialError("invalid generated-output policy")
    if output_probe not in OUTPUT_PROBES:
        raise QualifiedDifferentialError("invalid generated-output probe")
    if output_probe != "none" and output_policy != "optional":
        raise QualifiedDifferentialError(
            "generated-output probe requires optional output policy"
        )
    environment = os.environ.copy()
    environment.update({
        "LC_ALL": "C", "LANG": "C", "TZ": "UTC", "SOURCE_DATE_EPOCH": "0"
    })
    outputs: list[bytes] = []
    errors: list[bytes] = []
    return_codes: list[int] = []
    artifacts: list[dict[str, Any] | None] = []
    with tempfile.TemporaryDirectory(prefix="libdxfrw-semantic-adapter-") as temp:
        root = Path(temp)
        for index in range(2):
            output_path = root / f"run-{index}" / f"output{output_suffix}"
            probe_marker: Path | None = None
            if output_probe == "existing-directory":
                output_path.mkdir(parents=True)
                probe_marker = output_path / "preserve.txt"
                probe_marker.write_bytes(b"expected-failure probe\n")
            else:
                output_path.parent.mkdir()
            run_replacements = dict(replacements)
            run_replacements["{output}"] = str(output_path)
            command = _expand_command(command_template, run_replacements)
            try:
                completed = subprocess.run(
                    command, capture_output=True, check=False,
                    timeout=timeout_seconds, env=environment,
                )
            except (OSError, subprocess.SubprocessError) as exc:
                raise QualifiedDifferentialError(
                    f"adapter command failed to execute: {exc}"
                ) from exc
            if len(completed.stdout) > 64 * 1024 * 1024:
                raise QualifiedDifferentialError("adapter JSON output exceeds 64 MiB")
            outputs.append(completed.stdout)
            errors.append(completed.stderr)
            return_codes.append(completed.returncode)
            if output_path.is_file():
                detected_format, detected_version = _inspect_drawing_identity(
                    output_path
                )
                artifacts.append({
                    "sha256": _sha256_file(output_path),
                    "byteSize": output_path.stat().st_size,
                    "detectedFormat": detected_format,
                    "detectedVersion": detected_version,
                })
            else:
                artifacts.append(None)
            if output_probe == "existing-directory" and (
                not output_path.is_dir()
                or probe_marker is None
                or not probe_marker.is_file()
                or probe_marker.read_bytes() != b"expected-failure probe\n"
            ):
                raise QualifiedDifferentialError(
                    "write failure probe damaged its pre-existing destination"
                )
        if (
            outputs[0] != outputs[1]
            or errors[0] != errors[1]
            or return_codes[0] != return_codes[1]
        ):
            raise QualifiedDifferentialError(
                "adapter stdout/stderr/status is not byte-identical across two "
                "clean executions"
            )
        if artifacts[0] != artifacts[1]:
            raise QualifiedDifferentialError(
                "generated artifact is non-deterministic"
            )
        if output_policy == "required" and artifacts[0] is None:
            raise QualifiedDifferentialError("write adapter output is absent")
        if output_policy == "forbidden" and artifacts[0] is not None:
            raise QualifiedDifferentialError(
                "read adapter unexpectedly created an output artifact"
            )
    return outputs[0], return_codes[0], errors[0], artifacts[0]


def _artifact_path(receipt_path: Path, artifact: dict[str, Any]) -> Path:
    return _secure_receipt_artifact_path(
        receipt_path, artifact["path"], "receipt executable artifact"
    )


def _runner_by_side(manifest: dict[str, Any], side: str) -> dict[str, Any]:
    for runner in manifest["runners"]:
        if runner["side"] == side:
            return runner
    raise QualifiedDifferentialError(f"manifest has no {side} runner")


def _expected_failure_by_id(
    manifest: dict[str, Any], expected_failure_id: str | None,
) -> dict[str, Any] | None:
    if expected_failure_id is None:
        return None
    matches = [
        row for row in manifest["expectedFailures"]
        if row["id"] == expected_failure_id
    ]
    if len(matches) != 1:
        raise QualifiedDifferentialError(
            "expected-failure ID is absent or ambiguous"
        )
    return matches[0]


def _git_blob_digest(data: bytes) -> str:
    header = f"blob {len(data)}\0".encode("ascii")
    return hashlib.sha1(header + data).hexdigest()


def _ac_version(value: bytes) -> str | None:
    if re.fullmatch(rb"AC[0-9]{4}", value):
        return value.decode("ascii")
    return None


def _ascii_dxf_version(prefix: bytes) -> str | None:
    if b"\0" in prefix:
        return None
    lines = prefix.splitlines()
    expect_section_name = False
    in_header = False
    expect_version = False
    for index in range(0, len(lines) - 1, 2):
        code_text = lines[index].strip()
        if not re.fullmatch(rb"[0-9]{1,4}", code_text):
            return None
        code = int(code_text)
        if code > 1071:
            return None
        value = lines[index + 1].strip()
        if expect_section_name:
            in_header = code == 2 and value == b"HEADER"
            expect_section_name = False
        if expect_version:
            return _ac_version(value) if code == 1 else None
        if in_header and code == 9 and value == b"$ACADVER":
            expect_version = True
        elif code == 0 and value == b"SECTION":
            expect_section_name = True
            in_header = False
        elif code == 0 and value == b"ENDSEC":
            in_header = False
    return None


def _binary_dxf_version(prefix: bytes) -> str | None:
    sentinel = b"AutoCAD Binary DXF\r\n\x1a\x00"
    if not prefix.startswith(sentinel):
        return None
    for marker in (b"\x09\x00$ACADVER\x00\x01\x00", b"\x09$ACADVER\x00\x01"):
        found = prefix.find(marker, len(sentinel))
        if found < 0:
            continue
        offset = found + len(marker)
        if offset + 6 < len(prefix) and prefix[offset + 6] == 0:
            version = _ac_version(prefix[offset:offset + 6])
            if version is not None:
                return version
    return None


def _inspect_drawing_identity(path: Path) -> tuple[str, str]:
    with path.open("rb") as stream:
        prefix = stream.read(65536)
    dwg_version = _ac_version(prefix[:6])
    if dwg_version is not None:
        return "dwg", dwg_version
    binary_version = _binary_dxf_version(prefix)
    if binary_version is not None:
        return "dxf-binary", binary_version
    ascii_version = _ascii_dxf_version(prefix)
    if ascii_version is not None:
        return "dxf-ascii", ascii_version
    return "unknown", "unknown"


def _resolve_input_provenance(
    *,
    root: Path,
    manifest: dict[str, Any],
    input_path: Path,
    input_id: str,
    input_path_hint: str,
    origin_kind: str,
    input_repository: str | None,
    input_commit: str | None,
    generator_digest: str | None,
    source_digest: str,
    input_git_dir: Path | None,
) -> dict[str, str | None]:
    data = input_path.read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    if origin_kind == "localFromScratch":
        matches = [
            row
            for row in manifest["localRecipes"]
            if row["id"] == input_id and row["path"] == input_path_hint
        ]
        if len(matches) != 1:
            raise QualifiedDifferentialError(
                "local input does not match exactly one locked recipe"
            )
        recipe = matches[0]
        if digest != recipe["digest"] or len(data) != recipe["byteSize"]:
            raise QualifiedDifferentialError("local recipe bytes differ from manifest")
        if generator_digest != source_digest:
            raise QualifiedDifferentialError(
                "local recipe generator digest does not bind the adapter source"
            )
        return {
            "sourcePath": None,
            "sourceBlob": None,
            "registryDigest": None,
            "detectedFormat": recipe["format"],
            "detectedVersion": recipe["version"],
        }

    registry_path = root / manifest["inputRegistry"]["path"]
    registry = _read_json(registry_path)
    if not isinstance(registry, dict) or not isinstance(registry.get("fixtures"), list):
        raise QualifiedDifferentialError("fixture registry structure is invalid")
    matches = [
        row for row in registry["fixtures"]
        if isinstance(row, dict) and row.get("path") == input_path_hint
    ]
    if len(matches) != 1:
        raise QualifiedDifferentialError(
            "locked input does not match exactly one fixture-registry row"
        )
    row = matches[0]
    expected = {
        "originKind": "lockedRepositoryBlob",
        "sourceRepository": input_repository,
        "sourceCommit": input_commit,
        "sha256": digest,
        "size": len(data),
    }
    for key, value in expected.items():
        if row.get(key) != value:
            raise QualifiedDifferentialError(
                f"locked input registry {key} does not match invocation/bytes"
            )
    source_path = _path_hint(row.get("sourcePath"), "fixtureRegistry.sourcePath")
    source_blob = _commit(row.get("sourceBlob"), "fixtureRegistry.sourceBlob")
    if _git_blob_digest(data) != source_blob:
        raise QualifiedDifferentialError(
            "locked input bytes do not match the registered Git blob"
        )
    format_map = {
        "DXF ASCII": "dxf-ascii",
        "DXF Binary": "dxf-binary",
        "DWG": "dwg",
    }
    expected_format = format_map.get(row.get("format"))
    expected_version = row.get("version")
    if expected_format is None or not isinstance(expected_version, str) or not re.fullmatch(
        r"AC[0-9]{4}", expected_version
    ):
        raise QualifiedDifferentialError(
            "locked input registry format/version is invalid"
        )
    observed_format, observed_version = _inspect_drawing_identity(input_path)
    if (observed_format, observed_version) != (expected_format, expected_version):
        raise QualifiedDifferentialError(
            "locked input bytes contradict the registered format/version"
        )
    if input_git_dir is None:
        raise QualifiedDifferentialError(
            "locked input provenance requires --input-git-dir"
        )
    git_dir = input_git_dir.resolve()
    if not git_dir.exists():
        raise QualifiedDifferentialError("input Git directory is absent")
    try:
        completed = subprocess.run(
            [
                "git", "--git-dir=" + str(git_dir), "rev-parse",
                f"{input_commit}:{source_path}",
            ],
            capture_output=True,
            text=True,
            check=False,
            timeout=30,
            env={**os.environ, "LC_ALL": "C", "LANG": "C"},
        )
    except (OSError, subprocess.SubprocessError) as exc:
        raise QualifiedDifferentialError(
            f"cannot verify locked input Git provenance: {exc}"
        ) from exc
    if completed.returncode != 0 or completed.stdout.strip() != source_blob:
        raise QualifiedDifferentialError(
            "fixture registry commit:path does not resolve to sourceBlob"
        )
    return {
        "sourcePath": source_path,
        "sourceBlob": source_blob,
        "registryDigest": manifest["inputRegistry"]["digest"],
        "detectedFormat": expected_format,
        "detectedVersion": expected_version,
    }


def _validate_output_readback_version(
    result: dict[str, Any], artifact: dict[str, Any] | None, direction: str,
) -> None:
    if (result["schema"] == ADAPTER_RESULT_SCHEMA_V3
            and result["invocation"]["facade"] == "dwgRW"
            and direction == "write"):
        failure_stage = result["status"]["firstFailure"]["stage"]
        if failure_stage not in {"write", "writeResult"}:
            if artifact is None:
                raise QualifiedDifferentialError(
                    "DWG output readback has no generated artifact identity"
                )
            observed_version = result["observations"]["outputReadback"][
                "sourceVersion"
            ]
            if observed_version != artifact["detectedVersion"]:
                raise QualifiedDifferentialError(
                    "outputReadback sourceVersion differs from generated artifact"
                )


def _validate_generated_artifact(
    result: dict[str, Any], artifact: dict[str, Any] | None, direction: str,
    expected_operation_succeeded: bool,
) -> None:
    _validate_output_readback_version(result, artifact, direction)
    records = [
        row
        for row in result["records"]
        if row["recordClass"] == "WRITE_RESULT" or row["entity"] == "WRITE_RESULT"
    ]
    carriers = [
        row
        for row in result["opaqueCarriers"]
        if row["source"].endswith("/generatedOutput")
    ]
    if direction == "read":
        if artifact is not None or records or carriers:
            raise QualifiedDifferentialError(
                "read result unexpectedly claims a generated output artifact"
            )
        return
    failure_stage = result["status"]["firstFailure"]["stage"]
    if not expected_operation_succeeded:
        if failure_stage in {"write", "callback"}:
            if artifact is not None or records or carriers:
                raise QualifiedDifferentialError(
                    "pre-artifact write failure unexpectedly retained output evidence"
                )
            return
        if failure_stage == "writeResult":
            if artifact is None or records or carriers:
                raise QualifiedDifferentialError(
                    "writeResult failure requires only the deterministic on-disk artifact"
                )
        elif failure_stage != "readback":
            raise QualifiedDifferentialError(
                "write expected failure has an invalid first-failure stage"
            )
    if artifact is None or len(records) != 1 or len(carriers) != 1:
        if not expected_operation_succeeded and failure_stage == "writeResult":
            expected_format = (
                "dwg" if result["invocation"]["facade"] == "dwgRW" else "dxf-ascii"
            )
            if (
                artifact is None
                or artifact["detectedFormat"] != expected_format
                or artifact["detectedVersion"] != result["input"]["detectedVersion"]
                or not re.fullmatch(r"AC[0-9]{4}", artifact["detectedVersion"])
            ):
                raise QualifiedDifferentialError(
                    "writeResult failure artifact format/version is invalid"
                )
            return
        raise QualifiedDifferentialError(
            "write/readback result requires one on-disk artifact, WRITE_RESULT, "
            "and carrier"
        )
    record = records[0]
    if record["kind"] != "artifact":
        raise QualifiedDifferentialError("WRITE_RESULT record kind is not artifact")
    field_map = {field["name"]: field for field in record["fields"]}
    sha_field = field_map.get("sha256")
    size_field = field_map.get("byteSize")
    format_field = field_map.get("format")
    version_field = field_map.get("version")
    if (
        not isinstance(sha_field, dict)
        or sha_field.get("type") != "string"
        or sha_field.get("value") != artifact["sha256"]
        or not isinstance(size_field, dict)
        or size_field.get("type") != "uint64"
        or size_field.get("value") != artifact["byteSize"]
        or not isinstance(format_field, dict)
        or format_field.get("type") != "string"
        or format_field.get("value") != artifact["detectedFormat"]
        or not isinstance(version_field, dict)
        or version_field.get("type") != "string"
        or version_field.get("value") != artifact["detectedVersion"]
    ):
        raise QualifiedDifferentialError(
            "WRITE_RESULT fields do not match the on-disk artifact"
        )
    expected_format = (
        "dwg" if result["invocation"]["facade"] == "dwgRW" else "dxf-ascii"
    )
    if (
        artifact["detectedFormat"] != expected_format
        or artifact["detectedVersion"] != result["input"]["detectedVersion"]
        or not re.fullmatch(r"AC[0-9]{4}", artifact["detectedVersion"])
    ):
        raise QualifiedDifferentialError(
            "generated artifact format/version contradicts the invocation"
        )
    carrier = carriers[0]
    if (
        carrier["recordId"] != record["id"]
        or carrier["sha256"] != artifact["sha256"]
        or carrier["byteSize"] != artifact["byteSize"]
        or carrier["disposition"] != "observed"
    ):
        raise QualifiedDifferentialError(
            "generatedOutput carrier does not match the on-disk artifact"
        )


def _run_live_side(
    *,
    side: str,
    manifest: dict[str, Any],
    manifest_digest: str,
    source_digest: str,
    receipt: dict[str, Any],
    receipt_path: Path,
    input_path: Path,
    input_id: str,
    input_path_hint: str,
    origin_kind: str,
    input_repository: str | None,
    input_commit: str | None,
    input_source_path: str | None,
    input_source_blob: str | None,
    input_registry_digest: str | None,
    expected_detected_format: str,
    expected_detected_version: str,
    generator_digest: str | None,
    facade: str,
    direction: str,
    timeout_seconds: float,
    expected_operation_succeeded: bool,
    output_probe: str,
) -> dict[str, Any]:
    runner = _runner_by_side(manifest, side)
    identity = receipt["adapter"]
    if identity["side"] != side:
        raise QualifiedDifferentialError(f"{side} receipt has the wrong side")
    if identity["sourceDigest"] != source_digest:
        raise QualifiedDifferentialError(f"{side} receipt source digest is stale")
    if identity["configDigest"] != manifest_digest:
        raise QualifiedDifferentialError(f"{side} receipt config digest is stale")
    executable = _artifact_path(receipt_path, receipt["artifacts"]["executable"])
    replacements = {
        "{executable}": str(executable),
        "{input}": str(input_path),
        "{inputPathHint}": input_path_hint,
        "{originKind}": origin_kind,
        "{inputId}": input_id,
        "{inputRepository}": input_repository or "",
        "{inputCommit}": input_commit or "",
        "{inputSourcePath}": input_source_path or "",
        "{inputSourceBlob}": input_source_blob or "",
        "{inputRegistryDigest}": input_registry_digest or "",
        "{generatorDigest}": generator_digest or "",
        "{facade}": facade,
        "{direction}": direction,
        "{adapterName}": identity["name"],
        "{adapterSide}": identity["side"],
        "{adapterPackage}": identity["package"],
        "{adapterCommit}": identity["commit"],
        "{sourceDigest}": identity["sourceDigest"],
        "{configDigest}": identity["configDigest"],
        "{staticLibraryDigest}": identity["staticLibraryDigest"],
        "{binaryDigest}": identity["binaryDigest"],
        "{linkedClosureDigest}": identity["linkedClosureDigest"],
    }
    encoded, exit_code, encoded_stderr, generated_artifact = _run_adapter_twice(
        runner["runCommand"], replacements, timeout_seconds,
        output_policy=(
            "forbidden" if direction == "read"
            else "required" if expected_operation_succeeded
            else "optional"
        ),
        output_suffix=".dwg" if facade == "dwgRW" else ".dxf",
        output_probe=output_probe,
    )
    try:
        result = _loads_json(encoded.decode("utf-8"), f"{side} adapter stdout")
    except (UnicodeError, json.JSONDecodeError) as exc:
        stderr = encoded_stderr.decode("utf-8", errors="replace").strip()
        if len(stderr) > 2048:
            stderr = stderr[:2048] + "..."
        raise QualifiedDifferentialError(
            f"{side} adapter did not emit one JSON document "
            f"(exit {exit_code}, stderr={stderr!r})"
        ) from exc
    result = validate_result(result, identity)
    if result["schema"] != _manifest_adapter_result_schema(manifest):
        raise QualifiedDifferentialError(
            f"{side} adapter result schema does not match the manifest"
        )
    _validate_generated_artifact(
        result, generated_artifact, direction, expected_operation_succeeded
    )
    if result["status"]["exitCode"] != exit_code:
        raise QualifiedDifferentialError(
            f"{side} process/result exit codes differ"
        )
    expected_input = {
        "id": input_id,
        "pathHint": input_path_hint,
        "originKind": origin_kind,
        "repository": input_repository,
        "commit": input_commit,
        "sourcePath": input_source_path,
        "sourceBlob": input_source_blob,
        "registryDigest": input_registry_digest,
        "generatorDigest": generator_digest,
        "sha256": _sha256_file(input_path),
        "byteSize": input_path.stat().st_size,
        "detectedFormat": expected_detected_format,
        "detectedVersion": expected_detected_version,
    }
    for key, expected in expected_input.items():
        if result["input"][key] != expected:
            raise QualifiedDifferentialError(
                f"{side} result input.{key} does not match the invocation"
            )
    if result["invocation"]["facade"] != facade:
        raise QualifiedDifferentialError(f"{side} result facade is stale")
    if result["invocation"]["direction"] != direction:
        raise QualifiedDifferentialError(f"{side} result direction is stale")
    return result


def run_live(
    *,
    root: Path,
    manifest_path: Path,
    input_path: Path,
    target_receipt_path: Path,
    standalone_receipt_path: Path,
    input_id: str,
    input_path_hint: str,
    origin_kind: str,
    input_repository: str | None,
    input_commit: str | None,
    generator_digest: str | None,
    input_git_dir: Path | None,
    facade: str,
    direction: str,
    timeout_seconds: float,
    expected_operation_succeeded: bool = True,
    expected_failure: dict[str, Any] | None = None,
) -> dict[str, Any]:
    """Execute the receipt-bound target and standalone adapters twice each."""

    manifest = validate_manifest(_read_json(manifest_path), root=root)
    if expected_operation_succeeded == (expected_failure is not None):
        raise QualifiedDifferentialError(
            "operation expectation and expected-failure oracle disagree"
        )
    if expected_failure is not None:
        expected_failure = _validate_expected_failure(
            expected_failure, "expectedFailure"
        )
        manifest_failure = _expected_failure_by_id(
            manifest, expected_failure["id"]
        )
        if manifest_failure != expected_failure:
            raise QualifiedDifferentialError(
                "expected-failure oracle is not the manifest-bound case"
            )
    output_probe = (
        expected_failure["outputProbe"]
        if expected_failure is not None else "none"
    )
    manifest_digest = _sha256_file(manifest_path)
    source_digest = _sha256_file(root / manifest["adapterSource"]["path"])
    target_receipt = validate_receipt(
        _read_json(target_receipt_path), receipt_path=target_receipt_path,
        check_artifacts=True,
    )
    standalone_receipt = validate_receipt(
        _read_json(standalone_receipt_path), receipt_path=standalone_receipt_path,
        check_artifacts=True,
    )
    if target_receipt["targetLock"] != manifest["targetLock"]:
        raise QualifiedDifferentialError(
            "target build receipt does not bind the manifest target lock"
        )
    if standalone_receipt["targetLock"] is not None:
        raise QualifiedDifferentialError(
            "standalone receipt unexpectedly claims target-lock provenance"
        )
    _validate_receipt_pair(target_receipt, standalone_receipt)
    if not input_path.is_file():
        raise QualifiedDifferentialError(f"input is absent: {input_path}")
    _identifier(input_id, "inputId")
    _path_hint(input_path_hint, "inputPathHint")
    if origin_kind not in ORIGIN_KINDS:
        raise QualifiedDifferentialError("originKind is not admitted")
    if origin_kind == "lockedRepositoryBlob":
        _string(input_repository, "inputRepository")
        _commit(input_commit, "inputCommit")
        if generator_digest is not None:
            raise QualifiedDifferentialError(
                "locked input cannot have generatorDigest"
            )
    else:
        if input_repository is not None or input_commit is not None:
            raise QualifiedDifferentialError(
                "local-from-scratch input cannot claim repository/commit"
            )
        _digest(generator_digest, "generatorDigest")
    if facade not in FACADES or direction not in DIRECTIONS:
        raise QualifiedDifferentialError("facade/direction is invalid")
    if expected_failure is not None and (
        expected_failure["inputId"] != input_id
        or expected_failure["inputPathHint"] != input_path_hint
        or expected_failure["facade"] != facade
        or expected_failure["direction"] != direction
    ):
        raise QualifiedDifferentialError(
            "expected-failure case does not match the live invocation"
        )
    provenance = _resolve_input_provenance(
        root=root,
        manifest=manifest,
        input_path=input_path,
        input_id=input_id,
        input_path_hint=input_path_hint,
        origin_kind=origin_kind,
        input_repository=input_repository,
        input_commit=input_commit,
        generator_digest=generator_digest,
        source_digest=source_digest,
        input_git_dir=input_git_dir,
    )
    target = _run_live_side(
        side="target", manifest=manifest, manifest_digest=manifest_digest,
        source_digest=source_digest, receipt=target_receipt,
        receipt_path=target_receipt_path, input_path=input_path,
        input_id=input_id, origin_kind=origin_kind,
        input_path_hint=input_path_hint,
        input_repository=input_repository, input_commit=input_commit,
        input_source_path=provenance["sourcePath"],
        input_source_blob=provenance["sourceBlob"],
        input_registry_digest=provenance["registryDigest"],
        expected_detected_format=provenance["detectedFormat"],
        expected_detected_version=provenance["detectedVersion"],
        generator_digest=generator_digest, facade=facade, direction=direction,
        timeout_seconds=timeout_seconds,
        expected_operation_succeeded=expected_operation_succeeded,
        output_probe=output_probe,
    )
    standalone = _run_live_side(
        side="standalone", manifest=manifest, manifest_digest=manifest_digest,
        source_digest=source_digest, receipt=standalone_receipt,
        receipt_path=standalone_receipt_path, input_path=input_path,
        input_id=input_id, origin_kind=origin_kind,
        input_path_hint=input_path_hint,
        input_repository=input_repository, input_commit=input_commit,
        input_source_path=provenance["sourcePath"],
        input_source_blob=provenance["sourceBlob"],
        input_registry_digest=provenance["registryDigest"],
        expected_detected_format=provenance["detectedFormat"],
        expected_detected_version=provenance["detectedVersion"],
        generator_digest=generator_digest, facade=facade, direction=direction,
        timeout_seconds=timeout_seconds,
        expected_operation_succeeded=expected_operation_succeeded,
        output_probe=output_probe,
    )
    return compare_results(
        target, standalone, manifest["comparison"],
        manifest_digest=manifest_digest, target_lock=manifest["targetLock"],
        expected_operation_succeeded=expected_operation_succeeded,
        expected_failure=expected_failure,
    )


def _digest_text(text: str) -> str:
    return hashlib.sha256(text.encode("utf-8")).hexdigest()


def _base_result(side: str, config_digest: str) -> dict[str, Any]:
    options = {"applyExtrusion": False, "compatibilityProfile": "default"}
    return {
        "schema": ADAPTER_RESULT_SCHEMA_V2,
        "kind": RESULT_KIND,
        "input": {
            "id": "synthetic-line",
            "pathHint": "local/synthetic-line-v1",
            "originKind": "localFromScratch",
            "repository": None,
            "commit": None,
            "sourcePath": None,
            "sourceBlob": None,
            "registryDigest": None,
            "generatorDigest": _digest_text("generator-v1"),
            "sha256": _digest_text("0\nEOF\n"),
            "byteSize": 6,
            "detectedFormat": "dxf-ascii",
            "detectedVersion": "AC1009",
        },
        "adapter": {
            "name": f"libdxfrw_semantic_adapter_{side}",
            "side": side,
            "package": (
                "LibreCAD-bundled-libdxfrw" if side == "target"
                else "standalone-libdxfrw"
            ),
            "commit": (
                "512d8bd86612f17158d4d222fcdc8a7c2b052c57" if side == "target"
                else "92d7466ed9146badcd4fb44c82d1dd8302b3c7db"
            ),
            "sourceDigest": _digest_text("adapter-source"),
            "configDigest": config_digest,
            "staticLibraryDigest": _digest_text(side + "-static"),
            "binaryDigest": _digest_text(side + "-binary"),
            "linkedClosureDigest": _digest_text(side + "-closure"),
        },
        "invocation": {
            "facade": "dxfRW",
            "direction": "read",
            "options": options,
            "optionsDigest": hashlib.sha256(_canonical_bytes(options)).hexdigest(),
        },
        "status": {
            "outcome": "success",
            "operationSucceeded": True,
            "exitCode": 0,
            "firstFailure": {
                "stage": None, "code": None, "path": None,
                "diagnosticId": None,
            },
        },
        "callbacks": [
            {
                "id": "callback:0", "ordinal": 0, "kind": "addLine",
                "recordId": "record:1", "blockContextId": "block:model",
                "carrierIds": ["carrier:1"],
            },
            {
                "id": "callback:1", "ordinal": 1, "kind": "endSection",
                "recordId": None, "blockContextId": None, "carrierIds": [],
            },
        ],
        "records": [{
            "id": "record:1",
            "callbackOrdinal": 0,
            "kind": "entity",
            "recordClass": "LINE",
            "entity": "LINE",
            "sourceHandle": "1",
            "ownerHandle": None,
            "blockId": "block:model",
            "fields": [
                {"name": "layer", "type": "string", "value": "0"},
                {"name": "start", "type": "point3d",
                 "value": {"x": 0.0, "y": 0.0, "z": 0.0}},
                {"name": "recordClass", "type": "string", "value": "LINE"},
                {"name": "entity", "type": "string", "value": "LINE"},
            ],
        }],
        "graphNodes": [
            {"id": "block:model", "kind": "block", "recordId": None,
             "handle": "0"},
            {"id": "node:record:1", "kind": "record",
             "recordId": "record:1", "handle": "1"},
            {"id": "node:handle:1", "kind": "handle", "recordId": None,
             "handle": "1"},
        ],
        "graphEdges": [
            {"id": "edge:handle:1", "kind": "handleReference",
             "from": "node:record:1", "to": "node:handle:1",
             "disposition": "observed"},
            {"id": "edge:block:1", "kind": "blockMembership",
             "from": "node:record:1", "to": "block:model",
             "disposition": "observed"},
        ],
        "opaqueCarriers": [{
            "id": "carrier:1",
            "source": "/records/record:1/opaque",
            "byteSize": 0,
            "sha256": hashlib.sha256(b"").hexdigest(),
            "disposition": "preservedCanonical",
            "recordId": "record:1",
            "sourceHandle": "1",
        }],
        "unsupportedContent": [{
            "id": "unsupported:1",
            "kind": "ACAD_PROXY_ENTITY",
            "source": "/records/record:1/opaque",
            "recordId": "record:1",
            "carrierId": "carrier:1",
            "diagnosticId": None,
            "disposition": "preservedCanonical",
            "reason": "typed parser unavailable; canonical carrier retained",
        }],
        "diagnostics": [],
        "cardinality": {
            "callbackCount": 2,
            "recordCount": 1,
            "fieldCount": 4,
            "graphNodeCount": 3,
            "graphEdgeCount": 2,
            "opaqueCarrierCount": 1,
            "unsupportedContentCount": 1,
            "diagnosticCount": 0,
        },
    }


def _base_observation_envelope(
    phase: str,
    availability: str,
    *,
    source_version: str | None = None,
    reason: str | None = None,
) -> dict[str, Any]:
    if availability != "available":
        return {
            "phase": phase,
            "availability": availability,
            "reason": reason or "notAvailableForSyntheticTest",
            "sourceVersion": source_version,
            "scalarCounters": None,
            "namedSummaries": None,
            "integrity": None,
            "frames": None,
        }
    empty_named = {
        "rows": [],
        "retainedRowCount": 0,
        "sourceNameCount": 0,
        "totalInstanceCount": 0,
        "omittedNameCount": 0,
        "omittedInstanceCount": 0,
        "truncatedRetainedNameCount": 0,
        "omittedNameBytes": 0,
        "aggregateSaturated": False,
    }
    return {
        "phase": phase,
        "availability": "available",
        "reason": None,
        "sourceVersion": source_version,
        "scalarCounters": {key: 0 for key in sorted(OBSERVATION_COUNTER_KEYS)},
        "namedSummaries": {
            "skippedCustomClasses": copy.deepcopy(empty_named),
            "skippedUnsupportedObjects": copy.deepcopy(empty_named),
        },
        "integrity": {
            "rows": [],
            "retainedRowCount": 0,
            "sourceRecordedCount": 0,
            "extractionOmittedCount": 0,
            "reportedUpstreamDroppedLowerBound": 0,
            "truncatedRetainedNameCount": 0,
            "omittedNameBytes": 0,
            "aggregateSaturated": False,
        },
        "frames": {
            "status": "finalizedComplete",
            "reason": None,
            "sourceFrameCount": 1,
            "dispositions": {
                "pending": 0,
                "deferred": 0,
                "staged": 0,
                "published": 1,
                "quarantined": 0,
                "failed": 0,
                "unresolved": 0,
            },
            "unrecognizedDispositionCount": 0,
        },
    }


def _base_observations(facade: str = "dwgRW", direction: str = "read") -> dict[str, Any]:
    if facade == "dxfRW":
        return {
            "inputRead": _base_observation_envelope(
                "inputRead", "notApplicable", reason="facadeIsDxf"
            ),
            "outputReadback": _base_observation_envelope(
                "outputReadback", "notApplicable", reason="facadeIsDxf"
            ),
        }
    if direction == "read":
        return {
            "inputRead": _base_observation_envelope(
                "inputRead", "available", source_version="AC1024"
            ),
            "outputReadback": _base_observation_envelope(
                "outputReadback", "notApplicable", reason="readOnlyNoOutputReadback"
            ),
        }
    return {
        "inputRead": _base_observation_envelope(
            "inputRead", "notApplicable", reason="writeRecipeNotDwg"
        ),
        "outputReadback": _base_observation_envelope(
            "outputReadback", "available", source_version="AC1024"
        ),
    }


def _assert_rejected(value: Any, needle: str) -> None:
    try:
        validate_result(value)
    except QualifiedDifferentialError as exc:
        if needle not in str(exc):
            raise AssertionError(
                f"negative vector failed for unexpected reason: {exc}"
            ) from exc
    else:
        raise AssertionError(f"negative vector was accepted ({needle})")


def self_test() -> None:
    support_rows = [{
        "path": "tests/semantic_adapter_support.h",
        "snapshotPath": "tests/semantic_adapter_support.h",
        "digest": _digest_text("adapter support source"),
    }]
    _validate_adapter_support_inputs(
        support_rows, "self-test.adapterSupportInputs"
    )
    for input_keys in MODERN_CONFIG_INPUT_KEYSETS:
        with_support = {key: None for key in input_keys}
        with_support["adapterSupportInputs"] = copy.deepcopy(support_rows)
        without_support = _without_adapter_support_inputs(
            with_support, "self-test.configInputs"
        )
        assert set(without_support) == input_keys
    try:
        _without_adapter_support_inputs(
            {**{key: None for key in LEGACY_CONFIG_INPUT_KEYS},
             "adapterSupportInputs": []},
            "self-test.legacyConfigInputs",
        )
    except QualifiedDifferentialError as exc:
        assert "unsupported for this config identity shape" in str(exc)
    else:
        raise AssertionError("support inputs were admitted into a frozen legacy shape")
    malformed_support = copy.deepcopy(support_rows)
    malformed_support[0]["snapshotPath"] = "../escape.h"
    try:
        _validate_adapter_support_inputs(
            malformed_support, "self-test.malformedAdapterSupportInputs"
        )
    except QualifiedDifferentialError as exc:
        assert "normalized relative hint" in str(exc)
    else:
        raise AssertionError("unsafe adapter support snapshot path was accepted")

    cmake_identity = {
        key: "" for key in LEGACY_CONFIG_CMAKE_KEYS
    }
    cmake_identity.update({
        "cmakeVersion": "cmake version 4.4.4",
        "configuredCmakeVersion": "4.4.4",
        "cmakeExecutable": "/tool/cmake",
        "generator": "Ninja",
        "multiConfig": False,
        "configurationTypes": [],
        "compilerPath": "/tool/clang++",
        "compilerId": "AppleClang",
        "compilerVersion": "27.0.0",
        "sizeofDataPointer": "8",
        "systemName": "Darwin",
        "compilerBuiltinSpecs": {
            "schema": 2,
            "status": "unavailable",
            "method": "",
            "compilerId": "AppleClang",
            "byteCount": 0,
            "digest": "",
            "reason": "compiler-family-not-qualified",
            "includeClosure": {
                "schema": 1,
                "status": "unavailable",
                "builtinDirectiveCount": 0,
                "externalDirectiveCount": 0,
                "reason": "compiler-family-not-qualified",
            },
        },
        "cxxStandard": 17,
        "cxxExtensions": False,
        "cacheBuildType": "Release",
        "crossCompiling": False,
        "osxArchitectures": [],
        "toolBinaries": {
            "cmake": {"path": "/tool/cmake", "digest": _digest_text("cmake")},
            "buildProgram": {
                "path": "/tool/ninja", "digest": _digest_text("ninja")
            },
            "compiler": {
                "path": "/tool/clang++", "digest": _digest_text("compiler")
            },
            "compilerLinker": {"path": "", "digest": ""},
            "archiver": {"path": "", "digest": ""},
            "ranlib": {"path": "", "digest": ""},
            "compilerFrontend": {"path": "", "digest": ""},
            "compilerAssembler": {"path": "", "digest": ""},
            "compilerCollect2": {"path": "", "digest": ""},
            "compilerLtoWrapper": {"path": "", "digest": ""},
            "compilerSpecs": {"path": "", "digest": ""},
        },
    })
    _validate_cmake_identity(
        cmake_identity, "self-test.cmakeIdentity", "Release"
    )
    _validate_cmake_identity(
        {key: value for key, value in cmake_identity.items()
         if key not in {"toolBinaries", "compilerBuiltinSpecs"}},
        "self-test.legacyCmakeIdentity",
        "Release",
    )
    _validate_cmake_identity(
        {key: value for key, value in cmake_identity.items()
         if key != "compilerBuiltinSpecs"},
        "self-test.preBuiltinSpecsCmakeIdentity",
        "Release",
    )
    legacy_tools_cmake = copy.deepcopy(cmake_identity)
    for key in (
        "compilerFrontend", "compilerAssembler", "compilerCollect2",
        "compilerLtoWrapper", "compilerSpecs",
    ):
        del legacy_tools_cmake["toolBinaries"][key]
    _validate_cmake_identity(
        legacy_tools_cmake,
        "self-test.legacyToolBinaryShape",
        "Release",
    )
    gnu_tool_cmake = copy.deepcopy(cmake_identity)
    gnu_tool_cmake["compilerId"] = "GNU"
    gnu_tool_cmake["compilerBuiltinSpecs"] = {
        "schema": 1,
        "status": "captured",
        "method": COMPILER_BUILTIN_SPECS_METHOD,
        "compilerId": "GNU",
        "byteCount": 32,
        "digest": _digest_text("gcc built-in specs"),
        "reason": "",
    }
    for role, filename in (
        ("compilerFrontend", "cc1plus"),
        ("compilerAssembler", "as"),
        ("compilerCollect2", "collect2"),
        ("compilerLtoWrapper", "lto-wrapper"),
        ("compilerSpecs", "specs"),
    ):
        gnu_tool_cmake["toolBinaries"][role] = {
            "path": "/toolchain/libexec/" + filename,
            "digest": _digest_text(filename),
        }
    _validate_cmake_identity(
        gnu_tool_cmake, "self-test.gnuToolBinaryShape", "Release"
    )
    gnu_profile_v2 = copy.deepcopy(gnu_tool_cmake["compilerBuiltinSpecs"])
    gnu_profile_v2.update({
        "schema": 2,
        "includeClosure": {
            "schema": 1,
            "status": "partial",
            "builtinDirectiveCount": 1,
            "externalDirectiveCount": 0,
            "reason": "spec-includes-not-resolved-or-content-bound",
        },
    })
    gnu_tool_cmake_v2 = copy.deepcopy(gnu_tool_cmake)
    gnu_tool_cmake_v2["compilerBuiltinSpecs"] = gnu_profile_v2
    _validate_cmake_identity(
        gnu_tool_cmake_v2, "self-test.gnuSpecsIncludeProfile", "Release"
    )
    changed_include_profile = copy.deepcopy(gnu_tool_cmake_v2)
    changed_include_profile["compilerBuiltinSpecs"]["includeClosure"][
        "builtinDirectiveCount"
    ] = -1
    try:
        _validate_cmake_identity(
            changed_include_profile,
            "self-test.invalidGnuSpecsIncludeProfile",
            "Release",
        )
    except QualifiedDifferentialError:
        pass
    else:
        raise AssertionError("invalid GCC specs include count was accepted")
    posthoc_specs_cmake = copy.deepcopy(gnu_tool_cmake)
    posthoc_specs_cmake["compilerBuiltinSpecs"] = {
        "schema": 1,
        "status": "unavailable",
        "method": "",
        "compilerId": "GNU",
        "byteCount": 0,
        "digest": "",
        "reason": "existing-build-not-scanned-before-compilation",
    }
    _validate_cmake_identity(
        posthoc_specs_cmake,
        "self-test.posthocBuiltinSpecsProfile",
        "Release",
    )
    for mutation, edit in (
        ("digest", lambda row: row.update({"digest": "z" * 64})),
        ("compiler", lambda row: row.update({"compilerId": "Clang"})),
        ("unknownStatus", lambda row: row.update({"status": "partial"})),
        ("unknownCount", lambda row: row.update({"byteCount": True})),
    ):
        changed_cmake = copy.deepcopy(gnu_tool_cmake)
        edit(changed_cmake["compilerBuiltinSpecs"])
        try:
            _validate_cmake_identity(
                changed_cmake,
                "self-test.invalidBuiltinSpecsProfile",
                "Release",
            )
        except QualifiedDifferentialError:
            pass
        else:
            raise AssertionError(
                "invalid GCC built-in specs profile was accepted: " + mutation
            )
    for mutation in (
        "badDigest", "pathMismatch", "missingRequiredTool",
        "mismatchedOptionalTool",
    ):
        changed_cmake = copy.deepcopy(cmake_identity)
        if mutation == "badDigest":
            changed_cmake["toolBinaries"]["compiler"]["digest"] = "z" * 64
        elif mutation == "pathMismatch":
            changed_cmake["toolBinaries"]["compiler"]["path"] = "/other/clang++"
        elif mutation == "missingRequiredTool":
            changed_cmake["toolBinaries"]["buildProgram"] = {
                "path": "", "digest": ""
            }
        else:
            changed_cmake["toolBinaries"]["compilerFrontend"] = {
                "path": "", "digest": _digest_text("unpaired")
            }
        try:
            _validate_cmake_identity(
                changed_cmake,
                "self-test.invalidToolBinaryIdentity",
                "Release",
            )
        except QualifiedDifferentialError:
            pass
        else:
            raise AssertionError(
                f"invalid CMake tool binary identity was accepted: {mutation}"
            )

    assert _validate_compile_contract_disposition(
        {"compileContractEnforced": True}
    ) == "legacy"
    assert _validate_compile_contract_disposition({
        "compileContractMode": "direct-options-only",
        "compileOptionAdmissionEnforced": True,
        "compileContractEnforced": False,
    }) == "direct-options-only"
    for invalid_disposition in (
        {"compileContractEnforced": False},
        {"compileContractMode": "direct-options-only",
         "compileOptionAdmissionEnforced": False,
         "compileContractEnforced": False},
        {"compileContractMode": "direct-options-only",
         "compileOptionAdmissionEnforced": True,
         "compileContractEnforced": True},
        {"compileContractMode": "unknown",
         "compileOptionAdmissionEnforced": True,
         "compileContractEnforced": True},
    ):
        try:
            _validate_compile_contract_disposition(invalid_disposition)
        except QualifiedDifferentialError:
            pass
        else:
            raise AssertionError("invalid compile-contract disposition was accepted")
    controlled_context_payload = {
        "schema": 1,
        "mode": "controlled-driver-v1",
        "environmentDigest": _digest_text("environment"),
        "compilerPath": "/toolchain/bin/c++",
        "compilerPathDigest": hashlib.sha256(
            b"/toolchain/bin/c++"
        ).hexdigest(),
        "compilerBinaryDigest": _digest_text("compiler-binary"),
    }
    controlled_context = {
        **controlled_context_payload,
        "digest": hashlib.sha256(
            _canonical_bytes(controlled_context_payload) + b"\n"
        ).hexdigest(),
    }
    _validate_controlled_driver_context(
        controlled_context, "self-test.driverContext"
    )
    controlled_cmake_payload = {
        **controlled_context_payload,
        "cmakePath": "/tool/cmake",
        "cmakePathDigest": hashlib.sha256(b"/tool/cmake").hexdigest(),
        "cmakeBinaryDigest": _digest_text("cmake-binary"),
    }
    controlled_cmake_context = {
        **controlled_cmake_payload,
        "digest": hashlib.sha256(
            _canonical_bytes(controlled_cmake_payload) + b"\n"
        ).hexdigest(),
    }
    _validate_controlled_driver_context(
        controlled_cmake_context, "self-test.driverContextWithCmake"
    )
    changed_cmake_context = copy.deepcopy(controlled_cmake_context)
    changed_cmake_context["cmakePathDigest"] = "0" * 64
    changed_cmake_payload = {
        key: value for key, value in changed_cmake_context.items()
        if key != "digest"
    }
    changed_cmake_context["digest"] = hashlib.sha256(
        _canonical_bytes(changed_cmake_payload) + b"\n"
    ).hexdigest()
    try:
        _validate_controlled_driver_context(
            changed_cmake_context, "self-test.invalidDriverCmakePath"
        )
    except QualifiedDifferentialError:
        pass
    else:
        raise AssertionError("stale CMake path digest was accepted")
    def contract_profile(
        *, mode: str, disposition: str, checks: dict[str, str]
    ) -> dict[str, Any]:
        value: dict[str, Any] = {
            "schema": 1,
            "name": "effective-driver-contract-v1",
            "mode": mode,
            "disposition": disposition,
            "checks": checks,
        }
        value["digest"] = hashlib.sha256(
            _canonical_bytes(value) + b"\n"
        ).hexdigest()
        return value

    advisory_compile_profile = contract_profile(
        mode="controlled-fresh-build-v1",
        disposition="advisory",
        checks={
            "directOptions": "pass",
            "driverIdentity": "pass",
            "freshBuild": "pass",
            "ambientOverrides": "pass",
            "clangDefaultConfig": "pass",
            "implicitConfiguration": "partial",
            "effectiveOptionPolicy": "pass",
        },
    )
    _validate_compile_contract_profile(
        advisory_compile_profile,
        driver_context_present=True,
        compiler_id="AppleClang",
        path="self-test.compileContractProfile",
    )
    enforced_compile_profile = contract_profile(
        mode="controlled-fresh-build-v1",
        disposition="enforced",
        checks={
            "directOptions": "pass",
            "driverIdentity": "pass",
            "freshBuild": "pass",
            "ambientOverrides": "pass",
            "clangDefaultConfig": "pass",
            "implicitConfiguration": "pass",
            "effectiveOptionPolicy": "pass",
        },
    )
    _validate_compile_contract_profile(
        enforced_compile_profile,
        driver_context_present=True,
        compiler_id="AppleClang",
        path="self-test.enforcedCompileContractProfile",
    )
    assert _validate_compile_contract_disposition({
        "compileContractMode": "controlled-driver-profile-v1",
        "compileOptionAdmissionEnforced": True,
        "compileContractEnforced": True,
        "compileContractProfile": enforced_compile_profile,
    }) == "controlled-driver-profile-v1"
    inspected_compile_profile = contract_profile(
        mode="existing-build-inspection-v1",
        disposition="unavailable",
        checks={
            "directOptions": "pass",
            "driverIdentity": "unavailable",
            "freshBuild": "unavailable",
            "ambientOverrides": "unavailable",
            "clangDefaultConfig": "not-applicable",
            "implicitConfiguration": "unavailable",
            "effectiveOptionPolicy": "pending",
        },
    )
    _validate_compile_contract_profile(
        inspected_compile_profile,
        driver_context_present=False,
        compiler_id="GNU",
        path="self-test.inspectedCompileContractProfile",
    )
    for changed_profile, context_present, compiler_id in (
        (dict(advisory_compile_profile, digest="0" * 64), True, "AppleClang"),
        (advisory_compile_profile, False, "AppleClang"),
        (contract_profile(
            mode="controlled-fresh-build-v1",
            disposition="enforced",
            checks={**enforced_compile_profile["checks"],
                    "effectiveOptionPolicy": "pending"},
        ), True, "AppleClang"),
    ):
        try:
            _validate_compile_contract_profile(
                changed_profile,
                driver_context_present=context_present,
                compiler_id=compiler_id,
                path="self-test.invalidCompileContractProfile",
            )
        except QualifiedDifferentialError:
            pass
        else:
            raise AssertionError("invalid compile-contract profile was accepted")
    for key, value in (
        ("environmentDigest", "0" * 64),
        ("compilerPath", "/different/compiler"),
        ("compilerBinaryDigest", "f" * 64),
        ("mode", "uncontrolled"),
    ):
        changed = copy.deepcopy(controlled_context)
        changed[key] = value
        try:
            _validate_controlled_driver_context(changed, "self-test.driverContext")
        except QualifiedDifferentialError:
            pass
        else:
            raise AssertionError(f"tampered controlled driver {key} was accepted")

    target_interface_profile = {
        "schema": 1,
        "name": EXPECTED_INTERFACE_PROFILE_NAME,
        "side": "target",
        "commonMethodCount": 170,
        "commonMethodDigest": EXPECTED_COMMON_INTERFACE_DIGEST,
        "interfaceMethodCount": 170,
        "interfaceMethodDigest": _digest_text("target-interface"),
        "defaultedInterfaceExtras": [],
        "adapterSupplementalMethods": [],
        "unobservedDefaultedExtras": [],
    }
    standalone_interface_profile = {
        "schema": 1,
        "name": EXPECTED_INTERFACE_PROFILE_NAME,
        "side": "standalone",
        "commonMethodCount": 170,
        "commonMethodDigest": EXPECTED_COMMON_INTERFACE_DIGEST,
        "interfaceMethodCount": 176,
        "interfaceMethodDigest": _digest_text("standalone-interface"),
        "defaultedInterfaceExtras": EXPECTED_STANDALONE_INTERFACE_EXTRAS,
        "adapterSupplementalMethods": EXPECTED_STANDALONE_INTERFACE_EXTRAS,
        "unobservedDefaultedExtras": [],
    }
    _validate_interface_profile(
        target_interface_profile, "target", "self-test.targetInterfaceProfile"
    )
    _validate_interface_profile(
        standalone_interface_profile, "standalone",
        "self-test.standaloneInterfaceProfile",
    )
    legacy_standalone_interface_profile = copy.deepcopy(
        standalone_interface_profile
    )
    legacy_standalone_interface_profile.update({
        "name": LEGACY_INTERFACE_PROFILE_NAME,
        "interfaceMethodCount": 173,
        "defaultedInterfaceExtras": LEGACY_STANDALONE_INTERFACE_EXTRAS,
        "adapterSupplementalMethods": LEGACY_STANDALONE_INTERFACE_EXTRAS,
    })
    _validate_interface_profile(
        legacy_standalone_interface_profile, "standalone",
        "self-test.legacyStandaloneInterfaceProfile",
    )
    for key, bad_value in (
        ("commonMethodDigest", "0" * 64),
        ("interfaceMethodCount", True),
        ("defaultedInterfaceExtras", ["addBlockRecord"]),
        ("adapterSupplementalMethods", []),
    ):
        changed = copy.deepcopy(standalone_interface_profile)
        changed[key] = bad_value
        try:
            _validate_interface_profile(
                changed, "standalone", "self-test.standaloneInterfaceProfile"
            )
        except QualifiedDifferentialError:
            pass
        else:
            raise AssertionError(f"tampered interface profile {key} was accepted")

    capability_profiles = {
        side: {
            "schema": 1,
            "name": EXPECTED_CAPABILITY_PROFILE_NAME,
            "side": side,
            "capabilities": copy.deepcopy(
                EXPECTED_CAPABILITIES_BY_SIDE[side]
            ),
        }
        for side in ("target", "standalone")
    }
    for side, profile in capability_profiles.items():
        _validate_semantic_capability_profile(
            profile, side, f"self-test.{side}CapabilityProfile"
        )
        invalid_profiles = []
        changed = copy.deepcopy(profile)
        changed["schema"] = True
        invalid_profiles.append(changed)
        changed = copy.deepcopy(profile)
        changed["side"] = "standalone" if side == "target" else "target"
        invalid_profiles.append(changed)
        changed = copy.deepcopy(profile)
        changed["unknown"] = False
        invalid_profiles.append(changed)
        changed = copy.deepcopy(profile)
        changed["capabilities"][sorted(CAPABILITY_KEYS)[0]] = 1
        invalid_profiles.append(changed)
        changed = copy.deepcopy(profile)
        changed["capabilities"][sorted(CAPABILITY_KEYS)[0]] = not (
            changed["capabilities"][sorted(CAPABILITY_KEYS)[0]]
        )
        invalid_profiles.append(changed)
        for invalid in invalid_profiles:
            try:
                _validate_semantic_capability_profile(
                    invalid, side, f"self-test.{side}CapabilityProfile"
                )
            except QualifiedDifferentialError:
                pass
            else:
                raise AssertionError(
                    f"invalid {side} semantic capability profile was accepted"
                )

    def capability_test_record(side: str, record_id: str,
                               record_class: str) -> tuple[dict[str, Any], list[dict[str, Any]]]:
        capabilities = EXPECTED_CAPABILITIES_BY_SIDE[side]
        fields: list[dict[str, Any]] = []
        exclusions: list[dict[str, Any]] = []

        def add(feature: str, expected_fields: dict[str, str], excluded_name: str) -> None:
            if capabilities[feature]:
                for name, field_type in expected_fields.items():
                    value = {
                        "bool": False,
                        "int64": "0",
                        "uint64": "0",
                        "string": "",
                    }[field_type]
                    fields.append({"name": name, "type": field_type, "value": value})
            else:
                exclusions.append({
                    "kind": "semanticFields",
                    "source": (
                        f"/records/{_pointer_token(record_id)}/"
                        f"{_pointer_token(excluded_name)}"
                    ),
                    "recordId": record_id,
                    "disposition": "excluded",
                    "reason": "adapterFieldCoverageExcluded",
                })

        if record_class == "MODELER_GEOMETRY":
            add("modelerDwgSourceVersion", {"dwgSourceVersion": "int64"},
                "dwgSourceVersion")
            add("modelerPayloadStatusName", {"modelerPayloadStatus": "string"},
                "modelerPayloadStatus")
            add("modelerDwgAcisPayload", {
                "hasDwgInlineAcisPayload": "bool",
                "dwgInlineAcisByteCount": "uint64",
            }, "dwgInlineAcisPayload")
            add("modelerDxfPayloadChunks", {
                "dxfPayloadChunk.count": "uint64",
            }, "dxfPayloadChunks")
            fields.append({"name": "payloadRangeCount", "type": "uint64", "value": "2"})
            for index in range(2):
                fields.extend((
                    {"name": f"payloadRange.{index}.offset", "type": "uint64", "value": "0"},
                    {"name": f"payloadRange.{index}.length", "type": "uint64", "value": "0"},
                ))
                add("payloadRangeBitOffset", {
                    f"payloadRange.{index}.bitOffset": "uint64",
                }, f"payloadRange.{index}.bitOffset")
        else:
            add("surfaceDxfPayloadChunks", {
                "dxfAcisChunk.count": "uint64",
            }, "dxfAcisChunks")
        return ({
            "id": record_id,
            "kind": "entity",
            "recordClass": record_class,
            "fields": fields,
        }, exclusions)

    capability_test_rows: dict[str, tuple[list[dict[str, Any]], list[dict[str, Any]]]] = {}
    for side in ("target", "standalone"):
        modeler_record, modeler_exclusions = capability_test_record(
            side, f"record:{side}:modeler", "MODELER_GEOMETRY"
        )
        surface_record, surface_exclusions = capability_test_record(
            side, f"record:{side}:surface", "PLANESURFACE"
        )
        test_records = [modeler_record, surface_record]
        test_exclusions = modeler_exclusions + surface_exclusions
        _validate_adapter_field_capabilities(test_records, test_exclusions, side)
        capability_test_rows[side] = (test_records, test_exclusions)

    def expect_capability_rejection(records: list[dict[str, Any]],
                                    unsupported: list[dict[str, Any]],
                                    side: str, label: str) -> None:
        try:
            _validate_adapter_field_capabilities(records, unsupported, side)
        except QualifiedDifferentialError:
            return
        raise AssertionError(f"invalid capability/result mapping accepted: {label}")

    target_records, target_exclusions = capability_test_rows["target"]
    fabricated_target_field = copy.deepcopy(target_records)
    fabricated_target_field[0]["fields"].append({
        "name": "dwgSourceVersion", "type": "int64", "value": "1027",
    })
    expect_capability_rejection(
        fabricated_target_field, target_exclusions, "target", "fabricated target field"
    )
    missing_target_exclusion = copy.deepcopy(target_exclusions)
    missing_target_exclusion = [
        row for row in missing_target_exclusion
        if not row["source"].endswith("/dxfPayloadChunks")
    ]
    expect_capability_rejection(
        target_records, missing_target_exclusion, "target", "missing target exclusion"
    )
    inflated_range_count = copy.deepcopy(target_records)
    range_count_field = next(
        field for field in inflated_range_count[0]["fields"]
        if field["name"] == "payloadRangeCount"
    )
    range_count_field["value"] = "18446744073709551615"
    expect_capability_rejection(
        inflated_range_count, target_exclusions,
        "target", "inflated payload-range count",
    )
    fabricated_surface_field = copy.deepcopy(target_records)
    fabricated_surface_field[1]["fields"].append({
        "name": "dxfAcisChunk.count", "type": "uint64", "value": "0",
    })
    expect_capability_rejection(
        fabricated_surface_field, target_exclusions,
        "target", "fabricated target surface field",
    )
    extra_target_range_exclusion = copy.deepcopy(target_exclusions)
    extra_target_range_exclusion.append({
        "kind": "semanticFields",
        "source": "/records/record:target:modeler/payloadRange.99.bitOffset",
        "recordId": "record:target:modeler",
        "disposition": "excluded",
        "reason": "adapterFieldCoverageExcluded",
    })
    expect_capability_rejection(
        target_records, extra_target_range_exclusion,
        "target", "out-of-range target bit-offset exclusion",
    )

    standalone_records, standalone_exclusions = capability_test_rows["standalone"]
    missing_standalone_field = copy.deepcopy(standalone_records)
    missing_standalone_field[0]["fields"] = [
        field for field in missing_standalone_field[0]["fields"]
        if field["name"] != "dxfPayloadChunk.count"
    ]
    expect_capability_rejection(
        missing_standalone_field, standalone_exclusions,
        "standalone", "missing standalone zero-count field",
    )
    missing_standalone_surface_field = copy.deepcopy(standalone_records)
    missing_standalone_surface_field[1]["fields"] = [
        field for field in missing_standalone_surface_field[1]["fields"]
        if field["name"] != "dxfAcisChunk.count"
    ]
    expect_capability_rejection(
        missing_standalone_surface_field, standalone_exclusions,
        "standalone", "missing standalone surface zero-count field",
    )
    extra_standalone_range_field = copy.deepcopy(standalone_records)
    extra_standalone_range_field[0]["fields"].append({
        "name": "payloadRange.99.bitOffset", "type": "uint64", "value": "0",
    })
    expect_capability_rejection(
        extra_standalone_range_field, standalone_exclusions,
        "standalone", "out-of-range standalone bit-offset field",
    )

    config_digest = _digest_text("manifest-v2")
    # Path.is_absolute() is host-specific. Rooted POSIX paths have no drive
    # letter and are not absolute according to pathlib on Windows, so build
    # these synthetic dependency paths from the native working directory.
    toolchain_include = Path.cwd() / "toolchain" / "include"
    standard_header_path = str(toolchain_include / "standard-header.h")
    target_only_header_path = str(toolchain_include / "target-only.h")
    standalone_only_header_path = str(toolchain_include / "standalone-only.h")
    dependency_inputs = [{
        "path": standard_header_path,
        "size": 23,
        "digest": _digest_text("standard-header"),
    }]
    dependency_profile = {
        "schema": 1,
        "status": "captured",
        "method": "gnu-clang-m-dependency-scan-v1",
        "translationUnitCount": 2,
        "externalInputs": dependency_inputs,
        "externalInputsDigest": hashlib.sha256(
            _canonical_bytes(dependency_inputs) + b"\n"
        ).hexdigest(),
        "reason": "",
    }
    _validate_compiler_dependencies(
        dependency_profile, "self-test.compilerDependencies"
    )
    compiler_resource_path = (
        Path.cwd() / "toolchain" / "lib" / "clang" / "21"
    )
    compiler_resource_profile = {
        "schema": 1,
        "status": "captured",
        "method": "clang-resource-tree-v1",
        "compilerId": "AppleClang",
        "path": str(compiler_resource_path),
        "entryCount": 2,
        "fileCount": 1,
        "totalBytes": 23,
        "treeDigest": _digest_text("clang-resource-tree"),
        "reason": "",
    }
    _validate_compiler_resources(
        compiler_resource_profile, "self-test.compilerResources"
    )
    unavailable_resource_profile = {
        "schema": 1,
        "status": "unavailable",
        "method": "",
        "compilerId": "GNU",
        "path": "",
        "entryCount": 0,
        "fileCount": 0,
        "totalBytes": 0,
        "treeDigest": hashlib.sha256(_canonical_bytes([]) + b"\n").hexdigest(),
        "reason": "compiler-family-not-qualified",
    }
    _validate_compiler_resources(
        unavailable_resource_profile,
        "self-test.unavailableCompilerResources",
    )
    apple_sdk_manifests = [{
        "path": "SDKSettings.json", "size": 9,
        "digest": _digest_text("sdk-json"),
    }]
    apple_sdk_root = {
        "selectedPaths": [{
            "source": "CMAKE_CXX_COMPILER_APPLE_SYSROOT",
            "path": str(Path.cwd() / "toolchain" / "MacOSX.sdk"),
        }],
        "resolvedPath": str(Path.cwd() / "toolchain" / "MacOSX.sdk"),
        "metadataFiles": apple_sdk_manifests,
        "metadataDigest": hashlib.sha256(
            _canonical_bytes(apple_sdk_manifests) + b"\n"
        ).hexdigest(),
    }
    apple_sdk_roots = [apple_sdk_root]
    apple_sdk_profile = {
        "schema": 1,
        "status": "captured",
        "method": "apple-sdk-settings-v1",
        "systemName": "Darwin",
        "compilerId": "AppleClang",
        "roots": apple_sdk_roots,
        "rootsDigest": hashlib.sha256(
            _canonical_bytes(apple_sdk_roots) + b"\n"
        ).hexdigest(),
        "reason": "",
    }
    _validate_apple_sdk_profile(apple_sdk_profile, "self-test.appleSdk")
    unavailable_apple_sdk = {
        "schema": 1,
        "status": "unavailable",
        "method": "",
        "systemName": "Linux",
        "compilerId": "GNU",
        "roots": [],
        "rootsDigest": hashlib.sha256(_canonical_bytes([]) + b"\n").hexdigest(),
        "reason": "platform-not-qualified",
    }
    _validate_apple_sdk_profile(
        unavailable_apple_sdk, "self-test.unavailableAppleSdk"
    )
    malformed_apple_sdk = copy.deepcopy(apple_sdk_profile)
    malformed_apple_sdk["roots"][0]["metadataDigest"] = _digest_text("stale")
    try:
        _validate_apple_sdk_profile(
            malformed_apple_sdk, "self-test.malformedAppleSdk"
        )
    except QualifiedDifferentialError:
        pass
    else:
        raise AssertionError("malformed Apple SDK metadata identity was accepted")
    apple_runtime_nodes = [{
        "installName": "/usr/lib/libc++.1.dylib",
        "directLinkInput": True,
        "images": [{
            "architecture": "arm64e",
            "uuid": "A21743EE-6E1C-3E74-97EA-7AF793DE4BDF",
        }],
        "sdkStubs": [{
            "sdkRoot": "/toolchain/MacOSX.sdk",
            "path": "/toolchain/MacOSX.sdk/usr/lib/libc++.tbd",
            "resolvedPath": "/toolchain/MacOSX.sdk/usr/lib/libc++.tbd",
            "size": 100,
            "digest": _digest_text("libc++ stub"),
            "installName": "/usr/lib/libc++.1.dylib",
            "currentVersion": "2200.27",
            "compatibilityVersion": "1.0",
        }],
    }]
    apple_runtime_edges = [{
        "source": "adapter-executable",
        "target": "/usr/lib/libc++.1.dylib",
        "architecture": "arm64",
        "attribute": "load",
    }]
    apple_runtime_profile = {
        "schema": 1,
        "status": "partial",
        "method": "macho-dyld-uuid-closure-v1",
        "systemName": "Darwin",
        "compilerId": "AppleClang",
        "executableDigest": _digest_text("side-specific executable"),
        "tool": {
            "path": "/usr/bin/dyld_info",
            "digest": _digest_text("dyld_info tool"),
        },
        "nodes": apple_runtime_nodes,
        "edges": apple_runtime_edges,
        "nodesDigest": hashlib.sha256(
            _canonical_bytes(apple_runtime_nodes) + b"\n"
        ).hexdigest(),
        "edgesDigest": hashlib.sha256(
            _canonical_bytes(apple_runtime_edges) + b"\n"
        ).hexdigest(),
        "unresolvedImageCount": 0,
        "unmatchedSdkStubCount": 0,
        "maximumDepth": 1,
        "reason": "link-stub-selection-not-proven",
    }
    _validate_apple_runtime_profile(
        apple_runtime_profile, "self-test.appleRuntime"
    )
    apple_link_build_objects = [{
        "path": "CMakeFiles/adapter.dir/adapter.cpp.o",
        "size": 128,
        "digest": _digest_text("adapter object"),
    }]
    apple_link_archive_members = ["drw_entities.cpp.o"]
    apple_link_external_inputs = [{
        "sdkRoot": "/toolchain/MacOSX.sdk",
        "path": "usr/lib/libc++.tbd",
        "size": 100,
        "digest": _digest_text("selected libc++ stub"),
    }]
    apple_link_empty_digest = hashlib.sha256(_canonical_bytes([]) + b"\n").hexdigest()
    apple_link_inputs_profile = {
        "schema": 1,
        "status": "captured",
        "method": "apple-ld-map-v1",
        "systemName": "Darwin",
        "compilerId": "AppleClang",
        "architecture": "arm64",
        "executableDigest": _digest_text("side-specific executable"),
        "mapSize": 2048,
        "mapDigest": _digest_text("side-specific raw linker map"),
        "objectCount": 4,
        "synthesizedObjectCount": 1,
        "buildObjects": apple_link_build_objects,
        "buildObjectsDigest": hashlib.sha256(
            _canonical_bytes(apple_link_build_objects) + b"\n"
        ).hexdigest(),
        "linkedArchive": {
            "path": "<build>/lib/libdxfrw.a",
            "size": 1024,
            "digest": _digest_text("side-specific static archive"),
        },
        "archiveMembers": apple_link_archive_members,
        "archiveMembersDigest": hashlib.sha256(
            _canonical_bytes(apple_link_archive_members) + b"\n"
        ).hexdigest(),
        "externalInputs": apple_link_external_inputs,
        "externalInputsDigest": hashlib.sha256(
            _canonical_bytes(apple_link_external_inputs) + b"\n"
        ).hexdigest(),
        "unclassifiedCount": 0,
        "unclassifiedDigest": apple_link_empty_digest,
        "reason": "",
    }
    _validate_apple_link_inputs_profile(
        apple_link_inputs_profile, "self-test.appleLinkInputs"
    )
    matched_apple_link_inputs = copy.deepcopy(apple_link_inputs_profile)
    runtime_stub = apple_runtime_nodes[0]["sdkStubs"][0]
    matched_apple_link_inputs["externalInputs"][0]["digest"] = runtime_stub[
        "digest"
    ]
    matched_apple_link_inputs["externalInputsDigest"] = hashlib.sha256(
        _canonical_bytes(matched_apple_link_inputs["externalInputs"]) + b"\n"
    ).hexdigest()
    _validate_apple_link_inputs_profile(
        matched_apple_link_inputs, "self-test.matchedAppleLinkInputs"
    )
    selected_runtime_profile = copy.deepcopy(apple_runtime_profile)
    selected_runtime_profile.update({
        "schema": 2,
        "linkerSelectedStubs": [{
            "installName": "/usr/lib/libc++.1.dylib",
            "sdkRoot": "/toolchain/MacOSX.sdk",
            "path": "usr/lib/libc++.tbd",
            "size": 100,
            "digest": runtime_stub["digest"],
        }],
        "linkerSelectedStubsDigest": hashlib.sha256(
            _canonical_bytes([{
                "installName": "/usr/lib/libc++.1.dylib",
                "sdkRoot": "/toolchain/MacOSX.sdk",
                "path": "usr/lib/libc++.tbd",
                "size": 100,
                "digest": runtime_stub["digest"],
            }]) + b"\n"
        ).hexdigest(),
        "unselectedDirectInstallNameCount": 0,
        "reason": "runtime-byte-identity-not-proven",
    })
    _validate_apple_runtime_profile(
        selected_runtime_profile, "self-test.selectedAppleRuntime"
    )
    _validate_apple_runtime_linker_selection(
        selected_runtime_profile,
        matched_apple_link_inputs,
        "self-test",
    )
    stale_selected_runtime = copy.deepcopy(selected_runtime_profile)
    stale_selected_runtime["linkerSelectedStubs"][0]["digest"] = _digest_text(
        "unselected candidate digest"
    )
    stale_selected_runtime["linkerSelectedStubsDigest"] = hashlib.sha256(
        _canonical_bytes(stale_selected_runtime["linkerSelectedStubs"]) + b"\n"
    ).hexdigest()
    _validate_apple_runtime_profile(
        stale_selected_runtime, "self-test.staleSelectedAppleRuntime"
    )
    try:
        _validate_apple_runtime_linker_selection(
            stale_selected_runtime,
            matched_apple_link_inputs,
            "self-test",
        )
    except QualifiedDifferentialError:
        pass
    else:
        raise AssertionError("runtime/linker stub byte mismatch was accepted")
    unselected_link_inputs = copy.deepcopy(matched_apple_link_inputs)
    unselected_link_inputs["externalInputs"][0]["digest"] = _digest_text(
        "different linker-selected stub"
    )
    unselected_link_inputs["externalInputsDigest"] = hashlib.sha256(
        _canonical_bytes(unselected_link_inputs["externalInputs"]) + b"\n"
    ).hexdigest()
    unselected_runtime_profile = copy.deepcopy(selected_runtime_profile)
    unselected_runtime_profile.update({
        "linkerSelectedStubs": [],
        "linkerSelectedStubsDigest": hashlib.sha256(
            _canonical_bytes([]) + b"\n"
        ).hexdigest(),
        "unselectedDirectInstallNameCount": 1,
        "reason": "link-stub-selection-not-proven",
    })
    _validate_apple_runtime_profile(
        unselected_runtime_profile, "self-test.unselectedAppleRuntime"
    )
    _validate_apple_runtime_linker_selection(
        unselected_runtime_profile,
        unselected_link_inputs,
        "self-test",
    )
    stale_unselected_count = copy.deepcopy(unselected_runtime_profile)
    stale_unselected_count["unselectedDirectInstallNameCount"] = 0
    try:
        _validate_apple_runtime_linker_selection(
            stale_unselected_count,
            unselected_link_inputs,
            "self-test",
        )
    except QualifiedDifferentialError:
        pass
    else:
        raise AssertionError("stale unselected install-name count was accepted")
    malformed_apple_link_inputs = copy.deepcopy(apple_link_inputs_profile)
    malformed_apple_link_inputs["objectCount"] = 3
    try:
        _validate_apple_link_inputs_profile(
            malformed_apple_link_inputs, "self-test.malformedAppleLinkInputs"
        )
    except QualifiedDifferentialError:
        pass
    else:
        raise AssertionError("incomplete Apple linker map rows were accepted")
    malformed_apple_runtime = copy.deepcopy(apple_runtime_profile)
    malformed_apple_runtime["nodesDigest"] = _digest_text("stale")
    try:
        _validate_apple_runtime_profile(
            malformed_apple_runtime, "self-test.malformedAppleRuntime"
        )
    except QualifiedDifferentialError:
        pass
    else:
        raise AssertionError("malformed Apple runtime graph was accepted")
    changed_resource_profile = copy.deepcopy(compiler_resource_profile)
    changed_resource_profile["fileCount"] = 0
    try:
        _validate_compiler_resources(
            changed_resource_profile, "self-test.changedCompilerResources"
        )
    except QualifiedDifferentialError:
        pass
    else:
        raise AssertionError("malformed compiler resource identity was accepted")
    unavailable_dependencies = {
        "schema": 1,
        "status": "unavailable",
        "method": "",
        "translationUnitCount": 0,
        "externalInputs": [],
        "externalInputsDigest": hashlib.sha256(
            _canonical_bytes([]) + b"\n"
        ).hexdigest(),
        "reason": "compiler-family-not-qualified",
    }
    _validate_compiler_dependencies(
        unavailable_dependencies, "self-test.unavailableCompilerDependencies"
    )
    tampered_dependencies = copy.deepcopy(dependency_profile)
    tampered_dependencies["externalInputs"][0]["size"] += 1
    try:
        _validate_compiler_dependencies(
            tampered_dependencies, "self-test.tamperedCompilerDependencies"
        )
    except QualifiedDifferentialError:
        pass
    else:
        raise AssertionError("tampered compiler dependency closure was accepted")

    target = _base_result("target", config_digest)
    standalone = _base_result("standalone", config_digest)
    validate_result(target)
    validate_result(standalone)
    wrong_nonhistory_disposition = copy.deepcopy(target)
    wrong_nonhistory_disposition["graphEdges"][0]["disposition"] = "resolved"
    try:
        validate_result(wrong_nonhistory_disposition)
    except QualifiedDifferentialError:
        pass
    else:
        raise AssertionError("resolved disposition was accepted on identity edge")
    modeler_record = {
        "fields": [{
            "name": "historyHandle", "type": "handle", "value": "2",
        }],
    }
    modeler_nodes = {
        "node:handle:2": {"kind": "handle", "handle": "2"},
        "node:handle:3": {"kind": "handle", "handle": "3"},
    }
    history_edge = {
        "kind": "historyReference", "from": "node:record:1",
        "to": "node:handle:2", "disposition": "resolved",
    }
    _validate_modeler_history_reference(
        modeler_record, [history_edge], modeler_nodes, {"2"},
        "self-test.modeler-resolved",
    )
    unresolved_edge = {**history_edge, "disposition": "unresolved"}
    _validate_modeler_history_reference(
        modeler_record, [unresolved_edge], modeler_nodes, set(),
        "self-test.modeler-unresolved",
    )
    _validate_modeler_history_reference(
        {"fields": [{"name": "historyHandle", "type": "handle",
                     "value": "0"}]},
        [], modeler_nodes, set(), "self-test.modeler-null-history",
    )
    for malformed_edges, label in (
        ([], "missing"),
        ([history_edge, history_edge], "duplicate"),
        ([{**history_edge, "to": "node:handle:3"}], "wrong-target"),
        ([unresolved_edge], "wrong-resolution"),
    ):
        try:
            _validate_modeler_history_reference(
                modeler_record, malformed_edges, modeler_nodes, {"2"},
                f"self-test.modeler-{label}",
            )
        except QualifiedDifferentialError:
            pass
        else:
            raise AssertionError(
                f"{label} modeler history-reference edge was accepted"
            )
    try:
        _validate_modeler_history_reference(
            {"fields": [{"name": "historyHandle", "type": "handle",
                         "value": "0"}]},
            [history_edge], modeler_nodes, {"2"},
            "self-test.modeler-null-history-edge",
        )
    except QualifiedDifferentialError:
        pass
    else:
        raise AssertionError("history edge for a null modeler target was accepted")
    owner_record = {"ownerHandle": "2"}
    owner_edge = {
        "kind": "owner", "from": "node:record:1",
        "to": "node:handle:2", "disposition": "resolved",
    }
    owner_nodes = {
        **modeler_nodes,
        "node:handle:0": {"kind": "handle", "handle": "0"},
    }
    _validate_owner_reference(
        owner_record, [owner_edge], owner_nodes, {"2"},
        "self-test.owner-resolved",
    )
    _validate_owner_reference(
        {"ownerHandle": "3"},
        [{**owner_edge, "to": "node:handle:3", "disposition": "unresolved"}],
        owner_nodes, {"2"}, "self-test.owner-unresolved",
    )
    _validate_owner_reference(
        {"ownerHandle": None}, [], owner_nodes, {"2"},
        "self-test.owner-absent",
    )
    _validate_owner_reference(
        {"ownerHandle": "0"}, [], owner_nodes, {"2"},
        "self-test.owner-zero-null",
    )
    try:
        _validate_owner_reference(
            {"ownerHandle": "0"}, [owner_edge], owner_nodes, {"2"},
            "self-test.owner-zero-edge",
        )
    except QualifiedDifferentialError:
        pass
    else:
        raise AssertionError("owner edge for a zero ownerHandle was accepted")
    block_owner_nodes = {
        **owner_nodes,
        "node:block:4": {"kind": "block", "handle": "4"},
    }
    _validate_owner_reference(
        {"ownerHandle": "4"},
        [{**owner_edge, "to": "node:block:4", "disposition": "resolved"}],
        block_owner_nodes, {"4"}, "self-test.owner-block-target",
    )
    for malformed_edges, label in (
        ([], "missing"),
        ([owner_edge, owner_edge], "duplicate"),
        ([{**owner_edge, "to": "node:handle:3"}], "wrong-target"),
        ([{**owner_edge, "disposition": "unresolved"}], "stale-disposition"),
        ([{**owner_edge, "to": "node:handle:0"}], "zero-owner-edge"),
    ):
        try:
            _validate_owner_reference(
                owner_record, malformed_edges, owner_nodes, {"2"},
                f"self-test.owner-{label}",
            )
        except QualifiedDifferentialError:
            pass
        else:
            raise AssertionError(f"{label} owner edge was accepted")
    try:
        _validate_owner_reference(
            {"ownerHandle": None}, [owner_edge], owner_nodes, {"2"},
            "self-test.owner-edge-without-handle",
        )
    except QualifiedDifferentialError:
        pass
    else:
        raise AssertionError("owner edge without ownerHandle was accepted")
    identity_plus_reference = copy.deepcopy(target)
    identity_plus_reference["graphNodes"].append({
        "id": "node:handle:2", "kind": "handle", "recordId": None,
        "handle": "2",
    })
    identity_plus_reference["graphEdges"].append({
        "id": "edge:handle:2", "kind": "handleReference",
        "from": "node:record:1", "to": "node:handle:2",
        "disposition": "observed",
    })
    identity_plus_reference["cardinality"]["graphNodeCount"] += 1
    identity_plus_reference["cardinality"]["graphEdgeCount"] += 1
    validate_result(identity_plus_reference)

    reference_without_source_handle = copy.deepcopy(target)
    reference_without_source_handle["records"][0]["sourceHandle"] = None
    reference_without_source_handle["graphNodes"] = [
        row for row in reference_without_source_handle["graphNodes"]
        if row["id"] != "node:handle:1"
    ]
    record_node = next(
        row for row in reference_without_source_handle["graphNodes"]
        if row["id"] == "node:record:1"
    )
    record_node["handle"] = None
    reference_without_source_handle["graphEdges"] = [
        row for row in reference_without_source_handle["graphEdges"]
        if row["id"] != "edge:handle:1"
    ]
    reference_without_source_handle["graphNodes"].append({
        "id": "node:handle:2", "kind": "handle", "recordId": None,
        "handle": "2",
    })
    reference_without_source_handle["graphEdges"].append({
        "id": "edge:handle:2", "kind": "handleReference",
        "from": "node:record:1", "to": "node:handle:2",
        "disposition": "observed",
    })
    reference_without_source_handle["opaqueCarriers"][0]["sourceHandle"] = None
    validate_result(reference_without_source_handle)
    empty_rules = {"tolerances": [], "reviewedTargetDebt": [], "exclusions": []}
    manifest_v3 = _read_json(
        Path(__file__).resolve().parents[1]
        / "metadata/qualified-differential-v2.json"
    )
    validate_manifest(manifest_v3)
    if _manifest_adapter_result_schema(manifest_v3) != ADAPTER_RESULT_SCHEMA_V3:
        raise AssertionError("checked-in manifest does not select adapter schema 3")
    manifest_v2 = copy.deepcopy(manifest_v3)
    manifest_v2.pop("adapterResultSchema")
    manifest_v2.pop("normalizationSchema")
    manifest_v2["schema"] = MANIFEST_SCHEMA_V2
    validate_manifest(manifest_v2)
    validate_manifest(manifest_v3)
    assert _manifest_adapter_result_schema(manifest_v2) == ADAPTER_RESULT_SCHEMA_V2
    assert _manifest_adapter_result_schema(manifest_v3) == ADAPTER_RESULT_SCHEMA_V3
    unsupported_report_schema = copy.deepcopy(manifest_v3)
    unsupported_report_schema["normalizationSchema"] = NORMALIZATION_REPORT_SCHEMA + 1
    try:
        validate_manifest(unsupported_report_schema)
    except QualifiedDifferentialError:
        pass
    else:
        raise AssertionError("unsupported normalization schema was accepted")
    target_lock = {
        "path": "metadata/libdxfrw-target-lock.json",
        "digest": _digest_text("lock"),
        "targetCommit": "512d8bd86612f17158d4d222fcdc8a7c2b052c57",
        "snapshotRevision": "89b762bef636c90eb370cb1af3cec80fe759cb32",
        "archiveDigest": _digest_text("archive"),
        "manifestDigest": _digest_text("source-manifest"),
        "manifestEntries": 86,
    }
    target_v3 = copy.deepcopy(target)
    standalone_v3 = copy.deepcopy(standalone)
    for document in (target_v3, standalone_v3):
        document["schema"] = ADAPTER_RESULT_SCHEMA_V3
        document["input"]["detectedFormat"] = "dwg"
        document["input"]["detectedVersion"] = "AC1024"
        document["invocation"]["facade"] = "dwgRW"
        document["observations"] = _base_observations("dwgRW", "read")
    validate_result(target_v3)
    validate_result(standalone_v3)
    dxf_v3 = copy.deepcopy(target)
    dxf_v3["schema"] = ADAPTER_RESULT_SCHEMA_V3
    dxf_v3["observations"] = _base_observations("dxfRW", "read")
    validate_result(dxf_v3)
    for envelope_key, wrong_phase, expected_error in (
        ("inputRead", "outputReadback",
         "observations.inputRead phase does not match its role"),
        ("outputReadback", "inputRead",
         "observations.outputReadback phase does not match its role"),
    ):
        swapped_phase = copy.deepcopy(dxf_v3)
        swapped_phase["observations"][envelope_key]["phase"] = wrong_phase
        _assert_rejected(swapped_phase, expected_error)
    for field in ("phase", "availability"):
        for malformed_value in (None, [], {}):
            malformed_observation = copy.deepcopy(dxf_v3)
            malformed_observation["observations"]["inputRead"][field] = malformed_value
            _assert_rejected(
                malformed_observation,
                f"observations.inputRead.{field} is invalid",
            )
    for envelope_key in ("inputRead", "outputReadback"):
        fabricated_not_applicable_version = copy.deepcopy(dxf_v3)
        fabricated_not_applicable_version["observations"][envelope_key][
            "sourceVersion"
        ] = "AC1024"
        _assert_rejected(
            fabricated_not_applicable_version,
            f"observations.{envelope_key} notApplicable observations cannot claim a source version",
        )
    write_v3 = copy.deepcopy(target_v3)
    write_v3["input"]["detectedFormat"] = "recipe-json"
    write_v3["invocation"]["direction"] = "write"
    write_v3["observations"] = _base_observations("dwgRW", "write")
    synthetic_write_result = {
        "entity": "WRITE_RESULT",
        "recordClass": "WRITE_RESULT",
        "fields": [{"name": "version", "type": "string", "value": "AC1024"}],
    }
    _validate_write_result_readback_version(
        write_v3["invocation"], write_v3["status"],
        write_v3["observations"], [synthetic_write_result],
    )
    generated_dwg_identity = {
        "detectedFormat": "dwg", "detectedVersion": "AC1024",
    }
    _validate_output_readback_version(write_v3, generated_dwg_identity, "write")
    mismatched_readback_version = copy.deepcopy(write_v3)
    mismatched_readback_version["observations"]["outputReadback"][
        "sourceVersion"
    ] = "AC1021"
    try:
        _validate_output_readback_version(
            mismatched_readback_version, generated_dwg_identity, "write"
        )
    except QualifiedDifferentialError as exc:
        if "outputReadback sourceVersion differs from generated artifact" not in str(exc):
            raise
    else:
        raise AssertionError("DWG readback accepted a version unlike its artifact")
    try:
        _validate_write_result_readback_version(
            mismatched_readback_version["invocation"],
            mismatched_readback_version["status"],
            mismatched_readback_version["observations"],
            [synthetic_write_result],
        )
    except QualifiedDifferentialError as exc:
        if "outputReadback sourceVersion differs from WRITE_RESULT version" not in str(exc):
            raise
    else:
        raise AssertionError("DWG readback accepted a version unlike WRITE_RESULT")
    write_not_run = copy.deepcopy(write_v3)
    write_not_run["status"]["firstFailure"] = {
        "stage": "write", "code": 2, "path": "/output",
        "diagnosticId": None,
    }
    write_not_run["observations"]["outputReadback"] = _base_observation_envelope(
        "outputReadback", "notRun",
        reason="writeFailedBeforeReadback",
    )
    _validate_observations(write_not_run["observations"], write_not_run)
    fabricated_not_run_version = copy.deepcopy(write_not_run)
    fabricated_not_run_version["observations"]["outputReadback"][
        "sourceVersion"
    ] = "AC1024"
    try:
        _validate_observation_envelope(
            fabricated_not_run_version["observations"]["outputReadback"],
            "observations.outputReadback",
        )
    except QualifiedDifferentialError as exc:
        if "notRun observations cannot claim a source version" not in str(exc):
            raise
    else:
        raise AssertionError("notRun readback accepted a fabricated source version")
    clean_observation_report = compare_results(
        target_v3, standalone_v3, empty_rules,
        manifest_digest=config_digest, target_lock=target_lock,
    )
    assert clean_observation_report["schema"] == NORMALIZATION_REPORT_SCHEMA
    assert clean_observation_report["adapterResultSchema"] == ADAPTER_RESULT_SCHEMA_V3
    assert clean_observation_report["claimEligible"] is True

    partial_target = copy.deepcopy(target_v3)
    partial_peer = copy.deepcopy(standalone_v3)
    for document in (partial_target, partial_peer):
        envelope = document["observations"]["inputRead"]
        envelope["scalarCounters"]["entityParseFailures"] = 1
        envelope["frames"].update({
            "status": "finalizedPartial",
            "sourceFrameCount": 1,
            "dispositions": {
                key: int(key == "failed")
                for key in sorted(FRAME_DISPOSITION_KEYS)
            },
            "unrecognizedDispositionCount": 0,
        })
    partial_report = compare_results(
        partial_target, partial_peer, empty_rules,
        manifest_digest=config_digest, target_lock=target_lock,
    )
    assert partial_report["status"] == "reviewedNonPromoting"
    assert partial_report["claimEligible"] is False
    assert partial_report["targetAdapter"]["side"] == "target"
    assert partial_target["status"]["operationSucceeded"] is True
    assert "target.observations.inputRead.partial:entityParseFailures" in partial_report["eligibilityBlockers"]
    assert "target.observations.inputRead.frameCoverage:finalizedPartial" in partial_report["eligibilityBlockers"]

    for crc_counter in (
        "classesCrcMismatch", "reader21CrcMismatch",
        "reader18HeaderCrcMismatch",
    ):
        crc_target = copy.deepcopy(target_v3)
        crc_peer = copy.deepcopy(standalone_v3)
        for document in (crc_target, crc_peer):
            document["observations"]["inputRead"]["scalarCounters"][
                crc_counter
            ] = 1
        crc_report = compare_results(
            crc_target, crc_peer, empty_rules,
            manifest_digest=config_digest, target_lock=target_lock,
        )
        assert crc_report["status"] == "reviewedNonPromoting"
        assert crc_report["claimEligible"] is False
        assert crc_target["status"]["operationSucceeded"] is True
        assert (
            "target.observations.inputRead.crcMismatch:" + crc_counter
            in crc_report["eligibilityBlockers"]
        )
        assert (
            "standalone.observations.inputRead.crcMismatch:" + crc_counter
            in crc_report["eligibilityBlockers"]
        )

    opaque_target = copy.deepcopy(target_v3)
    opaque_peer = copy.deepcopy(standalone_v3)
    opaque_summary_row = {
        "bytesHex": "414341445f50524f5859",
        "prefixLength": 10,
        "sourceLength": 10,
        "truncated": False,
        "instances": 1,
    }
    for document in (opaque_target, opaque_peer):
        summary = document["observations"]["inputRead"][
            "namedSummaries"]["skippedUnsupportedObjects"]
        summary.update({
            "rows": [copy.deepcopy(opaque_summary_row)],
            "retainedRowCount": 1,
            "sourceNameCount": 1,
            "totalInstanceCount": 1,
        })
    opaque_report = compare_results(
        opaque_target, opaque_peer, empty_rules,
        manifest_digest=config_digest, target_lock=target_lock,
    )
    assert opaque_report["status"] == "reviewedNonPromoting"
    assert opaque_report["claimEligible"] is False
    assert "target.observations.inputRead.skipped:skippedUnsupportedObjects" in opaque_report["eligibilityBlockers"]

    missing_observation_key = copy.deepcopy(target_v3)
    del missing_observation_key["observations"]["inputRead"]["scalarCounters"][
        "entityParseFailures"
    ]
    _assert_rejected(missing_observation_key, "keys differ")
    extra_observation_key = copy.deepcopy(target_v3)
    extra_observation_key["observations"]["inputRead"]["unexpected"] = 0
    _assert_rejected(extra_observation_key, "keys differ")
    for bad_count in (None, True, -1, 2 ** 64):
        changed = copy.deepcopy(target_v3)
        changed["observations"]["inputRead"]["scalarCounters"][
            "entityParseFailures"
        ] = bad_count
        try:
            validate_result(changed)
        except QualifiedDifferentialError:
            pass
        else:
            raise AssertionError(f"invalid observation count was accepted: {bad_count!r}")

    # Exercise the uint64 domain across every schema-3 observation count
    # family, not just scalarCounters.  Keep at least one row in each bounded
    # collection so row-local counts and byte-prefix lengths are covered too.
    complete_observation = copy.deepcopy(target_v3)
    observation = complete_observation["observations"]["inputRead"]
    named_row = {
        "bytesHex": "4142", "prefixLength": 2, "sourceLength": 2,
        "truncated": False, "instances": 1,
    }
    for summary in observation["namedSummaries"].values():
        summary.update({
            "rows": [copy.deepcopy(named_row)],
            "retainedRowCount": 1,
            "sourceNameCount": 1,
            "totalInstanceCount": 1,
        })
    integrity_row = {
        "versionCode": 1, "severityCode": 1, "offsetSpaceCode": 0,
        "phaseCode": 1, "kindCode": 1, "logicalSectionId": 0,
        "sectionDescriptorId": 0,
        "sectionName": {
            "bytesHex": "534543", "prefixLength": 3,
            "sourceLength": 3, "truncated": False,
        },
        "pageId": 1, "fileOffset": 2, "logicalHandle": 3,
        "expected": 4, "observed": 5, "hasPageId": True,
        "hasFileOffset": True, "hasLogicalHandle": True,
        "hasExpected": True, "hasObserved": True,
    }
    observation["integrity"].update({
        "rows": [integrity_row], "retainedRowCount": 1,
        "sourceRecordedCount": 1,
    })
    validate_result(complete_observation)

    uint64_paths: list[tuple[Any, ...]] = [
        ("scalarCounters", key)
        for key in sorted(OBSERVATION_COUNTER_KEYS)
    ]
    for name, summary in observation["namedSummaries"].items():
        uint64_paths.extend(("namedSummaries", name, key) for key in (
            "retainedRowCount", "sourceNameCount", "totalInstanceCount",
            "omittedNameCount", "omittedInstanceCount",
            "truncatedRetainedNameCount", "omittedNameBytes",
        ))
        uint64_paths.extend(("namedSummaries", name, "rows", 0, key)
                            for key in ("prefixLength", "sourceLength", "instances"))
    uint64_paths.extend(("integrity", key) for key in (
        "retainedRowCount", "sourceRecordedCount", "extractionOmittedCount",
        "reportedUpstreamDroppedLowerBound", "truncatedRetainedNameCount",
        "omittedNameBytes",
    ))
    uint64_paths.extend(("integrity", "rows", 0, "sectionName", key)
                        for key in ("prefixLength", "sourceLength"))
    uint64_paths.extend(("integrity", "rows", 0, key)
                        for key in ("pageId", "fileOffset", "expected", "observed"))
    uint64_paths.append(("integrity", "rows", 0, "logicalHandle"))
    uint64_paths.extend(("frames", "dispositions", key)
                        for key in sorted(FRAME_DISPOSITION_KEYS))
    uint64_paths.extend(("frames", key) for key in (
        "sourceFrameCount", "unrecognizedDispositionCount",
    ))

    for path in uint64_paths:
        for invalid_count in (True, -1, 2 ** 64):
            changed = copy.deepcopy(complete_observation)
            target_value = changed["observations"]["inputRead"]
            for key in path[:-1]:
                target_value = target_value[key]
            target_value[path[-1]] = invalid_count
            try:
                validate_result(changed)
            except QualifiedDifferentialError:
                pass
            else:
                raise AssertionError(
                    "invalid schema-3 observation count was accepted: "
                    f"path={path!r}, value={invalid_count!r}"
                )

    duplicate_prefixes = copy.deepcopy(target_v3)
    duplicate_summary = duplicate_prefixes["observations"]["inputRead"][
        "namedSummaries"
    ]["skippedCustomClasses"]
    duplicate_row = {
        "bytesHex": "4142", "prefixLength": 2, "sourceLength": 2,
        "truncated": False, "instances": 1,
    }
    duplicate_summary.update({
        "rows": [copy.deepcopy(duplicate_row), copy.deepcopy(duplicate_row)],
        "retainedRowCount": 2, "sourceNameCount": 2,
        "totalInstanceCount": 2,
    })
    validate_result(duplicate_prefixes)

    omitted_and_saturated = copy.deepcopy(target_v3)
    omitted_summary = omitted_and_saturated["observations"]["inputRead"][
        "namedSummaries"
    ]["skippedCustomClasses"]
    omitted_summary.update({
        "sourceNameCount": 1, "omittedNameCount": 1,
        "omittedInstanceCount": 1, "omittedNameBytes": 64,
        "aggregateSaturated": True,
    })
    validate_result(omitted_and_saturated)
    omitted_report = compare_results(
        omitted_and_saturated, standalone_v3, empty_rules,
        manifest_digest=config_digest, target_lock=target_lock,
    )
    assert omitted_report["claimEligible"] is False
    assert any("incompleteNames" in blocker for blocker in omitted_report["eligibilityBlockers"])

    unavailable_pair = copy.deepcopy(target_v3)
    unavailable_envelope = _base_observation_envelope(
        "inputRead", "unavailable", source_version="AC1024",
        reason="readerApiNotPresent",
    )
    unavailable_pair["observations"]["inputRead"] = copy.deepcopy(unavailable_envelope)
    unavailable_peer = copy.deepcopy(standalone_v3)
    unavailable_peer["observations"]["inputRead"] = copy.deepcopy(unavailable_envelope)
    unavailable_report = compare_results(
        unavailable_pair, unavailable_peer, empty_rules,
        manifest_digest=config_digest, target_lock=target_lock,
    )
    assert unavailable_report["claimEligible"] is False
    assert "target.observations.inputRead.unavailable" in unavailable_report["eligibilityBlockers"]
    zero_instead_of_unknown = copy.deepcopy(unavailable_pair)
    zero_instead_of_unknown["observations"]["inputRead"]["scalarCounters"] = {
        key: 0 for key in sorted(OBSERVATION_COUNTER_KEYS)
    }
    _assert_rejected(zero_instead_of_unknown, "non-available observations")

    changed_observation_counter = copy.deepcopy(standalone_v3)
    changed_observation_counter["observations"]["inputRead"]["scalarCounters"][
        "decodedProxyPrimitives"
    ] = 1
    changed_counter_report = compare_results(
        target_v3, changed_observation_counter, empty_rules,
        manifest_digest=config_digest, target_lock=target_lock,
    )
    assert changed_counter_report["status"] == "mismatch"
    assert changed_counter_report["claimEligible"] is False

    for key, replacement in (("phase", "outputReadback"), ("sourceVersion", "AC1021")):
        changed = copy.deepcopy(target_v3)
        changed["observations"]["inputRead"][key] = replacement
        try:
            validate_result(changed)
        except QualifiedDifferentialError:
            pass
        else:
            raise AssertionError(f"DWG observation {key} mixup was accepted")

    unknown_frames = copy.deepcopy(target_v3)
    unknown_frame_row = unknown_frames["observations"]["inputRead"]["frames"]
    unknown_frame_row.update({
        "status": "notAvailable", "reason": "coverageNotPublished",
        "sourceFrameCount": None,
        "dispositions": {key: None for key in sorted(FRAME_DISPOSITION_KEYS)},
        "unrecognizedDispositionCount": None,
    })
    validate_result(unknown_frames)
    all_zero_unknown_frames = copy.deepcopy(unknown_frames)
    all_zero_unknown_row = all_zero_unknown_frames["observations"]["inputRead"][
        "frames"
    ]
    all_zero_unknown_row.update({
        "sourceFrameCount": 0,
        "dispositions": {key: 0 for key in sorted(FRAME_DISPOSITION_KEYS)},
        "unrecognizedDispositionCount": 0,
    })
    _assert_rejected(
        all_zero_unknown_frames,
        "unknown frame totals need a reason and null count",
    )
    unknown_frame_peer = copy.deepcopy(standalone_v3)
    unknown_frame_peer["observations"]["inputRead"]["frames"] = copy.deepcopy(
        unknown_frame_row
    )
    unknown_frame_report = compare_results(
        unknown_frames, unknown_frame_peer, empty_rules,
        manifest_digest=config_digest, target_lock=target_lock,
    )
    assert unknown_frame_report["claimEligible"] is False
    assert any("frameCoverage:notAvailable" in blocker
               for blocker in unknown_frame_report["eligibilityBlockers"])

    synthetic_scope = {
        "inputId": "synthetic-line",
        "inputPathHint": "local/synthetic-line-v1",
        "inputSha256": _digest_text("0\nEOF\n"),
        "facade": "dxfRW",
        "direction": "read",
        "optionsDigest": target["invocation"]["optionsDigest"],
    }
    pair_target = {
        "build": {
            "buildParityDigest": _digest_text("parity"),
            "buildParityIdentity": {
                "schema": 1, "configuration": "Release",
                "cmake": {
                    "compilerBuiltinSpecs": copy.deepcopy(gnu_profile_v2),
                },
                "compilerResources": copy.deepcopy(compiler_resource_profile),
                "appleSdk": copy.deepcopy(apple_sdk_profile),
                "appleRuntime": copy.deepcopy(apple_runtime_profile),
                "appleLinkInputs": copy.deepcopy(apple_link_inputs_profile),
            },
            "configIdentity": {
                "inputs": {
                    "verifiedInterfaceContractDigest": EXPECTED_COMMON_INTERFACE_DIGEST,
                    "interfaceMethodCount": EXPECTED_INTERFACE_METHOD_COUNT,
                }
            },
        }
    }
    pair_standalone = copy.deepcopy(pair_target)
    pair_standalone["build"]["buildParityIdentity"]["appleRuntime"][
        "executableDigest"
    ] = _digest_text("standalone executable")
    standalone_link_inputs = pair_standalone["build"]["buildParityIdentity"][
        "appleLinkInputs"
    ]
    standalone_link_inputs["executableDigest"] = _digest_text(
        "standalone executable"
    )
    standalone_link_inputs["mapDigest"] = _digest_text("standalone raw map")
    standalone_link_inputs["linkedArchive"]["digest"] = _digest_text(
        "standalone archive"
    )
    standalone_link_inputs["archiveMembers"] = ["other.cpp.o"]
    standalone_link_inputs["archiveMembersDigest"] = hashlib.sha256(
        _canonical_bytes(standalone_link_inputs["archiveMembers"]) + b"\n"
    ).hexdigest()
    _validate_receipt_pair(pair_target, pair_standalone)
    support_pair_target = copy.deepcopy(pair_target)
    support_pair_standalone = copy.deepcopy(pair_standalone)
    support_pair_target["build"]["configIdentity"]["inputs"][
        "adapterSupportInputs"
    ] = [{
        "path": "tests/semantic_adapter_support.h",
        "snapshotPath": "semantic_adapter_support.h",
        "digest": _digest_text("shared adapter support"),
    }]
    support_pair_standalone["build"]["configIdentity"]["inputs"][
        "adapterSupportInputs"
    ] = [{
        "path": "tests/semantic_adapter_support.h",
        "snapshotPath": "tests/semantic_adapter_support.h",
        "digest": _digest_text("shared adapter support"),
    }]
    _validate_receipt_pair(support_pair_target, support_pair_standalone)
    changed_support_pair = copy.deepcopy(support_pair_standalone)
    changed_support_pair["build"]["configIdentity"]["inputs"][
        "adapterSupportInputs"][0]["digest"] = _digest_text(
            "different adapter support"
        )
    try:
        _validate_receipt_pair(support_pair_target, changed_support_pair)
    except QualifiedDifferentialError as exc:
        assert "adapter support-input content differs" in str(exc)
    else:
        raise AssertionError("target/standalone support-input mismatch was accepted")
    asymmetric_support_pair = copy.deepcopy(pair_standalone)
    asymmetric_support_pair["build"]["configIdentity"]["inputs"][
        "adapterSupportInputs"
    ] = []
    try:
        _validate_receipt_pair(pair_target, asymmetric_support_pair)
    except QualifiedDifferentialError as exc:
        assert "adapter support-input evidence is asymmetric" in str(exc)
    else:
        raise AssertionError("asymmetric support-input evidence was accepted")
    changed_selected_input_pair = copy.deepcopy(pair_standalone)
    selected_inputs = changed_selected_input_pair["build"]["buildParityIdentity"][
        "appleLinkInputs"]["externalInputs"]
    selected_inputs[0]["digest"] = _digest_text("different selected linker stub")
    changed_selected_input_pair["build"]["buildParityIdentity"][
        "appleLinkInputs"]["externalInputsDigest"] = hashlib.sha256(
            _canonical_bytes(selected_inputs) + b"\n"
        ).hexdigest()
    try:
        _validate_receipt_pair(pair_target, changed_selected_input_pair)
    except QualifiedDifferentialError:
        pass
    else:
        raise AssertionError("target/standalone selected linker-input mismatch was accepted")
    changed_resource_pair = copy.deepcopy(pair_standalone)
    changed_resource_pair["build"]["buildParityIdentity"][
        "compilerResources"]["treeDigest"] = _digest_text("different-resource-tree")
    try:
        _validate_receipt_pair(pair_target, changed_resource_pair)
    except QualifiedDifferentialError:
        pass
    else:
        raise AssertionError("target/standalone compiler resource mismatch was accepted")
    changed_builtin_specs_pair = copy.deepcopy(pair_standalone)
    changed_builtin_specs_pair["build"]["buildParityIdentity"]["cmake"] = {
        "compilerBuiltinSpecs": {
            **gnu_profile_v2,
            "digest": _digest_text("different GCC built-in specs"),
        },
    }
    try:
        _validate_receipt_pair(pair_target, changed_builtin_specs_pair)
    except QualifiedDifferentialError:
        pass
    else:
        raise AssertionError(
            "target/standalone GCC built-in specs mismatch was accepted"
        )
    changed_apple_sdk_pair = copy.deepcopy(pair_standalone)
    changed_apple_sdk_pair["build"]["buildParityIdentity"]["appleSdk"] = (
        copy.deepcopy(apple_sdk_profile)
    )
    changed_apple_sdk_pair["build"]["buildParityIdentity"]["appleSdk"][
        "roots"][0]["metadataFiles"][0]["digest"] = _digest_text(
            "different-sdk-settings"
        )
    changed_apple_sdk_pair["build"]["buildParityIdentity"]["appleSdk"][
        "roots"][0]["metadataDigest"] = hashlib.sha256(
            _canonical_bytes(changed_apple_sdk_pair["build"]["buildParityIdentity"][
                "appleSdk"]["roots"][0]["metadataFiles"]) + b"\n"
        ).hexdigest()
    changed_apple_sdk_pair["build"]["buildParityIdentity"]["appleSdk"][
        "rootsDigest"] = hashlib.sha256(
            _canonical_bytes(changed_apple_sdk_pair["build"]["buildParityIdentity"][
                "appleSdk"]["roots"]) + b"\n"
        ).hexdigest()
    try:
        _validate_receipt_pair(pair_target, changed_apple_sdk_pair)
    except QualifiedDifferentialError:
        pass
    else:
        raise AssertionError("target/standalone Apple SDK mismatch was accepted")
    changed_runtime_pair = copy.deepcopy(pair_standalone)
    runtime_nodes = changed_runtime_pair["build"]["buildParityIdentity"][
        "appleRuntime"]["nodes"]
    runtime_nodes[0]["images"][0]["uuid"] = (
        "B21743EE-6E1C-3E74-97EA-7AF793DE4BDF"
    )
    changed_runtime_pair["build"]["buildParityIdentity"][
        "appleRuntime"]["nodesDigest"] = hashlib.sha256(
            _canonical_bytes(runtime_nodes) + b"\n"
        ).hexdigest()
    try:
        _validate_receipt_pair(pair_target, changed_runtime_pair)
    except QualifiedDifferentialError:
        pass
    else:
        raise AssertionError("target/standalone runtime UUID mismatch was accepted")
    target_dependency_pair = copy.deepcopy(pair_target)
    standalone_dependency_pair = copy.deepcopy(pair_standalone)
    target_dependency_pair["build"]["buildParityIdentity"][
        "compilerDependencies"
    ] = {
        **dependency_profile,
        "externalInputs": dependency_inputs + [{
            "path": target_only_header_path,
            "size": 8,
            "digest": _digest_text("target-only"),
        }],
    }
    target_dependency_pair["build"]["buildParityIdentity"][
        "compilerDependencies"]["externalInputs"] = sorted(
            target_dependency_pair["build"]["buildParityIdentity"][
                "compilerDependencies"]["externalInputs"],
            key=lambda row: row["path"],
        )
    target_dependency_inputs = target_dependency_pair["build"][
        "buildParityIdentity"]["compilerDependencies"]["externalInputs"]
    target_dependency_pair["build"]["buildParityIdentity"][
        "compilerDependencies"]["externalInputsDigest"] = hashlib.sha256(
            _canonical_bytes(target_dependency_inputs) + b"\n"
        ).hexdigest()
    standalone_dependency_pair["build"]["buildParityIdentity"][
        "compilerDependencies"
    ] = {
        **dependency_profile,
        "externalInputs": dependency_inputs + [{
            "path": standalone_only_header_path,
            "size": 13,
            "digest": _digest_text("standalone-only"),
        }],
    }
    standalone_dependency_pair["build"]["buildParityIdentity"][
        "compilerDependencies"]["externalInputs"] = sorted(
            standalone_dependency_pair["build"]["buildParityIdentity"][
                "compilerDependencies"]["externalInputs"],
            key=lambda row: row["path"],
        )
    standalone_dependency_inputs = standalone_dependency_pair["build"][
        "buildParityIdentity"]["compilerDependencies"]["externalInputs"]
    standalone_dependency_pair["build"]["buildParityIdentity"][
        "compilerDependencies"]["externalInputsDigest"] = hashlib.sha256(
            _canonical_bytes(standalone_dependency_inputs) + b"\n"
        ).hexdigest()
    _validate_receipt_pair(
        target_dependency_pair, standalone_dependency_pair
    )
    changed_shared_dependency = copy.deepcopy(standalone_dependency_pair)
    shared = next(
        row for row in changed_shared_dependency["build"]["buildParityIdentity"][
            "compilerDependencies"]["externalInputs"]
        if row["path"] == standard_header_path
    )
    shared["digest"] = _digest_text("changed-shared-header")
    changed_dependency_inputs = changed_shared_dependency["build"][
        "buildParityIdentity"]["compilerDependencies"]["externalInputs"]
    changed_shared_dependency["build"]["buildParityIdentity"][
        "compilerDependencies"]["externalInputsDigest"] = hashlib.sha256(
            _canonical_bytes(changed_dependency_inputs) + b"\n"
        ).hexdigest()
    try:
        _validate_receipt_pair(
            target_dependency_pair, changed_shared_dependency
        )
    except QualifiedDifferentialError:
        pass
    else:
        raise AssertionError(
            "target/standalone shared compiler dependency change was accepted"
        )
    direct_only_target = copy.deepcopy(pair_target)
    direct_only_standalone = copy.deepcopy(pair_standalone)
    for direct_only in (direct_only_target, direct_only_standalone):
        inputs = direct_only["build"]["configIdentity"]["inputs"]
        inputs["compileContractMode"] = "direct-options-only"
        inputs["compileOptionAdmissionEnforced"] = True
    direct_only_target["build"]["configIdentity"]["inputs"][
        "driverContext"
    ] = controlled_context
    controlled_input_shape = {
        key: None for key in CONTROLLED_CONTEXT_CONFIG_INPUT_KEYS
    }
    controlled_input_shape["driverContext"] = controlled_context
    _exact_keys(
        controlled_input_shape,
        CONTROLLED_CONTEXT_CONFIG_INPUT_KEYS,
        "self-test.controlled-context-inputs",
    )
    try:
        _validate_receipt_pair(direct_only_target, direct_only_standalone)
    except QualifiedDifferentialError as exc:
        assert "does not attest the effective compiler driver" in str(exc)
    else:
        raise AssertionError("direct-option-only receipt pair was eligible")
    advisory_target = copy.deepcopy(pair_target)
    advisory_standalone = copy.deepcopy(pair_standalone)
    for advisory in (advisory_target, advisory_standalone):
        inputs = advisory["build"]["configIdentity"]["inputs"]
        inputs.update({
            "compileContractMode": "controlled-driver-profile-v1",
            "compileOptionAdmissionEnforced": True,
            "compileContractEnforced": True,
            "compileContractProfile": advisory_compile_profile,
        })
    try:
        _validate_receipt_pair(advisory_target, advisory_standalone)
    except QualifiedDifferentialError as exc:
        assert "profile is advisory or unavailable" in str(exc)
    else:
        raise AssertionError("advisory controlled-driver pair was eligible")
    enforced_target = copy.deepcopy(pair_target)
    enforced_standalone = copy.deepcopy(pair_standalone)
    for pair, side, interface_profile in (
        (enforced_target, "target", target_interface_profile),
        (enforced_standalone, "standalone", standalone_interface_profile),
    ):
        inputs = pair["build"]["configIdentity"]["inputs"]
        inputs.update({
            "compileContractMode": "controlled-driver-profile-v1",
            "compileOptionAdmissionEnforced": True,
            "compileContractEnforced": True,
            "compileContractProfile": enforced_compile_profile,
            "driverContext": controlled_context,
            "interfaceProfile": interface_profile,
            "capabilityProfile": capability_profiles[side],
        })
    _validate_receipt_pair(enforced_target, enforced_standalone)
    missing_capability_pair = copy.deepcopy(enforced_target)
    missing_capability_pair["build"]["configIdentity"]["inputs"].pop(
        "capabilityProfile"
    )
    try:
        _validate_receipt_pair(missing_capability_pair, enforced_standalone)
    except QualifiedDifferentialError as exc:
        assert "capability profiles are asymmetric" in str(exc)
    else:
        raise AssertionError("asymmetric capability profiles were accepted")
    missing_both_capabilities = copy.deepcopy(enforced_target)
    missing_both_standalone = copy.deepcopy(enforced_standalone)
    missing_both_capabilities["build"]["configIdentity"]["inputs"].pop(
        "capabilityProfile"
    )
    missing_both_standalone["build"]["configIdentity"]["inputs"].pop(
        "capabilityProfile"
    )
    try:
        _validate_receipt_pair(
            missing_both_capabilities, missing_both_standalone
        )
    except QualifiedDifferentialError as exc:
        assert "omits the required semantic capability profile" in str(exc)
    else:
        raise AssertionError("controlled pair without capability profiles was eligible")
    changed_capability_pair = copy.deepcopy(enforced_standalone)
    changed_capability_pair["build"]["configIdentity"]["inputs"][
        "capabilityProfile"]["capabilities"][sorted(CAPABILITY_KEYS)[0]] = False
    try:
        _validate_receipt_pair(enforced_target, changed_capability_pair)
    except QualifiedDifferentialError as exc:
        assert "contradicts the reviewed standalone profile" in str(exc)
    else:
        raise AssertionError("contradictory standalone capabilities were eligible")
    uncaptured_target = copy.deepcopy(pair_target)
    uncaptured_standalone = copy.deepcopy(pair_standalone)
    uncaptured_profile = copy.deepcopy(standalone_interface_profile)
    uncaptured_profile["adapterSupplementalMethods"] = []
    uncaptured_profile["unobservedDefaultedExtras"] = (
        EXPECTED_STANDALONE_INTERFACE_EXTRAS
    )
    for pair, profile in (
        (uncaptured_target, target_interface_profile),
        (uncaptured_standalone, uncaptured_profile),
    ):
        inputs = pair["build"]["configIdentity"]["inputs"]
        inputs["verifiedInterfaceContractDigest"] = EXPECTED_COMMON_INTERFACE_DIGEST
        inputs["interfaceMethodCount"] = EXPECTED_INTERFACE_METHOD_COUNT
        inputs["interfaceProfile"] = profile
    try:
        _validate_receipt_pair(uncaptured_target, uncaptured_standalone)
    except QualifiedDifferentialError as exc:
        assert "supplementary callbacks are not captured" in str(exc)
    else:
        raise AssertionError("uncaptured standalone callbacks were eligible")
    pair_standalone["build"]["buildParityIdentity"]["configuration"] = "Debug"
    try:
        _validate_receipt_pair(pair_target, pair_standalone)
    except QualifiedDifferentialError as exc:
        assert "effective build profiles differ" in str(exc)
    else:
        raise AssertionError("mismatched build-parity identities were accepted")
    pair_standalone = copy.deepcopy(pair_target)
    pair_standalone["build"]["buildParityIdentity"]["configuration"] = "Release"
    pair_standalone["build"]["configIdentity"]["inputs"][
        "verifiedInterfaceContractDigest"
    ] = _digest_text("different-interface")
    try:
        _validate_receipt_pair(pair_target, pair_standalone)
    except QualifiedDifferentialError as exc:
        assert "interface callback inventories differ" in str(exc)
    else:
        raise AssertionError("mismatched interface callback inventories were accepted")
    exact = compare_results(
        target, standalone, empty_rules, manifest_digest=config_digest,
        target_lock=target_lock,
    )
    assert exact["status"] == "exact" and exact["claimEligible"]
    assert exact["outcomeCounts"]["mismatch"] == 0

    failed_target = copy.deepcopy(target)
    failed_standalone = copy.deepcopy(standalone)
    for failed in (failed_target, failed_standalone):
        failed["status"] = {
            "outcome": "failure",
            "operationSucceeded": False,
            "exitCode": 1,
            "firstFailure": {
                "stage": "entities",
                "code": 1,
                "path": "/input",
                "diagnosticId": "diagnostic:failure",
            },
        }
        failed["diagnostics"] = [{
            "id": "diagnostic:failure",
            "ordinal": 0,
            "severity": "error",
            "stage": "entities",
            "code": "read-error-1",
            "path": "/input",
            "messageDigest": _digest_text("synthetic failure"),
        }]
        failed["cardinality"]["diagnosticCount"] = 1
    try:
        compare_results(
            failed_target, failed_standalone, empty_rules,
            manifest_digest=config_digest, target_lock=target_lock,
        )
    except QualifiedDifferentialError as exc:
        assert "expected both adapter operations to succeed" in str(exc)
    else:
        raise AssertionError("identical operation failures qualified as success")
    failure_oracle = {
        "id": "failure:synthetic-read",
        "inputId": "synthetic-line",
        "inputPathHint": "local/synthetic-line-v1",
        "facade": "dxfRW",
        "direction": "read",
        "firstFailure": {
            "stage": "entities", "code": 1, "path": "/input",
        },
        "outputProbe": "none",
    }
    duplicate_oracle = copy.deepcopy(failure_oracle)
    duplicate_oracle["id"] = "failure:synthetic-read-duplicate"
    try:
        _validate_expected_failures(
            [failure_oracle, duplicate_oracle], []
        )
    except QualifiedDifferentialError as exc:
        assert "invocation tuples" in str(exc)
    else:
        raise AssertionError("duplicate expected-failure invocation was accepted")
    unregistered_write_oracle = copy.deepcopy(failure_oracle)
    unregistered_write_oracle.update({
        "id": "failure:unregistered-write",
        "direction": "write",
        "outputProbe": "existing-directory",
        "firstFailure": {"stage": "write", "code": 2, "path": "/output"},
    })
    try:
        _validate_expected_failures([unregistered_write_oracle], [])
    except QualifiedDifferentialError as exc:
        assert "local recipe" in str(exc)
    else:
        raise AssertionError("unregistered write-failure input was accepted")
    try:
        compare_results(
            failed_target, failed_standalone, empty_rules,
            manifest_digest=config_digest, target_lock=target_lock,
            expected_operation_succeeded=False,
        )
    except QualifiedDifferentialError as exc:
        assert "stage/code/path oracle" in str(exc)
    else:
        raise AssertionError("oracle-free expected failure was accepted")
    expected_failure_report = compare_results(
        failed_target, failed_standalone, empty_rules,
        manifest_digest=config_digest, target_lock=target_lock,
        expected_operation_succeeded=False,
        expected_failure=failure_oracle,
    )
    assert expected_failure_report["status"] == "reviewedNonPromoting"
    assert not expected_failure_report["claimEligible"]
    assert expected_failure_report["expectedFailure"] == failure_oracle
    assert "expectedOperationFailure" in expected_failure_report["eligibilityBlockers"]
    for key, wrong_value in (("stage", "objects"), ("code", 2)):
        wrong_oracle = copy.deepcopy(failure_oracle)
        wrong_oracle["firstFailure"][key] = wrong_value
        try:
            compare_results(
                failed_target, failed_standalone, empty_rules,
                manifest_digest=config_digest, target_lock=target_lock,
                expected_operation_succeeded=False,
                expected_failure=wrong_oracle,
            )
        except QualifiedDifferentialError as exc:
            assert f"first failure {key}" in str(exc)
        else:
            raise AssertionError(f"wrong expected failure {key} was accepted")
    wrong_path_oracle = copy.deepcopy(failure_oracle)
    wrong_path_oracle["firstFailure"]["path"] = "/output"
    try:
        compare_results(
            failed_target, failed_standalone, empty_rules,
            manifest_digest=config_digest, target_lock=target_lock,
            expected_operation_succeeded=False,
            expected_failure=wrong_path_oracle,
        )
    except QualifiedDifferentialError as exc:
        assert "path does not match its operation" in str(exc)
    else:
        raise AssertionError("wrong expected failure path was accepted")
    write_failure = copy.deepcopy(failed_target)
    write_failure["status"]["firstFailure"].update({
        "stage": "write", "code": 2, "path": "/output",
    })
    write_failure["diagnostics"][0].update({
        "stage": "write", "code": "write-error-2", "path": "/output",
    })
    write_failure["invocation"]["direction"] = "write"
    validate_result(write_failure)
    bad_write_failure = copy.deepcopy(write_failure)
    bad_write_failure["diagnostics"][0]["code"] = "read-error-2"
    _assert_rejected(bad_write_failure, "code does not match")
    bad_write_failure = copy.deepcopy(write_failure)
    bad_write_failure["status"]["firstFailure"]["path"] = "/input"
    bad_write_failure["diagnostics"][0]["path"] = "/input"
    _assert_rejected(bad_write_failure, "path does not match")
    for stage, code in (("section", 13), ("parseCode", 14)):
        staged_failure = copy.deepcopy(failed_target)
        staged_failure["status"]["firstFailure"].update({
            "stage": stage, "code": code,
        })
        staged_failure["diagnostics"][0].update({
            "stage": stage, "code": f"read-error-{code}",
        })
        validate_result(staged_failure)
    _validate_generated_artifact(
        failed_target, None, "read", expected_operation_succeeded=False
    )
    synthetic_artifact = {
        "sha256": _digest_text("partial"), "byteSize": 7,
        "detectedFormat": "dxf-ascii", "detectedVersion": "AC1009",
    }
    _validate_generated_artifact(
        write_failure, None, "write", expected_operation_succeeded=False
    )
    try:
        _validate_generated_artifact(
            write_failure, synthetic_artifact,
            "write", expected_operation_succeeded=False,
        )
    except QualifiedDifferentialError as exc:
        assert "pre-artifact" in str(exc)
    else:
        raise AssertionError("partial output from an expected failed write was accepted")
    write_result_failure = copy.deepcopy(write_failure)
    write_result_failure["status"]["firstFailure"].update({
        "stage": "writeResult", "code": 1,
    })
    write_result_failure["diagnostics"][0].update({
        "stage": "writeResult", "code": "write-result-error-1",
    })
    validate_result(write_result_failure)
    _validate_generated_artifact(
        write_result_failure, synthetic_artifact, "write",
        expected_operation_succeeded=False,
    )
    readback_failure = copy.deepcopy(write_failure)
    readback_failure["status"]["firstFailure"].update({
        "stage": "readback", "code": 14,
    })
    readback_failure["diagnostics"][0].update({
        "stage": "readback", "code": "readback-error-14",
    })
    readback_failure["records"].append({
        "id": "record:write-result", "callbackOrdinal": 2,
        "kind": "artifact", "recordClass": "WRITE_RESULT",
        "entity": "WRITE_RESULT", "sourceHandle": None,
        "ownerHandle": None, "blockId": None,
        "fields": [
            {"name": "recordClass", "type": "string", "value": "WRITE_RESULT"},
            {"name": "entity", "type": "string", "value": "WRITE_RESULT"},
            {"name": "sha256", "type": "string", "value": synthetic_artifact["sha256"]},
            {"name": "byteSize", "type": "uint64", "value": 7},
            {"name": "format", "type": "string", "value": "dxf-ascii"},
            {"name": "version", "type": "string", "value": "AC1009"},
        ],
    })
    readback_failure["opaqueCarriers"].append({
        "id": "carrier:write-result",
        "source": "/records/record:write-result/generatedOutput",
        "byteSize": 7, "sha256": synthetic_artifact["sha256"],
        "disposition": "observed", "recordId": "record:write-result",
        "sourceHandle": None,
    })
    _validate_generated_artifact(
        readback_failure, synthetic_artifact, "write",
        expected_operation_succeeded=False,
    )

    # Valid-but-different callback, graph, carrier, and diagnostic vectors are
    # all visible mismatches; no category may disappear silently.
    callback_delta = copy.deepcopy(standalone)
    callback_delta["callbacks"][0]["kind"] = "addCircle"
    assert compare_results(
        target, callback_delta, empty_rules, manifest_digest=config_digest,
        target_lock=target_lock,
    )["outcomeCounts"]["mismatch"] > 0
    graph_delta = copy.deepcopy(standalone)
    graph_delta["graphEdges"][0]["id"] = "edge:handle:changed"
    assert compare_results(
        target, graph_delta, empty_rules, manifest_digest=config_digest,
        target_lock=target_lock,
    )["outcomeCounts"]["mismatch"] > 0
    carrier_delta = copy.deepcopy(standalone)
    carrier_delta["opaqueCarriers"][0]["sha256"] = _digest_text("changed")
    assert compare_results(
        target, carrier_delta, empty_rules, manifest_digest=config_digest,
        target_lock=target_lock,
    )["outcomeCounts"]["mismatch"] > 0
    diagnostic_delta = copy.deepcopy(standalone)
    diagnostic_delta["diagnostics"] = [{
        "id": "diagnostic:1", "ordinal": 0, "severity": "warning",
        "stage": "entities", "code": "Synthetic",
        "path": "/records/record:1", "messageDigest": _digest_text("synthetic"),
    }]
    diagnostic_delta["cardinality"]["diagnosticCount"] = 1
    assert compare_results(
        target, diagnostic_delta, empty_rules, manifest_digest=config_digest,
        target_lock=target_lock,
    )["outcomeCounts"]["mismatch"] > 0

    tolerance_delta = copy.deepcopy(standalone)
    tolerance_delta["records"][0]["fields"][1]["value"]["x"] = 1e-10
    tolerance_rules = {
        "tolerances": [{
            "id": "tol-start-x",
            "scope": copy.deepcopy(synthetic_scope),
            "path": "/records/record:1/fields/start/value/x",
            "absolute": 1e-9,
            "relative": 1e-12,
            "expectedMatches": 1,
            "evidenceId": "oda-coordinate-normalization",
        }],
        "reviewedTargetDebt": [],
        "exclusions": [],
    }
    normalized = compare_results(
        target, tolerance_delta, tolerance_rules, manifest_digest=config_digest,
        target_lock=target_lock,
    )
    assert normalized["status"] == "qualifiedNormalized"
    assert normalized["outcomeCounts"]["toleranceNormalized"] == 1

    debt_delta = copy.deepcopy(standalone)
    debt_delta["records"][0]["fields"][0]["value"] = "TARGET-DEBT"
    debt_path = "/records/record:1/fields/layer/value"
    debt_rules = {
        "tolerances": [],
        "reviewedTargetDebt": [{
            "id": "debt-layer",
            "scope": copy.deepcopy(synthetic_scope),
            "path": debt_path,
            "targetDigest": _value_digest("0"),
            "standaloneDigest": _value_digest("TARGET-DEBT"),
            "expectedMatches": 1,
            "evidenceId": "reviewed-target-debt-1",
            "reason": "synthetic exact target-debt vector",
            "diagnosticContext": None,
        }],
        "exclusions": [],
    }
    reviewed = compare_results(
        target, debt_delta, debt_rules, manifest_digest=config_digest,
        target_lock=target_lock,
    )
    assert reviewed["status"] == "reviewedNonPromoting"
    assert not reviewed["claimEligible"]

    non_applicable_rules = copy.deepcopy(debt_rules)
    non_applicable_rules["reviewedTargetDebt"][0]["scope"].update(
        {
            "inputPathHint": "local/different-bytes-v1",
            "inputSha256": _digest_text("different bytes"),
        }
    )
    ignored = compare_results(
        target, standalone, non_applicable_rules,
        manifest_digest=config_digest, target_lock=target_lock,
    )
    assert ignored["status"] == "exact"
    wrong_scope = compare_results(
        target, debt_delta, non_applicable_rules,
        manifest_digest=config_digest, target_lock=target_lock,
    )
    assert wrong_scope["status"] == "mismatch"
    assert wrong_scope["outcomeCounts"]["reviewedTargetDebt"] == 0

    wrong_options_rules = copy.deepcopy(debt_rules)
    wrong_options_rules["reviewedTargetDebt"][0]["scope"]["optionsDigest"] = (
        _digest_text("different-options")
    )
    wrong_options = compare_results(
        target, debt_delta, wrong_options_rules,
        manifest_digest=config_digest, target_lock=target_lock,
    )
    assert wrong_options["status"] == "mismatch"
    assert wrong_options["outcomeCounts"]["reviewedTargetDebt"] == 0

    wrong_digest_rules = copy.deepcopy(debt_rules)
    wrong_digest_rules["reviewedTargetDebt"][0]["targetDigest"] = (
        _value_digest("WRONG-TARGET")
    )
    wrong_digest = compare_results(
        target, debt_delta, wrong_digest_rules,
        manifest_digest=config_digest, target_lock=target_lock,
    )
    assert wrong_digest["status"] == "mismatch"
    assert wrong_digest["outcomeCounts"]["reviewedTargetDebt"] == 0

    duplicate_scope_rules = copy.deepcopy(debt_rules)
    duplicate_rule = copy.deepcopy(duplicate_scope_rules["reviewedTargetDebt"][0])
    duplicate_rule["id"] = "debt-layer-duplicate"
    duplicate_scope_rules["reviewedTargetDebt"].append(duplicate_rule)
    try:
        _validate_rules(duplicate_scope_rules)
    except QualifiedDifferentialError as exc:
        assert "invocation/path selectors" in str(exc)
    else:
        raise AssertionError("duplicate scoped comparison rule was accepted")

    glob_debt_rules = copy.deepcopy(debt_rules)
    glob_debt_rules["reviewedTargetDebt"][0]["path"] = (
        "/records/record:1/fields/layer/val*"
    )
    try:
        _validate_rules(glob_debt_rules)
    except QualifiedDifferentialError as exc:
        assert "glob metacharacters" in str(exc)
    else:
        raise AssertionError("globbed reviewed-target-debt path was accepted")

    diagnostic_target = copy.deepcopy(target)
    diagnostic_standalone = copy.deepcopy(standalone)
    diagnostic_context = {
        "id": "diagnostic:00000000",
        "ordinal": 0,
        "severity": "warning",
        "stage": "integrity",
        "code": "dwg-integrity-1",
        "path": "/diagnostics/integrity",
    }
    for document, message in (
        (diagnostic_target, "target CRC payload"),
        (diagnostic_standalone, "standalone CRC payload"),
    ):
        document["diagnostics"] = [{
            **diagnostic_context,
            "messageDigest": _digest_text(message),
        }]
        document["cardinality"]["diagnosticCount"] = 1
    diagnostic_debt_rules = {
        "tolerances": [],
        "reviewedTargetDebt": [{
            "id": "debt-diagnostic-message",
            "scope": copy.deepcopy(synthetic_scope),
            "path": "/diagnostics/0/messageDigest",
            "targetDigest": _value_digest(_digest_text("target CRC payload")),
            "standaloneDigest": _value_digest(
                _digest_text("standalone CRC payload")
            ),
            "expectedMatches": 1,
            "evidenceId": "synthetic-diagnostic-context",
            "reason": "synthetic exact diagnostic-context vector",
            "diagnosticContext": {
                "target": copy.deepcopy(diagnostic_context),
                "standalone": copy.deepcopy(diagnostic_context),
            },
        }],
        "exclusions": [],
    }
    diagnostic_debt = compare_results(
        diagnostic_target, diagnostic_standalone, diagnostic_debt_rules,
        manifest_digest=config_digest, target_lock=target_lock,
    )
    assert diagnostic_debt["status"] == "reviewedNonPromoting"
    assert diagnostic_debt["outcomeCounts"]["mismatch"] == 0
    wrong_context_rules = copy.deepcopy(diagnostic_debt_rules)
    wrong_context_rules["reviewedTargetDebt"][0]["diagnosticContext"][
        "standalone"
    ]["code"] = "dwg-integrity-2"
    wrong_context = compare_results(
        diagnostic_target, diagnostic_standalone, wrong_context_rules,
        manifest_digest=config_digest, target_lock=target_lock,
    )
    assert wrong_context["status"] == "mismatch"
    assert wrong_context["outcomeCounts"]["reviewedTargetDebt"] == 0

    false_ordinal_rules = copy.deepcopy(diagnostic_debt_rules)
    false_ordinal_rules["reviewedTargetDebt"][0]["diagnosticContext"][
        "target"
    ]["ordinal"] = False
    try:
        _validate_rules(false_ordinal_rules)
    except QualifiedDifferentialError as exc:
        assert "must be an integer" in str(exc)
    else:
        raise AssertionError("boolean diagnostic-context ordinal was accepted")

    target_only_diagnostic = copy.deepcopy(diagnostic_target)
    standalone_without_diagnostic = copy.deepcopy(standalone)
    target_only_flat = _flatten(_semantic_view(target_only_diagnostic))
    standalone_without_flat = _flatten(
        _semantic_view(standalone_without_diagnostic)
    )
    target_only_paths = sorted(
        path for path in set(target_only_flat) | set(standalone_without_flat)
        if target_only_flat.get(path, _MISSING)
        != standalone_without_flat.get(path, _MISSING)
    )
    assert target_only_paths == [
        "/cardinality/diagnosticCount",
        "/diagnostics/0/@size",
        "/diagnostics/0/code",
        "/diagnostics/0/id",
        "/diagnostics/0/messageDigest",
        "/diagnostics/0/ordinal",
        "/diagnostics/0/path",
        "/diagnostics/0/severity",
        "/diagnostics/0/stage",
        "/diagnostics/@length",
    ]
    nullable_debt_rows: list[dict[str, Any]] = []
    for index, path in enumerate(target_only_paths):
        left = target_only_flat.get(path, _MISSING)
        right = standalone_without_flat.get(path, _MISSING)
        nullable_debt_rows.append({
            "id": f"nullable-debt-{index}",
            "scope": copy.deepcopy(synthetic_scope),
            "path": path,
            "targetDigest": None if left is _MISSING else _value_digest(left),
            "standaloneDigest": None if right is _MISSING else _value_digest(right),
            "expectedMatches": 1,
            "evidenceId": "synthetic-nullable-debt",
            "reason": "synthetic exact one-sided diagnostic debt",
            "diagnosticContext": (
                {
                    "target": copy.deepcopy(diagnostic_context),
                    "standalone": None,
                }
                if path == "/diagnostics/0/messageDigest" else None
            ),
        })
    nullable_debt_rules = {
        "tolerances": [],
        "reviewedTargetDebt": nullable_debt_rows,
        "exclusions": [],
    }
    nullable_reviewed = compare_results(
        target_only_diagnostic, standalone_without_diagnostic,
        nullable_debt_rules, manifest_digest=config_digest,
        target_lock=target_lock,
    )
    assert nullable_reviewed["status"] == "reviewedNonPromoting"
    assert nullable_reviewed["outcomeCounts"]["reviewedTargetDebt"] == 10
    assert nullable_reviewed["outcomeCounts"]["mismatch"] == 0

    changed_target_diagnostic = copy.deepcopy(target_only_diagnostic)
    changed_target_diagnostic["diagnostics"][0]["code"] = "dwg-integrity-2"
    changed_target = compare_results(
        changed_target_diagnostic, standalone_without_diagnostic,
        nullable_debt_rules, manifest_digest=config_digest,
        target_lock=target_lock,
    )
    assert changed_target["status"] == "mismatch"
    assert changed_target["outcomeCounts"]["mismatch"] >= 1

    unexpected_standalone_diagnostic = copy.deepcopy(standalone_without_diagnostic)
    unexpected_standalone_diagnostic["diagnostics"] = copy.deepcopy(
        target_only_diagnostic["diagnostics"]
    )
    unexpected_standalone_diagnostic["cardinality"]["diagnosticCount"] = 1
    try:
        compare_results(
            target_only_diagnostic, unexpected_standalone_diagnostic,
            nullable_debt_rules, manifest_digest=config_digest,
            target_lock=target_lock,
        )
    except QualifiedDifferentialError as exc:
        assert "expected 1" in str(exc)
    else:
        raise AssertionError("unexpected standalone diagnostic was accepted")

    nullable_wrong_scope = copy.deepcopy(nullable_debt_rules)
    for rule in nullable_wrong_scope["reviewedTargetDebt"]:
        rule["scope"]["optionsDigest"] = _digest_text("wrong-options")
    nullable_wrong_scope_report = compare_results(
        target_only_diagnostic, standalone_without_diagnostic,
        nullable_wrong_scope, manifest_digest=config_digest,
        target_lock=target_lock,
    )
    assert nullable_wrong_scope_report["status"] == "mismatch"
    assert nullable_wrong_scope_report["outcomeCounts"]["mismatch"] == 10

    null_null_rules = copy.deepcopy(nullable_debt_rules)
    null_null_rules["reviewedTargetDebt"][0]["targetDigest"] = None
    null_null_rules["reviewedTargetDebt"][0]["standaloneDigest"] = None
    try:
        _validate_rules(null_null_rules)
    except QualifiedDifferentialError as exc:
        assert "both sides" in str(exc)
    else:
        raise AssertionError("null/null reviewed target debt was accepted")

    stale_rules = copy.deepcopy(tolerance_rules)
    stale_rules["tolerances"][0]["expectedMatches"] = 2
    try:
        compare_results(
            target, tolerance_delta, stale_rules, manifest_digest=config_digest,
            target_lock=target_lock,
        )
    except QualifiedDifferentialError as exc:
        assert "expected 2" in str(exc)
    else:
        raise AssertionError("stale normalization-rule cardinality was accepted")

    # Missing values require an explicit, cardinality-bound exclusion.  The
    # same omission without these three rules remains a mismatch.
    missing_field = copy.deepcopy(standalone)
    del missing_field["records"][0]["fields"][0]
    missing_field["cardinality"]["fieldCount"] = 3
    missing_report = compare_results(
        target, missing_field, empty_rules, manifest_digest=config_digest,
        target_lock=target_lock,
    )
    assert missing_report["outcomeCounts"]["mismatch"] > 0
    missing_rules = {
        "tolerances": [],
        "reviewedTargetDebt": [],
        "exclusions": [
            {"id": "exclude-field-map-size",
             "scope": copy.deepcopy(synthetic_scope),
             "path": "/records/record:1/fields/@size",
             "reason": "synthetic missing-field vector", "expectedMatches": 1,
             "evidenceId": "synthetic-exclusion-1"},
            {"id": "exclude-layer-field",
             "scope": copy.deepcopy(synthetic_scope),
             "path": "/records/record:1/fields/layer/*",
             "reason": "synthetic missing-field vector", "expectedMatches": 3,
             "evidenceId": "synthetic-exclusion-2"},
            {"id": "exclude-field-count",
             "scope": copy.deepcopy(synthetic_scope),
             "path": "/cardinality/fieldCount",
             "reason": "synthetic missing-field vector", "expectedMatches": 1,
             "evidenceId": "synthetic-exclusion-3"},
        ],
    }
    explicitly_excluded = compare_results(
        target, missing_field, missing_rules, manifest_digest=config_digest,
        target_lock=target_lock,
    )
    assert explicitly_excluded["status"] == "reviewedNonPromoting"
    assert explicitly_excluded["outcomeCounts"]["mismatch"] == 0

    write_target = copy.deepcopy(target)
    write_standalone = copy.deepcopy(standalone)
    for document in (write_target, write_standalone):
        document["input"].update({
            "id": "minimal-line-v1",
            "pathHint": "local/minimal-line-v1",
            "sha256": _digest_text("minimal-line-v1-recipe"),
            "byteSize": len("minimal-line-v1-recipe"),
            "detectedFormat": "recipe-json",
            "detectedVersion": "AC1024",
        })
        document["invocation"]["direction"] = "write"
    write_report = compare_results(
        write_target, write_standalone, empty_rules, manifest_digest=config_digest,
        target_lock=target_lock,
    )
    assert write_report["status"] == "exact"

    # Strict schema and cross-reference negatives.
    bad = copy.deepcopy(target)
    bad["callbacks"].reverse()
    _assert_rejected(bad, "callback ordinals")
    bad = copy.deepcopy(target)
    bad["graphEdges"][0]["to"] = "missing-node"
    _assert_rejected(bad, "dangling")
    bad = copy.deepcopy(target)
    bad["graphNodes"].append({
        "id": "node:orphan", "kind": "record", "recordId": None,
        "handle": None,
    })
    bad["cardinality"]["graphNodeCount"] += 1
    _assert_rejected(bad, "orphaned")
    bad = copy.deepcopy(target)
    bad["graphNodes"].append({
        "id": "node:handle:duplicate", "kind": "handle", "recordId": None,
        "handle": "1",
    })
    bad["cardinality"]["graphNodeCount"] += 1
    _assert_rejected(bad, "not canonical")
    bad = copy.deepcopy(target)
    bad["opaqueCarriers"][0]["sha256"] = "bad"
    _assert_rejected(bad, "SHA-256")
    bad = copy.deepcopy(target)
    bad["callbacks"][0]["carrierIds"] = []
    _assert_rejected(bad, "exactly one callback")
    bad = copy.deepcopy(target)
    bad["diagnostics"] = [{
        "id": "diagnostic:1", "ordinal": 0, "severity": "info",
        "stage": "entities", "code": "x", "path": "/records/record:1",
        "messageDigest": _digest_text("x"),
    }]
    bad["cardinality"]["diagnosticCount"] = 1
    _assert_rejected(bad, "severity")
    bad = copy.deepcopy(target)
    del bad["unsupportedContent"]
    _assert_rejected(bad, "keys differ")
    bad = copy.deepcopy(target)
    bad["records"][0]["fields"][1]["value"]["x"] = float("inf")
    _assert_rejected(bad, "finite")
    bad = copy.deepcopy(target)
    bad["cardinality"]["callbackCount"] = True
    _assert_rejected(bad, "integer")
    bad = copy.deepcopy(target)
    bad["unexpected"] = 1
    _assert_rejected(bad, "extra=['unexpected']")
    bad = copy.deepcopy(target)
    bad["input"]["pathHint"] = "/private/tmp/leak.dxf"
    _assert_rejected(bad, "normalized relative")
    bad = copy.deepcopy(target)
    bad["input"]["pathHint"] = "local/../leak.dxf"
    _assert_rejected(bad, "normalized relative")
    bad = copy.deepcopy(target)
    bad["records"][0]["fields"][2]["value"] = "CIRCLE"
    _assert_rejected(bad, "recordClass field contradicts")
    bad = copy.deepcopy(target)
    bad["unsupportedContent"][0]["disposition"] = "preserved"
    _assert_rejected(bad, "preservation disposition")
    bad = copy.deepcopy(target)
    bad["records"][0]["callbackOrdinal"] = 1
    _assert_rejected(bad, "not reciprocal")
    bad = copy.deepcopy(target)
    bad["opaqueCarriers"][0]["recordId"] = None
    bad["opaqueCarriers"][0]["sourceHandle"] = None
    bad["callbacks"][0]["carrierIds"] = []
    bad["callbacks"][1]["carrierIds"] = ["carrier:1"]
    bad["unsupportedContent"][0]["carrierId"] = None
    bad["unsupportedContent"][0]["disposition"] = "excluded"
    _assert_rejected(bad, "recordless opaque carrier")
    bad = copy.deepcopy(target)
    bad["unsupportedContent"][0]["recordId"] = None
    bad["unsupportedContent"][0]["carrierId"] = None
    bad["unsupportedContent"][0]["disposition"] = "excluded"
    _assert_rejected(bad, "recordless unsupportedContent")
    bad = copy.deepcopy(target)
    bad["status"] = {
        "outcome": "failure",
        "operationSucceeded": False,
        "exitCode": 1,
        "firstFailure": {
            "stage": "entities", "code": 1,
            "path": "/records/record:1", "diagnosticId": "diagnostic:1",
        },
    }
    bad["diagnostics"] = [{
        "id": "diagnostic:1", "ordinal": 0, "severity": "error",
        "stage": "objects", "code": "SyntheticFailure",
        "path": "/records/record:1", "messageDigest": _digest_text("failure"),
    }]
    bad["cardinality"]["diagnosticCount"] = 1
    _assert_rejected(bad, "does not match")
    bad = copy.deepcopy(failed_target)
    bad["diagnostics"][0]["code"] = "completely-unrelated-1"
    _assert_rejected(bad, "code does not match")
    try:
        _loads_json('{"schema":2,"schema":2}', "duplicate-key-vector")
    except QualifiedDifferentialError as exc:
        assert "duplicate JSON key" in str(exc)
    else:
        raise AssertionError("duplicate JSON key was accepted")

    bad_tolerance_rules = {
        "tolerances": [{
            "id": "tol-cardinality",
            "scope": copy.deepcopy(synthetic_scope),
            "path": "/cardinality/fieldCount",
            "absolute": 1, "relative": 0, "expectedMatches": 1,
            "evidenceId": "invalid-cardinality-tolerance",
        }],
        "reviewedTargetDebt": [],
        "exclusions": [],
    }
    try:
        compare_results(
            target, standalone, bad_tolerance_rules,
            manifest_digest=config_digest, target_lock=target_lock,
        )
    except QualifiedDifferentialError as exc:
        assert "typed record field values" in str(exc)
    else:
        raise AssertionError("numeric tolerance outside a typed field was accepted")

    # The low-level executor must require byte-identical output, not merely
    # equivalent parsed JSON.
    with tempfile.TemporaryDirectory(prefix="libdxfrw-qualified-v2-") as temp:
        root = Path(temp)
        recipe = root / "recipe.json"
        recipe.write_bytes(b"local recipe\n")
        source = root / "adapter.cpp"
        source.write_bytes(b"// adapter\n")
        source_digest = _sha256_file(source)
        local_manifest = {
            "localRecipes": [{
                "id": "recipe:local", "path": "recipe.json",
                "digest": _sha256_file(recipe), "byteSize": recipe.stat().st_size,
                "generatorSource": "adapter.cpp", "format": "recipe-json",
                "version": "AC1024",
            }],
        }
        provenance = _resolve_input_provenance(
            root=root, manifest=local_manifest, input_path=recipe,
            input_id="recipe:local", input_path_hint="recipe.json",
            origin_kind="localFromScratch", input_repository=None,
            input_commit=None, generator_digest=source_digest,
            source_digest=source_digest, input_git_dir=None,
        )
        assert provenance == {
            "sourcePath": None, "sourceBlob": None, "registryDigest": None,
            "detectedFormat": "recipe-json", "detectedVersion": "AC1024",
        }
        binary_dxf = root / "identity-binary.dxf"
        binary_dxf.write_bytes(
            b"AutoCAD Binary DXF\r\n\x1a\x00"
            b"\x09\x00$ACADVER\x00\x01\x00AC1021\x00"
        )
        assert _inspect_drawing_identity(binary_dxf) == ("dxf-binary", "AC1021")
        ascii_dxf = root / "identity-ascii.dxf"
        ascii_dxf.write_bytes(
            b"  0\nSECTION\n  2\nHEADER\n  9\n$ACADVER\n  1\nAC1027\n"
            b"  0\nENDSEC\n  0\nEOF\n"
        )
        assert _inspect_drawing_identity(ascii_dxf) == ("dxf-ascii", "AC1027")
        dwg = root / "identity.dwg"
        dwg.write_bytes(b"AC1024" + b"\x00" * 16)
        assert _inspect_drawing_identity(dwg) == ("dwg", "AC1024")
        bogus = root / "identity-bogus.json"
        bogus.write_bytes(b'{"version":"AC1024"}\n')
        assert _inspect_drawing_identity(bogus) == ("unknown", "unknown")
        truncated_binary = root / "identity-truncated.dxf"
        truncated_binary.write_bytes(b"AutoCAD Binary DXFAC1024")
        assert _inspect_drawing_identity(truncated_binary) == ("unknown", "unknown")
        # Bind the real live functions so required provenance values cannot
        # silently drift between the command-line entry point and each side.
        live_arguments = inspect.signature(run_live).bind(
            root=root, manifest_path=root / "manifest.json", input_path=recipe,
            target_receipt_path=root / "target.json",
            standalone_receipt_path=root / "standalone.json",
            input_id="recipe:local", input_path_hint="recipe.json",
            origin_kind="localFromScratch", input_repository=None,
            input_commit=None, generator_digest=source_digest,
            input_git_dir=None, facade="dxfRW", direction="read",
            timeout_seconds=1.0,
        )
        assert "input_git_dir" in live_arguments.arguments
        side_arguments = inspect.signature(_run_live_side).bind(
            side="target", manifest={}, manifest_digest=_digest_text("manifest"),
            source_digest=source_digest, receipt={}, receipt_path=root / "receipt.json",
            input_path=recipe, input_id="recipe:local",
            input_path_hint="recipe.json", origin_kind="localFromScratch",
            input_repository=None, input_commit=None, input_source_path=None,
            input_source_blob=None, input_registry_digest=None,
            expected_detected_format="recipe-json",
            expected_detected_version="AC1024",
            generator_digest=source_digest, facade="dxfRW", direction="read",
            timeout_seconds=1.0, expected_operation_succeeded=True,
            output_probe="none",
        )
        assert "input_source_path" in side_arguments.arguments
        assert "input_git_dir" not in side_arguments.arguments
        payload = json.dumps(target, sort_keys=True, separators=(",", ":"))
        deterministic_command = [sys.executable, "-c", f"print({payload!r})"]
        encoded, code = _run_twice(deterministic_command, 5.0)
        assert code == 0 and json.loads(encoded) == target
        counter = root / "counter"
        nondeterministic = [
            sys.executable,
            "-c",
            (
                "from pathlib import Path; "
                f"p=Path({str(counter)!r}); "
                "n=int(p.read_text())+1 if p.exists() else 1; "
                "p.write_text(str(n)); print(n)"
            ),
        ]
        try:
            _run_twice(nondeterministic, 5.0)
        except QualifiedDifferentialError as exc:
            assert "byte-identical" in str(exc)
        else:
            raise AssertionError("non-deterministic runner was accepted")
        try:
            _secure_receipt_artifact_path(
                root / "receipt.json", "/private/tmp/external-artifact",
                "synthetic receipt artifact",
            )
        except QualifiedDifferentialError as exc:
            assert "logical POSIX" in str(exc) or "normalized relative" in str(exc)
        else:
            raise AssertionError("absolute receipt artifact path was accepted")
        artifact_counter = root / "artifact-counter"
        artifact_command = [
            sys.executable,
            "-c",
            (
                "from pathlib import Path; import sys; "
                f"p=Path({str(artifact_counter)!r}); "
                "n=int(p.read_text())+1 if p.exists() else 1; p.write_text(str(n)); "
                "Path(sys.argv[1]).write_text(str(n)); print('{}')"
            ),
            "{output}",
        ]
        try:
            _run_adapter_twice(
                artifact_command, {}, 5.0, output_policy="required",
                output_suffix=".dxf",
            )
        except QualifiedDifferentialError as exc:
            assert "non-deterministic" in str(exc)
        else:
            raise AssertionError("non-deterministic generated artifact was accepted")
    print("run_qualified_differential_v2 self-test: PASS")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parent.parent)
    parser.add_argument("--manifest", type=Path)
    parser.add_argument("--target-result", type=Path)
    parser.add_argument("--standalone-result", type=Path)
    parser.add_argument("--target-receipt", type=Path)
    parser.add_argument("--standalone-receipt", type=Path)
    parser.add_argument("--input", type=Path)
    parser.add_argument("--input-id")
    parser.add_argument("--input-path-hint")
    parser.add_argument("--origin-kind", choices=sorted(ORIGIN_KINDS))
    parser.add_argument("--input-repository")
    parser.add_argument("--input-commit")
    parser.add_argument("--generator-digest")
    parser.add_argument("--input-git-dir", type=Path)
    parser.add_argument("--facade", choices=sorted(FACADES), default="dxfRW")
    parser.add_argument("--direction", choices=sorted(DIRECTIONS), default="read")
    parser.add_argument("--timeout", type=float, default=30.0)
    parser.add_argument("--expect-operation-failure", action="store_true")
    parser.add_argument("--expected-failure-id")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args(argv)
    try:
        if args.self_test:
            self_test()
            if args.manifest is not None:
                validate_manifest(_read_json(args.manifest), root=args.root.resolve())
                print("qualified differential v2 manifest: PASS")
            return 0
        if args.manifest is None:
            parser.error("--manifest is required unless --self-test is used")
        root = args.root.resolve()
        manifest_path = args.manifest.resolve()
        manifest = validate_manifest(_read_json(manifest_path), root=root)
        expected_failure = _expected_failure_by_id(
            manifest, args.expected_failure_id
        )
        if args.expect_operation_failure != (expected_failure is not None):
            raise QualifiedDifferentialError(
                "--expect-operation-failure and --expected-failure-id "
                "must be supplied together"
            )
        if args.target_result is not None or args.standalone_result is not None:
            if args.target_result is None or args.standalone_result is None:
                parser.error("--target-result and --standalone-result are paired")
            manifest_digest = _sha256_file(manifest_path)
            report = compare_results(
                _read_json(args.target_result), _read_json(args.standalone_result),
                manifest["comparison"], manifest_digest=manifest_digest,
                target_lock=manifest["targetLock"],
                expected_operation_succeeded=not args.expect_operation_failure,
                expected_failure=expected_failure,
            )
            report["claimEligible"] = False
            report["eligibilityBlockers"] = sorted(set(
                report["eligibilityBlockers"] + ["offlineUnattestedResults"]
            ))
            for row in report["comparisons"]:
                row["claimEligible"] = False
            if report["status"] in {"exact", "qualifiedNormalized"}:
                report["status"] = "reviewedNonPromoting"
        elif args.input is not None:
            required = {
                "--target-receipt": args.target_receipt,
                "--standalone-receipt": args.standalone_receipt,
                "--input-id": args.input_id,
                "--input-path-hint": args.input_path_hint,
                "--origin-kind": args.origin_kind,
            }
            missing = [name for name, value in required.items() if value is None]
            if missing:
                parser.error("live mode requires " + ", ".join(missing))
            report = run_live(
                root=root, manifest_path=manifest_path,
                input_path=args.input.resolve(),
                target_receipt_path=args.target_receipt.resolve(),
                standalone_receipt_path=args.standalone_receipt.resolve(),
                input_id=args.input_id, input_path_hint=args.input_path_hint,
                origin_kind=args.origin_kind,
                input_repository=args.input_repository,
                input_commit=args.input_commit,
                generator_digest=args.generator_digest,
                input_git_dir=args.input_git_dir,
                facade=args.facade, direction=args.direction,
                timeout_seconds=args.timeout,
                expected_operation_succeeded=not args.expect_operation_failure,
                expected_failure=expected_failure,
            )
        else:
            print("qualified differential v2 manifest: PASS")
            return 0
        encoded = json.dumps(report, sort_keys=True, indent=2, allow_nan=False) + "\n"
        if args.output is not None:
            args.output.write_text(encoded, encoding="utf-8")
        else:
            sys.stdout.write(encoded)
        return 1 if report["status"] == "mismatch" else 0
    except (OSError, UnicodeError, json.JSONDecodeError,
            QualifiedDifferentialError, AssertionError) as exc:
        print(f"qualified differential v2: FAIL: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
