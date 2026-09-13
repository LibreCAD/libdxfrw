#!/usr/bin/env python3
"""Create a deterministic, source-only public API surface report.

The report is intentionally lexical: it is safe to run against a locked
archive before the target sources are activated in the build.  It records the
names and include edges that must be rechecked after import; it does not claim
that a declaration is ABI-compatible or semantically implemented.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
import tempfile
from pathlib import Path


SCHEMA = 1
CLASS_RE = re.compile(r"\b(?:class|struct)\s+(?:DRW_API\s+)?([A-Za-z_]\w*)")
ENUM_RE = re.compile(r"\benum(?:\s+class)?\s+([A-Za-z_]\w*)?")
TYPEDEF_RE = re.compile(r"\btypedef\s+(.+?)\s+([A-Za-z_]\w*)\s*;")
USING_RE = re.compile(r"\busing\s+([A-Za-z_]\w*)\s*=")
DEFINE_RE = re.compile(r"^\s*#\s*define\s+([A-Za-z_]\w*)", re.MULTILINE)
INCLUDE_RE = re.compile(r'^\s*#\s*include\s*[\"<]([^\">]+)[\">]', re.MULTILINE)
CALLBACK_RE = re.compile(r"\b(?:virtual\s+)?[^;{}\n]*\b(add[A-Za-z_]\w*)\s*\(")
ENUM_VALUE_RE = re.compile(r"^\s*([A-Z][A-Za-z0-9_]*)\s*(?:=|,|$)")


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    digest.update(path.read_bytes())
    return digest.hexdigest()


def analyze(root: Path) -> dict:
    source_root = root / "src"
    if not source_root.is_dir():
        raise ValueError("root must contain a src directory: %s" % root)
    headers = []
    all_classes = set()
    all_enums = set()
    all_enum_values = set()
    all_typedefs = set()
    all_macros = set()
    all_callbacks = set()
    dependency_edges = []
    for path in sorted(source_root.rglob("*.h")):
        rel = path.relative_to(root).as_posix()
        text = path.read_text(encoding="utf-8", errors="replace")
        classes = sorted(set(CLASS_RE.findall(text)))
        enums = sorted(set(name for name in ENUM_RE.findall(text) if name))
        typedefs = sorted(set(TYPEDEF_RE.findall(text) and [match[1] for match in TYPEDEF_RE.findall(text)] or []))
        typedefs += sorted(set(USING_RE.findall(text)))
        typedefs = sorted(set(typedefs))
        macros = sorted(set(DEFINE_RE.findall(text)))
        callbacks = sorted(set(CALLBACK_RE.findall(text)))
        enum_values = []
        in_enum = False
        for line in text.splitlines():
            if re.search(r"\benum(?:\s+class)?\b", line):
                in_enum = True
            if in_enum:
                enum_values.extend(ENUM_VALUE_RE.findall(line))
            if in_enum and "}" in line:
                in_enum = False
        enum_values = sorted(set(enum_values))
        includes = sorted(set(INCLUDE_RE.findall(text)))
        for include in includes:
            dependency_edges.append({"from": rel, "to": include})
        all_classes.update(classes)
        all_enums.update(enums)
        all_enum_values.update(enum_values)
        all_typedefs.update(typedefs)
        all_macros.update(macros)
        all_callbacks.update(callbacks)
        headers.append({
            "path": rel,
            "sha256": sha256(path),
            "classes": classes,
            "enums": enums,
            "enumValues": enum_values,
            "typedefs": typedefs,
            "macros": macros,
            "callbacks": callbacks,
            "includes": includes,
        })
    public_headers = [entry for entry in headers if not entry["path"].startswith("src/intern/")]
    return {
        "schema": SCHEMA,
        "lexicalOnly": True,
        "sourceRoot": "src",
        "headerCount": len(headers),
        "publicHeaderCount": len(public_headers),
        "headers": headers,
        "publicEnums": sorted(all_enums),
        "publicEnumValues": sorted(all_enum_values),
        "typedefs": sorted(all_typedefs),
        "macroEffects": sorted(all_macros),
        "callbacks": sorted(all_callbacks),
        "includeDependencies": sorted(dependency_edges, key=lambda edge: (edge["from"], edge["to"])),
    }


def self_test() -> None:
    with tempfile.TemporaryDirectory() as directory:
        root = Path(directory)
        (root / "src/intern").mkdir(parents=True)
        (root / "src/drw_test.h").write_text(
            "#define DRW_API\n#include \"intern/x.h\"\nenum Version { AC1009, AC1012 };\n"
            "typedef int drw_int;\nclass DRW_Test {};\nvirtual void addTest();\n",
            encoding="utf-8",
        )
        (root / "src/intern/x.h").write_text("#define INTERNAL\n", encoding="utf-8")
        result = analyze(root)
        assert result["schema"] == SCHEMA
        assert result["headerCount"] == 2
        assert result["publicHeaderCount"] == 1
        assert "DRW_Test" in result["headers"][0]["classes"]
        assert "Version" in result["publicEnums"]
        assert "drw_int" in result["typedefs"]
        assert "DRW_API" in result["macroEffects"]
        assert "addTest" in result["callbacks"]
        assert result["includeDependencies"] == [{"from": "src/drw_test.h", "to": "intern/x.h"}]


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, help="unpacked tree containing src/")
    parser.add_argument("--output", type=Path, help="write JSON report to this path")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args(argv)
    if args.self_test:
        self_test()
        print("api surface self-test: PASS")
        return 0
    if args.root is None:
        parser.error("--root is required unless --self-test is used")
    report = analyze(args.root.resolve())
    encoded = json.dumps(report, indent=2, sort_keys=True) + "\n"
    if args.output:
        args.output.write_text(encoded, encoding="utf-8")
    else:
        sys.stdout.write(encoded)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
