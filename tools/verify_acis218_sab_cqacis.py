#!/usr/bin/env python3
"""Observe one locally authored ACIS-218 box with optional cq-acis/CadQuery.

The named v2 report separates numerical geometry, parser-declared units,
default conversion scaling, diagnostic/raw coverage and qualification. PASS
means the scoped geometry checks passed, not that every record was decoded.
Unknown observations remain advisory; qualificationEligible is always false.
This is not AutoCAD, DWG, ACDSDATA, native FreeCAD, independent-kernel or general
ACIS evidence. The two-positional invocation and historical metric keys remain;
old unversioned reports are not rewritten. --self-test needs no CAD dependency.
Byte/observation caps are not an external parser/kernel time or memory sandbox.
The v2 local input-admission profile is POSIX nonblocking regular files,
including regular-file symlinks. Non-POSIX admission is explicitly unsupported;
Windows platform qualification remains open. No filesystem deadline is claimed.
"""

from __future__ import annotations

import argparse
from dataclasses import fields, is_dataclass
import hashlib
import importlib.metadata
import json
import math
import os
from pathlib import Path
import stat
import sys
from typing import Any


REPORT_SCHEMA = "libdxfrw-acis218-cqacis-observation-v2"
PROFILE_ID = "cq-acis-0.3.8-typed-acis218-box-v1"
PROFILE_SOURCE = "a2eefa38eaeeb95f17dcdabe8132bcbb18e32310"
MAX_SAB_BYTES = 8 * 1024 * 1024
MAX_MANIFEST_BYTES = 64 * 1024
MAX_REPORT_BYTES = 256 * 1024
MAX_ENTITIES = 8192
MAX_REFERENCES = 65536
MAX_DIAGNOSTICS = 32
MAX_RAW_ROWS = 32
MAX_TEXT_CHARS = 256


class VerificationError(ValueError):
    """A witness or observation failed this deliberately narrow contract."""


def require(condition: bool, message: str) -> None:
    if not condition:
        raise VerificationError(message)


def finite_number(value: object, label: str, *, positive: bool = False) -> float:
    require(type(value) in (int, float), f"{label} must be numeric, not Boolean/text")
    try:
        result = float(value)
    except OverflowError as exc:
        raise VerificationError(f"{label} exceeds finite numeric range") from exc
    require(math.isfinite(result), f"{label} must be finite")
    require(not positive or result > 0, f"{label} must be positive")
    return result


def integer(value: object, label: str, *, maximum: int) -> int:
    require(type(value) is int and 0 <= value <= maximum,
            f"{label} must be an unsigned integer at most {maximum}")
    return value


def text(value: object, label: str, *, maximum: int = MAX_TEXT_CHARS) -> str:
    require(type(value) is str and len(value) <= maximum,
            f"{label} must be a bounded string")
    try:
        value.encode("utf-8", errors="strict")
    except UnicodeError as exc:
        raise VerificationError(f"{label} contains an invalid Unicode scalar") from exc
    return value


def read_vec3(value: object, label: str) -> tuple[float, float, float]:
    require(type(value) in (list, tuple) and len(value) == 3,
            f"{label} must be a three-value array")
    return tuple(finite_number(item, label) for item in value)  # type: ignore[return-value]


def close_values(actual: tuple[float, ...], expected: tuple[float, ...]) -> bool:
    return len(actual) == len(expected) and all(
        math.isclose(a, e, rel_tol=1e-8, abs_tol=1e-7)
        for a, e in zip(actual, expected)
    )


def bounded_read(path: Path, limit: int) -> bytes:
    # Local POSIX nonblocking admission avoids waiting for a FIFO writer.
    # This is not a deadline for arbitrary filesystems. Other platforms need
    # a separately reviewed admission primitive, not a blocking-open fallback.
    require(os.name == "posix" and hasattr(os, "O_NONBLOCK"),
            "nonblocking regular-file admission is unsupported on this platform")
    descriptor = os.open(path, os.O_RDONLY | os.O_NONBLOCK)
    try:
        require(stat.S_ISREG(os.fstat(descriptor).st_mode),
                "witness inputs must be regular files")
        # Keep explicit descriptor ownership across fdopen/read exceptions.
        # Symlinks to regular files stay supported; the opened target decides.
        with os.fdopen(descriptor, "rb", closefd=False) as stream:
            data = stream.read(limit + 1)
    finally:
        os.close(descriptor)
    require(len(data) <= limit, "witness input exceeds its byte limit")
    return data


def strict_json_loads(data: bytes) -> dict[str, Any]:
    require(type(data) is bytes and len(data) <= MAX_MANIFEST_BYTES,
            "manifest byte budget exceeded")
    def pairs(items: list[tuple[str, Any]]) -> dict[str, Any]:
        result: dict[str, Any] = {}
        for key, value in items:
            require(key not in result, "duplicate manifest JSON key")
            result[key] = value
        return result

    def constant(token: str) -> Any:
        raise VerificationError(f"nonfinite JSON constant: {token}")

    try:
        value = json.loads(data.decode("utf-8", errors="strict"),
                           object_pairs_hook=pairs, parse_constant=constant,
                           parse_float=lambda token: finite_number(float(token), "JSON number"))
    except (UnicodeError, json.JSONDecodeError) as exc:
        raise VerificationError("manifest is not strict UTF-8 JSON") from exc
    require(type(value) is dict, "manifest root must be a JSON object")
    pending = [value]
    while pending:
        item = pending.pop()
        if type(item) is str:
            try:
                item.encode("utf-8", errors="strict")
            except UnicodeError as exc:
                raise VerificationError("manifest contains an invalid Unicode scalar") from exc
        elif type(item) is dict:
            pending.extend(item.keys())
            pending.extend(item.values())
        elif type(item) is list:
            pending.extend(item)
    return value


