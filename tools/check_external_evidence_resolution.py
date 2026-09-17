#!/usr/bin/env python3
"""Validate the immutable, non-promoting external-evidence resolution registry."""

from __future__ import annotations

import argparse
import copy
import hashlib
import json
import re
from collections import Counter
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_REGISTRY = ROOT / "metadata/external-evidence-resolution-v1.json"
TARGET_REPORT = ROOT / "metadata/target-advisory-differential-v1.json"
LIBREDWG_REPORT = ROOT / "metadata/libredwg-advisory-differential-v1.json"
AC1024_REPORT = ROOT / "metadata/libredwg-ac1024-advisory-v1.json"
FIXTURE_REGISTRY = ROOT / "metadata/fixture-registry.json"

SHA256_RE = re.compile(r"[0-9a-f]{64}\Z")
GIT_RE = re.compile(r"[0-9a-f]{40}\Z")
EXPECTED_SET_DIGESTS = {
    "J256": "e09d421cd501801f925984e52e7d1626b47b6e6c08a7b2903d02109cb40b1538",
    "J260": "741bb8e2fb2468a3aa40e1ab8dda9d9738538337a879315cf104687bdd3063b7",
    "J284": "93761f9ea5325b55be657baa72be740e12962f42560d165cf994ec763c937af8",
    "J293": "741bb8e2fb2468a3aa40e1ab8dda9d9738538337a879315cf104687bdd3063b7",
    "J295": "7f6a09903097f45ac0368270c724f33ae95d9e9b277e7fc2f228d6f95dd6d897",
}
EXPECTED_SUCCESSORS = {
    "J256": ["J359"],
    "J260": ["J359"],
    "J268": ["J359"],
    "J284": ["J361"],
    "J293": ["J359"],
    "J295": ["J362"],
}
EXPECTED_SLICES = {
    "J256": "S280", "J260": "S284", "J268": "S292",
    "J284": "S308", "J293": "S317", "J295": "S319",
}
EXPECTED_REPOSITORY_BLOB_JOIN_SHA256 = (
    "4f1f6f62a7079fe16d74097ca78c34c950d3cdb24acda6c4316ad1ea1bff1cef"
)
EXPECTED_PRIVATE_ALIAS_JOIN_SHA256 = (
    "1fa70999f2cdbefc66da40d848b1c72b05d71aeb876708fd7217e6a84d046541"
)
EXPECTED_REF_REPOSITORIES = {
    "libdxfrw-local-2026-09-16": "LibreCAD/libdxfrw local clone",
    "librecad-local-2026-09-16": "LibreCAD/LibreCAD local clone",
}
EXPECTED_PROVENANCE_SOURCES = {
    "autodesk-official-samples": {
        "kind": "publicUrl",
        "locator": "https://www.autodesk.com/support/technical/article/caas/tsarticles/ts/01em4r6LLJgnQQVBlk5GqD.html",
        "publisher": "Autodesk",
        "artifactLicenseState": "pending-explicit-artifact-license",
    },
    "engineering-technology-course": {
        "kind": "publicUrl",
        "locator": "https://engineeringtechnology.org/et-curriculum-and-lecture-notes/course-notes-graphics-and-descriptive-geometry/autocad-lab-assignments/autocad-lab-4-geometric-construction/",
        "publisher": "Engineering Technology",
        "artifactLicenseState": "pending-explicit-artifact-license",
    },
    "librecad-history-3c028612": {
        "kind": "repositoryCommit",
        "locator": "https://github.com/LibreCAD/LibreCAD/commit/3c028612dd8e75d98692ac346635c190500eea94",
        "publisher": "LibreCAD/LibreCAD contributors",
        "artifactLicenseState": "repository-covered-GPL-2.0-or-later",
    },
    "unresolved-private": {
        "kind": "unresolved", "locator": None, "publisher": None,
        "artifactLicenseState": "unknown",
    },
    "unresolved-local-derived": {
        "kind": "unresolvedLocalDerived", "locator": None, "publisher": None,
        "artifactLicenseState": "unknown",
    },
}
EXPECTED_COUNTS = {
    "J256": {"pathCount": 20, "uniqueStreamCount": 20, "dwgCount": 16,
             "nonDwgCount": 4, "converted": 10, "failed": 9, "timeout": 1,
             "improvements": None, "regressions": None},
    "J260": {"pathCount": 29, "uniqueStreamCount": 28, "dwgCount": 24,
             "nonDwgCount": 4, "converted": 18, "failed": 10, "timeout": 1,
             "improvements": None, "regressions": None},
    "J268": {"pathCount": 28, "uniqueStreamCount": None, "dwgCount": None,
             "nonDwgCount": None, "converted": None, "failed": None,
             "timeout": 1, "improvements": 5, "regressions": 0},
    "J284": {"pathCount": 21, "uniqueStreamCount": 21, "dwgCount": 21,
             "nonDwgCount": 0, "converted": 21, "failed": 0, "timeout": 0,
             "improvements": None, "regressions": None},
    "J293": {"pathCount": 29, "uniqueStreamCount": 28, "dwgCount": 24,
             "nonDwgCount": 4, "converted": 20, "failed": 8, "timeout": 1,
             "improvements": None, "regressions": None},
    "J295": {"pathCount": 9, "uniqueStreamCount": 9, "dwgCount": 9,
             "nonDwgCount": 0, "converted": 9, "failed": 0, "timeout": 0,
             "improvements": None, "regressions": None},
}
EXPECTED_REF_AUDITS = {
    "libdxfrw-local-2026-09-16": (17, 15,
        "bd11ff8307e3442c3a797504245002d90d10edc10708e76b04d39ebd1e47db5e"),
    "librecad-local-2026-09-16": (509, 382,
        "813de9c681efd120a53777f5f6e1a3a1bbc6d6f27690953d362598bbc5f9e286"),
}
EXPECTED_ADMITTED = [
    "tests/fixtures/dwg/large_radial.dwg",
    "tests/fixtures/dwg/mpolygon_solid.dwg",
    "tests/fixtures/dwg/rtext_arctext.dwg",
]
BANNED_KEYS = {
    "claimstate", "claimstatus", "promoted", "promotion", "promotionstate",
    "promotionstatus", "qualified", "qualificationstatus", "support",
    "supportclaim", "supportclaims", "supportstate", "supportstatus", "supported",
}


