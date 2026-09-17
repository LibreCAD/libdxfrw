#!/usr/bin/env python3
"""Validate and run the fail-closed normalized differential schema v2.

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


RESULT_SCHEMA = 2
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
GRAPH_EDGE_KINDS = {"handleReference", "owner", "blockMembership"}
GRAPH_EDGE_DISPOSITIONS = {"observed"}
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

ROOT_KEYS = {
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


def validate_result(value: Any, expected_adapter: dict[str, Any] | None = None) -> dict[str, Any]:
    """Validate one adapter result and return it unchanged."""

    document = _exact_keys(value, ROOT_KEYS, "result")
    if document["schema"] != RESULT_SCHEMA or document["kind"] != RESULT_KIND:
        raise QualifiedDifferentialError("unexpected adapter result schema or kind")
    _validate_input(document["input"])
    adapter = _validate_adapter(document["adapter"])
    if expected_adapter is not None and adapter != expected_adapter:
        raise QualifiedDifferentialError(
            "adapter result identity does not match its build receipt"
        )
    _validate_invocation(document["invocation"])
    status = _validate_status(document["status"])
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
        if edge["kind"] in {"handleReference", "owner"}:
            if target_node["kind"] not in {"handle", "block"} or not target_node["handle"]:
                raise QualifiedDifferentialError(
                    f"{edge['kind']} edge must target a handled node"
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
        if record["sourceHandle"] is None:
            if handle_edges:
                raise QualifiedDifferentialError(
                    f"record {record_id} has a handle edge without sourceHandle"
                )
        elif len(handle_edges) != 1 or (
            node_by_id[handle_edges[0]["to"]]["handle"] != record["sourceHandle"]
        ):
            raise QualifiedDifferentialError(
                f"record {record_id} sourceHandle lacks one reciprocal edge"
            )
        if record["ownerHandle"] is None:
            if owner_edges:
                raise QualifiedDifferentialError(
                    f"record {record_id} has an owner edge without ownerHandle"
                )
        elif len(owner_edges) != 1 or (
            node_by_id[owner_edges[0]["to"]]["handle"] != record["ownerHandle"]
        ):
            raise QualifiedDifferentialError(
                f"record {record_id} ownerHandle lacks one reciprocal edge"
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
    "configurationInterproceduralOptimization",
}
CONFIG_INPUT_KEYS = {
    "adapterSourceDigest", "semanticManifestPath", "semanticManifestDigest",
    "packageCommit", "packageArchiveDigest", "configFiles",
    "verifiedLinkCommandDigest", "linkClosureEnforced",
    "linkVerificationMethod", "linkedLibraryTarget",
    "verifiedCompileCommandDigest", "compileContractEnforced",
    "verifiedInterfaceContractDigest", "interfaceContractEnforced",
    "interfaceMethodCount",
}
EXPECTED_INTERFACE_METHOD_COUNT = 170
CONFIG_FILE_KEYS = {"path", "digest"}
CONFIG_DEFINITION_KEYS = {
    "sideMacro", "adapterName", "adapterCommit", "embeddedConfigDigest"
}
CONFIG_WARNING_KEYS = {"library", "adapter"}
CONFIG_TARGET_KEYS = {"executable", "staticLibrary"}
BUILD_PARITY_KEYS = {
    "schema", "configuration", "cmake", "adapterCompile", "libraryCompile",
    "adapterLink",
}
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


def _validate_cmake_identity(value: Any, path: str,
                             configuration: str) -> dict[str, Any]:
    cmake = _exact_keys(value, CONFIG_CMAKE_KEYS, path)
    required_strings = {
        "cmakeVersion", "configuredCmakeVersion", "cmakeExecutable", "generator",
        "compilerPath", "compilerId", "compilerVersion", "sizeofDataPointer",
        "systemName",
    }
    array_keys = {"configurationTypes", "osxArchitectures"}
    bool_keys = {"multiConfig", "crossCompiling", "cxxExtensions"}
    for key in sorted(CONFIG_CMAKE_KEYS - array_keys - bool_keys - {"cxxStandard"}):
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


def _validate_build_parity_identity(value: Any, path: str,
                                    configuration: str) -> dict[str, Any]:
    identity = _exact_keys(value, BUILD_PARITY_KEYS, path)
    if identity["schema"] != 1 or identity["configuration"] != configuration:
        raise QualifiedDifferentialError(f"{path} schema/configuration is stale")
    _validate_cmake_identity(identity["cmake"], f"{path}.cmake", configuration)
    _validate_compile_profile(identity["adapterCompile"], f"{path}.adapterCompile")
    _validate_compile_profile(identity["libraryCompile"], f"{path}.libraryCompile")
    _validate_link_profile(identity["adapterLink"], f"{path}.adapterLink")
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
    inputs = _exact_keys(identity["inputs"], CONFIG_INPUT_KEYS,
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
    if inputs["compileContractEnforced"] is not True:
        raise QualifiedDifferentialError("adapter compile contract was not enforced")
    _digest(
        inputs["verifiedInterfaceContractDigest"],
        "receipt.build.configIdentity.inputs.verifiedInterfaceContractDigest",
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
    if target_build["buildParityDigest"] != standalone_build["buildParityDigest"]:
        raise QualifiedDifferentialError(
            "target/standalone build-parity digests differ"
        )
    if target_build["buildParityIdentity"] != standalone_build["buildParityIdentity"]:
        raise QualifiedDifferentialError(
            "target/standalone effective build profiles differ"
        )
    target_inputs = target_build["configIdentity"]["inputs"]
    standalone_inputs = standalone_build["configIdentity"]["inputs"]
    for key in ("verifiedInterfaceContractDigest", "interfaceMethodCount"):
        if target_inputs[key] != standalone_inputs[key]:
            raise QualifiedDifferentialError(
                "target/standalone interface callback inventories differ"
            )


MANIFEST_KEYS = {
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
    manifest = _exact_keys(value, MANIFEST_KEYS, "manifest")
    if manifest["schema"] != RESULT_SCHEMA or manifest["kind"] != MANIFEST_KIND:
        raise QualifiedDifferentialError("unexpected differential manifest schema or kind")
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
    return {
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
    if mismatch:
        status = "mismatch"
    elif reviewed or eligibility_blockers:
        status = "reviewedNonPromoting"
    elif complete_counts["toleranceNormalized"]:
        status = "qualifiedNormalized"
    else:
        status = "exact"
    report = {
        "schema": RESULT_SCHEMA,
        "kind": REPORT_KIND,
        "fixturePolicy": FIXTURE_POLICY,
        "manifestDigest": manifest_digest,
        "targetLock": copy.deepcopy(target_lock),
        "input": copy.deepcopy(target["input"]),
        "invocation": copy.deepcopy(target["invocation"]),
        "targetAdapter": copy.deepcopy(target["adapter"]),
        "standaloneAdapter": copy.deepcopy(standalone["adapter"]),
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


def _validate_generated_artifact(
    result: dict[str, Any], artifact: dict[str, Any] | None, direction: str,
    expected_operation_succeeded: bool,
) -> None:
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
        "schema": RESULT_SCHEMA,
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
            "ownerHandle": "0",
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
            {"id": "node:handle:0", "kind": "handle", "recordId": None,
             "handle": "0"},
        ],
        "graphEdges": [
            {"id": "edge:handle:1", "kind": "handleReference",
             "from": "node:record:1", "to": "node:handle:1",
             "disposition": "observed"},
            {"id": "edge:block:1", "kind": "blockMembership",
             "from": "node:record:1", "to": "block:model",
             "disposition": "observed"},
            {"id": "edge:owner:1", "kind": "owner",
             "from": "node:record:1", "to": "node:handle:0",
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
            "graphNodeCount": 4,
            "graphEdgeCount": 3,
            "opaqueCarrierCount": 1,
            "unsupportedContentCount": 1,
            "diagnosticCount": 0,
        },
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
    config_digest = _digest_text("manifest-v2")
    target = _base_result("target", config_digest)
    standalone = _base_result("standalone", config_digest)
    validate_result(target)
    validate_result(standalone)
    target_lock = {
        "path": "metadata/libdxfrw-target-lock.json",
        "digest": _digest_text("lock"),
        "targetCommit": "512d8bd86612f17158d4d222fcdc8a7c2b052c57",
        "snapshotRevision": "89b762bef636c90eb370cb1af3cec80fe759cb32",
        "archiveDigest": _digest_text("archive"),
        "manifestDigest": _digest_text("source-manifest"),
        "manifestEntries": 86,
    }
    empty_rules = {"tolerances": [], "reviewedTargetDebt": [], "exclusions": []}
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
            "buildParityIdentity": {"schema": 1, "configuration": "Release"},
            "configIdentity": {
                "inputs": {
                    "verifiedInterfaceContractDigest": _digest_text("interface"),
                    "interfaceMethodCount": EXPECTED_INTERFACE_METHOD_COUNT,
                }
            },
        }
    }
    pair_standalone = copy.deepcopy(pair_target)
    _validate_receipt_pair(pair_target, pair_standalone)
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
