#!/usr/bin/env python3
"""Audit locally retrieved 41368245231 focused archives; never promotes claims.

Pass the directory containing p9-{linux,macos,windows}-focused.zip. The ZIPs
must be obtained from the GitHub Actions artifact IDs recorded below; drawing
payloads are neither extracted nor produced. Git-tree digests concern commits,
not a dirty working tree, and cannot replace the qualification verifier.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import zipfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from check_qualification_digest import _selected, digest_records  # noqa: E402

HISTORICAL = "df03267650f5f51c325867752fa3219bd5c0c7b3"
IMPLEMENTATION = "3614a582653b3493e251f5ed5d8896422e8b48e911d2b155f23d117f5914e486"
ARTIFACTS = {
    "linux": (13107869885, "4739474c65a9fa28000fc56b2353309c59d91d07a23d006ae70a062787005241", "linux-gcc", "Linux", "x86_64"),
    "macos": (13108660833, "90edbf3d8365304ed06546674f1d1a1144b9afa6d2f6b8ea82b87600c088b104", "macos-clang", "Darwin", "arm64"),
    "windows": (13107733970, "6d8c9ccc01188a5ab3ae0eb83a47f495ab22f36afb3a1b213a61937f4fc1fded", "windows-msvc", "Windows", "AMD64"),
}


def sha(data):
    return hashlib.sha256(data).hexdigest()


def committed_digest(revision):
    def git(*args):
        return subprocess.check_output(["git", "-C", str(ROOT), *args])
    commit = git("rev-parse", revision + "^{commit}").decode().strip()
    manifest = json.loads(git("show", commit + ":metadata/qualification-implementation-inputs-v1.json"))
    entries = []
    for entry in git("ls-tree", "-r", "-z", commit).split(b"\0"):
        if not entry:
            continue
        meta, path = entry.split(b"\t", 1)
        mode, kind, oid = meta.decode().split()
        name = path.decode("utf-8")
        if kind == "blob" and _selected(name, manifest["selection"]):
            entries.append((name, mode, oid))
    return commit, digest_records(entries)[0]


def check_archive(path, expected):
    artifact_id, archive_digest, key, system, machine = expected
    raw = path.read_bytes()
    if sha(raw) != archive_digest:
        raise ValueError(f"{key}: archive does not match GitHub artifact digest")
    with zipfile.ZipFile(path) as archive:
        receipt_bytes = archive.read("native-qualification-receipt-v1.json")
        receipt_hash = archive.read("native-qualification-receipt-v1.sha256").decode().split()[0]
        observation = archive.read("native-qualification-observation-v1.json")
    receipt = json.loads(receipt_bytes)
    if (sha(receipt_bytes) != receipt_hash or sha(observation) != receipt["observationSha256"]
            or receipt["repository"]["checkedOutCommit"] != HISTORICAL
            or receipt["run"]["id"] != "41368245231"
            or receipt["run"]["attempt"] != "1"
            or receipt["suite"] != "focused"
            or receipt["qualificationConclusion"] != "success"
            or receipt["platform"]["key"] != key
            or receipt["platform"]["system"] != system
            or receipt["platform"]["machine"] != machine
            or receipt["qualificationDigests"]["implementation"]["value"] != IMPLEMENTATION
            or receipt["suppliesIndependentSemanticEvidence"] is not False
            or receipt["summary"] != {"failed": 0, "notRun": 0, "passed": 7, "skipped": 0, "total": 7}):
        raise ValueError(f"{key}: receipt lineage or result mismatch")
    tests = receipt["tests"]
    if any(t["status"] != "passed" or t["suppliesIndependentSemanticEvidence"] is not False for t in tests):
        raise ValueError(f"{key}: a test status or evidence scope changed")
    return {"artifactId": artifact_id, "archiveSha256": archive_digest,
            "receiptSha256": receipt_hash, "platform": key,
            "tests": [{"id": t["testId"], "mode": t["observedExecutionMode"]} for t in tests]}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--artifact-dir", type=Path, required=True)
    parser.add_argument("--compare-commit", default="HEAD")
    args = parser.parse_args()
    old_commit, old_digest = committed_digest(HISTORICAL)
    if old_commit != HISTORICAL or old_digest != IMPLEMENTATION:
        raise ValueError("historical committed source digest does not match receipts")
    current_commit, current_digest = committed_digest(args.compare_commit)
    outputs = {name: check_archive(args.artifact_dir / f"p9-{name}-focused.zip", value)
               for name, value in ARTIFACTS.items()}
    print(json.dumps({"historicalCommit": old_commit, "historicalDigest": old_digest,
                      "comparedCommit": current_commit, "comparedDigest": current_digest,
                      "sameDigest": current_digest == old_digest,
                      "scope": "historical focused platform metadata; no independent semantics or promotion",
                      "artifacts": outputs}, sort_keys=True))


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, KeyError, zipfile.BadZipFile, subprocess.CalledProcessError) as exc:
        print(f"P9 historical artifact audit failed: {exc}", file=sys.stderr)
        sys.exit(1)
