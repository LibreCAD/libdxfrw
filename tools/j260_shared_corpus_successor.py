#!/usr/bin/env python3
"""Replay J260/J293 registered streams once without admitting drawing bytes."""
import argparse
import copy
import hashlib
import json
from pathlib import Path, PurePosixPath
import re
import shutil
import subprocess
import sys
import tempfile

import run_external_advisory as advisory

ROOT = Path(__file__).resolve().parent.parent
SET_DIGEST = "741bb8e2fb2468a3aa40e1ab8dda9d9738538337a879315cf104687bdd3063b7"


def set_digest(hashes):
    return hashlib.sha256(("\n".join(sorted(set(hashes))) + "\n").encode("ascii")).hexdigest()


def validate_members(members, corpus, expected_digest=SET_DIGEST):
    if len(members) != 28 or len(set(members)) != 28 or set_digest(members) != expected_digest:
        raise ValueError("unexpected unique source membership")
    matches = [c for c in corpus if c.get("sha256") in members]
    if len(matches) != 28 or {c["sha256"] for c in matches} != set(members):
        raise ValueError("corpus membership missing or duplicated")
    names = []
    for item in matches:
        for alias in item["aliases"]:
            path = PurePosixPath(alias)
            if not alias or path.is_absolute() or ".." in path.parts or str(path) != alias:
                raise ValueError("unsafe corpus alias")
            names.append(alias)
    if len(names) != 29 or len(set(names)) != 29:
        raise ValueError("expected 29 unique path aliases")
    return matches


def self_test():
    hashes = [format(i, "064x") for i in range(28)]
    corpus = [{"sha256": h, "aliases": ["f%d.dwg" % i]} for i, h in enumerate(hashes)]
    corpus[0]["aliases"].append("second/f0.dwg")
    expected = set_digest(hashes)
    assert len(validate_members(hashes, corpus, expected)) == 28
    wrong = copy.deepcopy(corpus)
    wrong[1]["aliases"] = ["../escape.dwg"]
    for ids, rows in ((hashes[:-1], corpus), (hashes, wrong), (hashes, corpus[:-1])):
        try:
            validate_members(ids, rows, expected)
        except ValueError:
            continue
        raise AssertionError("invalid corpus accepted")
    print("J260/J293 shared-corpus successor self-test: PASS")


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--self-test", action="store_true")
    p.add_argument("--source-root", type=Path)
    p.add_argument("--converter", type=Path)
    p.add_argument("--output", type=Path)
    args = p.parse_args()
    if args.self_test:
        self_test()
        return
    if not all((args.source_root, args.converter, args.output)):
        p.error("--source-root, --converter and --output are required")
    source = args.source_root.resolve(strict=True)
    converter = args.converter.resolve(strict=True)
    registry = json.loads((ROOT / "metadata/external-evidence-resolution-v1.json").read_text())
    reports = {x["itemId"]: x for x in registry["reports"]}
    hashes = reports["J260"]["memberSha256s"]
    if hashes != reports["J293"]["memberSha256s"]:
        raise ValueError("historical source sets differ")
    records = validate_members(hashes, registry["corpus"])
    original = []
    with tempfile.TemporaryDirectory(prefix="j260-shared-corpus-") as directory:
        scratch = Path(directory)
        for record in records:
            for alias in record["aliases"]:
                path = source / alias
                if path.is_symlink() or not path.is_file() or advisory.digest(path) != record["sha256"]:
                    raise ValueError("missing or changed registered source: " + alias)
                target = scratch / alias
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(path, target)
                if advisory.digest(target) != record["sha256"]:
                    raise ValueError("scratch copy mismatch: " + alias)
                original.append((path, record["sha256"]))
        binary_hash = advisory.digest(converter)
        # Observe the same single converter invocation used by summarize.
        # Retain stable failure categories and message hashes, not raw logs.
        diagnostics = {}
        original_run = advisory.subprocess.run
        def observed_run(command, **kwargs):
            input_path = Path(command[2 if converter.suffix.lower() == ".py" else 1])
            kwargs["stdout"] = subprocess.PIPE
            kwargs["stderr"] = subprocess.PIPE
            try:
                result = original_run(command, **kwargs)
            except subprocess.TimeoutExpired:
                diagnostics[advisory.digest(input_path)] = {"category": "timeout"}
                raise
            text = (result.stdout + result.stderr).decode("utf-8", errors="replace")
            failure = re.search(r"^DXF export (?:preflight failed|failed|stopped during):[^\r\n]*", text, re.MULTILINE)
            if "Error reading file" in text or "DWG file error:" in text:
                category = "read-failed"
            elif "DXF export preflight failed:" in text:
                category = "export-preflight-refused"
            elif "DXF export failed:" in text:
                category = "export-serialization-failed"
            elif result.returncode == 0:
                category = "converter-success"
            else:
                category = "other-failure"
            entry = {"category": category}
            if failure:
                entry["messageSha256"] = hashlib.sha256(failure.group(0).encode()).hexdigest()
            diagnostics[advisory.digest(input_path)] = entry
            return result
        try:
            advisory.subprocess.run = observed_run
            report = advisory.summarize(scratch, converter, -1, timeout_seconds=4)
        finally:
            advisory.subprocess.run = original_run
        if not advisory.verify_receipt(report):
            raise ValueError("advisory receipt mismatch")
        if (report["pathCount"] != 29 or report["uniqueStreamCount"] != 28
                or report["duplicatePathCount"] != 1 or report["dwgCount"] != 24
                or report["nonDwgCount"] != 4 or report["selectedDwgCount"] != 24
                or report["mislabeledPathCount"] != 4 or report["invalidPathCount"] != 0):
            raise ValueError("unexpected format or deduplication count")
        if {r["sourceSha256"] for r in report["inventoryRows"]} != set(hashes):
            raise ValueError("runtime inventory differs from historical stream set")
        if set(diagnostics) != {r["sourceSha256"] for r in report["rows"]}:
            raise ValueError("missing or unexpected per-stream diagnostic")
        for row in report["rows"]:
            if row["status"] == "converted" and (row.get("outputSize", 0) <= 0 or "outputSha256" not in row):
                raise ValueError("successful converter exit without nonempty output")
            category = diagnostics[row["sourceSha256"]]["category"]
            if ((row["status"] == "converted") != (category == "converter-success")
                    or (row["status"] == "timeout") != (category == "timeout")):
                raise ValueError("diagnostic category disagrees with converter status")
        if advisory.digest(converter) != binary_hash or any(advisory.digest(p) != h for p, h in original):
            raise ValueError("source or executable changed during the run")
    output = {"schema": 1, "kind": "j260-j293-shared-stream-successor-v1",
              "historicalItems": ["J260", "J293"], "historicalSetDigestSha256": SET_DIGEST,
              "advisory": True, "promotesSupportClaims": False,
              "diagnosticsBySourceSha256": diagnostics,
              "run": report,
              "limitations": "Content classification and strict-output behavior differ from historical suffix-only runs; exit status is not semantic or platform qualification. No input or generated drawing is retained."}
    args.output.write_text(json.dumps(output, sort_keys=True, indent=2, allow_nan=False) + "\n")


if __name__ == "__main__":
    try:
        main()
    except (ValueError, KeyError, OSError) as error:
        print("J260/J293 shared-corpus successor: " + str(error), file=sys.stderr)
        sys.exit(1)