class RegistryError(RuntimeError):
    """Raised when the evidence registry is incomplete or internally inconsistent."""


def load(path: Path) -> dict[str, object]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as exc:
        raise RegistryError(f"could not read {path}: {exc}") from exc
    if not isinstance(value, dict):
        raise RegistryError(f"{path} must contain a JSON object")
    return value


def _keys(value: object, expected: set[str], label: str) -> dict[str, object]:
    if not isinstance(value, dict):
        raise RegistryError(f"{label} must be an object")
    actual = set(value)
    if actual != expected:
        raise RegistryError(
            f"{label} keys differ: missing={sorted(expected - actual)}, "
            f"extra={sorted(actual - expected)}"
        )
    return value


def _hex(value: object, label: str, git: bool = False) -> str:
    pattern = GIT_RE if git else SHA256_RE
    if not isinstance(value, str) or pattern.fullmatch(value) is None:
        raise RegistryError(f"{label} must be a lowercase {'Git' if git else 'SHA-256'} id")
    return value


def _nonnegative(value: object, label: str) -> int:
    if not isinstance(value, int) or isinstance(value, bool) or value < 0:
        raise RegistryError(f"{label} must be a non-negative integer")
    return value


def _reject_banned_fields(value: object, label: str = "registry") -> None:
    if isinstance(value, dict):
        for key, child in value.items():
            normalized = "".join(character for character in key.lower() if character.isalnum())
            if (normalized in BANNED_KEYS
                    or normalized.startswith("support")
                    or normalized.startswith("promot")
                    or normalized.startswith("claim")):
                raise RegistryError(f"{label}.{key} is a forbidden support/promotion field")
            _reject_banned_fields(child, f"{label}.{key}")
    elif isinstance(value, list):
        for index, child in enumerate(value):
            _reject_banned_fields(child, f"{label}[{index}]")


