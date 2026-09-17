#!/usr/bin/env python3
"""Validate the frozen AC1024 successor candidate-field contract.

The v2 contract is a pre-oracle successor to the immutable v1 probe. It reuses
v1's pinned generator, LibreDWG, J360, and discrepancy contracts while replacing
only the four predeclared RTEXT claims corrected by v1 evidence. Generated DWG
and oracle JSON payloads remain private temporary data and are never written to
the repository or to an evidence report.
"""

from __future__ import annotations

import argparse
import contextlib
import copy
import hashlib
import importlib.util
import io
import json
import math
import re
import sys
import tempfile
from pathlib import Path
from typing import Any


sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parent.parent
DEFAULT_MANIFEST = ROOT / "metadata/ac1024-candidate-fields-v2.json"
FROZEN_MANIFEST_SHA256 = (
    "7b7baf1351e418cc2aa91faf4f4750e9eeeeff31a03518571e5be05e59f667dd"
)
PREDECESSOR_MANIFEST_SHA256 = (
    "e9f6d18df974e7cbc23701ad906fe33e4ca6542fa24861bef9b5d86a7a4764f0"
)
PREDECESSOR_CHECKER_SHA256 = (
    "77b461e663a70ae29d632658636b28bf119132f085f6d14d1af9daf20f474c35"
)
PREDECESSOR_EVIDENCE_BINDING_SHA256 = (
    "fc07381ab378229952861c6d46dc6b186f9b5c5e214d79174d23b8ebd8371aef"
)
RESOLVED_CLAIM_SET_SHA256 = (
    "27f326633f773d4cd6337101359249563d3762475da593947440c84a09994dfd"
)
EXPECTED_OVERRIDE_IDS = (
    "rtext-insertion-x",
    "rtext-insertion-y",
    "rtext-insertion-z",
    "rtext-rotation",
)
EXPECTED_INHERITED_IDS = (
    "line-record-class",
    "line-start-x",
    "line-start-y",
    "line-start-z",
    "line-end-x",
    "line-end-y",
    "line-end-z",
    "rtext-record-class",
    "rtext-text",
    "rtext-flags",
    "rtext-extrusion-z",
    "rtext-height",
)
EXPECTED_GENERATED_INPUT_SHA256 = (
    "6acb7d4a0289d200d3dd448abc77082250f1678ec2ad815870e4617fa16abffd"
)
EXPECTED_ORACLE_JSON_SHA256 = (
    "e5e7a39ccf2d005c117660f289ef1bd3c9339f2a1ba49657e1925aa8026b37b5"
)
EXPECTED_ORACLE_DEPENDENCY_DIGEST = (
    "0ff71eff96ce45bd0fbbcc760741225936096437e3145f577c84e9408aab4af2"
)
EXPECTED_ORACLE_CLOSURE_DIGEST = (
    "4170272163a077731dad3b4d77602dbf38d32f3232c877b135d6b67e0895f1b0"
)
EXPECTED_DISCREPANCY_SET_SHA256 = (
    "ff6767deea5cb26064fc8949daf2bd4ec52ab8ae511b9c639acec00f9627400a"
)
FIXED_ROTATION_TOLERANCE_DEGREES = 1e-12


class SuccessorError(RuntimeError):
    """A fail-closed successor qualification error."""


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SuccessorError(message)


def sha256_bytes(value: bytes) -> str:
    return hashlib.sha256(value).hexdigest()


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def canonical_bytes(value: Any) -> bytes:
    return json.dumps(
        value, sort_keys=True, separators=(",", ":"), ensure_ascii=True,
        allow_nan=False,
    ).encode("utf-8")