def encode_report(value: dict[str, Any]) -> str:
    result = json.dumps(value, sort_keys=True, ensure_ascii=True, allow_nan=False)
    require(len(result.encode("utf-8")) <= MAX_REPORT_BYTES,
            "observation report exceeds its byte limit")
    return result


def source_span(value: object, source_id: str, size: int) -> dict[str, Any] | None:
    if value is None:
        return None
    domain = text(value.source_id, "diagnostic source domain", maximum=256)
    start = integer(value.start_offset, "source start", maximum=size)
    end = integer(value.end_offset, "source end", maximum=size)
    require(domain == source_id and start <= end,
            "source span must name this SAB's half-open byte domain")
    return {"sourceId": domain, "startOffset": start, "endOffset": end,
            "offsetBasis": "sab-byte", "endExclusive": True}


def observe_model(model: Any, api_types: Any, sab: bytes, source_id: str,
                  profile_matched: bool) -> tuple[dict[str, Any], list[dict[str, Any]],
                                                  list[dict[str, Any]]]:
    """Inspect a bounded ordinary typed-body profile, not extension geometry.

    cq-acis0.3.8 sab.rs297-315/393-451 documents only the required record-zero
    asmheader and separate opaque history as non-geometric exceptions here.
    This does not classify the raw record from any historical receipt.
    """
    entities = model.entities
    diagnostics = model.diagnostics
    require(type(entities) in (tuple, list) and len(entities) <= MAX_ENTITIES,
            "model entity inventory is unavailable or exceeds this box profile")
    require(type(diagnostics) in (tuple, list), "diagnostics must be an inventoried sequence")
    bodies = []
    for index, entity in enumerate(entities):
        require(integer(entity.index, "model index", maximum=MAX_ENTITIES) == index,
                "model indices must be contiguous and zero-based")
        require(isinstance(entity, api_types.RawEntity)
                or (is_dataclass(entity) and isinstance(entity.raw, api_types.RawEntity)),
                "model entity layout is unavailable")
        if isinstance(entity, api_types.BodyEntity):
            bodies.append(index)
        if isinstance(entity, api_types.RawEntity) and entity.type_name == "body":
            raise VerificationError("an expected source body remains undecoded")
    require(len(bodies) == 1, f"expected one decoded source body, got {len(bodies)}")

    # Only explicit semantic EntityRef fields are geometry edges. Raw source
    # attachments and attribute/pattern references are not geometry claims.
    # The external converter still validates actual supported geometry/closure.
    pending = bodies[:]
    geometry_targets: set[int] = set()
    references = 0
    while pending:
        index = pending.pop()
        if index in geometry_targets:
            continue
        geometry_targets.add(index)
        entity = entities[index]
        require(not isinstance(entity, api_types.RawEntity),
                "this typed box profile references undecoded geometry")
        require(is_dataclass(entity), "typed geometry entity layout is unavailable")
        for field in fields(entity):
            if field.name in ("raw", "pattern", "attributes"):
                continue
            reference = getattr(entity, field.name)
            if not isinstance(reference, api_types.EntityRef):
                continue
            references += 1
            require(references <= MAX_REFERENCES, "model reference budget exhausted")
            target = reference.index
            require(type(target) is int and -1 <= target < len(entities),
                    "model geometry reference is invalid")
            if target != -1:
                pending.append(target)

    raw_rows = []
    raw_kinds: dict[int, str] = {}
    raw_count = 0
    unknown_raw = 0
    compared_raw_bytes = 0
    source_view = memoryview(sab)
    for index, entity in enumerate(entities):
        if not isinstance(entity, api_types.RawEntity):
            continue
        raw_count += 1
        name = text(entity.type_name, "raw entity type", maximum=128)
        span = source_span(entity.source, source_id, len(sab))
        raw_data = entity.raw_data
        require(raw_data is None or type(raw_data) is bytes,
                "raw source data must be bytes or unavailable")
        if raw_data is not None:
            compared_raw_bytes += len(raw_data)
            require(compared_raw_bytes <= len(sab), "raw source comparison byte budget exhausted")
        matched_source = (span is not None and raw_data is not None
                          and raw_data == source_view[span["startOffset"]:span["endOffset"]])
        disposition = "unclassified"
        if profile_matched and matched_source and span["startOffset"] < span["endOffset"]:
            if name == "asmheader" and index == 0:
                disposition = "reviewedNonGeometricHeader"
            elif name == "__opaque_sab_history__" and span["endOffset"] == len(sab):
                disposition = "reviewedOpaqueHistoryGeometryAdvisory"
        raw_kinds[index] = disposition
        unknown_raw += disposition == "unclassified"
        if len(raw_rows) < MAX_RAW_ROWS:
            raw_rows.append({"entityIndex": index, "type": name, "source": span,
                             "sourceBytesMatched": matched_source,
                             "rawBytes": len(raw_data) if raw_data is not None else None,
                             "disposition": disposition})

    diagnostic_rows = []
    unknown_diagnostics = 0
    for diagnostic in diagnostics[:MAX_DIAGNOSTICS]:
        code = text(diagnostic.code, "diagnostic code", maximum=128)
        require(bool(code), "diagnostic code must not be empty")
        message = diagnostic.message
        require(type(message) is str, "diagnostic message must be text")
        prefix = text(message[:MAX_TEXT_CHARS], "diagnostic message prefix")
        target = diagnostic.entity_index
        if target is not None:
            integer(target, "diagnostic target", maximum=max(0, len(entities) - 1))
            require(target < len(entities), "diagnostic target is outside this model")
        span = source_span(diagnostic.source, source_id, len(sab))
        disposition = "unclassified"
        if (code in ("sab.entity_schema_unsupported", "sab.geometry_subtype_unsupported")
                and target in geometry_targets):
            raise VerificationError("parser reports unsupported referenced geometry")
        if profile_matched:
            if (code == "sab.raw_entities" and target is None and span is None
                    and unknown_raw == 0):
                disposition = "reviewedNonGeometricSummary"
            elif (code == "sab.history_opaque" and target is not None
                  and raw_kinds.get(target) == "reviewedOpaqueHistoryGeometryAdvisory"
                  and span is not None
                  and span == source_span(entities[target].source, source_id, len(sab))):
                disposition = "reviewedOpaqueHistoryGeometryAdvisory"
        unknown_diagnostics += disposition == "unclassified"
        diagnostic_rows.append({"code": code, "message": prefix,
                                "messageCharacters": len(message),
                                "messageTruncated": len(message) > MAX_TEXT_CHARS,
                                "entityIndex": target, "source": span,
                                "disposition": disposition})
    omitted = max(0, len(diagnostics) - MAX_DIAGNOSTICS)
    raw_omitted = max(0, raw_count - MAX_RAW_ROWS)
    classified = (profile_matched and not unknown_raw and not unknown_diagnostics
                  and not omitted and not raw_omitted)
    topology_types = {
        name: getattr(api_types, class_name, None)
        for name, class_name in (("body", "BodyEntity"), ("lump", "LumpEntity"),
                                 ("shell", "ShellEntity"), ("face", "FaceEntity"))
    }
    source_counts = {
        name: sum(isinstance(entity, entity_type) for entity in entities)
        if isinstance(entity_type, type) else None
        for name, entity_type in topology_types.items()
    }
    reachable_counts = {
        name: sum(isinstance(entities[index], entity_type) for index in geometry_targets)
        if isinstance(entity_type, type) else None
        for name, entity_type in topology_types.items()
    }
    expected_counts = {"body": 1, "lump": 1, "shell": 1, "face": 6}
    topology_status = ("unavailable" if any(value is None for value in source_counts.values())
                       else "matched" if source_counts == expected_counts
                       and reachable_counts == expected_counts else "mismatch")
    coverage = {
        "status": "observedModelClassified" if classified else "unknown",
        "scope": "ordinary-typed-box-geometry-not-history-or-full-file-semantics",
        "sourceBodyCount": len(bodies), "modelEntityCount": len(entities),
        "geometryEntityCount": len(geometry_targets), "geometryReferencesExamined": references,
        "rawEntityCount": raw_count, "unclassifiedRawEntityCount": unknown_raw,
        "rawRowsOmitted": raw_omitted, "rawSourceBytesCompared": compared_raw_bytes,
        "diagnosticCount": len(diagnostics), "unclassifiedReportedDiagnostics": unknown_diagnostics,
        "diagnosticsOmitted": omitted,
        "unclassifiedOmittedDiagnostics": None if omitted else 0,
        "historySemantics": "notEstablished", "sourceRecordDenominator": None,
        "sourceTopology": {
            "basis": "observed-typed-model-not-original-wire-records",
            "counts": source_counts, "geometryReachableCounts": reachable_counts,
            "expectedBoxCounts": expected_counts, "status": topology_status,
        },
    }
    return coverage, diagnostic_rows, raw_rows


