#!/usr/bin/env python3
"""Index test-source mentions of public dxfRW writer methods.

This is a navigation aid for P6-G, not a coverage verifier: a textual call
reference does not prove rollback, retry, readback, or version-matrix coverage.
"""

from __future__ import annotations

import argparse
import json
import re
from pathlib import Path


def strip_comments(text: str) -> str:
    """Remove C++ comments while preserving line numbers."""
    return re.sub(
        r'/\*.*?\*/|//[^\n]*',
        lambda match: "\n" * match.group(0).count("\n"),
        text,
        flags=re.DOTALL,
    )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    header_path = args.root / "src/libdxfrw.h"
    header = strip_comments(header_path.read_text(encoding="utf-8"))
    class_start = header.index("class dxfRW {")
    private_start = header.index("\nprivate:", class_start)
    public_api = header[class_start:private_start]

    declarations: dict[str, list[int]] = {}
    for match in re.finditer(r"\b(write[A-Z][A-Za-z0-9_]*)\s*\(", public_api):
        name = match.group(1)
        declaration_line = header[:class_start + match.start()].count("\n") + 1
        declarations.setdefault(name, []).append(declaration_line)

    test_sources = sorted(
        path for path in (args.root / "tests").rglob("*")
        if path.is_file() and path.suffix in {".cpp", ".h"}
    )
    contents = {path: strip_comments(path.read_text(encoding="utf-8", errors="replace"))
                for path in test_sources}

    references_by_method: dict[str, dict[Path, dict[str, int]]] = {
        name: {} for name in declarations
    }
    call_pattern = re.compile(r"\b(write[A-Z][A-Za-z0-9_]*)\s*\(")
    for path, source in contents.items():
        line = 1
        previous = 0
        for match in call_pattern.finditer(source):
            name = match.group(1)
            if name not in references_by_method:
                continue
            line += source.count("\n", previous, match.start())
            previous = match.start()
            references_by_file = references_by_method[name]
            if path not in references_by_file:
                references_by_file[path] = {"line": line, "occurrences": 0}
            references_by_file[path]["occurrences"] += 1

    rows = []
    for name, declaration_lines in sorted(declarations.items()):
        references = [
            {"file": path.relative_to(args.root).as_posix(), **reference}
            for path, reference in sorted(references_by_method[name].items())
        ]
        rows.append({
            "method": name,
            "declaration_count": len(declaration_lines),
            "header_lines": declaration_lines,
            "test_source_reference_count": len(references),
            "test_source_references": references,
            "allocation_transaction_test_reference": any(
                ref["file"] == "tests/dxf_write_allocation_transaction_tests.cpp"
                for ref in references
            ),
        })

    result = {
        "schema": "libdxfrw.public-writer-test-mention-index.v1",
        "scope": "Textual source index only; mentions do not prove assertions, rollback, retry, readback, version coverage, or semantics.",
        "repository_head": __import__("subprocess").run(
            ["git", "-C", str(args.root), "rev-parse", "HEAD"],
            check=True, capture_output=True, text=True,
        ).stdout.strip(),
        "public_writer_method_count": len(rows),
        "public_writer_declaration_count": sum(row["declaration_count"] for row in rows),
        "methods_with_no_test_source_reference": [
            row["method"] for row in rows if row["test_source_reference_count"] == 0
        ],
        "methods_without_reference_in_whole_file_allocation_test": [
            row["method"] for row in rows if not row["allocation_transaction_test_reference"]
        ],
        "methods": rows,
    }

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps({
        "output": str(args.output),
        "public_writer_method_count": result["public_writer_method_count"],
        "public_writer_declaration_count": result["public_writer_declaration_count"],
        "methods_with_no_test_source_reference": result["methods_with_no_test_source_reference"],
        "methods_without_reference_in_whole_file_allocation_test": result["methods_without_reference_in_whole_file_allocation_test"],
    }, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