def _set_digest(hashes: list[str]) -> str:
    payload = "".join(f"{value}\n" for value in sorted(set(hashes))).encode("ascii")
    return hashlib.sha256(payload).hexdigest()


def _load_rows(path: Path) -> list[dict[str, object]]:
    value = load(path).get("rows")
    if not isinstance(value, list) or any(not isinstance(row, dict) for row in value):
        raise RegistryError(f"{path} has invalid rows")
    return value


def validate(registry: dict[str, object]) -> None:
    _reject_banned_fields(registry)
    _keys(registry, {
        "schema", "kind", "evidenceDate", "fixturePolicy", "aliasBasis",
        "setDigestDefinition", "localRefAudits", "provenanceSources", "reports",
        "corpus", "joins",
    }, "registry")
    if registry["schema"] != 1 or registry["kind"] != "external-evidence-resolution-v1":
        raise RegistryError("unexpected registry schema or kind")
    if registry["evidenceDate"] != "2026-09-16":
        raise RegistryError("evidenceDate drifted")
    if registry["fixturePolicy"] != "metadata-only; no drawing payloads; non-promoting":
        raise RegistryError("fixturePolicy drifted")
    if registry["aliasBasis"] != (
        "local relative-name to retained report sourceSha256 join; retained private reports have no paths"
    ):
        raise RegistryError("aliasBasis must disclose the local hash join")
    if registry["setDigestDefinition"] != (
        "SHA-256 over LF-terminated sorted unique lowercase source SHA-256 values"
    ):
        raise RegistryError("setDigestDefinition drifted")

    audits = registry["localRefAudits"]
    if not isinstance(audits, list) or len(audits) != 2:
        raise RegistryError("localRefAudits must contain exactly two snapshots")
    audit_ids: set[str] = set()
    for index, raw in enumerate(audits):
        row = _keys(raw, {"id", "repository", "refNamespaces", "evidenceDate",
                          "canonicalRecordFormat", "refCount", "uniqueTipCount",
                          "refTipRecordsSha256", "result"}, f"localRefAudits[{index}]")
        audit_id = row["id"]
        if not isinstance(audit_id, str) or audit_id in audit_ids:
            raise RegistryError("local ref audit ids must be unique strings")
        audit_ids.add(audit_id)
        if audit_id not in EXPECTED_REF_AUDITS:
            raise RegistryError(f"unknown local ref audit: {audit_id}")
        expected_count, expected_unique, expected_digest = EXPECTED_REF_AUDITS[audit_id]
        if row["repository"] != EXPECTED_REF_REPOSITORIES[audit_id]:
            raise RegistryError(f"{audit_id} repository identity drifted")
        if row["refNamespaces"] != ["refs/heads", "refs/remotes", "refs/tags"]:
            raise RegistryError(f"{audit_id} ref namespace scope drifted")
        if row["evidenceDate"] != registry["evidenceDate"]:
            raise RegistryError(f"{audit_id} evidence date drifted")
        if row["canonicalRecordFormat"] != "sorted refname NUL object-id LF":
            raise RegistryError(f"{audit_id} canonical format drifted")
        if (row["refCount"], row["uniqueTipCount"], row["refTipRecordsSha256"]) != (
            expected_count, expected_unique, expected_digest
        ):
            raise RegistryError(f"{audit_id} ref-tip receipt drifted")
        if row["result"] != "zero private-corpus blobs reachable from audited commit tips":
            raise RegistryError(f"{audit_id} result drifted")

    sources = registry["provenanceSources"]
    if not isinstance(sources, list) or len(sources) != 5:
        raise RegistryError("provenanceSources must contain the five audited dispositions")
    source_by_id: dict[str, dict[str, object]] = {}
    for index, raw in enumerate(sources):
        row = _keys(raw, {"id", "kind", "locator", "publisher", "artifactLicenseState",
                          "localRefAuditIds", "note"}, f"provenanceSources[{index}]")
        source_id = row["id"]
        if not isinstance(source_id, str) or source_id in source_by_id:
            raise RegistryError("provenance source ids must be unique strings")
        refs = row["localRefAuditIds"]
        if not isinstance(refs, list) or not refs or any(ref not in audit_ids for ref in refs):
            raise RegistryError(f"{source_id} references an unknown ref audit")
        expected_source = EXPECTED_PROVENANCE_SOURCES.get(source_id)
        if expected_source is None:
            raise RegistryError(f"unknown provenance source: {source_id}")
        for field, expected_value in expected_source.items():
            if row[field] != expected_value:
                raise RegistryError(f"{source_id}.{field} drifted")
        if row["kind"] in {"publicUrl", "repositoryCommit"}:
            if not isinstance(row["locator"], str) or not row["locator"].startswith("https://"):
                raise RegistryError(f"{source_id} must have an HTTPS locator")
            if not isinstance(row["publisher"], str) or not row["publisher"]:
                raise RegistryError(f"{source_id} lacks publisher provenance")
            if not isinstance(row["artifactLicenseState"], str) or not row["artifactLicenseState"]:
                raise RegistryError(f"{source_id} lacks artifact license state")
        source_by_id[source_id] = row
    if set(source_by_id) != set(EXPECTED_PROVENANCE_SOURCES):
        raise RegistryError("provenance source inventory drifted")
    if source_by_id["autodesk-official-samples"]["artifactLicenseState"] != "pending-explicit-artifact-license":
        raise RegistryError("Autodesk artifact license must remain pending")
    if source_by_id["engineering-technology-course"]["artifactLicenseState"] != "pending-explicit-artifact-license":
        raise RegistryError("ET artifact license must remain pending")

    reports = registry["reports"]
    if not isinstance(reports, list) or len(reports) != 6:
        raise RegistryError("reports must contain exactly J256/J260/J268/J284/J293/J295")
    report_by_id: dict[str, dict[str, object]] = {}
    for index, raw in enumerate(reports):
        row = _keys(raw, {"itemId", "sliceId", "membershipBasis", "evidenceDate",
                          "historicalDisposition", "archivedNonPromoting", "supersededBy",
                          "reportedCounts", "memberSha256s", "setDigestSha256", "membershipNote"},
                    f"reports[{index}]")
        item_id = row["itemId"]
        if not isinstance(item_id, str) or item_id in report_by_id:
            raise RegistryError("report item ids must be unique strings")
        report_by_id[item_id] = row
        if row["evidenceDate"] != registry["evidenceDate"]:
            raise RegistryError(f"{item_id} evidenceDate drifted")
        if row["sliceId"] != EXPECTED_SLICES.get(item_id):
            raise RegistryError(f"{item_id} slice binding drifted")
        if row["historicalDisposition"] != "DEFERRED_EXTERNAL":
            raise RegistryError(f"{item_id} historical disposition was rewritten")
        if row["archivedNonPromoting"] is not True:
            raise RegistryError(f"{item_id} is not explicitly archived/non-promoting")
        if row["supersededBy"] != EXPECTED_SUCCESSORS.get(item_id):
            raise RegistryError(f"{item_id} successor mapping drifted")
        if row["reportedCounts"] != EXPECTED_COUNTS.get(item_id):
            raise RegistryError(f"{item_id} report counts drifted")
        if item_id == "J268":
            if row["membershipBasis"] != "reconstructed":
                raise RegistryError("J268 must remain reconstructed")
            if row["memberSha256s"] is not None or row["setDigestSha256"] is not None:
                raise RegistryError("J268 cannot claim exact membership or a set digest")
            if row["membershipNote"] != "no durable row report; exact membership and digest unprovable":
                raise RegistryError("J268 uncertainty note drifted")
            continue
        if row["membershipBasis"] != "observed":
            raise RegistryError(f"{item_id} must have observed membership")
        members = row["memberSha256s"]
        if (not isinstance(members, list) or not members
                or any(not isinstance(value, str) or SHA256_RE.fullmatch(value) is None
                       for value in members)
                or members != sorted(members) or len(members) != len(set(members))):
            raise RegistryError(f"{item_id} member hashes must be sorted and unique")
        digest = _set_digest(members)
        if digest != row["setDigestSha256"] or digest != EXPECTED_SET_DIGESTS.get(item_id):
            raise RegistryError(f"{item_id} set digest drifted")
    if set(report_by_id) != set(EXPECTED_SUCCESSORS):
        raise RegistryError("report item inventory drifted")
    if report_by_id["J260"]["memberSha256s"] != report_by_id["J293"]["memberSha256s"]:
        raise RegistryError("J260 and J293 must retain the same unique-stream set")
    if not set(report_by_id["J256"]["memberSha256s"]) < set(report_by_id["J260"]["memberSha256s"]):
        raise RegistryError("J256 must remain a strict subset of J260")
    if not set(report_by_id["J295"]["memberSha256s"]) < set(report_by_id["J260"]["memberSha256s"]):
        raise RegistryError("J295 must remain a strict subset of J260")

    corpus = registry["corpus"]
    if not isinstance(corpus, list) or len(corpus) != 49:
        raise RegistryError("corpus must contain exactly 49 unique hash rows")
    corpus_by_hash: dict[str, dict[str, object]] = {}
    source_counts: Counter[str] = Counter()
    format_counts: Counter[str] = Counter()
    admission_counts: Counter[str] = Counter()
    for index, raw in enumerate(corpus):
        row = _keys(raw, {"sha256", "byteSize", "aliases", "detectedFormat", "detectedMagic",
                          "detectedVersion", "duplicateRelationship", "originDisposition",
                          "provenanceSourceId", "provenanceDisposition", "repositoryBlob",
                          "admissionDisposition"}, f"corpus[{index}]")
        sha = _hex(row["sha256"], f"corpus[{index}].sha256")
        if sha in corpus_by_hash:
            raise RegistryError(f"duplicate corpus hash: {sha}")
        corpus_by_hash[sha] = row
        _nonnegative(row["byteSize"], f"corpus[{index}].byteSize")
        aliases = row["aliases"]
        if (not isinstance(aliases, list) or not aliases or aliases != sorted(aliases)
                or len(aliases) != len(set(aliases))
                or any(not isinstance(alias, str) or not alias or alias.startswith("/")
                       or ".." in Path(alias).parts for alias in aliases)):
            raise RegistryError(f"corpus[{index}].aliases must be unique safe relative names")
        duplicate = row["duplicateRelationship"]
        if len(aliases) == 1:
            if duplicate is not None:
                raise RegistryError(f"corpus[{index}] invents a duplicate relationship")
        else:
            expected_duplicate = {"canonicalAlias": aliases[0], "duplicateAliases": aliases[1:]}
            if duplicate != expected_duplicate:
                raise RegistryError(f"corpus[{index}] duplicate relationship drifted")
        if row["detectedFormat"] not in {"DWG", "DXF_ASCII"}:
            raise RegistryError(f"corpus[{index}] has an invalid detected format")
        if row["detectedFormat"] == "DWG":
            if row["detectedMagic"] != row["detectedVersion"]:
                raise RegistryError(f"corpus[{index}] DWG magic/version mismatch")
        elif row["detectedMagic"] != "DXF_ASCII_GROUP_CODES" or row["detectedVersion"] != "AC1021":
            raise RegistryError(f"corpus[{index}] mislabeled DXF classification drifted")
        source_id = row["provenanceSourceId"]
        if source_id not in source_by_id:
            raise RegistryError(f"corpus[{index}] references an unknown provenance source")
        source_counts[source_id] += 1
        format_counts[row["detectedFormat"]] += 1
        admission_counts[row["admissionDisposition"]] += 1
        repository_blob = row["repositoryBlob"]
        expected_dispositions = {
            "autodesk-official-samples": (
                "reproducible-public-origin",
                "official-public-download-sha256-observed",
                "external-origin-not-admitted",
            ),
            "engineering-technology-course": (
                "reproducible-public-origin",
                "public-download-sha256-observed-artifact-license-unconfirmed",
                "external-origin-not-admitted",
            ),
            "unresolved-private": (
                "unresolved-origin",
                "unresolved-no-public-or-reachable-repository-match",
                "unresolved-external-not-admitted",
            ),
            "unresolved-local-derived": (
                "unresolved-local-derived-origin",
                "filename-related-local-derived-bytes-no-source-proof",
                "non-dwg-excluded-not-admitted",
            ),
        }
        if source_id in expected_dispositions:
            actual_dispositions = (
                row["originDisposition"], row["provenanceDisposition"],
                row["admissionDisposition"],
            )
            if actual_dispositions != expected_dispositions[source_id]:
                raise RegistryError(f"corpus[{index}] provenance/admission disposition drifted")
        if source_id == "librecad-history-3c028612":
            blob = _keys(repository_blob, {"repository", "commit", "path", "blob"},
                         f"corpus[{index}].repositoryBlob")
            if blob["repository"] != "LibreCAD/LibreCAD":
                raise RegistryError("J284 repository provenance drifted")
            _hex(blob["commit"], f"corpus[{index}].repositoryBlob.commit", git=True)
            _hex(blob["blob"], f"corpus[{index}].repositoryBlob.blob", git=True)
            if blob["commit"] != "3c028612dd8e75d98692ac346635c190500eea94":
                raise RegistryError("J284 historical corpus commit drifted")
            if blob["path"] != f"librecad/src/lib/filters/tests/testdata/{aliases[0]}":
                raise RegistryError("J284 repository path does not match its alias")
            if (row["originDisposition"] != "historical-repository-origin"
                    or row["provenanceDisposition"] != "historical-librecad-repository-blob"
                    or row["admissionDisposition"] not in {
                        "admitted-fixture-exact-sha256",
                        "historical-repository-blob-not-admitted",
                    }):
                raise RegistryError(f"corpus[{index}] historical disposition drifted")
        elif repository_blob is not None:
            raise RegistryError(f"corpus[{index}] has repository fields without repository provenance")
    if list(corpus_by_hash) != sorted(corpus_by_hash):
        raise RegistryError("corpus rows must be sorted by SHA-256")
    if source_counts != Counter({
        "autodesk-official-samples": 17,
        "engineering-technology-course": 1,
        "librecad-history-3c028612": 21,
        "unresolved-private": 6,
        "unresolved-local-derived": 4,
    }):
        raise RegistryError(f"provenance disposition counts drifted: {dict(source_counts)}")
    if format_counts != Counter({"DWG": 45, "DXF_ASCII": 4}):
        raise RegistryError(f"detected format counts drifted: {dict(format_counts)}")
    if admission_counts != Counter({
        "external-origin-not-admitted": 18,
        "historical-repository-blob-not-admitted": 18,
        "unresolved-external-not-admitted": 6,
        "non-dwg-excluded-not-admitted": 4,
        "admitted-fixture-exact-sha256": 3,
    }):
        raise RegistryError(f"admission disposition counts drifted: {dict(admission_counts)}")
    blob_records = []
    for sha, row in corpus_by_hash.items():
        blob = row["repositoryBlob"]
        if blob is not None:
            blob_records.append(f"{sha}\0{blob['path']}\0{blob['blob']}\n")
    blob_join_digest = hashlib.sha256(
        "".join(sorted(blob_records)).encode("utf-8")
    ).hexdigest()
    if blob_join_digest != EXPECTED_REPOSITORY_BLOB_JOIN_SHA256:
        raise RegistryError("historical repository path/blob join drifted")
    observed_union: set[str] = set()
    for item_id, row in report_by_id.items():
        if item_id != "J268":
            observed_union.update(row["memberSha256s"])
    if observed_union != set(corpus_by_hash):
        raise RegistryError("corpus rows do not exactly cover all observed report memberships")
    private_hashes = set(report_by_id["J260"]["memberSha256s"])
    private_aliases = [
        alias for sha in private_hashes for alias in corpus_by_hash[sha]["aliases"]
    ]
    private_alias_records = [
        f"{sha}\0{alias}\n"
        for sha in private_hashes for alias in corpus_by_hash[sha]["aliases"]
    ]
    duplicated_private = [
        sha for sha in private_hashes if len(corpus_by_hash[sha]["aliases"]) > 1
    ]
    if len(private_aliases) != 29 or duplicated_private != [
        "21b49e383cc43171fd13e2bbcd1c235bc1a57804be97f63a0d581fb944e743ff"
    ]:
        raise RegistryError("private alias/duplicate inventory drifted")
    if hashlib.sha256(
        "".join(sorted(private_alias_records)).encode("utf-8")
    ).hexdigest() != EXPECTED_PRIVATE_ALIAS_JOIN_SHA256:
        raise RegistryError("private hash/alias join drifted")
    if any("/Users/" in alias or "\\" in alias for alias in private_aliases):
        raise RegistryError("private aliases must not disclose host paths")
    if any(corpus_by_hash[sha]["detectedVersion"] != "AC1024"
           for sha in report_by_id["J295"]["memberSha256s"]):
        raise RegistryError("J295 must contain only detected AC1024 streams")
    private_format_versions = Counter(
        (corpus_by_hash[sha]["detectedFormat"],
         corpus_by_hash[sha]["detectedVersion"])
        for sha in private_hashes
    )
    if private_format_versions != Counter({
        ("DWG", "AC1015"): 1, ("DWG", "AC1018"): 2,
        ("DWG", "AC1021"): 11, ("DWG", "AC1024"): 9,
        ("DWG", "AC1027"): 1, ("DXF_ASCII", "AC1021"): 4,
    }):
        raise RegistryError("private format/version inventory drifted")

    joins = _keys(registry["joins"], {"j284", "j295"}, "joins")
    j284 = _keys(joins["j284"], {"targetReport", "independentReport", "joinedHashCount",
                                      "admittedFixtureIds"}, "joins.j284")
    if (j284["targetReport"] != "metadata/target-advisory-differential-v1.json"
            or j284["independentReport"] != "metadata/libredwg-advisory-differential-v1.json"
            or j284["joinedHashCount"] != 21
            or j284["admittedFixtureIds"] != EXPECTED_ADMITTED):
        raise RegistryError("J284 join declaration drifted")
    target_rows = _load_rows(TARGET_REPORT)
    independent_rows = _load_rows(LIBREDWG_REPORT)
    target_by_hash = {row.get("sourceSha256"): row for row in target_rows}
    independent_by_hash = {row.get("sourceSha256"): row for row in independent_rows}
    j284_members = set(report_by_id["J284"]["memberSha256s"])
    if set(target_by_hash) != j284_members or set(independent_by_hash) != j284_members:
        raise RegistryError("J284 does not exactly join both committed 21-row reports")
    for sha in j284_members:
        target_row = target_by_hash[sha]
        independent_row = independent_by_hash[sha]
        corpus_row = corpus_by_hash[sha]
        if target_row.get("path") != independent_row.get("path") or corpus_row["aliases"] != [target_row.get("path")]:
            raise RegistryError(f"J284 alias join drifted for {sha}")
        if target_row.get("sourceSize") != independent_row.get("sourceSize") or corpus_row["byteSize"] != target_row.get("sourceSize"):
            raise RegistryError(f"J284 size join drifted for {sha}")
    fixture_rows = load(FIXTURE_REGISTRY).get("fixtures")
    if not isinstance(fixture_rows, list):
        raise RegistryError("fixture registry rows are missing")
    fixture_by_path = {row.get("path"): row for row in fixture_rows if isinstance(row, dict)}
    admitted_hashes: set[str] = set()
    for fixture_id in EXPECTED_ADMITTED:
        fixture = fixture_by_path.get(fixture_id)
        if not isinstance(fixture, dict):
            raise RegistryError(f"missing admitted fixture: {fixture_id}")
        sha = fixture.get("sha256")
        if sha not in j284_members or corpus_by_hash[sha]["admissionDisposition"] != "admitted-fixture-exact-sha256":
            raise RegistryError(f"admitted J284 fixture join drifted: {fixture_id}")
        admitted_hashes.add(sha)
    if {sha for sha in j284_members if corpus_by_hash[sha]["admissionDisposition"] == "admitted-fixture-exact-sha256"} != admitted_hashes:
        raise RegistryError("J284 contains an undeclared admitted fixture")

    j295 = _keys(joins["j295"], {"independentReport", "joinedHashCount"}, "joins.j295")
    if (j295["independentReport"] != "metadata/libredwg-ac1024-advisory-v1.json"
            or j295["joinedHashCount"] != 9):
        raise RegistryError("J295 join declaration drifted")
    ac1024_hashes = {row.get("sourceSha256") for row in _load_rows(AC1024_REPORT)}
    if ac1024_hashes != set(report_by_id["J295"]["memberSha256s"]):
        raise RegistryError("J295 does not exactly join the committed nine-row report")


