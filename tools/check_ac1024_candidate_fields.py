#!/usr/bin/env python3
"""Qualify only frozen AC1024 candidate fields against J360 and LibreDWG.

The generated DWG and LibreDWG JSON never leave a private temporary directory
or process memory.  The report contains provenance, digests, bounded candidate
values, and discrepancy bindings; it never contains the generated payload.
"""

from __future__ import annotations

import argparse
import copy
import contextlib
import hashlib
import importlib.util
import io
import json
import math
import os
import platform
import re
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Any


sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parent.parent
DEFAULT_MANIFEST = ROOT / "metadata/ac1024-candidate-fields-v1.json"
FROZEN_MANIFEST_SHA256 = (
    "e9f6d18df974e7cbc23701ad906fe33e4ca6542fa24861bef9b5d86a7a4764f0"
)
MAX_CAPTURE_BYTES = 256 * 1024 * 1024
SYSTEM_LIBRARY_PREFIXES = ("/System/Library/", "/usr/lib/")
MINIMAL_ENVIRONMENT = {
    "LANG": "C",
    "LC_ALL": "C",
    "PATH": "/usr/bin:/bin",
    "SOURCE_DATE_EPOCH": "0",
    "TZ": "UTC",
}
TARGET_ONLY_DIAGNOSTIC_PATHS = (
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
)
TARGET_ONLY_DIAGNOSTIC_CONTEXT = {
    "id": "diagnostic:00000000",
    "ordinal": 0,
    "severity": "warning",
    "stage": "integrity",
    "code": "dwg-integrity-1",
    "path": "/diagnostics/integrity",
}
EXPECTED_GENERATED_INPUT_SHA256 = (
    "6acb7d4a0289d200d3dd448abc77082250f1678ec2ad815870e4617fa16abffd"
)
EXPECTED_TARGET_ONLY_MESSAGE_DIGEST = (
    "74132423f243658951851e1f0bd9dff7e36ae81d89931163e63a45599bf51185"
)
EXPECTED_TARGET_ONLY_BINDING_DIGEST = (
    "cc08cc93aaa46bdcff05e54454daaade43931e8da7644b424594de278b69b63d"
)
EXPECTED_TARGET_ONLY_PATH_DIGESTS: dict[str, tuple[str, str | None]] = {
    "/cardinality/diagnosticCount": (
        "6b86b273ff34fce19d6b804eff5a3f5747ada4eaa22f1d49c01e52ddb7875b4b",
        "5feceb66ffc86f38d952786c6d696c79c2dbc239dd4e91b46729d73a27fb57e9",
    ),
    "/diagnostics/0/@size": (
        "7902699be42c8a8e46fbbb4501726517e86b22c56a189f7625a6da49081b2451",
        None,
    ),
    "/diagnostics/0/code": (
        "db695f3dac765ac4b095d871254e1e4a179565206a0dd547304744a8d5098843",
        None,
    ),
    "/diagnostics/0/id": (
        "97dc378eaf37151a3a5b53d4b42399c99637ffbee7ccf69ff6a55507899072aa",
        None,
    ),
    "/diagnostics/0/messageDigest": (
        "73c56122743aff9d6aa64f7a058ae3571d7b16531a3744fb8ef5d972c8075a1b",
        None,
    ),
    "/diagnostics/0/ordinal": (
        "5feceb66ffc86f38d952786c6d696c79c2dbc239dd4e91b46729d73a27fb57e9",
        None,
    ),
    "/diagnostics/0/path": (
        "44a7847e9f9ed2f3c4ea9f8152f63794a326fcc33dd2e2e2e6d0d106317bdb4f",
        None,
    ),
    "/diagnostics/0/severity": (
        "c94f9df6f3f7bc69e76a374ce4cd3e5d8766f2145a742ccb34d26e1954efd067",
        None,
    ),
    "/diagnostics/0/stage": (
        "55e10ce778eee689bf769828e2cf4ac96f60ce63a501fe00a7b06501d4e2de87",
        None,
    ),
    "/diagnostics/@length": (
        "6b86b273ff34fce19d6b804eff5a3f5747ada4eaa22f1d49c01e52ddb7875b4b",
        "5feceb66ffc86f38d952786c6d696c79c2dbc239dd4e91b46729d73a27fb57e9",
    ),
}


class CandidateError(RuntimeError):
    """A fail-closed qualification error."""


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


def require(condition: bool, message: str) -> None:
    if not condition:
        raise CandidateError(message)