def strict_json_loads(text: str, label: str) -> Any:
    def object_pairs(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
        result: dict[str, Any] = {}
        for key, value in pairs:
            if key in result:
                raise SuccessorError(f"duplicate JSON key in {label}: {key}")
            result[key] = value
        return result

    def reject_constant(token: str) -> Any:
        raise SuccessorError(f"non-finite JSON value in {label}: {token}")

    def finite_float(token: str) -> float:
        value = float(token)
        if not math.isfinite(value):
            raise SuccessorError(f"non-finite JSON number in {label}: {token}")
        return value

    try:
        return json.loads(
            text, object_pairs_hook=object_pairs,
            parse_constant=reject_constant, parse_float=finite_float,
        )
    except json.JSONDecodeError as exc:
        raise SuccessorError(f"invalid JSON in {label}: {exc}") from exc


def read_json(path: Path) -> dict[str, Any]:
    try:
        value = strict_json_loads(path.read_text(encoding="utf-8"), str(path))
    except (OSError, UnicodeError) as exc:
        raise SuccessorError(f"cannot read JSON {path}: {exc}") from exc
    require(isinstance(value, dict), f"JSON root is not an object: {path}")
    return value


def load_module(name: str, path: Path) -> Any:
    spec = importlib.util.spec_from_file_location(name, path)
    require(spec is not None and spec.loader is not None,
            f"cannot load module: {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def load_predecessor_checker() -> Any:
    path = ROOT / "tools/check_ac1024_candidate_fields.py"
    require(path.is_file(), "immutable v1 checker is absent")
    require(sha256_file(path) == PREDECESSOR_CHECKER_SHA256,
            "immutable v1 checker digest changed")
    return load_module("ac1024_candidate_fields_v1", path)


def validate_predecessor_evidence(manifest: dict[str, Any]) -> None:
    predecessor = manifest.get("predecessor")
    require(isinstance(predecessor, dict), "v1 predecessor binding is missing")
    require(predecessor.get("recordedByCommit") ==
            "b957ab0adcec2eb60b39394525616bb9d8d3ec9c",
            "v1 evidence commit changed")
    predecessor_manifest = predecessor.get("manifest")
    predecessor_checker = predecessor.get("checker")
    require(predecessor_manifest == {
        "path": "metadata/ac1024-candidate-fields-v1.json",
        "sha256": PREDECESSOR_MANIFEST_SHA256,
    }, "v1 manifest binding changed")
    require(predecessor_checker == {
        "path": "tools/check_ac1024_candidate_fields.py",
        "sha256": PREDECESSOR_CHECKER_SHA256,
    }, "v1 checker binding changed")
    for item, label in ((predecessor_manifest, "manifest"),
                        (predecessor_checker, "checker")):
        path = ROOT / item["path"]
        require(path.is_file() and sha256_file(path) == item["sha256"],
                f"v1 {label} artifact changed")

    evidence = predecessor.get("evidence")
    require(isinstance(evidence, dict), "v1 evidence binding is missing")
    require(evidence.get("bindingAlgorithm") ==
            "sha256(canonical-json(evidenceCore))",
            "v1 evidence binding algorithm changed")
    core = evidence.get("evidenceCore")
    require(isinstance(core, dict), "v1 bounded evidence core is missing")
    observed_binding = sha256_bytes(canonical_bytes(core))
    require(evidence.get("bindingSha256") ==
            PREDECESSOR_EVIDENCE_BINDING_SHA256
            and observed_binding == PREDECESSOR_EVIDENCE_BINDING_SHA256,
            "v1 bounded evidence digest changed")
    require(core.get("status") == "new-child-required"
            and core.get("promotesSupport") is False
            and core.get("generatedPayloadsRetained") is False,
            "v1 evidence disposition changed")
    require(core.get("generatedInputSha256") ==
            EXPECTED_GENERATED_INPUT_SHA256,
            "v1 generated input binding changed")
    require(core.get("oracleDiscrepancyCount") == 34,
            "v1 oracle discrepancy count changed")
    require(core.get("claimOutcomeCounts") == {
        "validated": 12, "excluded": 4,
    }, "v1 candidate outcome counts changed")
    require(core.get("excludedClaimIds") == list(EXPECTED_OVERRIDE_IDS),
            "v1 excluded claim IDs changed")
    require(core.get("j360") == {
        "comparisonStatus": "mismatch",
        "exactCount": 17304,
        "mismatchCount": 10,
        "bindingSha256":
            "cc08cc93aaa46bdcff05e54454daaade43931e8da7644b424594de278b69b63d",
    }, "v1 J360 evidence binding changed")
    bounded = core.get("boundedObservations")
    require(isinstance(bounded, dict), "v1 bounded observations are missing")
    for name in ("oracleInsertion", "semanticInsertion",
                 "oracleRotation", "semanticRotation"):
        row = bounded.get(name)
        require(isinstance(row, dict), f"v1 {name} observation is missing")
        if name == "oracleInsertion":
            value = row.get("value")
        elif name == "semanticInsertion":
            require(row.get("targetValue") == row.get("standaloneValue"),
                    "v1 semantic insertion sides changed")
            value = row.get("targetValue")
        elif name == "oracleRotation":
            value = row.get("value")
        else:
            require(row.get("targetValue") == row.get("standaloneValue"),
                    "v1 semantic rotation sides changed")
            value = row.get("targetValue")
        require(sha256_bytes(canonical_bytes(value)) ==
                row.get("canonicalValueSha256"),
                f"v1 {name} bounded value digest changed")


def validate_normalization(manifest: dict[str, Any]) -> None:
    definitions = manifest.get("normalizationDefinitions")
    require(isinstance(definitions, dict)
            and set(definitions) == {"degreesFromRadians"},
            "successor normalization set changed")
    definition = definitions["degreesFromRadians"]
    require(definition.get("semanticUnit") == "degrees"
            and definition.get("oracleUnit") == "radians",
            "degreesFromRadians unit contract changed")
    require(definition.get("transform") ==
            "oracleRadians * 180.0 / pi"
            and definition.get("comparison") ==
            "absolute-difference-in-degrees",
            "degreesFromRadians operation changed")
    tolerance = definition.get("fixedToleranceDegrees")
    require(tolerance == FIXED_ROTATION_TOLERANCE_DEGREES,
            "degreesFromRadians fixed tolerance changed")
    justification = definition.get("justification")
    require(isinstance(justification, dict),
            "degreesFromRadians justification is missing")
    quantum = justification.get("oracleJsonDecimalQuantumRadians")
    ulp_count = justification.get("binary64UlpBudgetAt15Degrees")
    require(quantum == 1e-14 and ulp_count == 16,
            "degreesFromRadians error-budget inputs changed")
    half_quantum_degrees = 0.5 * quantum * 180.0 / math.pi
    binary64_budget = ulp_count * math.ulp(15.0)
    derived_bound = half_quantum_degrees + binary64_budget
    require(justification.get("halfQuantumConvertedToDegrees") ==
            half_quantum_degrees,
            "degreesFromRadians decimal-quantization bound changed")
    require(justification.get("binary64BudgetDegrees") == binary64_budget,
            "degreesFromRadians binary64 bound changed")
    require(justification.get("derivedUpperBoundDegrees") == derived_bound,
            "degreesFromRadians derived bound changed")
    require(justification.get("minimumSafetyFactor") == 3.0
            and justification.get("actualSafetyFactor") ==
            tolerance / derived_bound
            and tolerance >= 3.0 * derived_bound
            and tolerance < 1e-9,
            "degreesFromRadians fixed tolerance lacks its frozen safety bound")

    evidence_rotation = manifest["predecessor"]["evidence"][
        "evidenceCore"]["boundedObservations"]["oracleRotation"]["value"]
    fractional = format(evidence_rotation, ".14f").partition(".")[2]
    require(len(fractional) == 14
            and float(format(evidence_rotation, ".14f")) == evidence_rotation,
            "predecessor rotation does not justify the frozen JSON quantum")

    sources = definition.get("sourceBindings")
    require(isinstance(sources, list) and len(sources) == 4,
            "degreesFromRadians source bindings changed")
    expected_paths = {
        "tests/dwg_local_roundtrip_tests.cpp",
        "src/drw_entities.cpp",
        "src/drw_base.h",
        "tests/semantic_differential_adapter.cpp",
    }
    require({row.get("path") for row in sources
             if isinstance(row, dict)} == expected_paths,
            "degreesFromRadians source-binding membership changed")
    for row in sources:
        require(isinstance(row, dict)
                and isinstance(row.get("symbol"), str)
                and isinstance(row.get("fact"), str),
                "degreesFromRadians source binding is malformed")
        source = ROOT / row["path"]
        require(source.is_file() and sha256_file(source) == row.get("sha256"),
                f"degreesFromRadians source changed: {row['path']}")


def resolve_contract(manifest: dict[str, Any], base: Any
                     ) -> tuple[dict[str, Any], dict[str, Any]]:
    predecessor_path = ROOT / manifest["predecessor"]["manifest"]["path"]
    predecessor = base.validate_frozen_manifest(predecessor_path)
    reuse = manifest.get("contractReuse")
    require(isinstance(reuse, dict), "v2 contract reuse declaration missing")
    expected_keys = [
        "frozenContract", "oracle", "sharedDifferential", "records",
        "discrepancyBinding",
    ]
    require(reuse.get("mode") == "copy-v1-and-apply-exact-claim-overrides"
            and reuse.get("inheritedTopLevelKeys") == expected_keys,
            "v2 inheritance contract changed")
    require(reuse.get("expectedInheritedClaimIds") ==
            list(EXPECTED_INHERITED_IDS)
            and reuse.get("requiredOverrideClaimIds") ==
            list(EXPECTED_OVERRIDE_IDS)
            and reuse.get("expectedTotalClaimCount") == 16,
            "v2 claim-set declaration changed")

    overrides = manifest.get("claimOverrides")
    require(isinstance(overrides, list) and len(overrides) == 4,
            "v2 must have exactly four claim overrides")
    override_map = {
        row.get("id"): row for row in overrides if isinstance(row, dict)
    }
    require(tuple(override_map) == EXPECTED_OVERRIDE_IDS,
            "v2 claim override order or membership changed")
    require(len(override_map) == len(overrides),
            "v2 claim override IDs are duplicated")

    claims = []
    inherited_ids = []
    for claim in predecessor["claims"]:
        claim_id = claim["id"]
        if claim_id in override_map:
            claims.append(copy.deepcopy(override_map[claim_id]))
        else:
            inherited_ids.append(claim_id)
            claims.append(copy.deepcopy(claim))
    require(tuple(inherited_ids) == EXPECTED_INHERITED_IDS,
            "v2 inherited claim order or membership changed")
    require(sha256_bytes(canonical_bytes(claims)) ==
            RESOLVED_CLAIM_SET_SHA256
            and reuse.get("resolvedClaimSetSha256") ==
            RESOLVED_CLAIM_SET_SHA256,
            "v2 resolved claim-set digest changed")

    for index, axis in enumerate(("x", "y", "z")):
        claim = override_map[f"rtext-insertion-{axis}"]
        require(claim.get("recordId") == "rtext-ed00"
                and claim.get("semanticField") == f"insertion.{axis}"
                and claim.get("oracleField") == f"pt[{index}]"
                and claim.get("semanticUnit") == "drawing-coordinate"
                and claim.get("oracleUnit") == "drawing-coordinate"
                and claim.get("normalization") == "finiteNumber"
                and claim.get("tolerance") == 0.0,
                f"v2 RTEXT insertion-{axis} mapping changed")
    rotation = override_map["rtext-rotation"]
    require(rotation.get("recordId") == "rtext-ed00"
            and rotation.get("semanticField") == "rotation"
            and rotation.get("oracleField") == "rotation"
            and rotation.get("semanticUnit") == "degrees"
            and rotation.get("oracleUnit") == "radians"
            and rotation.get("normalization") == "degreesFromRadians"
            and rotation.get("tolerance") ==
            FIXED_ROTATION_TOLERANCE_DEGREES
            and rotation.get("expectedSemantic") == 15.0
            and rotation.get("expectedOracle") == math.radians(15.0),
            "v2 RTEXT rotation mapping changed")
    require(abs(math.degrees(rotation["expectedOracle"])
                - rotation["expectedSemantic"])
            <= FIXED_ROTATION_TOLERANCE_DEGREES,
            "v2 RTEXT expected rotation units are inconsistent")

    resolved = copy.deepcopy(predecessor)
    for key in expected_keys:
        require(resolved[key] == predecessor[key],
                f"v2 mutated inherited contract key: {key}")
    resolved["claims"] = claims
    return predecessor, resolved


def validate_contract_data(manifest: dict[str, Any], base: Any
                           ) -> tuple[dict[str, Any], dict[str, Any]]:
    require(manifest.get("schema") == 2,
            "successor manifest schema is not 2")
    require(manifest.get("kind") ==
            "libdxfrw-local-ac1024-independent-candidate-fields-successor",
            "successor manifest kind changed")
    require(manifest.get("status") == "EXPERIMENTAL"
            and manifest.get("frozenBeforeOracle") is True,
            "successor manifest is not frozen/experimental")
    require(manifest.get("fixturePolicy") ==
            "local-from-scratch-runtime-only; generated-dwg-dxf-and-json-temporary; no-generated-payloads",
            "successor fixture policy changed")
    boundary = manifest.get("promotionBoundary")
    require(isinstance(boundary, dict)
            and boundary.get("promotesSupport") is False
            and boundary.get("selfReadDisposition") == "reachability-only"
            and boundary.get("unlistedFieldDisposition") == "excluded"
            and boundary.get("wholeFixtureClaimsForbidden") is True
            and boundary.get("wholeVersionClaimsForbidden") is True,
            "successor promotion boundary changed")
    validate_predecessor_evidence(manifest)
    validate_normalization(manifest)
    return resolve_contract(manifest, base)


def validate_frozen_manifest(path: Path, base: Any
                             ) -> tuple[dict[str, Any], dict[str, Any], dict[str, Any]]:
    require(path.resolve() == DEFAULT_MANIFEST.resolve(),
            "only the repository frozen v2 candidate manifest is admitted")
    actual_digest = sha256_file(path)
    require(actual_digest == FROZEN_MANIFEST_SHA256,
            "v2 candidate manifest changed after its pre-oracle freeze: "
            f"expected {FROZEN_MANIFEST_SHA256}, got {actual_digest}")
    manifest = read_json(path)
    predecessor, resolved = validate_contract_data(manifest, base)
    return manifest, predecessor, resolved


def finite_number(value: Any) -> bool:
    return (isinstance(value, (int, float)) and not isinstance(value, bool)
            and math.isfinite(float(value)))


def degree_checks(claim: dict[str, Any], target: Any, standalone: Any,
                  oracle: Any) -> tuple[dict[str, bool], float | None]:
    if not all(finite_number(value)
               for value in (target, standalone, oracle)):
        return ({
            "targetExpected": False,
            "standaloneExpected": False,
            "oracleRawExpected": False,
            "oracleExpected": False,
            "targetStandaloneSame": False,
            "targetOracleSame": False,
            "standaloneOracleSame": False,
        }, None)
    tolerance = float(claim["tolerance"])
    expected_semantic = float(claim["expectedSemantic"])
    expected_oracle = float(claim["expectedOracle"])
    target_degrees = float(target)
    standalone_degrees = float(standalone)
    oracle_radians = float(oracle)
    oracle_degrees = math.degrees(oracle_radians)
    raw_tolerance = math.radians(tolerance)
    checks = {
        "targetExpected": abs(target_degrees - expected_semantic) <= tolerance,
        "standaloneExpected": (
            abs(standalone_degrees - expected_semantic) <= tolerance),
        "oracleRawExpected": (
            abs(oracle_radians - expected_oracle) <= raw_tolerance),
        "oracleExpected": abs(oracle_degrees - expected_semantic) <= tolerance,
        "targetStandaloneSame": (
            abs(target_degrees - standalone_degrees) <= tolerance),
        "targetOracleSame": abs(target_degrees - oracle_degrees) <= tolerance,
        "standaloneOracleSame": (
            abs(standalone_degrees - oracle_degrees) <= tolerance),
    }
    return checks, oracle_degrees


def validate_claims(resolved: dict[str, Any], oracle_payload: dict[str, Any],
                    target: dict[str, Any], standalone: dict[str, Any],
                    oracle_module: Any, base: Any) -> list[dict[str, Any]]:
    record_specs = {row["id"]: row for row in resolved["records"]}
    selected: dict[str, dict[str, Any]] = {}
    for record_id, spec in record_specs.items():
        selected[record_id] = {}
        selectors = (
            ("target", lambda: base.find_semantic_record(
                target, spec["semantic"])),
            ("standalone", lambda: base.find_semantic_record(
                standalone, spec["semantic"])),
            ("oracle", lambda: base.find_oracle_record(
                oracle_payload, spec["oracle"], oracle_module)),
        )
        for side, selector in selectors:
            try:
                selected[record_id][side] = selector()
            except base.CandidateError as exc:
                selected[record_id][side] = exc

    rows = []
    for claim in resolved["claims"]:
        records = selected[claim["recordId"]]
        errors = {
            side: str(value) for side, value in records.items()
            if isinstance(value, base.CandidateError)
        }
        try:
            if errors:
                raise SuccessorError("; ".join(
                    f"{side}: {message}"
                    for side, message in sorted(errors.items())))
            target_value = base.semantic_value(
                records["target"], claim["semanticField"])
            standalone_value = base.semantic_value(
                records["standalone"], claim["semanticField"])
            oracle_value = base.nested_value(
                records["oracle"], claim["oracleField"])
        except (SuccessorError, base.CandidateError) as exc:
            rows.append({
                "id": claim["id"],
                "recordId": claim["recordId"],
                "semanticField": claim["semanticField"],
                "oracleField": claim["oracleField"],
                "normalization": claim["normalization"],
                "tolerance": claim["tolerance"],
                "outcome": "excluded",
                "checks": {},
                "observedDigests": {
                    "target": None, "standalone": None, "oracle": None,
                },
                "reason": "predeclared-v2-field-unavailable: " + str(exc),
            })
            continue

        if claim["normalization"] == "degreesFromRadians":
            checks, normalized_oracle = degree_checks(
                claim, target_value, standalone_value, oracle_value)
        else:
            normalization = claim["normalization"]
            tolerance = claim["tolerance"]
            checks = {
                "targetExpected": base.values_equal(
                    target_value, claim["expectedSemantic"],
                    normalization, tolerance),
                "standaloneExpected": base.values_equal(
                    standalone_value, claim["expectedSemantic"],
                    normalization, tolerance),
                "oracleExpected": base.values_equal(
                    oracle_value, claim["expectedOracle"],
                    normalization, tolerance),
                "targetStandaloneSame": base.values_equal(
                    target_value, standalone_value, normalization, tolerance),
            }
            normalized_oracle = None
        valid = all(checks.values())
        observed = {
            "target": sha256_bytes(canonical_bytes(target_value)),
            "standalone": sha256_bytes(canonical_bytes(standalone_value)),
            "oracle": sha256_bytes(canonical_bytes(oracle_value)),
        }
        if normalized_oracle is not None:
            observed["oracleDegrees"] = sha256_bytes(
                canonical_bytes(normalized_oracle))
        rows.append({
            "id": claim["id"],
            "recordId": claim["recordId"],
            "semanticField": claim["semanticField"],
            "oracleField": claim["oracleField"],
            "semanticUnit": claim.get("semanticUnit"),
            "oracleUnit": claim.get("oracleUnit"),
            "normalization": claim["normalization"],
            "tolerance": claim["tolerance"],
            "outcome": "validated" if valid else "excluded",
            "checks": checks,
            "observedDigests": observed,
            "reason": (None if valid else
                       "predeclared-v2-field-value-not-independently-validated"),
        })
    return rows


def successor_condition(claims: list[dict[str, Any]]) -> dict[str, Any] | None:
    failures = [row for row in claims if row.get("outcome") != "validated"]
    if not failures:
        return None
    core = [{
        "id": row.get("id"),
        "checks": row.get("checks"),
        "reason": row.get("reason"),
        "observedDigests": row.get("observedDigests"),
    } for row in failures]
    return {
        "required": True,
        "reason": "predeclared-v2-claim-validation-failed",
        "failedClaimIds": [row["id"] for row in failures],
        "failureBindingSha256": sha256_bytes(canonical_bytes(core)),
        "disposition": "freeze-a-new-successor-before-any-retry",
    }


def validate_live_oracle_bindings(provenance: dict[str, Any],
                                  discrepancies: list[dict[str, Any]]) -> str:
    require(provenance.get("jsonSha256") == EXPECTED_ORACLE_JSON_SHA256,
            "successor oracle full-JSON digest changed")
    require(provenance.get("oracleDependencyDigest") ==
            EXPECTED_ORACLE_DEPENDENCY_DIGEST,
            "successor oracle dependency digest changed")
    require(provenance.get("transitiveClosureDigest") ==
            EXPECTED_ORACLE_CLOSURE_DIGEST,
            "successor oracle closure digest changed")
    discrepancy_digest = sha256_bytes(canonical_bytes(discrepancies))
    require(discrepancy_digest == EXPECTED_DISCREPANCY_SET_SHA256,
            "successor oracle discrepancy set changed")
    return discrepancy_digest


def run(args: argparse.Namespace, base: Any) -> dict[str, Any]:
    manifest_path = args.manifest.resolve()
    manifest, predecessor, resolved = validate_frozen_manifest(
        manifest_path, base)
    oracle_module = base.load_module(
        "local_dwg_object_oracle_v2",
        ROOT / resolved["frozenContract"]["sourceOnlyOracleContract"]["path"])
    with tempfile.TemporaryDirectory(
            prefix="libdxfrw-ac1024-candidates-v2-") as temp:
        private_root = Path(temp)
        generated, generation = base.generate_ac1024(
            args.writer.resolve(), private_root, resolved, args.timeout)
        require(generation["generatedSha256"] ==
                EXPECTED_GENERATED_INPUT_SHA256,
                "successor generator did not reproduce the v1 input bytes")
        oracle_payload, oracle_provenance, discrepancy_messages = (
            base.inspect_oracle(
                args.oracle.resolve(), generated, resolved,
                oracle_module, args.timeout))
        try:
            target, standalone, j360_report = base.run_j360_pair(
                generated, resolved, args.target_receipt.resolve(),
                args.standalone_receipt.resolve(), args.timeout)
        except Exception as exc:
            if exc.__class__.__name__ == "QualifiedDifferentialError":
                raise SuccessorError(
                    f"J360 rejected the successor evidence: {exc}") from exc
            raise
        qd = base.load_module(
            "qualified_differential_v2_successor_binding",
            ROOT / resolved["sharedDifferential"]["runnerSource"])
        j360_binding = base.validate_j360_evidence(
            target, standalone, j360_report,
            generation["generatedSha256"], qd)
        claims = validate_claims(
            resolved, oracle_payload, target, standalone,
            oracle_module, base)
        discrepancies = base.bind_discrepancies(
            discrepancy_messages,
            oracle_provenance["oracleDependencyDigest"],
            resolved["discrepancyBinding"]["expectedCurrentRunCount"])
        discrepancy_digest = validate_live_oracle_bindings(
            oracle_provenance, discrepancies)

    condition = successor_condition(claims)
    excluded = [row["id"] for row in claims
                if row["outcome"] != "validated"]
    return {
        "schema": 2,
        "kind": "libdxfrw-local-ac1024-candidate-evidence-successor",
        "status": "complete" if condition is None else "new-successor-required",
        "promotesSupport": False,
        "manifest": {
            "path": str(manifest_path.relative_to(ROOT)),
            "sha256": FROZEN_MANIFEST_SHA256,
            "predeclaredClaimCount": len(resolved["claims"]),
            "resolvedClaimSetSha256": RESOLVED_CLAIM_SET_SHA256,
            "unlistedFieldDisposition": "excluded",
        },
        "predecessorEvidence": {
            "manifestSha256": PREDECESSOR_MANIFEST_SHA256,
            "checkerSha256": PREDECESSOR_CHECKER_SHA256,
            "evidenceBindingSha256": PREDECESSOR_EVIDENCE_BINDING_SHA256,
            "recordedByCommit": manifest["predecessor"]["recordedByCommit"],
            "priorStatus": manifest["predecessor"]["evidence"][
                "evidenceCore"]["status"],
        },
        "normalization": {
            "degreesFromRadians": manifest["normalizationDefinitions"][
                "degreesFromRadians"],
            "definitionSha256": sha256_bytes(canonical_bytes(
                manifest["normalizationDefinitions"]["degreesFromRadians"])),
        },
        "generation": generation,
        "oracleProvenance": oracle_provenance,
        "j360": {
            "sameInputSha256": generation["generatedSha256"],
            "manifest": {
                "path": resolved["sharedDifferential"]["manifest"],
                "sha256": resolved["sharedDifferential"][
                    "manifestSha256AtFreeze"],
            },
            "runnerSource": {
                "path": resolved["sharedDifferential"]["runnerSource"],
                "sha256": resolved["sharedDifferential"][
                    "runnerSourceSha256AtFreeze"],
            },
            "targetReceiptSha256": base.sha256_file(
                args.target_receipt.resolve()),
            "standaloneReceiptSha256": base.sha256_file(
                args.standalone_receipt.resolve()),
            "targetAdapter": target["adapter"],
            "standaloneAdapter": standalone["adapter"],
            "deterministicRunsPerSide": 2,
            "comparisonStatus": j360_report["status"],
            "comparisonOutcomeCounts": j360_report["outcomeCounts"],
            "differenceBinding": j360_binding,
            "promotesSupport": False,
        },
        "claims": claims,
        "claimOutcomeCounts": {
            "validated": len(claims) - len(excluded),
            "excluded": len(excluded),
        },
        "excludedClaims": excluded,
        "newSuccessorRequired": condition is not None,
        "successorCondition": condition,
        "discrepancyCount": len(discrepancies),
        "discrepancySetSha256": discrepancy_digest,
        "discrepancies": discrepancies,
        "generatedPayloadsRetained": False,
        "selfReadDisposition": "reachability-only",
    }


def expect_contract_rejection(mutated: dict[str, Any], base: Any,
                              label: str) -> None:
    try:
        validate_contract_data(mutated, base)
    except (SuccessorError, base.CandidateError):
        return
    raise SuccessorError(f"{label} mutation was unexpectedly accepted")


def self_test(manifest_path: Path, base: Any) -> None:
    manifest, predecessor, resolved = validate_frozen_manifest(
        manifest_path, base)
    require(predecessor["claims"] != resolved["claims"],
            "v2 did not replace the predecessor claims")
    with contextlib.redirect_stdout(io.StringIO()):
        base.self_test(ROOT / manifest["predecessor"]["manifest"]["path"])

    mutations: list[tuple[str, dict[str, Any]]] = []
    wrong_key = copy.deepcopy(manifest)
    wrong_key["claimOverrides"][0]["oracleField"] = "ins_pt[0]"
    mutations.append(("oracle-key", wrong_key))
    wrong_normalization = copy.deepcopy(manifest)
    wrong_normalization["claimOverrides"][3]["normalization"] = "finiteNumber"
    mutations.append(("normalization", wrong_normalization))
    wrong_semantic_unit = copy.deepcopy(manifest)
    wrong_semantic_unit["claimOverrides"][3]["semanticUnit"] = "radians"
    mutations.append(("semantic-unit", wrong_semantic_unit))
    wrong_oracle_unit = copy.deepcopy(manifest)
    wrong_oracle_unit["claimOverrides"][3]["oracleUnit"] = "degrees"
    mutations.append(("oracle-unit", wrong_oracle_unit))
    wrong_tolerance = copy.deepcopy(manifest)
    wrong_tolerance["normalizationDefinitions"]["degreesFromRadians"][
        "fixedToleranceDegrees"] = 1e-9
    mutations.append(("normalization-tolerance", wrong_tolerance))
    wrong_evidence_digest = copy.deepcopy(manifest)
    wrong_evidence_digest["predecessor"]["evidence"]["bindingSha256"] = "0" * 64
    mutations.append(("predecessor-evidence-digest", wrong_evidence_digest))
    wrong_source_digest = copy.deepcopy(manifest)
    wrong_source_digest["normalizationDefinitions"]["degreesFromRadians"][
        "sourceBindings"][0]["sha256"] = "0" * 64
    mutations.append(("normalization-source-digest", wrong_source_digest))
    wrong_claim_digest = copy.deepcopy(manifest)
    wrong_claim_digest["contractReuse"]["resolvedClaimSetSha256"] = "0" * 64
    mutations.append(("resolved-claim-digest", wrong_claim_digest))
    for label, mutated in mutations:
        expect_contract_rejection(mutated, base, label)

    rotation = next(
        claim for claim in resolved["claims"]
        if claim["id"] == "rtext-rotation")
    checks, converted = degree_checks(
        rotation, 14.999999999999998, 14.999999999999998,
        0.26179938779915)
    require(all(checks.values())
            and converted == math.degrees(0.26179938779915),
            "valid degreesFromRadians vector was rejected")
    outside = math.radians(15.0 + 2.0e-12)
    bad_checks, _ = degree_checks(
        rotation, 15.0, 15.0, outside)
    require(not all(bad_checks.values()),
            "out-of-tolerance radians mutation was accepted")
    nonfinite_checks, converted = degree_checks(
        rotation, 15.0, 15.0, float("nan"))
    require(not all(nonfinite_checks.values()) and converted is None,
            "non-finite radians mutation was accepted")

    synthetic = [{
        "id": claim["id"], "outcome": "validated", "checks": {},
        "reason": None, "observedDigests": {},
    } for claim in resolved["claims"]]
    require(successor_condition(synthetic) is None,
            "all-valid claims unexpectedly requested a successor")
    synthetic[-1]["outcome"] = "excluded"
    synthetic[-1]["reason"] = "mutation"
    condition = successor_condition(synthetic)
    require(condition is not None
            and condition["failedClaimIds"] == ["rtext-rotation"]
            and re.fullmatch(r"[0-9a-f]{64}",
                             condition["failureBindingSha256"]) is not None,
            "excluded claim did not produce an exact successor condition")
    print("check_ac1024_candidate_fields_v2 self-test: PASS")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument("--writer", type=Path)
    parser.add_argument("--oracle", type=Path)
    parser.add_argument("--target-receipt", type=Path)
    parser.add_argument("--standalone-receipt", type=Path)
    parser.add_argument("--timeout", type=float, default=30.0)
    args = parser.parse_args(argv)
    base = None
    try:
        base = load_predecessor_checker()
        if args.self_test:
            self_test(args.manifest.resolve(), base)
            return 0
        missing = [name for name in (
            "writer", "oracle", "target_receipt", "standalone_receipt"
        ) if getattr(args, name) is None]
        if missing:
            parser.error("live mode requires " + ", ".join(
                "--" + name.replace("_", "-") for name in missing))
        require(args.timeout > 0.0, "timeout must be positive")
        report = run(args, base)
        sys.stdout.write(json.dumps(
            report, sort_keys=True, indent=2, allow_nan=False) + "\n")
        return 0 if report["status"] == "complete" else 1
    except (SuccessorError, OSError, UnicodeError, json.JSONDecodeError,
            ValueError, AssertionError) as exc:
        print(f"AC1024 successor candidate evidence: FAIL: {exc}",
              file=sys.stderr)
        return 2
    except Exception as exc:
        if base is not None and isinstance(exc, base.CandidateError):
            print(f"AC1024 successor candidate evidence: FAIL: {exc}",
                  file=sys.stderr)
            return 2
        raise


if __name__ == "__main__":
    raise SystemExit(main())