def self_test() -> None:
    registry = load(DEFAULT_REGISTRY)
    validate(registry)
    invalid = copy.deepcopy(registry)
    invalid["reports"][0]["supportState"] = "qualified"
    try:
        validate(invalid)
    except RegistryError:
        pass
    else:
        raise AssertionError("support-state field was accepted")
    invalid = copy.deepcopy(registry)
    invalid["reports"][2]["memberSha256s"] = []
    try:
        validate(invalid)
    except RegistryError:
        pass
    else:
        raise AssertionError("invented J268 membership was accepted")
    invalid = copy.deepcopy(registry)
    invalid["reports"][0]["memberSha256s"] = invalid["reports"][0]["memberSha256s"][1:]
    try:
        validate(invalid)
    except RegistryError:
        pass
    else:
        raise AssertionError("set membership drift was accepted")
    invalid = copy.deepcopy(registry)
    invalid["corpus"][0]["sha256"] = invalid["corpus"][1]["sha256"]
    try:
        validate(invalid)
    except RegistryError:
        pass
    else:
        raise AssertionError("duplicate corpus hash was accepted")
    invalid = copy.deepcopy(registry)
    invalid["provenanceSources"][0]["localRefAuditIds"] = ["unknown-audit"]
    try:
        validate(invalid)
    except RegistryError:
        pass
    else:
        raise AssertionError("unknown ref audit was accepted")
    print("external evidence resolution self-test: PASS")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--registry", type=Path, default=DEFAULT_REGISTRY)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args(argv)
    try:
        if args.self_test:
            self_test()
        else:
            validate(load(args.registry))
            print("external evidence resolution: PASS (49 hashes; 6 archived reports; non-promoting)")
        return 0
    except (RegistryError, AssertionError) as exc:
        print(f"external evidence resolution: FAIL: {exc}")
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