def verify(sab: bytes, manifest: dict[str, Any], api: Any, api_types: Any,
           versions: dict[str, str], source_id: str) -> dict[str, Any]:
    require(type(sab) is bytes and len(sab) <= MAX_SAB_BYTES, "SAB byte budget exceeded")
    require(type(manifest) is dict, "manifest root must be a JSON object")
    require(sab.startswith(b"ACIS BinaryFile"), "input lacks the SAB signature")
    digest = hashlib.sha256(sab).hexdigest()
    expected_digest = text(manifest.get("sab_sha256"), "SAB SHA-256", maximum=64)
    require(len(expected_digest) == 64 and all(c in "0123456789abcdef" for c in expected_digest),
            "manifest SAB SHA-256 must be lowercase hexadecimal")
    require(digest == expected_digest, "SAB SHA-256 does not match the manifest")
    require(len(sab) == integer(manifest.get("sab_bytes"), "SAB length", maximum=MAX_SAB_BYTES),
            "SAB length does not match the manifest")
    expected_min = read_vec3(manifest.get("expected_min_acis_model_coordinates"), "manifest minimum")
    expected_max = read_vec3(manifest.get("expected_max_acis_model_coordinates"), "manifest maximum")
    require(all(low < high for low, high in zip(expected_min, expected_max)),
            "manifest box bounds must have positive dimensions")
    volume = finite_number(manifest.get("expected_volume_acis_model_units3"), "manifest volume", positive=True)
    area = finite_number(manifest.get("expected_surface_area_acis_model_units2"), "manifest area", positive=True)

    model = api.parse_sab_model(sab, source_id=source_id)
    header_units = model.metadata.units_mm
    known_units = header_units is not None
    units = finite_number(header_units, "ACIS header scale", positive=True) if known_units else None
    effective_units = units if units is not None else 1.0
    container = getattr(model.metadata.container, "value", model.metadata.container)
    save_version = model.metadata.save_version
    require(save_version is None or type(save_version) is int,
            "model save version must be an integer or unavailable")
    require(container is None or type(container) is str,
            "model container must be text or unavailable")
    for key in ("cq_acis", "cadquery", "occt"):
        text(versions.get(key), f"runtime {key}", maximum=128)
    profile_matched = versions["cq_acis"] == "0.3.8" and container == "sab" and save_version == 21800
    coverage, diagnostics, raw_rows = observe_model(model, api_types, sab, source_id, profile_matched)

    shapes = api.convert_model(model)
    require(len(shapes) == 1, f"expected one reconstructed shape, got {len(shapes)}")
    shape = shapes[0]
    solids, shells = shape.Solids(), shape.Shells()
    require(shape.ShapeType() == "Solid", "expected one Solid")
    valid = shape.isValid()
    require(type(valid) is bool and valid, "CadQuery/OCCT reports an invalid shape")
    require(len(solids) == 1 and len(shells) == 1, "expected one solid and one shell")
    closed = shells[0].Closed()
    require(type(closed) is bool and closed, "expected a closed boundary shell")
    faces, edges = shape.Faces(), shape.Edges()
    require(len(faces) == 6 and len(edges) == 12, "expected six box faces and twelve edges")
    bounds = shape.BoundingBox()
    actual_min = read_vec3((bounds.xmin, bounds.ymin, bounds.zmin), "observed minimum")
    actual_max = read_vec3((bounds.xmax, bounds.ymax, bounds.zmax), "observed maximum")
    expected_min_scaled = tuple(finite_number(x * effective_units, "scaled minimum") for x in expected_min)
    expected_max_scaled = tuple(finite_number(x * effective_units, "scaled maximum") for x in expected_max)
    require(close_values(actual_min, expected_min_scaled), "ACIS model-coordinate minimum differs")
    require(close_values(actual_max, expected_max_scaled), "ACIS model-coordinate maximum differs")
    expected_volume = finite_number(volume * effective_units * effective_units * effective_units,
                                    "scaled volume", positive=True)
    expected_area = finite_number(area * effective_units * effective_units, "scaled area", positive=True)
    actual_volume = finite_number(shape.Volume(), "observed volume", positive=True)
    actual_area = finite_number(shape.Area(), "observed area", positive=True)
    require(math.isclose(actual_volume, expected_volume, rel_tol=1e-8, abs_tol=1e-7), "volume differs")
    require(math.isclose(actual_area, expected_area, rel_tol=1e-8, abs_tol=1e-7), "area differs")

    blockers = ["locally-authored-not-source-native-dimensional-truth", "shared-OCCT-kernel",
                "no-DWG-ACDS-native-consumer-or-general-ACIS-qualification"]
    if not known_units:
        blockers.append("source-unit-scale-unavailable-compatibility-default-used")
    if coverage["status"] != "observedModelClassified":
        blockers.append("model-diagnostic-or-raw-coverage-unclassified")
    if coverage["sourceTopology"]["status"] != "matched":
        blockers.append("observed-source-topology-unavailable-or-not-box-profile")
    return {
        "schema": REPORT_SCHEMA, "schemaVersion": 2,
        "profile": {"id": PROFILE_ID, "sourceRevision": PROFILE_SOURCE,
                    "matched": profile_matched, "runtimeAttestation": "version-strings-only"},
        "result": "PASS", "geometryCheck": "passed",
        "geometryObservationEligible": (known_units
            and coverage["status"] == "observedModelClassified"
            and coverage["sourceTopology"]["status"] == "matched"),
        "qualificationEligible": False, "qualificationBlockers": blockers,
        "reader": "cq-acis Rust SAB parser + CadQuery/OCCT geometry adapter",
        "cq_acis_version": versions["cq_acis"], "cadquery_version": versions["cadquery"],
        "occt_binding_version": versions["occt"], "sab_sha256": digest, "sab_bytes": len(sab),
        "shape_type": "Solid", "valid": valid, "solid_count": len(solids),
        "closed_shell_count": 1, "faces": len(faces), "edges": len(edges),
        "acis_header_units_mm": units,
        "effective_acis_header_scale_mm_per_source_unit": effective_units,
        "sourceUnitEvidence": {"status": "parserHeaderDeclared" if known_units else "unavailable",
                               "conversionScaleBasis": "parserHeader" if known_units else "compatibilityDefault",
                               "independentlyEstablished": False, "drawingUnitTruth": "notEstablished"},
        "bounds_after_acis_header_scale_mm": actual_min + actual_max,
        "volume_after_acis_header_scale_mm3": actual_volume,
        "surface_area_after_acis_header_scale_mm2": actual_area,
        "coverage": coverage, "parser_diagnostics": diagnostics, "raw_entities": raw_rows,
        "qualification_limit": "Local box geometry and parser-scale arithmetic only; unknown/raw/history observations are not full semantics, independent units, DWG/ACDS or native-consumer evidence.",
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("sab", type=Path, nargs="?", help="SAB emitted by the local witness generator")
    parser.add_argument("manifest", type=Path, nargs="?", help="matching JSON manifest")
    parser.add_argument("--self-test", action="store_true", help="run dependency-free mocked contract controls")
    args = parser.parse_args()
    if args.self_test:
        if args.sab is not None or args.manifest is not None:
            parser.error("self-test does not accept drawing paths")
        self_test()
        return 0
    if args.sab is None or args.manifest is None:
        parser.error("SAB and manifest paths are required unless --self-test is used")
    try:
        manifest = strict_json_loads(bounded_read(args.manifest, MAX_MANIFEST_BYTES))
        sab = bounded_read(args.sab, MAX_SAB_BYTES)
        import cq_acis
        import cq_acis.model as model_types
        versions = {"cq_acis": importlib.metadata.version("cq-acis"),
                    "cadquery": importlib.metadata.version("cadquery"),
                    "occt": __import__("OCP").__version__}
        result = verify(sab, manifest, cq_acis, model_types, versions, args.sab.name)
        print(encode_report(result))
        return 0
    except (OSError, KeyError, TypeError, ValueError, ImportError, RuntimeError,
            AttributeError, OverflowError) as exc:
        print(f"ACIS witness verification failed: {str(exc)[:1024]}", file=sys.stderr)
        return 1


def self_test() -> None:
    """No parser/kernel executes: deliberately synthetic models test predicates."""
    import copy
    from dataclasses import dataclass
    import io
    import tempfile
    from types import SimpleNamespace
    from unittest.mock import patch

    @dataclass
    class EntityRef:
        index: int

    @dataclass
    class RawEntity:
        index: int
        type_name: str
        source: Any = None
        raw_data: Any = None

    @dataclass
    class BodyEntity:
        raw: Any
        lump: Any

        @property
        def index(self) -> int:
            return self.raw.index

    @dataclass
    class LumpEntity:
        raw: Any
        shell: Any
        body: Any

        @property
        def index(self) -> int:
            return self.raw.index

    @dataclass
    class ShellEntity:
        raw: Any
        face: Any
        lump: Any

        @property
        def index(self) -> int:
            return self.raw.index

    @dataclass
    class FaceEntity:
        raw: Any
        next_face: Any
        shell: Any

        @property
        def index(self) -> int:
            return self.raw.index

    class Shell:
        def Closed(self) -> bool:
            return True

    class Shape:
        factor = 1.0

        def Solids(self): return [self]
        def Shells(self): return [Shell()]
        def ShapeType(self): return "Solid"
        def isValid(self): return True
        def Faces(self): return [None] * 6
        def Edges(self): return [None] * 12
        def Volume(self): return 385.0 * self.factor**3
        def Area(self): return 334.0 * self.factor**2
        def BoundingBox(self):
            return SimpleNamespace(xmin=-2.5*self.factor, ymin=-3.5*self.factor,
                                   zmin=-5.5*self.factor, xmax=2.5*self.factor,
                                   ymax=3.5*self.factor, zmax=5.5*self.factor)

    sab = b"ACIS BinaryFile mocked-harness-contract-not-a-real-SAB"
    manifest = {"sab_sha256": hashlib.sha256(sab).hexdigest(), "sab_bytes": len(sab),
                "expected_min_acis_model_coordinates": [-2.5, -3.5, -5.5],
                "expected_max_acis_model_coordinates": [2.5, 3.5, 5.5],
                "expected_volume_acis_model_units3": 385.0,
                "expected_surface_area_acis_model_units2": 334.0}
    versions = {"cq_acis": "0.3.8", "cadquery": "mock", "occt": "mock"}
    types = SimpleNamespace(EntityRef=EntityRef, RawEntity=RawEntity, BodyEntity=BodyEntity,
                            LumpEntity=LumpEntity, ShellEntity=ShellEntity, FaceEntity=FaceEntity)
    baseline_entity_count = 10
    count = 0

    def span(start=0, end=4):
        return SimpleNamespace(source_id="mock.sab", start_offset=start, end_offset=end)

    def run(*, units=1.0, raw_type=None, code=None, referenced=False,
            shapes=1, factor=1.0, mutate=None, manifest_mutate=None):
        header = RawEntity(0, "asmheader", span(), sab[:4])
        body = BodyEntity(RawEntity(1, "body"), EntityRef(2))
        lump = LumpEntity(RawEntity(2, "lump"), EntityRef(3), EntityRef(1))
        shell = ShellEntity(RawEntity(3, "shell"), EntityRef(4), EntityRef(2))
        source_faces = [FaceEntity(RawEntity(index, "face"),
                                  EntityRef(index + 1 if index < 9 else -1), EntityRef(3))
                        for index in range(4, baseline_entity_count)]
        model = SimpleNamespace(metadata=SimpleNamespace(units_mm=units, container="sab", save_version=21800),
                                entities=[header, body, lump, shell, *source_faces], diagnostics=[])
        if raw_type is not None:
            extra_span = span(4, len(sab))
            model.entities.append(RawEntity(baseline_entity_count, raw_type, extra_span, sab[4:]))
            if referenced:
                body.lump = EntityRef(baseline_entity_count)
        if code is not None:
            target = baseline_entity_count if raw_type else None
            model.diagnostics.append(SimpleNamespace(code=code, message="mock diagnostic",
                                                      entity_index=target,
                                                      source=span(4, len(sab)) if raw_type else None))
        shape = Shape()
        shape.factor = factor
        context = SimpleNamespace(model=model, shape=shape, versions=versions.copy(),
                                  types=SimpleNamespace(**vars(types)))
        if mutate:
            mutate(context)
        source_manifest = copy.deepcopy(manifest)
        if manifest_mutate:
            manifest_mutate(source_manifest)
        api = SimpleNamespace(parse_sab_model=lambda data, source_id: context.model,
                              convert_model=lambda model: [context.shape] * shapes)
        report = verify(sab, source_manifest, api, context.types, context.versions, "mock.sab")
        encode_report(report)
        return report

    def reject(label, operation):
        nonlocal count
        try:
            operation()
        except VerificationError:
            count += 1
            return
        raise AssertionError(f"self-test accepted {label}")

    baseline = run()
    assert baseline["geometryObservationEligible"] and not baseline["qualificationEligible"]
    assert baseline["coverage"]["sourceTopology"]["counts"] == {
        "body": 1, "lump": 1, "shell": 1, "face": 6}
    assert baseline["coverage"]["sourceTopology"]["geometryReachableCounts"] == {
        "body": 1, "lump": 1, "shell": 1, "face": 6}
    assert baseline["coverage"]["sourceRecordDenominator"] is None
    count += 1
    missing = run(units=None)
    assert missing["acis_header_units_mm"] is None and missing["effective_acis_header_scale_mm_per_source_unit"] == 1.0
    assert missing["sourceUnitEvidence"]["status"] == "unavailable" and not missing["geometryObservationEligible"]
    count += 1
    for code in ("sab.entity_schema_unsupported", "future.loss.marker"):
        result = run(raw_type="unclassified-attribute", code=code)
        assert result["coverage"]["status"] == "unknown" and not result["geometryObservationEligible"]
        assert result["parser_diagnostics"][0]["entityIndex"] == baseline_entity_count
        assert result["parser_diagnostics"][0]["source"]["startOffset"] == 4
        count += 1
    result = run(raw_type="unclassified-attribute")
    assert result["coverage"]["unclassifiedRawEntityCount"] == 1
    count += 1
    run(units=25.4, factor=25.4)
    count += 1
    reject("mismatched scale", lambda: run(units=25.4))
    reject("extra returned shape", lambda: run(shapes=2))

    history = run(raw_type="__opaque_sab_history__", code="sab.history_opaque")
    assert history["coverage"]["status"] == "observedModelClassified"
    assert history["parser_diagnostics"][0]["disposition"] == "reviewedOpaqueHistoryGeometryAdvisory"
    assert history["coverage"]["historySemantics"] == "notEstablished"
    count += 1
    result = run(code="sab.raw_entities")
    assert result["parser_diagnostics"][0]["disposition"] == "reviewedNonGeometricSummary"
    count += 1
    reject("referenced raw geometry", lambda: run(raw_type="unknown-surface", referenced=True))
    reject("raw undecoded body", lambda: run(raw_type="body"))
    reject("missing source body", lambda: run(mutate=lambda c: c.model.entities.__setitem__(
        1, RawEntity(1, "not-a-body"))))
    for class_name, key in (("LumpEntity", "lump"), ("ShellEntity", "shell"), ("FaceEntity", "face")):
        result = run(mutate=lambda c, class_name=class_name: delattr(c.types, class_name))
        assert result["coverage"]["sourceTopology"]["counts"][key] is None
        assert result["coverage"]["sourceTopology"]["status"] == "unavailable"
        assert not result["geometryObservationEligible"]
        count += 1
    result = run(mutate=lambda c: setattr(c.types, "LumpEntity", 1))
    assert result["coverage"]["sourceTopology"]["counts"]["lump"] is None
    assert not result["geometryObservationEligible"]
    count += 1
    for extra, key in ((LumpEntity(RawEntity(baseline_entity_count, "lump"), EntityRef(3), EntityRef(1)), "lump"),
                       (ShellEntity(RawEntity(baseline_entity_count, "shell"), EntityRef(4), EntityRef(2)), "shell"),
                       (FaceEntity(RawEntity(baseline_entity_count, "face"), EntityRef(-1), EntityRef(3)), "face")):
        result = run(mutate=lambda c, extra=extra: c.model.entities.append(extra))
        assert result["geometryCheck"] == "passed" and result["faces"] == 6
        assert result["coverage"]["sourceTopology"]["counts"][key] == (7 if key == "face" else 2)
        assert result["coverage"]["sourceTopology"]["status"] == "mismatch"
        assert not result["geometryObservationEligible"]
        count += 1
    def remove_last_face(context):
        context.model.entities.pop()
        context.model.entities[-1].next_face = EntityRef(-1)
    result = run(mutate=remove_last_face)
    assert result["geometryCheck"] == "passed" and result["faces"] == 6
    assert result["coverage"]["sourceTopology"]["counts"]["face"] == 5
    assert result["coverage"]["sourceTopology"]["geometryReachableCounts"]["face"] == 5
    assert not result["geometryObservationEligible"]
    count += 1
    result = run(mutate=lambda c: c.versions.update(cq_acis="future"))
    assert not result["profile"]["matched"] and result["coverage"]["status"] == "unknown"
    count += 1
    result = run(mutate=lambda c: c.model.diagnostics.extend([
        SimpleNamespace(code="sab.raw_entities", message="x", entity_index=None, source=None)
    ] * (MAX_DIAGNOSTICS + 1)))
    assert result["coverage"]["diagnosticsOmitted"] == 1
    assert result["coverage"]["unclassifiedOmittedDiagnostics"] is None
    assert not result["geometryObservationEligible"]
    count += 1
    for value in (True, "1", float("nan"), float("inf"), 0.0, -1.0):
        reject("invalid header units", lambda value=value: run(units=value))
    for key, value in (("sab_bytes", True), ("sab_bytes", "51"),
                       ("expected_volume_acis_model_units3", True),
                       ("expected_surface_area_acis_model_units2", float("inf")),
                       ("expected_min_acis_model_coordinates", [False, -3.5, -5.5])):
        reject("manifest type/finite", lambda key=key, value=value: run(
            manifest_mutate=lambda m: m.update({key: value})))
    for mutate in (
        lambda c: setattr(c.shape, "Volume", lambda: True),
        lambda c: setattr(c.shape, "Area", lambda: float("nan")),
        lambda c: setattr(c.shape, "isValid", lambda: 1),
        lambda c: setattr(c.shape, "Shells", lambda: [SimpleNamespace(Closed=lambda: 1)]),
        lambda c: setattr(c.shape, "Faces", lambda: [None] * 5),
        lambda c: setattr(c.shape, "Edges", lambda: [None] * 13),
        lambda c: setattr(c.shape, "ShapeType", lambda: "Shell"),
        lambda c: setattr(c.shape, "BoundingBox", lambda: SimpleNamespace(
            xmin=float("nan"), ymin=-3.5, zmin=-5.5, xmax=2.5, ymax=3.5, zmax=5.5)),
        lambda c: setattr(c.model.metadata, "save_version", True),
        lambda c: setattr(c.model.entities[0].source, "start_offset", True),
        lambda c: setattr(c.model.entities[0].source, "source_id", "other.sab"),
        lambda c: setattr(c.model.entities[0].source, "end_offset", len(sab) + 1),
        lambda c: setattr(c.model.entities[0], "raw_data", sab + b"x"),
    ):
        reject("invalid observation type/domain", lambda mutate=mutate: run(mutate=mutate))
    reject("known loss on decoded geometry", lambda: run(code="sab.entity_schema_unsupported",
        mutate=lambda c: setattr(c.model.diagnostics[0], "entity_index", 1)))
    result = run(code="future.observation", mutate=lambda c: setattr(
        c.model.diagnostics[0], "entity_index", 1))
    assert result["coverage"]["status"] == "unknown"
    count += 1
    for mutate in (
        lambda c: setattr(c.model.diagnostics[0], "entity_index", True),
        lambda c: setattr(c.model.diagnostics[0], "entity_index", 100),
        lambda c: setattr(c.model.diagnostics[0], "message", 1),
        lambda c: setattr(c.model.diagnostics[0], "code", "x" * 129),
    ):
        reject("invalid diagnostic type", lambda mutate=mutate: run(code="future.observation", mutate=mutate))
    result = run(code="sab.raw_entities", mutate=lambda c: setattr(
        c.model.diagnostics[0], "message", "x" * (MAX_TEXT_CHARS + 1)))
    assert result["parser_diagnostics"][0]["messageTruncated"]
    assert result["parser_diagnostics"][0]["messageCharacters"] == MAX_TEXT_CHARS + 1
    count += 1
    result = run(mutate=lambda c: c.model.entities.extend([
        RawEntity(index + baseline_entity_count, "unclassified", span(4, 4), b"")
        for index in range(MAX_RAW_ROWS + 1)]))
    assert len(result["raw_entities"]) == MAX_RAW_ROWS and result["coverage"]["rawRowsOmitted"] == 2
    assert not result["geometryObservationEligible"]
    count += 1
    reject("model inventory limit", lambda: run(mutate=lambda c: c.model.entities.extend(
        [RawEntity(index + baseline_entity_count, "unclassified") for index in range(MAX_ENTITIES)])))
    class InputStream(io.BytesIO):
        def fileno(self): return 42

    def read_input(data, regular=True):
        stream = InputStream(data)
        path = object()
        with patch.object(os, "name", "posix"), \
                patch.object(os, "O_NONBLOCK", 0x800, create=True), \
                patch.object(os, "open", return_value=42) as opened, \
                patch.object(os, "fdopen", return_value=stream), \
                patch.object(os, "close") as closed, \
                patch.object(os, "fstat", return_value=SimpleNamespace(
                    st_mode=stat.S_IFREG if regular else stat.S_IFIFO)):
            try:
                return bounded_read(path, 4)
            finally:
                opened.assert_called_once_with(path, os.O_RDONLY | os.O_NONBLOCK)
                closed.assert_called_once_with(42)

    for size in (3, 4):
        assert read_input(b"x" * size) == b"x" * size
        count += 1
    reject("input byte limit", lambda: read_input(b"x" * 5))
    reject("nonregular input", lambda: read_input(b"x", regular=False))
    with patch.object(os, "name", "nt"):
        reject("unreviewed platform admission", lambda: bounded_read(None, 4))
    with patch.object(os, "name", "posix"), \
            patch.object(os, "O_NONBLOCK", create=True):
        # Removing, rather than guessing, the primitive exercises capability
        # absence independently of the platform name.
        del os.O_NONBLOCK
        reject("missing nonblocking primitive", lambda: bounded_read(None, 4))
    for failed_operation in ("fstat", "fdopen", "read"):
        stream = InputStream(b"xxxx")
        owner = stream if failed_operation == "read" else os
        with patch.object(os, "name", "posix"), \
                patch.object(os, "O_NONBLOCK", 0x800, create=True), \
                patch.object(os, "open", return_value=42), \
                patch.object(os, "fstat", return_value=SimpleNamespace(st_mode=stat.S_IFREG)), \
                patch.object(os, "fdopen", return_value=stream), \
                patch.object(os, "close") as closed, \
                patch.object(owner, failed_operation, side_effect=OSError("mock failure")) as failed:
            try:
                bounded_read(object(), 4)
            except OSError:
                count += 1
            else:
                raise AssertionError("self-test swallowed descriptor setup failure")
            failed.assert_called_once()
            closed.assert_called_once_with(42)
    if os.name == "posix" and hasattr(os, "O_NONBLOCK"):
        # Isolated Python omits the script directory from sys.path. Load only
        # this trusted sibling for self-test subprocess controls; do not alter
        # normal verification imports or weaken the interpreter's isolation.
        from importlib.util import module_from_spec, spec_from_file_location
        helper_spec = spec_from_file_location(
            "_libdxfrw_acis_self_test_process",
            Path(__file__).resolve().with_name("bounded_process.py"))
        require(helper_spec is not None and helper_spec.loader is not None,
                "self-test process helper is unavailable")
        process_helper = module_from_spec(helper_spec)
        helper_spec.loader.exec_module(process_helper)
        run_bounded = process_helper.run_bounded
        with tempfile.TemporaryDirectory(prefix="libdxfrw-acis-input-") as temporary:
            directory = Path(temporary)
            regular = directory / "regular"
            original_open = os.open

            def checked_file(path, limit):
                descriptors = []
                def capture_open(*args, **kwargs):
                    descriptor = original_open(*args, **kwargs)
                    descriptors.append(descriptor)
                    return descriptor
                try:
                    with patch.object(os, "open", side_effect=capture_open):
                        return bounded_read(path, limit)
                finally:
                    assert len(descriptors) == 1
                    try:
                        os.fstat(descriptors[0])
                    except OSError:
                        pass
                    else:
                        raise AssertionError("self-test leaked an input descriptor")

            for size in (3, 4):
                regular.write_bytes(b"x" * size)
                assert checked_file(regular, 4) == b"x" * size
                count += 1
            regular.write_bytes(b"x" * 5)
            reject("actual regular limit plus one", lambda: checked_file(regular, 4))
            regular.write_bytes(b"x" * 4)
            linked_regular = directory / "linked-regular"
            linked_regular.symlink_to(regular)
            assert checked_file(linked_regular, 4) == b"x" * 4
            count += 1
            if hasattr(os, "mkfifo"):
                fifo = directory / "fifo"
                os.mkfifo(fifo)
                linked_fifo = directory / "linked-fifo"
                linked_fifo.symlink_to(fifo)
                for path in (fifo, linked_fifo):
                    result = run_bounded(
                        [sys.executable, "-B", str(Path(__file__).resolve()),
                         str(regular), str(path)], timeout=2, output_limit=16384,
                    )
                    assert result.returncode == 1 and b"must be regular files" in result.stderr
                    count += 1
    for data in (b'{"x":1,"x":2}', b'{"x":NaN}', b'{"x":1e999}', b'[]', b'\xff',
                 b'{"x":"\\ud800"}', b'{"\\udfff":1}', b"x" * (MAX_MANIFEST_BYTES + 1)):
        reject("non-strict JSON", lambda data=data: strict_json_loads(data))
    reject("report byte budget", lambda: encode_report({"text": "x" * MAX_REPORT_BYTES}))
    reject("short-vector equality", lambda: require(close_values((1.0,), (1.0, 2.0)), "length"))
    print(encode_report({"schema": REPORT_SCHEMA, "result": "PASS", "selfTestControls": count,
                         "evidenceKind": "contract-controls-no-parser-or-kernel-executed",
                         "qualificationEligible": False}))


if __name__ == "__main__":
    raise SystemExit(main())
