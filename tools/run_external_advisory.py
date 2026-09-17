#!/usr/bin/env python3
"""Summarize an external drawing corpus without admitting its bytes to Git.

The report is intentionally non-reconstructive: it records content hashes,
sizes, magic-derived formats/versions, normalized root-relative path aliases,
exit statuses, and bounded diagnostic categories. It never records the
external root, converter path, tool prose, or drawing bytes. Converted DXF
files remain in a temporary directory.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import re
import subprocess
import sys
import tempfile
from collections import Counter
from pathlib import Path


DWG_MAGIC = re.compile(
    rb"(?:AC\d{4}|AC1\.40|AC1\.50|AC2\.10|AC1\.2\x00|AC1\.4\x00|MC0\.0\x00)"
)
BINARY_DXF_MAGIC = b"AutoCAD Binary DXF\r\n\x1a\x00"
PREFIX_SIZE = 64 * 1024


def digest(path: Path) -> str:
    hasher = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            hasher.update(chunk)
    return hasher.hexdigest()


def canonical_digest(value) -> str:
    encoded = json.dumps(
        value, ensure_ascii=True, separators=(",", ":"), sort_keys=True
    ).encode("ascii")
    return hashlib.sha256(encoded).hexdigest()


def ascii_dxf_version(prefix: bytes) -> tuple[bool, str]:
    """Recognize an ASCII DXF prologue and return recognition plus version."""

    if prefix.startswith(b"\xef\xbb\xbf"):
        prefix = prefix[3:]
    lines = [line.strip() for line in prefix.splitlines()]
    while lines and not lines[0]:
        lines.pop(0)
    pairs = list(zip(lines[0::2], lines[1::2]))
    index = 0
    # Group 999 comments are legal before the first SECTION. libdxfrw itself
    # emits such a producer comment, including in the historical mislabeled
    # corpus inputs that motivated this classifier.
    while index < len(pairs) and pairs[index][0] == b"999":
        index += 1
    if (
        index >= len(pairs)
        or pairs[index][0] != b"0"
        or pairs[index][1].upper() != b"SECTION"
    ):
        return False, "UNKNOWN"
    for pair_index, (code, value) in enumerate(pairs[index + 1:], index + 1):
        if code == b"9" and value.upper() == b"$ACADVER":
            if pair_index + 1 < len(pairs):
                version_code, version_value = pairs[pair_index + 1]
                if version_code == b"1" and DWG_MAGIC.fullmatch(version_value.upper()):
                    return True, version_value.decode("ascii").upper()
            break
    # HEADER/$ACADVER is optional in a syntactically valid ASCII DXF.  The
    # leading 0/SECTION pair is therefore enough to classify the format, but
    # never enough to invent a version.
    return True, "UNKNOWN"


def classify_prefix(prefix: bytes) -> tuple[str, str, str]:
    """Return (format, version, stable magic label) from content only."""

    if DWG_MAGIC.fullmatch(prefix[:6]):
        dwg_version = prefix[:6].decode("ascii")
        return "DWG", dwg_version, dwg_version
    if prefix.startswith(BINARY_DXF_MAGIC):
        return "DXF_BINARY", "UNKNOWN", "AutoCAD Binary DXF"
    is_ascii_dxf, dxf_version = ascii_dxf_version(prefix)
    if is_ascii_dxf:
        return "DXF_ASCII", dxf_version, "0/SECTION"
    return "UNKNOWN", "UNKNOWN", "UNKNOWN"


def declared_format(path: Path) -> str:
    suffix = path.suffix.lower()
    if suffix == ".dwg":
        return "DWG"
    if suffix == ".dxf":
        return "DXF"
    return "UNRECOGNIZED"


def broad_format(detected_format: str) -> str:
    if detected_format == "DWG":
        return "DWG"
    if detected_format in {"DXF_ASCII", "DXF_BINARY"}:
        return "DXF"
    return "UNKNOWN"


def path_issue(declared: str, detected: str) -> tuple[str, str] | None:
    actual = broad_format(detected)
    if actual == "UNKNOWN":
        if declared in {"DWG", "DXF"}:
            return "invalid", "drawing-suffix-with-unknown-magic"
        return None
    if declared == "UNRECOGNIZED":
        return "mislabeled", "drawing-magic-with-unrecognized-suffix"
    if declared != actual:
        return "mislabeled", "suffix-does-not-match-magic"
    return None


def discover(root: Path) -> tuple[list[dict], list[dict], list[dict]]:
    """Classify candidates by magic, then group identical content by SHA-256."""

    streams: dict[str, dict] = {}
    aliases: list[dict] = []
    issues: list[dict] = []
    candidates = sorted(
        path for path in root.rglob("*")
        if path.is_file() and not path.is_symlink()
    )
    for path in candidates:
        with path.open("rb") as stream:
            prefix = stream.read(PREFIX_SIZE)
        detected, input_version, magic = classify_prefix(prefix)
        declared = declared_format(path)

        # Unknown unrelated files are not corpus candidates. Known drawing
        # magic is included regardless of suffix; drawing suffixes are
        # included even when their content is invalid or mislabeled.
        if detected == "UNKNOWN" and declared == "UNRECOGNIZED":
            continue

        source_hash = digest(path)
        relative = path.relative_to(root).as_posix()
        size = path.stat().st_size
        aliases.append({
            "declaredFormat": declared,
            "pathAlias": relative,
            "sourceSha256": source_hash,
        })

        existing = streams.get(source_hash)
        if existing is None:
            streams[source_hash] = {
                "_paths": [path],
                "detectedFormat": detected,
                "detectedMagic": magic,
                "inputVersion": input_version,
                "pathAliases": [relative],
                "sourceSha256": source_hash,
                "sourceSize": size,
            }
        else:
            # Equal SHA-256 values are the deduplication identity. Retain
            # every normalized alias so duplicates stay visible.
            if (
                existing["sourceSize"] != size
                or existing["detectedFormat"] != detected
                or existing["inputVersion"] != input_version
                or existing["detectedMagic"] != magic
            ):
                raise OSError("inconsistent metadata for identical source SHA-256")
            existing["_paths"].append(path)
            existing["pathAliases"].append(relative)

        issue = path_issue(declared, detected)
        if issue is not None:
            classification, reason = issue
            issues.append({
                "classification": classification,
                "declaredFormat": declared,
                "detectedFormat": detected,
                "pathAlias": relative,
                "reason": reason,
                "sourceSha256": source_hash,
            })

    ordered_streams = []
    for source_hash in sorted(streams):
        stream = streams[source_hash]
        paired = sorted(
            zip(stream["pathAliases"], stream["_paths"]),
            key=lambda item: item[0],
        )
        stream["pathAliases"] = [item[0] for item in paired]
        stream["_paths"] = [item[1] for item in paired]
        ordered_streams.append(stream)
    aliases.sort(key=lambda item: (item["pathAlias"], item["sourceSha256"]))
    issues.sort(key=lambda item: (item["pathAlias"], item["sourceSha256"]))
    return ordered_streams, aliases, issues


def public_stream(stream: dict) -> dict:
    return {key: value for key, value in stream.items() if not key.startswith("_")}


def source_path(stream: dict) -> Path:
    """Prefer a correctly suffixed alias when invoking the DWG converter."""

    return min(
        stream["_paths"],
        key=lambda path: (path.suffix.lower() != ".dwg", path.as_posix()),
    )


def summarize(root: Path, converter: Path | None, limit: int,
              timeout_seconds: float = 0.0) -> dict:
    if limit < -1:
        raise ValueError("limit must be -1 (unlimited) or non-negative")
    if not math.isfinite(timeout_seconds) or timeout_seconds < 0:
        raise ValueError("timeout_seconds must be finite and non-negative")

    streams, aliases, issues = discover(root)
    dwg_streams = [stream for stream in streams if stream["detectedFormat"] == "DWG"]
    selected = dwg_streams if limit == -1 else dwg_streams[:limit]
    converter_hash = digest(converter) if converter is not None else None

    rows = []
    with tempfile.TemporaryDirectory(prefix="libdxfrw-advisory-") as temp:
        output_root = Path(temp)
        for index, stream in enumerate(selected):
            row = public_stream(stream)
            row["status"] = "inventoried"
            if converter is not None:
                output = output_root / (str(index) + ".dxf")
                command = [
                    *([sys.executable, str(converter)]
                      if converter.suffix.lower() == ".py"
                      else [str(converter)]),
                    str(source_path(stream)), "-y", "-v2010", str(output),
                ]
                try:
                    result = subprocess.run(
                        command,
                        stdout=subprocess.DEVNULL,
                        stderr=subprocess.DEVNULL,
                        check=False,
                        timeout=timeout_seconds if timeout_seconds > 0 else None,
                    )
                    row["exitStatus"] = result.returncode
                    row["status"] = "converted" if result.returncode == 0 else "failed"
                    row["diagnosticCode"] = (
                        "success" if result.returncode == 0 else "conversion-failed"
                    )
                    if output.is_file():
                        row["outputSha256"] = digest(output)
                        row["outputSize"] = output.stat().st_size
                except subprocess.TimeoutExpired:
                    # A partial timeout output is intentionally ignored.
                    row["status"] = "timeout"
                    row["diagnosticCode"] = "timeout"
            rows.append(row)

    statuses = Counter(row["status"] for row in rows)
    formats = Counter(stream["detectedFormat"] for stream in streams)
    versions = Counter(stream["inputVersion"] for stream in dwg_streams)
    selected_versions = Counter(row["inputVersion"] for row in rows)
    invalid_count = sum(issue["classification"] == "invalid" for issue in issues)
    mislabeled_count = sum(issue["classification"] == "mislabeled" for issue in issues)
    inventory_rows = [public_stream(stream) for stream in streams]
    report = {
        "schema": 2,
        "advisory": True,
        "fixturePolicy": "external-only; never committed",
        "classificationPolicy": "content magic before suffix; SHA-256 deduplication before limit",
        "pathCount": len(aliases),
        "uniqueStreamCount": len(streams),
        "duplicatePathCount": len(aliases) - len(streams),
        "dwgCount": len(dwg_streams),
        "nonDwgCount": len(streams) - len(dwg_streams),
        "invalidPathCount": invalid_count,
        "mislabeledPathCount": mislabeled_count,
        "invalidMislabeledPathCount": len(issues),
        "selectedDwgCount": len(selected),
        # inputCount remains the number of result rows for compatibility with
        # earlier advisory reports; schema 2 makes its selected-DWG meaning
        # explicit through selectedDwgCount.
        "inputCount": len(rows),
        "formatCounts": dict(sorted(formats.items())),
        "statusCounts": dict(sorted(statuses.items())),
        "dwgVersionCounts": dict(sorted(versions.items())),
        "versionCounts": dict(sorted(selected_versions.items())),
        "inventoryRows": inventory_rows,
        "invalidMislabeledRows": issues,
        "rows": rows,
        "generation": {
            "converterEnabled": converter is not None,
            "converterSha256": converter_hash,
            "generator": "tools/run_external_advisory.py",
            "generatorSchema": 2,
            "inventorySha256": canonical_digest(inventory_rows),
            "limit": None if limit == -1 else limit,
            "selectionOrder": "detected DWG streams ordered by sourceSha256",
            "timeoutSeconds": float(timeout_seconds),
        },
    }
    report["generationReceipt"] = {
        "algorithm": "sha256-canonical-json-v1",
        "sha256": canonical_digest(report),
    }
    return report


def verify_receipt(report: dict) -> bool:
    candidate = dict(report)
    receipt = candidate.pop("generationReceipt", None)
    return (
        isinstance(receipt, dict)
        and receipt.get("algorithm") == "sha256-canonical-json-v1"
        and receipt.get("sha256") == canonical_digest(candidate)
    )


def self_test() -> None:
    with tempfile.TemporaryDirectory(prefix="libdxfrw-advisory-test-") as temp:
        root = Path(temp)
        corpus = root / "corpus"
        corpus.mkdir()

        first = b"AC1021" + b"first-local-drawing"
        second = b"AC1024" + b"second-local-drawing"
        ascii_dxf = (
            b"999\ndxfrw 0.6.3\n"
            b"  0\nSECTION\n  2\nHEADER\n  9\n$ACADVER\n  1\nAC1027\n"
            b"  0\nENDSEC\n  0\nEOF\n"
        )
        (corpus / "00-first.dwg").write_bytes(first)
        (corpus / "01-first-duplicate.dwg").write_bytes(first)
        (corpus / "02-dxf-disguised.dwg").write_bytes(ascii_dxf)
        (corpus / "03-invalid.dwg").write_bytes(b"not-a-drawing")
        (corpus / "04-correct.dxf").write_bytes(ascii_dxf)
        (corpus / "99-second.dwg").write_bytes(second)
        (corpus / "README.txt").write_text("ignored", encoding="utf-8")

        full = summarize(corpus, None, -1)
        assert full["pathCount"] == 6
        assert full["uniqueStreamCount"] == 4
        assert full["duplicatePathCount"] == 2
        assert full["dwgCount"] == 2
        assert full["nonDwgCount"] == 2
        assert full["invalidPathCount"] == 1
        assert full["mislabeledPathCount"] == 1
        assert full["invalidMislabeledPathCount"] == 2
        assert full["selectedDwgCount"] == 2
        assert len(full["rows"]) == 2
        assert verify_receipt(full)

        first_hash = hashlib.sha256(first).hexdigest()
        second_hash = hashlib.sha256(second).hexdigest()
        dxf_hash = hashlib.sha256(ascii_dxf).hexdigest()
        first_row = next(
            row for row in full["inventoryRows"]
            if row["sourceSha256"] == first_hash
        )
        assert first_row["pathAliases"] == [
            "00-first.dwg", "01-first-duplicate.dwg"
        ]
        dxf_row = next(
            row for row in full["inventoryRows"]
            if row["sourceSha256"] == dxf_hash
        )
        assert dxf_row["detectedFormat"] == "DXF_ASCII"
        assert dxf_row["inputVersion"] == "AC1027"
        assert dxf_row["pathAliases"] == [
            "02-dxf-disguised.dwg", "04-correct.dxf"
        ]

        # Old path-before-limit behavior would consume both slots with the
        # duplicate and never reach the second DWG. Content deduplication and
        # non-DWG classification must happen before this bound is applied.
        limited = summarize(corpus, None, 2)
        assert {row["sourceSha256"] for row in limited["rows"]} == {
            first_hash, second_hash
        }
        assert limited["inputCount"] == 2
        assert limited["pathCount"] == full["pathCount"]
        assert limited["uniqueStreamCount"] == full["uniqueStreamCount"]
        assert limited == summarize(corpus, None, 2)
        assert verify_receipt(limited)

        binary_format = classify_prefix(BINARY_DXF_MAGIC + b"payload")
        assert binary_format == (
            "DXF_BINARY", "UNKNOWN", "AutoCAD Binary DXF"
        )
        assert classify_prefix(b"0\nSECTION\n2\nENTITIES\n") == (
            "DXF_ASCII", "UNKNOWN", "0/SECTION"
        )
        assert classify_prefix(b"AC2.10" + b"legacy-local-drawing") == (
            "DWG", "AC2.10", "AC2.10"
        )

        timeout_corpus = root / "timeout-corpus"
        timeout_corpus.mkdir()
        (timeout_corpus / "sample.dwg").write_bytes(first)
        converter = root / "slow-converter.py"
        converter.write_text(
            "#!/usr/bin/env python3\nimport time\ntime.sleep(1)\n",
            encoding="utf-8",
        )
        converter.chmod(0o755)
        timed = summarize(timeout_corpus, converter, -1, timeout_seconds=0.01)
        assert timed["rows"][0]["status"] == "timeout"
        assert timed["rows"][0]["diagnosticCode"] == "timeout"
        assert "outputSha256" not in timed["rows"][0]
        assert verify_receipt(timed)
    print("run_external_advisory self-test: PASS")


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--root", type=Path)
    parser.add_argument("--converter", type=Path)
    parser.add_argument("--limit", type=int, default=8)
    parser.add_argument(
        "--timeout", type=float, default=0.0,
        help="per-input converter timeout in seconds (0 disables)",
    )
    parser.add_argument("--output", type=Path)
    args = parser.parse_args(argv)
    try:
        if args.self_test:
            self_test()
            return 0
        if args.root is None:
            parser.error("--root is required unless --self-test is used")
        if args.limit < -1:
            parser.error("--limit must be -1 (unlimited) or non-negative")
        if not math.isfinite(args.timeout) or args.timeout < 0:
            parser.error("--timeout must be finite and non-negative")
        report = summarize(args.root, args.converter, args.limit, args.timeout)
        encoded = json.dumps(report, sort_keys=True, indent=2) + "\n"
        if args.output:
            args.output.write_text(encoded, encoding="utf-8")
        else:
            sys.stdout.write(encoded)
        return 0
    except (OSError, UnicodeError, ValueError, subprocess.SubprocessError) as exc:
        print("external advisory: FAIL: %s" % exc, file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
