#!/usr/bin/env python3
"""Create a scratch RTEXT DXF from the repository's authored ellipse control.

The output is experimental input to an independent producer, not a fixture;
ODA may represent the custom entity as a proxy instead of a semantic RTEXT.
"""
import argparse
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "tests/fixtures/dxf/ac1015_ellipse_freecad_control.dxf"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    lines = SOURCE.read_text(encoding="ascii").splitlines()
    if len(lines) % 2:
        raise ValueError("source DXF does not contain complete tag pairs")
    tags = [(lines[i].strip(), lines[i + 1]) for i in range(0, len(lines), 2)]
    if any(code in {"5", "105"} and value.upper() == "ED00" for code, value in tags):
        raise ValueError("chosen RTEXT handle is already used")
    entities = [i for i, tag in enumerate(tags) if tag == ("2", "ENTITIES")
                and i and tags[i - 1] == ("0", "SECTION")]
    tables = [i for i, tag in enumerate(tags) if tag == ("2", "TABLES")
              and i and tags[i - 1] == ("0", "SECTION")]
    if len(entities) != 1 or len(tables) != 1:
        raise ValueError("expected one ENTITIES and one TABLES section")
    end = next((i for i in range(entities[0] + 1, len(tags))
                if tags[i] == ("0", "ENDSEC")), None)
    if end is None:
        raise ValueError("ENTITIES terminator is missing")
    entity = [(0, "RTEXT"), (5, "ED00"), (330, "1"), (100, "AcDbEntity"),
              (8, "0"), (100, "RText"), (7, "STANDARD"), (10, 90.0),
              (20, 91.0), (30, 0.0), (40, 2.0), (50, 15.0), (70, 1),
              (1, "LOCAL_RTEXT"), (210, 0.0), (220, 0.0), (230, 1.0)]
    tags[end:end] = [(str(code), str(value)) for code, value in entity]
    classes = [("0", "SECTION"), ("2", "CLASSES"), ("0", "CLASS"),
               ("1", "RTEXT"), ("2", "AcDbRText"), ("3", "EXPRESS"),
               ("90", "1025"), ("91", "1"), ("280", "0"), ("281", "1"),
               ("0", "ENDSEC")]
    tags[tables[0] - 1:tables[0] - 1] = classes
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text("".join(f"{code}\n{value}\n" for code, value in tags), encoding="ascii")


if __name__ == "__main__":
    main()
