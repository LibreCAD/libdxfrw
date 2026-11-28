#!/usr/bin/env python3
"""Compare a scratch ASCII DXF header against the Autodesk R14 HTML table.

The reference HTML and drawing are inputs, not redistributed fixtures. An
unlisted name is an investigation candidate, not proof of invalid semantics.
"""

import argparse
import json
import re
from html.parser import HTMLParser
from pathlib import Path


class TableRows(HTMLParser):
    def __init__(self):
        super().__init__()
        self.rows = []
        self.row = None
        self.cell = None

    def handle_starttag(self, tag, attrs):
        del attrs
        if tag.lower() == "tr":
            self.row = []
        elif tag.lower() in ("td", "th") and self.row is not None:
            self.cell = []

    def handle_data(self, data):
        if self.cell is not None:
            self.cell.append(data)

    def handle_endtag(self, tag):
        if tag.lower() in ("td", "th") and self.cell is not None:
            self.row.append(" ".join("".join(self.cell).split()))
            self.cell = None
        elif tag.lower() == "tr" and self.row is not None:
            self.rows.append(self.row)
            self.row = None


def reference_names(path):
    table = TableRows()
    table.feed(path.read_text(encoding="utf-8", errors="replace"))
    names = set()
    for row in table.rows:
        if not row:
            continue
        name = row[0].strip()
        numbered = re.fullmatch(r"(\$[A-Z_]+)([0-9]+)\s*-\s*([0-9]+)", name)
        if numbered:
            start, stop = int(numbered[2]), int(numbered[3])
            if start > stop or stop - start > 100:
                raise ValueError("invalid numbered reference-variable range")
            names.update(numbered[1] + str(i) for i in range(start, stop + 1))
        elif re.fullmatch(r"\$[A-Z0-9_]+", name):
            names.add(name)
    if len(names) < 150:
        raise ValueError("reference table appears incomplete")
    return names


def header_names(path):
    lines = path.read_text(encoding="utf-8", errors="surrogateescape").splitlines()
    if len(lines) % 2:
        raise ValueError("incomplete ASCII DXF group pair")
    tags = [(lines[i].strip(), lines[i + 1].strip())
            for i in range(0, len(lines), 2)]
    inside = False
    names = set()
    for i, (code, value) in enumerate(tags):
        if (code, value) == ("0", "SECTION") and i + 1 < len(tags):
            inside = tags[i + 1] == ("2", "HEADER")
        elif inside and (code, value) == ("0", "ENDSEC"):
            return names
        elif inside and code == "9":
            names.add(value)
    raise ValueError("complete HEADER section not found")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reference-html", type=Path, required=True)
    parser.add_argument("--ascii-dxf", type=Path, required=True)
    parser.add_argument("--comparison-json", type=Path)
    args = parser.parse_args()
    reference = reference_names(args.reference_html)
    output = header_names(args.ascii_dxf)
    result = {"referenceCount": len(reference), "outputCount": len(output),
              "outputAbsentFromReference": sorted(output - reference)}
    if args.comparison_json:
        earlier = json.loads(args.comparison_json.read_text(encoding="utf-8"))
        candidates = {item["name"] for item in earlier["independentCheck"]["variables"]}
        result["earlierCandidatesPresentInReference"] = sorted(candidates & reference)
        result["earlierCandidatesAbsentFromReference"] = sorted(candidates - reference)
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