def strict_json_loads(text: str, label: str) -> Any:
    def object_pairs(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
        result: dict[str, Any] = {}
        for key, value in pairs:
            if key in result:
                raise CandidateError(f"duplicate JSON key in {label}: {key}")
            result[key] = value
        return result

    def nonfinite_constant(token: str) -> Any:
        raise CandidateError(f"non-finite JSON value in {label}: {token}")

    def finite_float(token: str) -> float:
        value = float(token)
        if not math.isfinite(value):
            raise CandidateError(f"non-finite JSON number in {label}: {token}")
        return value

    try:
        return json.loads(
            text, object_pairs_hook=object_pairs,
            parse_constant=nonfinite_constant, parse_float=finite_float,
        )
    except json.JSONDecodeError as exc:
        raise CandidateError(f"invalid JSON in {label}: {exc}") from exc


def read_json(path: Path) -> dict[str, Any]:
    try:
        value = strict_json_loads(path.read_text(encoding="utf-8"), str(path))
    except (OSError, UnicodeError) as exc:
        raise CandidateError(f"cannot read JSON {path}: {exc}") from exc
    require(isinstance(value, dict), f"JSON root is not an object: {path}")
    return value


def load_module(name: str, path: Path) -> Any:
    spec = importlib.util.spec_from_file_location(name, path)
    require(spec is not None and spec.loader is not None,
            f"cannot load module: {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def validate_frozen_manifest(path: Path) -> dict[str, Any]:
    actual_digest = sha256_file(path)
    require(actual_digest == FROZEN_MANIFEST_SHA256,
            "candidate manifest changed after its pre-oracle freeze: "
            f"expected {FROZEN_MANIFEST_SHA256}, got {actual_digest}")
    manifest = read_json(path)
    require(manifest.get("schema") == 1, "candidate manifest schema is not 1")
    require(manifest.get("kind") ==
            "libdxfrw-local-ac1024-independent-candidate-fields",
            "candidate manifest kind is wrong")
    require(manifest.get("frozenBeforeOracle") is True,
            "candidate manifest is not marked frozen-before-oracle")
    require(manifest.get("status") == "EXPERIMENTAL",
            "candidate manifest must remain EXPERIMENTAL")
    boundary = manifest.get("promotionBoundary")
    require(isinstance(boundary, dict), "promotion boundary is missing")
    require(boundary.get("promotesSupport") is False,
            "candidate lane must not promote support")
    require(boundary.get("selfReadDisposition") == "reachability-only",
            "self-read must be reachability-only")
    require(boundary.get("unlistedFieldDisposition") == "excluded",
            "unlisted fields must be excluded")

    contract = manifest.get("frozenContract")
    require(isinstance(contract, dict), "frozen generator contract is missing")
    generator_source = ROOT / str(contract.get("generatorSource", ""))
    require(generator_source.is_file(), "generator source is absent")
    require(sha256_file(generator_source) == contract.get("generatorSourceSha256"),
            "generator source changed after field predeclaration")
    source_contract = contract.get("sourceOnlyOracleContract")
    require(isinstance(source_contract, dict), "source-only oracle contract missing")
    source_contract_path = ROOT / str(source_contract.get("path", ""))
    require(source_contract_path.is_file(), "source-only oracle contract absent")
    require(sha256_file(source_contract_path) == source_contract.get("sha256"),
            "source-only oracle contract changed after field predeclaration")
    require(contract.get("generatedBasename") ==
            "libdxfrw-local-roundtrip-16.dwg",
            "generated basename is not the frozen AC1024 carrier")
    require(contract.get("version") == "AC1024",
            "frozen generator version is not AC1024")

    shared = manifest.get("sharedDifferential")
    require(isinstance(shared, dict), "shared differential provenance is missing")
    for path_key, digest_key, label in (
        ("manifest", "manifestSha256AtFreeze", "J360 manifest"),
        ("runnerSource", "runnerSourceSha256AtFreeze", "J360 runner"),
        ("adapterSource", "adapterSourceSha256AtFreeze", "J360 adapter"),
    ):
        source = ROOT / str(shared.get(path_key, ""))
        require(source.is_file(), f"{label} source is absent")
        require(sha256_file(source) == shared.get(digest_key),
                f"{label} changed after final pre-oracle freeze")

    records = manifest.get("records")
    claims = manifest.get("claims")
    require(isinstance(records, list) and isinstance(claims, list),
            "predeclared records or claims are missing")
    require(len(claims) == 16, "the frozen candidate set must contain 16 claims")
    record_ids = {row.get("id") for row in records if isinstance(row, dict)}
    require(len(record_ids) == len(records) and None not in record_ids,
            "candidate record IDs are not unique")
    claim_ids: set[str] = set()
    for claim in claims:
        require(isinstance(claim, dict), "candidate claim is not an object")
        claim_id = claim.get("id")
        require(isinstance(claim_id, str) and claim_id not in claim_ids,
                "candidate claim IDs are absent or duplicated")
        claim_ids.add(claim_id)
        require(claim.get("recordId") in record_ids,
                f"candidate {claim_id} names an unknown record")
        require(claim.get("normalization") in {
            "mappedIdentity", "finiteNumber", "exactString", "integer"
        }, f"candidate {claim_id} has an unknown normalization")
        require(isinstance(claim.get("semanticField"), str)
                and isinstance(claim.get("oracleField"), str),
                f"candidate {claim_id} has an invalid field path")

    discrepancy = manifest.get("discrepancyBinding")
    require(isinstance(discrepancy, dict), "discrepancy contract is missing")
    require(discrepancy.get("expectedCurrentRunCount") == 34,
            "current-run discrepancy count is not frozen at 34")
    require(discrepancy.get("oracleDependencyDigestAlgorithm") ==
            "sha256(executableClosureDigest NUL commandDigest NUL sourceSha256 "
            "NUL buildRecipeDigest NUL runtimeEnvironmentDigest NUL platformDigest)",
            "oracle dependency digest algorithm is not frozen")
    oracle = manifest.get("oracle", {})
    runtime = oracle.get("runtimeEnvironment", {})
    require(runtime.get("inheritParent") is False
            and runtime.get("variables") == MINIMAL_ENVIRONMENT,
            "oracle runtime environment is not the minimal frozen allowlist")
    closure = oracle.get("dynamicClosure", {})
    require(closure.get("inspectionCommand") ==
            ["/usr/bin/otool", "-L", "{artifact}"],
            "oracle closure inspector is not absolute/frozen")
    require(closure.get("digestAlgorithm") == {
        "hash": "sha256",
        "sortKey": ["role-utf8", "basename-utf8", "fileSha256-ascii"],
        "rowFraming": (
            "utf8(role) + NUL + utf8(basename) + NUL + "
            "ascii(decimalByteSize) + NUL + ascii(lowercaseFileSha256) + LF"
        ),
    },
            "oracle closure digest algorithm is not frozen")
    require(re.fullmatch(r"[0-9a-f]{64}", str(
        closure.get("expectedClosureDigest", ""))) is not None,
        "oracle expected closure digest is invalid")
    return manifest


def deterministic_environment() -> dict[str, str]:
    return dict(MINIMAL_ENVIRONMENT)


@contextlib.contextmanager
def sanitized_process_environment() -> Any:
    original = dict(os.environ)
    os.environ.clear()
    os.environ.update(deterministic_environment())
    try:
        yield
    finally:
        os.environ.clear()
        os.environ.update(original)


def run_capture(command: list[str], timeout: float,
                *, text: bool = False) -> subprocess.CompletedProcess[Any]:
    require(bool(command) and Path(command[0]).is_absolute(),
            "subprocess executable must be an absolute path")
    try:
        result = subprocess.run(
            command, capture_output=True, check=False, timeout=timeout,
            env=deterministic_environment(), text=text,
        )
    except (OSError, subprocess.SubprocessError) as exc:
        raise CandidateError(f"command failed to execute: {command[0]}: {exc}") from exc
    stdout_size = (len(result.stdout.encode("utf-8")) if text
                   else len(result.stdout))
    stderr_size = (len(result.stderr.encode("utf-8")) if text
                   else len(result.stderr))
    require(stdout_size <= MAX_CAPTURE_BYTES and stderr_size <= MAX_CAPTURE_BYTES,
            f"command output exceeded {MAX_CAPTURE_BYTES} bytes")
    return result


def generate_ac1024(writer: Path, output_dir: Path, manifest: dict[str, Any],
                    timeout: float) -> tuple[Path, dict[str, Any]]:
    require(writer.is_file(), f"writer does not exist: {writer}")
    require(output_dir.is_dir() and not output_dir.is_symlink(),
            "private output directory is invalid")
    command = [str(writer), "--keep-dir", str(output_dir)]
    result = run_capture(command, timeout)
    require(result.returncode == 0,
            f"local-from-scratch writer exited {result.returncode}: "
            + result.stderr.decode("utf-8", errors="replace")[-2000:])
    descendants = list(output_dir.rglob("*"))
    require(descendants, "writer produced no temporary output")
    for path in descendants:
        require(not path.is_symlink(), f"writer produced a symlink: {path.name}")
        if path.is_file():
            require(path.suffix == ".dwg",
                    f"writer produced a non-DWG payload: {path.name}")
    basename = manifest["frozenContract"]["generatedBasename"]
    generated = output_dir / basename
    require(generated.is_file(), f"writer did not produce {basename}")
    with generated.open("rb") as stream:
        require(stream.read(6) == b"AC1024", "generated file is not AC1024")
    return generated, {
        "command": [str(writer.resolve()), "--keep-dir", "{privateTempDirectory}"],
        "writerSha256": sha256_file(writer),
        "generatedBasename": basename,
        "generatedSha256": sha256_file(generated),
        "generatedByteSize": generated.stat().st_size,
        "generatedPayloadRetained": False,
        "selfReadDisposition": "reachability-only",
    }


def otool_dependencies(path: Path, timeout: float) -> list[str]:
    result = run_capture(["/usr/bin/otool", "-L", str(path)], timeout, text=True)
    require(result.returncode == 0,
            f"otool failed for {path}: {result.stderr[-2000:]}")
    dependencies: list[str] = []
    for line in result.stdout.splitlines()[1:]:
        stripped = line.strip()
        if not stripped:
            continue
        dependencies.append(stripped.split(" (compatibility version", 1)[0])
    return dependencies


def resolve_dynamic_path(dependency: str, loader: Path,
                         executable: Path) -> Path | None:
    if dependency.startswith(SYSTEM_LIBRARY_PREFIXES):
        return None
    if dependency.startswith("@loader_path/"):
        return (loader.parent / dependency[len("@loader_path/"):]).resolve()
    if dependency.startswith("@executable_path/"):
        return (executable.parent /
                dependency[len("@executable_path/"):]).resolve()
    if dependency.startswith("@rpath/"):
        suffix = dependency[len("@rpath/"):]
        candidates = [loader.parent / suffix, executable.parent / suffix]
        matches = [candidate.resolve() for candidate in candidates
                   if candidate.exists()]
        require(len(matches) == 1,
                f"cannot uniquely resolve dynamic dependency {dependency}")
        return matches[0]
    path = Path(dependency)
    require(path.is_absolute(), f"dynamic dependency is not absolute: {dependency}")
    return path.resolve()


def inspect_dynamic_closure(executable: Path, timeout: float
                            ) -> tuple[list[dict[str, Any]], str]:
    executable = executable.resolve()
    require(executable.is_file(), f"oracle executable is absent: {executable}")
    queue: list[tuple[str, Path]] = [("executable", executable)]
    visited: set[Path] = set()
    rows: list[dict[str, Any]] = []
    while queue:
        role, path = queue.pop(0)
        path = path.resolve()
        if path in visited:
            continue
        require(path.is_file(), f"dynamic closure artifact is absent: {path}")
        visited.add(path)
        rows.append({
            "role": role,
            "basename": path.name,
            "realpath": str(path),
            "byteSize": path.stat().st_size,
            "sha256": sha256_file(path),
        })
        for dependency in otool_dependencies(path, timeout):
            resolved = resolve_dynamic_path(dependency, path, executable)
            if resolved is not None and resolved not in visited:
                queue.append(("library", resolved))
    rows.sort(key=lambda row: (row["role"], row["basename"], row["sha256"]))
    framed = bytearray()
    for row in rows:
        for value in (row["role"], row["basename"], str(row["byteSize"])):
            framed.extend(value.encode("utf-8"))
            framed.append(0)
        framed.extend(row["sha256"].encode("ascii"))
        framed.append(10)
    return rows, sha256_bytes(bytes(framed))


def validate_expected_artifacts(manifest: dict[str, Any],
                                closure: list[dict[str, Any]]) -> None:
    keys = ("role", "basename", "byteSize", "sha256")
    expected = sorted(manifest["oracle"]["expectedArtifacts"],
                      key=lambda row: tuple(row[key] for key in keys))
    observed = sorted(
        ({key: row[key] for key in keys} for row in closure),
        key=lambda row: tuple(row[key] for key in keys),
    )
    require(observed == expected,
            "oracle transitive non-system closure membership changed")


def parse_oracle_json(raw: bytes, oracle_module: Any) -> dict[str, Any]:
    try:
        text = raw.decode("utf-8")
        payload = oracle_module.parse_json_output(text)
    except (UnicodeError, json.JSONDecodeError, ValueError) as exc:
        raise CandidateError(f"LibreDWG did not emit full JSON: {exc}") from exc
    require(isinstance(payload, dict), "LibreDWG JSON root is not an object")
    require(isinstance(payload.get("FILEHEADER"), dict)
            and isinstance(payload.get("CLASSES"), list)
            and isinstance(payload.get("OBJECTS"), list),
            "LibreDWG output is not the required full JSON shape")
    return payload


SCOPE_RULES: tuple[tuple[str, str], ...] = (
    ("DICTIONARYWDFLT", "OBJECTS.DICTIONARYWDFLT.items/defaultid"),
    ("RapidRT", "OBJECTS.RAPIDRTRENDERSETTINGS.render-fields"),
    ("MENTALRAY", "OBJECTS.MENTALRAYRENDERSETTINGS.payload"),
    ("POINTCLOUD/POINTCLOUDEX", "ENTITIES.POINTCLOUD+POINTCLOUDEX.payload"),
    ("POINTCLOUDDEFINITION", "OBJECTS.POINTCLOUDDEFINITION.payload"),
    ("POINTCLOUDCOLORMAP", "OBJECTS.POINTCLOUDCOLORMAP.payload"),
    ("NAVISWORKSMODELDEF", "OBJECTS.NAVISWORKSMODELDEF.payload"),
    ("NAVISWORKSMODEL", "ENTITIES.NAVISWORKSMODEL.payload"),
    ("DIMASSOC/EVALUATION_GRAPH", "OBJECTS.DIMASSOC+EVALUATION_GRAPH.payload"),
    ("SECTION_MANAGER/SECTION_SETTINGS", "OBJECTS.SECTION_MANAGER+SECTION_SETTINGS.payload"),
    ("VXCONTROL/VXTABLERECORD", "OBJECTS.VXCONTROL+VXTABLERECORD.payload"),
    ("CURVEPATH/POINTPATH", "OBJECTS.CURVEPATH+POINTPATH.payload"),
    ("IMAGE/IMAGEDEF", "ENTITIES.IMAGE+OBJECTS.IMAGEDEF.payload"),
    ("PDF/DGN/DWF UNDERLAY", "ENTITIES.UNDERLAY.payload"),
    ("BLOCKREPRESENTATIONDATA", "OBJECTS.BLOCKREPRESENTATIONDATA.payload"),
    ("PARTIAL_VIEWING_INDEX", "OBJECTS.PARTIAL_VIEWING_INDEX.entries"),
    ("GEOPOSITIONMARKER", "ENTITIES.GEOPOSITIONMARKER.payload"),
    ("VXTABLERECORD", "OBJECTS.VXTABLERECORD.payload"),
    ("VXCONTROL", "OBJECTS.VXCONTROL.payload"),
    ("SPATIAL_INDEX", "OBJECTS.SPATIAL_INDEX.payload"),
    ("TABLESTYLE", "OBJECTS.TABLESTYLE.payload"),
    ("DBCOLOR", "OBJECTS.DBCOLOR.color-book"),
    ("LIGHTLIST", "OBJECTS.LIGHTLIST.members"),
    ("SUNSTUDY", "OBJECTS.SUNSTUDY.references"),
    ("MOTIONPATH", "OBJECTS.MOTIONPATH.payload"),
    ("BACKGROUND", "OBJECTS.BACKGROUND.payload"),
    ("GEODATA", "OBJECTS.GEODATA.payload"),
    ("MATERIAL", "OBJECTS.MATERIAL.visual-properties"),
    ("WIPEOUT", "ENTITIES.WIPEOUT.payload"),
    ("SURFACE", "ENTITIES.SURFACE.modeler-payload"),
    ("MLINE", "ENTITIES.MLINE.payload"),
    ("MESH", "ENTITIES.MESH.topology"),
    ("SHX", "ENTITIES.SHAPE.glyph-payload"),
    ("SHAPE", "ENTITIES.SHAPE.payload"),
    ("LIGHT", "ENTITIES.LIGHT.payload"),
)


def discrepancy_scope(message: str) -> str:
    for marker, scope in SCOPE_RULES:
        if marker in message:
            return scope
    raise CandidateError(f"unclassified discrepancy field scope: {message}")


def discrepancy_outcome(message: str) -> str:
    lower = message.lower()
    if "external" in lower or "resources are absent" in lower:
        return "external-resource-absent"
    if "gated off" in lower or "before ac" in lower:
        return "version-gated"
    if "local-self-read" in lower or "local self-read" in lower:
        return "self-read-only-reachability"
    if "intentionally" in lower:
        return "implementation-exclusion"
    return "oracle-limitation"


def bind_discrepancies(messages: list[str], oracle_dependency_digest: str,
                       expected_count: int) -> list[dict[str, Any]]:
    require(len(messages) == expected_count,
            f"expected {expected_count} discrepancies, observed {len(messages)}")
    require(len(set(messages)) == len(messages), "duplicate discrepancy messages")
    rows = []
    ids: set[str] = set()
    for message in messages:
        require(isinstance(message, str) and message,
                "empty oracle discrepancy")
        scope = discrepancy_scope(message)
        outcome = discrepancy_outcome(message)
        digest_input = (scope + "\0" + outcome + "\0" + message).encode("utf-8")
        stable_id = "ac1024-disc-" + sha256_bytes(digest_input)[:16]
        require(stable_id not in ids, "discrepancy stable-ID collision")
        ids.add(stable_id)
        rows.append({
            "id": stable_id,
            "fieldScope": scope,
            "outcome": outcome,
            "message": message,
            "oracleDependencyDigest": oracle_dependency_digest,
        })
    rows.sort(key=lambda row: row["id"])
    return rows


def inspect_oracle(executable: Path, generated: Path,
                   manifest: dict[str, Any], oracle_module: Any,
                   timeout: float) -> tuple[dict[str, Any], dict[str, Any], list[str]]:
    closure, closure_digest = inspect_dynamic_closure(executable, timeout)
    validate_expected_artifacts(manifest, closure)
    require(closure_digest == manifest["oracle"]["dynamicClosure"][
        "expectedClosureDigest"], "oracle transitive closure digest changed")
    expected_platform = manifest["oracle"]["expectedPlatform"]
    observed_platform = {
        "system": platform.system(),
        "architecture": platform.machine(),
        "osVersion": platform.mac_ver()[0],
    }
    require(observed_platform == expected_platform,
            "host platform does not match the frozen oracle platform")
    resolved_executable = executable.resolve()
    cellar = resolved_executable.parent.parent
    formula = cellar / ".brew/libredwg.rb"
    expected_formula_digest = manifest["oracle"]["buildRecipe"]["formulaSha256"]
    require(formula.is_file() and sha256_file(formula) == expected_formula_digest,
            "installed Homebrew formula does not match the pinned build recipe")
    sbom_path = cellar / "sbom.spdx.json"
    sbom = read_json(sbom_path)
    packages = sbom.get("packages")
    require(isinstance(packages, list), "installed Homebrew SBOM is malformed")
    source = manifest["oracle"]["source"]
    bottle_digest = manifest["oracle"]["buildRecipe"]["bottleSha256"]
    source_matches = [
        row for row in packages
        if isinstance(row, dict)
        and row.get("name") == "libredwg"
        and row.get("versionInfo") == manifest["oracle"]["release"]
        and row.get("downloadLocation") == source["url"]
        and {item.get("checksumValue") for item in row.get("checksums", [])
             if isinstance(item, dict)} == {source["sha256"]}
    ]
    bottle_matches = [
        row for row in packages
        if isinstance(row, dict)
        and row.get("name") == "libredwg"
        and row.get("versionInfo") == manifest["oracle"]["release"]
        and str(row.get("downloadLocation", "")).endswith(bottle_digest)
        and {item.get("checksumValue") for item in row.get("checksums", [])
             if isinstance(item, dict)} == {bottle_digest}
    ]
    require(len(source_matches) == 1 and len(bottle_matches) == 1,
            "Homebrew SBOM does not bind the pinned source and bottle")
    version_result = run_capture([str(resolved_executable), "--version"], timeout)
    require(version_result.returncode == 0, "dwgread --version failed")
    version_text = (version_result.stdout + version_result.stderr).decode(
        "utf-8", errors="replace").strip()
    require(manifest["oracle"]["versionOutput"] in version_text,
            f"unexpected dwgread version: {version_text!r}")
    command = [str(resolved_executable), "-O", "JSON", str(generated)]
    runs = [run_capture(command, timeout), run_capture(command, timeout)]
    for result in runs:
        require(result.returncode == 0,
                f"LibreDWG exited {result.returncode}: "
                + result.stderr.decode("utf-8", errors="replace")[-2000:])
        require(result.stderr.strip() == b"SUCCESS",
                "LibreDWG full-JSON run did not report SUCCESS")
    require((runs[0].stdout, runs[0].stderr, runs[0].returncode)
            == (runs[1].stdout, runs[1].stderr, runs[1].returncode),
            "LibreDWG full-JSON output is non-deterministic")
    result = runs[0]
    payload = parse_oracle_json(result.stdout, oracle_module)
    summary = oracle_module.check_objects(payload, "AC1024")
    discrepancies = summary.get("oracleDiscrepancies")
    require(isinstance(discrepancies, list),
            "source-only oracle contract returned no discrepancies")

    sanitized_command = [str(resolved_executable), "-O", "JSON",
                         "{privateTempDirectory}/" + generated.name]
    command_digest = sha256_bytes(canonical_bytes(sanitized_command))
    build_recipe = manifest["oracle"]["buildRecipe"]
    build_recipe_digest = sha256_bytes(canonical_bytes(build_recipe))
    source_digest = manifest["oracle"]["source"]["sha256"]
    runtime_environment_digest = sha256_bytes(canonical_bytes(
        manifest["oracle"]["runtimeEnvironment"]))
    platform_digest = sha256_bytes(canonical_bytes(expected_platform))
    dependency_digest = sha256_bytes(
        (closure_digest + "\0" + command_digest + "\0" + source_digest
         + "\0" + build_recipe_digest + "\0" + runtime_environment_digest
         + "\0" + platform_digest).encode("utf-8")
    )
    receipt = resolved_executable.parent.parent / "INSTALL_RECEIPT.json"
    provenance = {
        "name": manifest["oracle"]["name"],
        "release": manifest["oracle"]["release"],
        "versionOutput": version_text,
        "license": manifest["oracle"]["license"],
        "source": manifest["oracle"]["source"],
        "buildRecipe": build_recipe,
        "buildRecipeDigest": build_recipe_digest,
        "formula": {
            "path": str(formula),
            "sha256": sha256_file(formula),
            "byteSize": formula.stat().st_size,
        },
        "sbom": {
            "path": str(sbom_path),
            "sha256": sha256_file(sbom_path),
            "byteSize": sbom_path.stat().st_size,
            "sourceAndBottlePinsVerified": True,
        },
        "installReceipt": ({
            "path": str(receipt),
            "sha256": sha256_file(receipt),
            "byteSize": receipt.stat().st_size,
        } if receipt.is_file() else None),
        "executable": closure[[row["role"] for row in closure].index("executable")],
        "transitiveNonSystemDynamicClosure": closure,
        "transitiveClosureDigest": closure_digest,
        "exactCommand": command,
        "reproductionCommandTemplate": sanitized_command,
        "commandDigest": command_digest,
        "runtimeEnvironment": manifest["oracle"]["runtimeEnvironment"],
        "runtimeEnvironmentDigest": runtime_environment_digest,
        "platform": {
            **observed_platform,
            "kernelRelease": platform.release(),
            "python": platform.python_version(),
        },
        "platformDigest": platform_digest,
        "oracleDependencyDigest": dependency_digest,
        "jsonSha256": sha256_bytes(result.stdout),
        "jsonByteSize": len(result.stdout),
        "deterministicRuns": 2,
        "jsonPayloadRetained": False,
    }
    return payload, provenance, discrepancies


def callback_kind(result: dict[str, Any], record: dict[str, Any]) -> str:
    ordinal = record.get("callbackOrdinal")
    callbacks = result.get("callbacks")
    require(isinstance(callbacks, list) and isinstance(ordinal, int)
            and 0 <= ordinal < len(callbacks), "semantic callback ordinal invalid")
    callback = callbacks[ordinal]
    require(isinstance(callback, dict), "semantic callback is not an object")
    return str(callback.get("kind"))


def field_map(record: dict[str, Any]) -> dict[str, Any]:
    fields = record.get("fields")
    require(isinstance(fields, list), "semantic record has no fields")
    result: dict[str, Any] = {}
    for row in fields:
        require(isinstance(row, dict) and isinstance(row.get("name"), str)
                and "value" in row, "semantic field row is malformed")
        require(row["name"] not in result, "duplicate semantic field")
        result[row["name"]] = row["value"]
    return result


def nested_value(value: Any, path: str) -> Any:
    cursor = 0
    current = value
    while cursor < len(path):
        match = re.match(r"([A-Za-z_][A-Za-z0-9_]*)", path[cursor:])
        if match:
            key = match.group(1)
            require(isinstance(current, dict) and key in current,
                    f"field path is absent: {path}")
            current = current[key]
            cursor += len(key)
        elif path[cursor] == "[":
            close = path.find("]", cursor)
            require(close > cursor + 1, f"invalid field path: {path}")
            index_text = path[cursor + 1:close]
            require(index_text.isdigit() and isinstance(current, list),
                    f"invalid array field path: {path}")
            index = int(index_text)
            require(index < len(current), f"array field path is absent: {path}")
            current = current[index]
            cursor = close + 1
        elif path[cursor] == ".":
            cursor += 1
        else:
            raise CandidateError(f"invalid field path syntax: {path}")
    return current


def semantic_value(record: dict[str, Any], path: str) -> Any:
    if path == "recordClass":
        return record.get("recordClass")
    fields = field_map(record)
    root, separator, remainder = path.partition(".")
    require(root in fields, f"semantic field is absent: {path}")
    return nested_value(fields[root], remainder) if separator else fields[root]


def normalized_handle(value: Any) -> str:
    if isinstance(value, int) and not isinstance(value, bool):
        return format(value, "x")
    require(isinstance(value, str), "handle is neither string nor integer")
    result = value.lower().removeprefix("0x").lstrip("0")
    return result or "0"


def find_semantic_record(result: dict[str, Any], spec: dict[str, Any]
                         ) -> dict[str, Any]:
    records = result.get("records")
    require(isinstance(records, list), "semantic result has no records")
    matches = []
    for record in records:
        if not isinstance(record, dict):
            continue
        if record.get("recordClass") != spec.get("recordClass"):
            continue
        if callback_kind(result, record) != spec.get("callback"):
            continue
        source_handle = spec.get("sourceHandle")
        if source_handle is not None and normalized_handle(
                record.get("sourceHandle")) != normalized_handle(source_handle):
            continue
        selector = spec.get("selector")
        if selector is not None:
            if semantic_value(record, selector["field"]) != selector["equals"]:
                continue
        matches.append(record)
    require(len(matches) == 1,
            f"semantic selector matched {len(matches)} {spec.get('recordClass')} records")
    return matches[0]


def find_oracle_record(payload: dict[str, Any], spec: dict[str, Any],
                       oracle_module: Any) -> dict[str, Any]:
    records = payload.get("OBJECTS")
    require(isinstance(records, list), "oracle JSON has no OBJECTS")
    matches = []
    for record in records:
        if not isinstance(record, dict) or record.get("entity") != spec.get("entity"):
            continue
        wanted_handle = spec.get("handle")
        if wanted_handle is not None:
            observed_handle = oracle_module.record_handle(record)
            if observed_handle is None or normalized_handle(
                    observed_handle) != normalized_handle(wanted_handle):
                continue
        selector = spec.get("selector")
        if selector is not None:
            if nested_value(record, selector["field"]) != selector["equals"]:
                continue
        matches.append(record)
    require(len(matches) == 1,
            f"oracle selector matched {len(matches)} {spec.get('entity')} records")
    return matches[0]


def values_equal(actual: Any, expected: Any, normalization: str,
                 tolerance: Any) -> bool:
    if normalization == "finiteNumber":
        if (not isinstance(actual, (int, float)) or isinstance(actual, bool)
                or not math.isfinite(float(actual))):
            return False
        return abs(float(actual) - float(expected)) <= float(tolerance)
    if normalization == "integer":
        return (isinstance(actual, int) and not isinstance(actual, bool)
                and actual == expected)
    if normalization in {"exactString", "mappedIdentity"}:
        return isinstance(actual, str) and actual == expected
    raise CandidateError(f"unknown normalization: {normalization}")


def validate_claims(manifest: dict[str, Any], oracle_payload: dict[str, Any],
                    target: dict[str, Any], standalone: dict[str, Any],
                    oracle_module: Any) -> list[dict[str, Any]]:
    record_specs = {row["id"]: row for row in manifest["records"]}
    selected: dict[str, dict[str, Any | CandidateError]] = {}
    for record_id, spec in record_specs.items():
        selected[record_id] = {}
        selectors = (
            ("target", lambda: find_semantic_record(target, spec["semantic"])),
            ("standalone", lambda: find_semantic_record(
                standalone, spec["semantic"])),
            ("oracle", lambda: find_oracle_record(
                oracle_payload, spec["oracle"], oracle_module)),
        )
        for side, selector in selectors:
            try:
                selected[record_id][side] = selector()
            except CandidateError as exc:
                selected[record_id][side] = exc
    rows = []
    for claim in manifest["claims"]:
        records = selected[claim["recordId"]]
        normalization = claim["normalization"]
        tolerance = claim["tolerance"]
        selection_errors = {
            side: str(value) for side, value in records.items()
            if isinstance(value, CandidateError)
        }
        try:
            if selection_errors:
                raise CandidateError("; ".join(
                    f"{side}: {message}"
                    for side, message in sorted(selection_errors.items())))
            target_value = semantic_value(
                records["target"], claim["semanticField"])
            standalone_value = semantic_value(
                records["standalone"], claim["semanticField"])
            oracle_value = nested_value(records["oracle"], claim["oracleField"])
        except CandidateError as exc:
            rows.append({
                "id": claim["id"],
                "recordId": claim["recordId"],
                "semanticField": claim["semanticField"],
                "oracleField": claim["oracleField"],
                "normalization": normalization,
                "tolerance": tolerance,
                "outcome": "excluded",
                "checks": {
                    "targetExpected": False,
                    "standaloneExpected": False,
                    "oracleExpected": False,
                    "targetStandaloneSame": False,
                },
                "observedDigests": {
                    "target": None, "standalone": None, "oracle": None,
                },
                "reason": "predeclared-field-unavailable: " + str(exc),
            })
            continue
        checks = {
            "targetExpected": values_equal(
                target_value, claim["expectedSemantic"], normalization, tolerance),
            "standaloneExpected": values_equal(
                standalone_value, claim["expectedSemantic"], normalization, tolerance),
            "oracleExpected": values_equal(
                oracle_value, claim["expectedOracle"], normalization, tolerance),
            "targetStandaloneSame": values_equal(
                target_value, standalone_value, normalization, tolerance),
        }
        rows.append({
            "id": claim["id"],
            "recordId": claim["recordId"],
            "semanticField": claim["semanticField"],
            "oracleField": claim["oracleField"],
            "normalization": normalization,
            "tolerance": tolerance,
            "outcome": "validated" if all(checks.values()) else "excluded",
            "checks": checks,
            "observedDigests": {
                "target": sha256_bytes(canonical_bytes(target_value)),
                "standalone": sha256_bytes(canonical_bytes(standalone_value)),
                "oracle": sha256_bytes(canonical_bytes(oracle_value)),
            },
            "reason": (None if all(checks.values())
                       else "predeclared-field-value-not-independently-validated"),
        })
    return rows


def build_exclusion_evidence(manifest: dict[str, Any],
                             oracle_payload: dict[str, Any],
                             target: dict[str, Any], standalone: dict[str, Any],
                             oracle_module: Any) -> list[dict[str, Any]]:
    spec = next(row for row in manifest["records"] if row["id"] == "rtext-ed00")
    oracle_record = find_oracle_record(
        oracle_payload, spec["oracle"], oracle_module)
    target_record = find_semantic_record(target, spec["semantic"])
    standalone_record = find_semantic_record(standalone, spec["semantic"])
    require("ins_pt" not in oracle_record and oracle_record.get("pt") ==
            [90.0, 91.0, 0.0],
            "RTEXT exclusion evidence no longer has the observed pt key/value")
    target_insertion = semantic_value(target_record, "insertion")
    standalone_insertion = semantic_value(standalone_record, "insertion")
    require(target_insertion == {"x": 90, "y": 91, "z": 0}
            and standalone_insertion == target_insertion,
            "RTEXT semantic insertion exclusion evidence changed")
    coordinates = []
    for index, axis in enumerate(("x", "y", "z")):
        coordinates.append({
            "claimId": f"rtext-insertion-{axis}",
            "disposition": "excluded-new-child-required",
            "predeclaredOracleField": f"ins_pt[{index}]",
            "predeclaredOracleFieldPresent": False,
            "observedOracleField": f"pt[{index}]",
            "observedOracleValue": oracle_record["pt"][index],
            "observedTargetSemanticField": f"insertion.{axis}",
            "observedTargetValue": target_insertion[axis],
            "observedStandaloneValue": standalone_insertion[axis],
            "unit": "drawing-coordinate",
            "reason": "frozen oracle key mismatch; correction requires a new child",
        })

    target_rotation = semantic_value(target_record, "rotation")
    standalone_rotation = semantic_value(standalone_record, "rotation")
    oracle_rotation = oracle_record.get("rotation")
    require(isinstance(target_rotation, (int, float))
            and isinstance(standalone_rotation, (int, float))
            and isinstance(oracle_rotation, (int, float)),
            "RTEXT rotation exclusion evidence is non-numeric")
    require(target_rotation == standalone_rotation
            and math.isclose(float(target_rotation), 15.0,
                             rel_tol=0.0, abs_tol=1e-12)
            and math.isclose(math.degrees(float(oracle_rotation)), 15.0,
                             rel_tol=0.0, abs_tol=1e-12),
            "RTEXT rotation exclusion evidence changed")
    coordinates.append({
        "claimId": "rtext-rotation",
        "disposition": "excluded-new-child-required",
        "predeclaredNormalization": "finiteNumber; tolerance=0; expected=15",
        "observedOracleField": "rotation",
        "observedOracleValue": oracle_rotation,
        "observedOracleUnit": "radians",
        "observedTargetValue": target_rotation,
        "observedStandaloneValue": standalone_rotation,
        "observedSemanticUnit": "degrees",
        "oracleValueConvertedToDegrees": math.degrees(float(oracle_rotation)),
        "reason": (
            "frozen claim omitted radians-to-degrees normalization and exact "
            "floating tolerance; correction requires a new child"
        ),
    })
    return coordinates


def run_j360_pair(input_path: Path, manifest: dict[str, Any],
                  target_receipt_path: Path, standalone_receipt_path: Path,
                  timeout: float) -> tuple[dict[str, Any], dict[str, Any], dict[str, Any]]:
    qd = load_module("qualified_differential_v2",
                     ROOT / "tools/run_qualified_differential_v2.py")
    qd_manifest_path = ROOT / manifest["sharedDifferential"]["manifest"]
    qd_manifest = qd.validate_manifest(qd._read_json(qd_manifest_path), root=ROOT)
    qd_manifest_digest = qd._sha256_file(qd_manifest_path)
    require(qd_manifest_digest == manifest["sharedDifferential"][
        "manifestSha256AtFreeze"], "J360 manifest changed after pre-oracle freeze")
    source_digest = qd._sha256_file(ROOT / qd_manifest["adapterSource"]["path"])
    require(source_digest == manifest["sharedDifferential"][
        "adapterSourceSha256AtFreeze"], "J360 adapter source changed after freeze")
    runner_source = ROOT / manifest["sharedDifferential"]["runnerSource"]
    require(qd._sha256_file(runner_source) == manifest["sharedDifferential"][
        "runnerSourceSha256AtFreeze"], "J360 runner source changed after freeze")
    target_receipt = qd.validate_receipt(
        qd._read_json(target_receipt_path), receipt_path=target_receipt_path,
        check_artifacts=True)
    standalone_receipt = qd.validate_receipt(
        qd._read_json(standalone_receipt_path),
        receipt_path=standalone_receipt_path, check_artifacts=True)
    qd._validate_receipt_pair(target_receipt, standalone_receipt)
    require(target_receipt["targetLock"] == qd_manifest["targetLock"],
            "J360 target receipt does not match target lock")
    require(standalone_receipt["targetLock"] is None,
            "J360 standalone receipt unexpectedly has a target lock")
    generator_digest = manifest["frozenContract"]["generatorSourceSha256"]
    common = {
        "manifest": qd_manifest,
        "manifest_digest": qd_manifest_digest,
        "source_digest": source_digest,
        "input_path": input_path,
        "input_id": manifest["sharedDifferential"]["inputId"],
        "input_path_hint": manifest["sharedDifferential"]["inputPathHint"],
        "origin_kind": "localFromScratch",
        "input_repository": None,
        "input_commit": None,
        # This runtime-only drawing cannot be a byte-fixed localRecipe.  Its
        # provenance is instead the frozen generator source/command above.
        "input_source_path": None,
        "input_source_blob": None,
        "input_registry_digest": None,
        "expected_detected_format": "dwg",
        "expected_detected_version": "AC1024",
        "generator_digest": generator_digest,
        "facade": "dwgRW",
        "direction": "read",
        "timeout_seconds": timeout,
        "expected_operation_succeeded": True,
        "output_probe": "none",
    }
    with sanitized_process_environment():
        target = qd._run_live_side(
            side="target", receipt=target_receipt,
            receipt_path=target_receipt_path, **common)
        standalone = qd._run_live_side(
            side="standalone", receipt=standalone_receipt,
            receipt_path=standalone_receipt_path, **common)
    expected_digest = sha256_file(input_path)
    require(target["input"]["sha256"] == expected_digest
            and standalone["input"]["sha256"] == expected_digest,
            "J360 did not execute both sides on the generated bytes")
    report = qd.compare_results(
        target, standalone, qd_manifest["comparison"],
        manifest_digest=qd_manifest_digest,
        target_lock=qd_manifest["targetLock"],
        expected_operation_succeeded=True, expected_failure=None)
    return target, standalone, report


def validate_j360_evidence(target: dict[str, Any], standalone: dict[str, Any],
                           report: dict[str, Any], input_sha256: str,
                           qd: Any) -> dict[str, Any]:
    require(re.fullmatch(r"[0-9a-f]{64}", input_sha256) is not None,
            "generated input digest is invalid")
    require(input_sha256 == EXPECTED_GENERATED_INPUT_SHA256,
            "local-from-scratch generator bytes changed after live binding")
    require(target["input"]["sha256"] == input_sha256
            and standalone["input"]["sha256"] == input_sha256,
            "J360 evidence is not bound to the generated input SHA-256")
    require(target["status"]["operationSucceeded"] is True
            and standalone["status"]["operationSucceeded"] is True,
            "a J360 side did not complete the read operation")
    require(report.get("promotesSupport") is False
            and report.get("deterministicRunsPerSide") == 2,
            "J360 evidence is not deterministic/non-promoting")
    counts = report.get("outcomeCounts")
    completeness = report.get("completeness")
    require(isinstance(counts, dict) and isinstance(completeness, dict),
            "J360 comparison summary is malformed")
    nonexact = sum(int(counts.get(key, 0)) for key in (
        "toleranceNormalized", "reviewedTargetDebt", "excluded", "mismatch"
    ))

    if nonexact == 0:
        require(completeness.get("missingTargetCount") == 0
                and completeness.get("missingStandaloneCount") == 0,
                "exact J360 evidence reports missing values")
        require(target.get("diagnostics") == []
                and standalone.get("diagnostics") == []
                and target["cardinality"]["diagnosticCount"] == 0
                and standalone["cardinality"]["diagnosticCount"] == 0,
                "zero-difference J360 evidence contains an integrity diagnostic")
        return {
            "disposition": "exact-non-promoting",
            "inputSha256": input_sha256,
            "pathCount": 0,
            "paths": [],
            "bindingDigest": sha256_bytes(canonical_bytes({
                "inputSha256": input_sha256, "paths": [],
            })),
        }

    require(counts.get("mismatch") == 10 and nonexact == 10,
            "J360 has differences beyond the one allowed diagnostic")
    require(completeness.get("missingTargetCount") == 0
            and completeness.get("missingStandaloneCount") == 8,
            "target-only diagnostic has unexpected missing-value cardinality")
    target_diagnostics = target.get("diagnostics")
    standalone_diagnostics = standalone.get("diagnostics")
    require(isinstance(target_diagnostics, list)
            and len(target_diagnostics) == 1
            and standalone_diagnostics == [],
            "J360 mismatch is not one target-only diagnostic")
    target_diagnostic = target_diagnostics[0]
    require(isinstance(target_diagnostic, dict),
            "target diagnostic is malformed")
    context = {key: target_diagnostic.get(key)
               for key in TARGET_ONLY_DIAGNOSTIC_CONTEXT}
    require(context == TARGET_ONLY_DIAGNOSTIC_CONTEXT,
            "target-only diagnostic context is not the S386 CRC boundary case")
    require(re.fullmatch(r"[0-9a-f]{64}", str(
        target_diagnostic.get("messageDigest", ""))) is not None,
        "target-only diagnostic message digest is invalid")
    require(target_diagnostic["messageDigest"] ==
            EXPECTED_TARGET_ONLY_MESSAGE_DIGEST,
            "target-only S386 diagnostic message digest changed")
    require(target["cardinality"]["diagnosticCount"] == 1
            and standalone["cardinality"]["diagnosticCount"] == 0,
            "target-only diagnostic cardinality is inconsistent")

    missing = object()
    expected_values: dict[str, tuple[Any, Any]] = {
        "/cardinality/diagnosticCount": (1, 0),
        "/diagnostics/@length": (1, 0),
        "/diagnostics/0/@size": (7, missing),
        "/diagnostics/0/code": (target_diagnostic["code"], missing),
        "/diagnostics/0/id": (target_diagnostic["id"], missing),
        "/diagnostics/0/messageDigest": (
            target_diagnostic["messageDigest"], missing),
        "/diagnostics/0/ordinal": (target_diagnostic["ordinal"], missing),
        "/diagnostics/0/path": (target_diagnostic["path"], missing),
        "/diagnostics/0/severity": (target_diagnostic["severity"], missing),
        "/diagnostics/0/stage": (target_diagnostic["stage"], missing),
    }
    differing_rows = [
        row for row in report.get("comparisons", [])
        if row.get("outcome") != "exact"
    ]
    require(sorted(row.get("path") for row in differing_rows)
            == list(TARGET_ONLY_DIAGNOSTIC_PATHS),
            "J360 target-only diagnostic differs on unexpected paths")
    row_by_path = {row["path"]: row for row in differing_rows}
    bound_paths: list[dict[str, Any]] = []
    for path in TARGET_ONLY_DIAGNOSTIC_PATHS:
        row = row_by_path[path]
        left, right = expected_values[path]
        expected_left_digest = qd._value_digest(left)
        expected_right_digest = None if right is missing else qd._value_digest(right)
        require((expected_left_digest, expected_right_digest) ==
                EXPECTED_TARGET_ONLY_PATH_DIGESTS[path],
                f"hard-bound J360 path digest changed at {path}")
        require(row.get("outcome") == "mismatch"
                and row.get("targetPresent") is True
                and row.get("standalonePresent") is (right is not missing)
                and row.get("targetDigest") == expected_left_digest
                and row.get("standaloneDigest") == expected_right_digest,
                f"J360 diagnostic binding changed at {path}")
        bound_paths.append({
            "path": path,
            "targetDigest": expected_left_digest,
            "standaloneDigest": expected_right_digest,
            "targetPresent": True,
            "standalonePresent": right is not missing,
        })
    binding_core = {
        "inputSha256": input_sha256,
        "diagnosticContext": {
            "target": {**context,
                       "messageDigest": target_diagnostic["messageDigest"]},
            "standalone": None,
        },
        "paths": bound_paths,
    }
    binding_digest = sha256_bytes(canonical_bytes(binding_core))
    require(binding_digest == EXPECTED_TARGET_ONLY_BINDING_DIGEST,
            "J360 target-only diagnostic binding digest changed")
    return {
        "disposition": "excluded-non-promoting",
        "reason": "S386 target-only extended-CLASSES CRC-boundary diagnostic",
        "inputSha256": input_sha256,
        "pathCount": 10,
        "diagnosticContext": binding_core["diagnosticContext"],
        "paths": bound_paths,
        "bindingDigest": binding_digest,
    }


def run(args: argparse.Namespace) -> dict[str, Any]:
    manifest_path = args.manifest.resolve()
    require(manifest_path == DEFAULT_MANIFEST.resolve(),
            "only the repository frozen candidate manifest is admitted")
    manifest = validate_frozen_manifest(manifest_path)
    oracle_module = load_module(
        "local_dwg_object_oracle",
        ROOT / manifest["frozenContract"]["sourceOnlyOracleContract"]["path"])
    with tempfile.TemporaryDirectory(prefix="libdxfrw-ac1024-candidates-") as temp:
        private_root = Path(temp)
        generated, generation = generate_ac1024(
            args.writer.resolve(), private_root, manifest, args.timeout)
        oracle_payload, oracle_provenance, discrepancy_messages = inspect_oracle(
            args.oracle.resolve(), generated, manifest, oracle_module, args.timeout)
        try:
            target, standalone, j360_report = run_j360_pair(
                generated, manifest, args.target_receipt.resolve(),
                args.standalone_receipt.resolve(), args.timeout)
        except Exception as exc:
            if exc.__class__.__name__ == "QualifiedDifferentialError":
                raise CandidateError(f"J360 rejected the live evidence: {exc}") from exc
            raise
        qd_binding = load_module(
            "qualified_differential_v2_binding",
            ROOT / manifest["sharedDifferential"]["runnerSource"])
        j360_binding = validate_j360_evidence(
            target, standalone, j360_report,
            generation["generatedSha256"], qd_binding)
        claims = validate_claims(
            manifest, oracle_payload, target, standalone, oracle_module)
        exclusion_evidence = build_exclusion_evidence(
            manifest, oracle_payload, target, standalone, oracle_module)
        discrepancies = bind_discrepancies(
            discrepancy_messages,
            oracle_provenance["oracleDependencyDigest"],
            manifest["discrepancyBinding"]["expectedCurrentRunCount"])

    excluded = [row["id"] for row in claims if row["outcome"] != "validated"]
    return {
        "schema": 1,
        "kind": "libdxfrw-local-ac1024-candidate-evidence",
        "status": "complete" if not excluded else "new-child-required",
        "promotesSupport": False,
        "manifest": {
            "path": str(manifest_path.relative_to(ROOT)),
            "sha256": FROZEN_MANIFEST_SHA256,
            "predeclaredClaimCount": len(manifest["claims"]),
            "unlistedFieldDisposition": "excluded",
        },
        "generation": generation,
        "oracleProvenance": oracle_provenance,
        "j360": {
            "sameInputSha256": generation["generatedSha256"],
            "manifest": {
                "path": manifest["sharedDifferential"]["manifest"],
                "sha256": manifest["sharedDifferential"][
                    "manifestSha256AtFreeze"],
            },
            "runnerSource": {
                "path": manifest["sharedDifferential"]["runnerSource"],
                "sha256": manifest["sharedDifferential"][
                    "runnerSourceSha256AtFreeze"],
            },
            "targetReceiptSha256": sha256_file(args.target_receipt.resolve()),
            "standaloneReceiptSha256": sha256_file(
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
        "excludedClaims": excluded,
        "exclusionEvidence": exclusion_evidence,
        "newChildRequired": bool(excluded),
        "discrepancyCount": len(discrepancies),
        "discrepancies": discrepancies,
        "generatedPayloadsRetained": False,
        "selfReadDisposition": "reachability-only",
    }


def self_test(manifest_path: Path) -> None:
    manifest = validate_frozen_manifest(manifest_path)
    original_environment = dict(os.environ)
    try:
        os.environ.update({
            "HOME": "/poison-home",
            "DYLD_INSERT_LIBRARIES": "/poison.dylib",
            "LIBREDWG_TRACE": "9",
            "DWG_DEBUG": "all",
            "DEBUG": "1",
            "TRACE": "1",
        })
        environment_probe = run_capture(
            ["/usr/bin/env"], 10.0, text=True)
        require(environment_probe.returncode == 0,
                "sanitized environment probe failed")
        observed_environment = dict(
            line.split("=", 1) for line in environment_probe.stdout.splitlines()
            if "=" in line)
        require(observed_environment == MINIMAL_ENVIRONMENT,
                "subprocess inherited a forbidden environment variable")
        with sanitized_process_environment():
            require(dict(os.environ) == MINIMAL_ENVIRONMENT,
                    "J360 environment sanitizer retained parent state")
    finally:
        os.environ.clear()
        os.environ.update(original_environment)
    invalid_json = (
        '{"schema":1,"schema":1}',
        '{"value":NaN}',
        '{"value":Infinity}',
        '{"value":-Infinity}',
        '{"value":1e999}',
    )
    for index, encoded in enumerate(invalid_json):
        try:
            strict_json_loads(encoded, f"self-test-{index}")
        except CandidateError:
            pass
        else:
            raise CandidateError(
                f"strict JSON mutation {index} was unexpectedly accepted")
    oracle_module = load_module(
        "local_dwg_object_oracle_selftest",
        ROOT / manifest["frozenContract"]["sourceOnlyOracleContract"]["path"])
    captured: dict[str, Any] = {}
    original = oracle_module.check_objects

    def wrapper(payload: dict[str, Any], version: str) -> dict[str, Any]:
        value = original(payload, version)
        captured.update(value)
        return value

    oracle_module.check_objects = wrapper
    with contextlib.redirect_stdout(io.StringIO()):
        oracle_module.self_test()
    messages = list(captured["oracleDiscrepancies"])
    messages.append(
        "LibreDWG 0.14 omits DICTIONARYWDFLT item/default handles for AC1024")
    first = bind_discrepancies(messages, "0" * 64, 34)
    second = bind_discrepancies(list(reversed(messages)), "0" * 64, 34)
    require(first == second, "discrepancy bindings are not order-independent")
    require(len({row["fieldScope"] for row in first}) >= 25,
            "discrepancy scope classification is unexpectedly coarse")
    unavailable = validate_claims(
        manifest, {"OBJECTS": []}, {"records": [], "callbacks": []},
        {"records": [], "callbacks": []}, oracle_module)
    require(len(unavailable) == 16
            and all(row["outcome"] == "excluded" for row in unavailable),
            "unavailable predeclared fields did not request a new child")

    qd = load_module(
        "qualified_differential_v2_selftest",
        ROOT / manifest["sharedDifferential"]["runnerSource"])
    config_digest = manifest["sharedDifferential"]["manifestSha256AtFreeze"]
    qd_manifest = read_json(ROOT / manifest["sharedDifferential"]["manifest"])
    empty_rules = {"tolerances": [], "reviewedTargetDebt": [], "exclusions": []}
    exact_target = qd._base_result("target", config_digest)
    exact_standalone = qd._base_result("standalone", config_digest)
    exact_target["input"]["sha256"] = EXPECTED_GENERATED_INPUT_SHA256
    exact_standalone["input"]["sha256"] = EXPECTED_GENERATED_INPUT_SHA256
    exact_report = qd.compare_results(
        exact_target, exact_standalone, empty_rules,
        manifest_digest=config_digest, target_lock=qd_manifest["targetLock"])
    exact_binding = validate_j360_evidence(
        exact_target, exact_standalone, exact_report,
        exact_target["input"]["sha256"], qd)
    require(exact_binding["disposition"] == "exact-non-promoting",
            "exact J360 mutation vector was not accepted")

    target_only = copy.deepcopy(exact_target)
    target_only["diagnostics"] = [{
        **TARGET_ONLY_DIAGNOSTIC_CONTEXT,
        "messageDigest": EXPECTED_TARGET_ONLY_MESSAGE_DIGEST,
    }]
    target_only["cardinality"]["diagnosticCount"] = 1
    target_only_report = qd.compare_results(
        target_only, exact_standalone, empty_rules,
        manifest_digest=config_digest, target_lock=qd_manifest["targetLock"])
    target_only_binding = validate_j360_evidence(
        target_only, exact_standalone, target_only_report,
        target_only["input"]["sha256"], qd)
    require(target_only_binding["pathCount"] == 10
            and target_only_binding["disposition"] == "excluded-non-promoting",
            "target-only diagnostic vector was not bound exactly")

    mutations: list[tuple[dict[str, Any], dict[str, Any]]] = []
    wrong_context = copy.deepcopy(target_only)
    wrong_context["diagnostics"][0]["code"] = "dwg-integrity-2"
    mutations.append((wrong_context, copy.deepcopy(exact_standalone)))
    wrong_message = copy.deepcopy(target_only)
    wrong_message["diagnostics"][0]["messageDigest"] = sha256_bytes(
        b"different CRC payload")
    mutations.append((wrong_message, copy.deepcopy(exact_standalone)))
    extra_difference = copy.deepcopy(target_only)
    extra_difference["records"][0]["fields"][0]["value"] = "BROKEN"
    mutations.append((extra_difference, copy.deepcopy(exact_standalone)))
    unexpected_standalone = copy.deepcopy(exact_standalone)
    unexpected_standalone["diagnostics"] = copy.deepcopy(
        target_only["diagnostics"])
    unexpected_standalone["cardinality"]["diagnosticCount"] = 1
    mutations.append((copy.deepcopy(target_only), unexpected_standalone))
    for index, (mutated_target, mutated_standalone) in enumerate(mutations):
        mutated_report = qd.compare_results(
            mutated_target, mutated_standalone, empty_rules,
            manifest_digest=config_digest, target_lock=qd_manifest["targetLock"])
        try:
            validate_j360_evidence(
                mutated_target, mutated_standalone, mutated_report,
                mutated_target["input"]["sha256"], qd)
        except CandidateError:
            pass
        else:
            raise CandidateError(
                f"J360 mismatch mutation {index} was unexpectedly accepted")
    expected_exclusions = [
        "rtext-insertion-x", "rtext-insertion-y",
        "rtext-insertion-z", "rtext-rotation",
    ]
    exclusion_report = {
        "excludedClaims": list(expected_exclusions),
        "status": "new-child-required",
        "newChildRequired": True,
    }
    validate_expected_exclusions(exclusion_report, expected_exclusions)
    exclusion_mutations = []
    missing_exclusion = copy.deepcopy(exclusion_report)
    missing_exclusion["excludedClaims"].pop()
    exclusion_mutations.append(missing_exclusion)
    extra_exclusion = copy.deepcopy(exclusion_report)
    extra_exclusion["excludedClaims"].append("unexpected-claim")
    exclusion_mutations.append(extra_exclusion)
    wrong_status = copy.deepcopy(exclusion_report)
    wrong_status["status"] = "complete"
    exclusion_mutations.append(wrong_status)
    for index, mutated in enumerate(exclusion_mutations):
        try:
            validate_expected_exclusions(mutated, expected_exclusions)
        except CandidateError:
            pass
        else:
            raise CandidateError(
                f"excluded-claim mutation {index} was unexpectedly accepted")
    print("check_ac1024_candidate_fields self-test: PASS")


def validate_expected_exclusions(report: dict[str, Any],
                                 expected: list[str]) -> None:
    require(len(expected) == len(set(expected)),
            "expected excluded-claim IDs contain duplicates")
    require(sorted(expected) == sorted(report["excludedClaims"]),
            "observed excluded claims differ from the exact expectation")
    require(report["status"] == "new-child-required"
            and report["newChildRequired"] is True,
            "expected exclusions did not preserve new-child-required status")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument("--writer", type=Path)
    parser.add_argument("--oracle", type=Path)
    parser.add_argument("--target-receipt", type=Path)
    parser.add_argument("--standalone-receipt", type=Path)
    parser.add_argument("--expect-excluded-claims", nargs="*")
    parser.add_argument("--timeout", type=float, default=30.0)
    args = parser.parse_args(argv)
    try:
        if args.self_test:
            self_test(args.manifest.resolve())
            return 0
        missing = [name for name in (
            "writer", "oracle", "target_receipt", "standalone_receipt"
        ) if getattr(args, name) is None]
        if missing:
            parser.error("live mode requires " + ", ".join(
                "--" + name.replace("_", "-") for name in missing))
        require(args.timeout > 0.0, "timeout must be positive")
        report = run(args)
        expected_exclusions = args.expect_excluded_claims
        if expected_exclusions is not None:
            validate_expected_exclusions(report, expected_exclusions)
            report["expectedExcludedClaims"] = sorted(expected_exclusions)
        sys.stdout.write(json.dumps(
            report, sort_keys=True, indent=2, allow_nan=False) + "\n")
        return (0 if expected_exclusions is not None
                else 0 if report["status"] == "complete" else 1)
    except (CandidateError, OSError, UnicodeError, json.JSONDecodeError,
            ValueError, AssertionError) as exc:
        print(f"AC1024 candidate evidence: FAIL: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
