#!/usr/bin/env python3
"""Generate and compare from-scratch DBCOLOR group-62/EED differentials.

Preparation needs ezdxf to create a valid empty DXF scaffold; all DBCOLOR,
DICTIONARY and EED tags are authored by this tool. Run a CAD converter on the
prepared input directory, then compare its output; this program never invokes
an external executable or treats its output as a support qualification.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import tempfile

VARIANTS = (
    ("none_254", None, 254, False),
    ("none_1", None, 1, False),
    ("none_0", None, 0, False),
    ("aci1_254", 1, 254, False),
    ("aci254_1", 254, 1, False),
    ("none_tagother", None, 254, True),
)
NAME = "RAL CLASSIC$RAL 7047"
RGB = 13619407
MAX_INPUT_BYTES = 64 * 1024 * 1024


def digest(path: Path) -> str:
    result = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            result.update(chunk)
    return result.hexdigest()


def read_pairs(path: Path) -> list[tuple[str, str]]:
    if path.stat().st_size > MAX_INPUT_BYTES:
        raise ValueError(f"input too large: {path}")
    lines = path.read_text(encoding="utf-8", errors="strict").splitlines()
    if len(lines) % 2:
        raise ValueError(f"odd group/value line count: {path}")
    return [(lines[i].strip(), lines[i + 1]) for i in range(0, len(lines), 2)]


def record_bounds(pairs: list[tuple[str, str]], name: str) -> tuple[int, int]:
    starts = [i for i, item in enumerate(pairs) if item == ("0", name)]
    if len(starts) != 1:
        raise ValueError(f"expected exactly one {name} record, found {len(starts)}")
    start = starts[0]
    end = next((i for i in range(start + 1, len(pairs)) if pairs[i][0] == "0"), len(pairs))
    return start, end


def make_base(scaffold: list[tuple[str, str]]) -> list[tuple[str, str]]:
    pairs = list(scaffold)
    if any(code in ("5", "105") and value.strip().upper() == "3B0"
           for code, value in pairs):
        raise ValueError("scaffold already uses the DBCOLOR handle")
    root_start = next(i for i, item in enumerate(pairs) if item == ("0", "DICTIONARY"))
    root_end = next(i for i in range(root_start + 1, len(pairs)) if pairs[i][0] == "0")
    root = pairs[root_start:root_end]
    key = next(i for i, item in enumerate(root) if item == ("3", "ACAD_COLOR"))
    if key + 1 >= len(root) or root[key + 1][0] not in ("350", "360"):
        raise ValueError("scaffold ACAD_COLOR dictionary has no target")
    owner = root[key + 1][1].strip()
    dictionary = next(i for i in range(root_end, len(pairs) - 1)
        if pairs[i] == ("0", "DICTIONARY")
        and pairs[i + 1][0] == "5" and pairs[i + 1][1].strip() == owner)
    dictionary_end = next(i for i in range(dictionary + 1, len(pairs)) if pairs[i][0] == "0")
    pairs[dictionary_end:dictionary_end] = [("3", NAME), ("350", "3B0")]
    objects = next(i for i, item in enumerate(pairs)
        if item == ("2", "OBJECTS") and i > 0 and pairs[i - 1] == ("0", "SECTION"))
    end = next(i for i in range(objects, len(pairs)) if pairs[i] == ("0", "ENDSEC"))
    record = [("0", "DBCOLOR"), ("5", "3B0"), ("102", "{ACAD_REACTORS"),
              ("330", owner), ("102", "}"), ("330", owner), ("100", "AcDbColor"),
              ("420", str(RGB)), ("430", NAME), ("1001", "ACAD")]
    record += [(c, v) for _ in range(13) for c, v in
               (("1000", "DBColXD"), ("1070", "254"), ("1071", "-1026567985"), ("1000", NAME))]
    pairs[end:end] = record
    return pairs


def vary(base: list[tuple[str, str]], aci: int | None, eed: int,
         rename_tag: bool) -> list[tuple[str, str]]:
    start, end = record_bounds(base, "DBCOLOR")
    output: list[tuple[str, str]] = []
    xdata = False
    for code, value in base[start:end]:
        if code == "420" and aci is not None:
            output.append(("62", str(aci)))
        if code == "1001":
            xdata = True
        if xdata and code == "1070":
            value = str(eed)
        if xdata and code == "1000" and value == "DBColXD" and rename_tag:
            value = "NOTDBCOL"
        output.append((code, value))
    return base[:start] + output + base[end:]


def fields(pairs: list[tuple[str, str]]) -> dict:
    start, end = record_bounds(pairs, "DBCOLOR")
    record = pairs[start:end]
    return {"group62": [v.strip() for c, v in record if c == "62"],
            "group420": [v.strip() for c, v in record if c == "420"],
            "group430": [v for c, v in record if c == "430"],
            "eed1070": [v.strip() for c, v in record if c == "1070"],
            "dbcolxdCount": sum((c, v) == ("1000", "DBColXD") for c, v in record),
            "otherTagCount": sum((c, v) == ("1000", "NOTDBCOL") for c, v in record)}


def prepare(directory: Path) -> None:
    import ezdxf  # Optional dependency; not needed by the comparator or self-test.
    directory.mkdir(parents=True, exist_ok=False)
    with tempfile.TemporaryDirectory(prefix="dbcolor-scaffold-") as temporary:
        scaffold = Path(temporary) / "scaffold.dxf"
        document = ezdxf.new("R2007")
        document.header["$HANDSEED"] = "400"
        document.saveas(scaffold)
        base = make_base(read_pairs(scaffold))
    rows = []
    for name, aci, eed, rename_tag in VARIANTS:
        path = directory / f"{name}.dxf"
        pairs = vary(base, aci, eed, rename_tag)
        path.write_text("".join(f"{code}\n{value}\n" for code, value in pairs), encoding="utf-8")
        rows.append({"name": name, "sha256": digest(path), "fields": fields(pairs)})
    print(json.dumps({"ezdxfVersion": ezdxf.__version__, "inputs": rows}, indent=2))


def compare(inputs: Path, outputs: Path) -> None:
    rows = []
    for name, _, _, _ in VARIANTS:
        source, output = inputs / f"{name}.dxf", outputs / f"{name}.dxf"
        if not source.is_file() or not output.is_file() or output.with_suffix(".dxf.err").exists():
            raise ValueError(f"missing input/output or converter error for {name}")
        rows.append({"name": name, "inputSha256": digest(source),
                     "outputSha256": digest(output), "input": fields(read_pairs(source)),
                     "output": fields(read_pairs(output))})
    print(json.dumps({"comparison": rows, "supportClaimEffect": "none"}, indent=2))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)
    p = sub.add_parser("prepare")
    p.add_argument("directory", type=Path)
    p = sub.add_parser("compare")
    p.add_argument("inputs", type=Path)
    p.add_argument("outputs", type=Path)
    sub.add_parser("self-test")
    args = parser.parse_args()
    if args.command == "prepare":
        prepare(args.directory)
    elif args.command == "compare":
        compare(args.inputs, args.outputs)
    else:
        sample = [("0", "DBCOLOR"), ("420", str(RGB)), ("430", NAME),
                  ("1001", "ACAD"), ("1000", "DBColXD"), ("1070", "254"),
                  ("0", "ENDSEC")]
        observed = fields(vary(sample, 1, 0, True))
        assert observed["group62"] == ["1"] and observed["eed1070"] == ["0"]
        assert observed["dbcolxdCount"] == 0 and observed["otherTagCount"] == 1
        print("DBCOLOR differential generator self-test: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
