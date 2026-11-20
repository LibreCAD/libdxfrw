#!/usr/bin/env python3
"""Compare the selected R14 source's peer JSON with emitted DXFs.

Usage: compare_ac1014_profile_ezdxf.py dwgread.json ascii.dxf binary.dxf
The caller must separately verify each output's actual DXF version: ezdxf
upgrades R14 in memory. This is a bounded field/graph comparison, not native
CAD or whole-format qualification.
"""
import collections
import json
import math
import sys

import ezdxf


def close(actual, expected):
    return len(actual) == len(expected) and all(
        math.isclose(float(a), float(b), rel_tol=0.0, abs_tol=1e-9)
        for a, b in zip(actual, expected))


def require(condition, message):
    if not condition:
        raise ValueError(message)


def compare(source, path):
    drawing = ezdxf.readfile(path)
    entities = list(drawing.modelspace())
    lines = [e for e in entities if e.dxftype() == "LINE"]
    source_lines = [x for x in source if x.get("entity") == "LINE"]
    require(len(lines) == len(source_lines) == 16, "LINE count")
    unmatched = list(lines)
    for record in source_lines:
        match = next((e for e in unmatched
                      if close(e.dxf.start, record["start"])
                      and close(e.dxf.end, record["end"])), None)
        require(match is not None, "LINE endpoint mismatch")
        unmatched.remove(match)
    source_text = [x for x in source if x.get("entity") == "TEXT"]
    texts = [e for e in entities if e.dxftype() == "TEXT"]
    require(len(texts) == len(source_text) == 1, "TEXT count")
    require(texts[0].dxf.text == source_text[0]["text_value"], "TEXT value")
    require(close(tuple(texts[0].dxf.insert)[:2], source_text[0]["ins_pt"])
            and math.isclose(texts[0].dxf.height, source_text[0]["height"],
                             rel_tol=0.0, abs_tol=1e-9), "TEXT geometry")

    selected = [x for x in source if x.get("object") in {"DICTIONARYVAR", "MLINESTYLE"}]
    source_objects = {x["object"]: x for x in selected}
    require(len(selected) == len(source_objects) == 2, "source objects")
    root = drawing.rootdict
    group = root.get("ACAD_GROUP")
    require(group.dxftype() == "DICTIONARY" and group.dxf.owner == root.dxf.handle
            and not list(group.items()), "group dictionary")
    owner = root.get("ACDBVARIABLEDICTIONARY")
    value = owner.get("DIMADEC")
    require(owner.dxf.owner == root.dxf.handle
            and value.dxftype() == "DICTIONARYVAR"
            and value.dxf.owner == owner.dxf.handle
            and value.dxf.value == source_objects["DICTIONARYVAR"]["strvalue"],
            "DICTIONARYVAR graph or value")
    owner = root.get("ACAD_MLINESTYLE")
    style = owner.get("STANDARD")
    require(owner.dxf.owner == root.dxf.handle and style.dxftype() == "MLINESTYLE"
            and style.dxf.owner == owner.dxf.handle,
            "MLINESTYLE graph")
    source_elements = source_objects["MLINESTYLE"]["lines"]
    require(all(x["lt_index"] == 32767 for x in source_elements)
            and [(x.offset, x.color, x.linetype.upper()) for x in style.elements]
            == [(x["offset"], x["color"], "BYLAYER") for x in source_elements],
            "MLINESTYLE elements")
    return collections.Counter(e.dxftype() for e in entities)


def main():
    if len(sys.argv) != 4:
        raise SystemExit(__doc__)
    with open(sys.argv[1], encoding="utf-8") as stream:
        source = json.load(stream)["OBJECTS"]
    for path in sys.argv[2:]:
        print(path, dict(compare(source, path)))


if __name__ == "__main__":
    main()
