"""Optional CELLSTYLEMAP source-span observation, never support eligibility.

Corroborates flat code/scalar positions, all handle slots and local definitions.
It does not independently certify complete rich payloads, handle-role semantics,
target classes, dependency closure, output, native layouts or modeler semantics.
"""

import argparse
import hashlib
import json
from pathlib import Path
import re
import sys


PATTERN = re.compile(r"^map-reference handle=([0-9a-f]+) kind=(\d+) index=(\d+) code=(\d+) value=([0-9a-f]+)$", re.M)
CODES = {0: 5, 1: 330, 2: 330, 3: 360, 4: 340, 5: 340,
         6: 90, 7: 1005, 8: 1001, 9: 1003}


def is_handle(code):
    return code == 5 or 320 <= code <= 369 or 390 <= code <= 399 or code in (480, 481, 1005)


def compare(tags, events):
    if not events or len(tags) > 65536 or len(events) > len(tags):
        raise ValueError("empty/oversized map projection")
    previous = -1
    handles, definitions, names = set(), set(), set()
    for kind, index, code, value in events:
        if not 0 <= value <= 0xFFFFFFFF or index <= previous or not 0 <= index < len(tags):
            raise ValueError("duplicate/reordered/out-of-range map event")
        previous = index
        if tags[index][0] != code or (kind != 10 and CODES.get(kind) != code):
            raise ValueError("event code/role differs")
        scalar = tags[index][1]
        if kind == 6:
            if type(scalar) is not int or scalar != value or index == 0 or tags[index - 1] != (1, "CELLSTYLE_BEGIN"):
                raise ValueError("local definition differs")
            definitions.add(index)
        elif kind in (8, 9):
            if type(scalar) is not str or value != 0:
                raise ValueError("named namespace event differs")
            names.add(index)
        else:
            if not is_handle(code) or type(scalar) is not str or int(scalar, 16) != value:
                raise ValueError("handle span differs")
            handles.add(index)
    if handles != {i for i, (code, _) in enumerate(tags) if is_handle(code)}:
        raise ValueError("omitted/invented map handle")
    expected, expected_names = set(), set()
    depth = 0
    for i, (code, scalar) in enumerate(tags):
        if code == 102:
            if type(scalar) is not str:
                raise ValueError("unexpanded application group")
            if scalar.startswith("{"):
                depth += 1
            elif scalar == "}" and depth:
                depth -= 1
            else:
                raise ValueError("bad application framing")
        elif not depth:
            if (code, scalar) == (1, "CELLSTYLE_BEGIN"):
                expected.add(i + 1)
            if code in (1001, 1003):
                expected_names.add(i)
    if depth:
        raise ValueError("unclosed application group")
    if definitions != expected:
        raise ValueError("omitted/invented local definition")
    if names != expected_names:
        raise ValueError("omitted/invented named namespace position")
    return {"handleSlots": len(handles), "localDefinitions": len(definitions), "referenceEvents": len(events)}


def flatten(xtags):
    tags = []
    for subclass in xtags.subclasses:
        for tag in subclass:
            if tag.code == 102 and type(tag.value) is int:
                if not 0 <= tag.value < len(xtags.appdata):
                    raise ValueError("bad independent appData placeholder")
                tags.extend((t.code, t.value) for t in xtags.appdata[tag.value])
            elif hasattr(tag, "dxftags"):
                tags.extend((t.code, t.value) for t in tag.dxftags())
            else:
                tags.append((tag.code, tag.value))
    for block in xtags.xdata:
        for tag in block:
            if hasattr(tag, "dxftags"):
                tags.extend((t.code, t.value) for t in tag.dxftags())
            else:
                tags.append((tag.code, tag.value))
    return tags


def observe(source, projection):
    import ezdxf
    if ezdxf.__version__ != "1.4.4":
        raise ValueError("observer requires ezdxf 1.4.4")
    if Path(source).stat().st_size > 64 * 1024 * 1024 or Path(projection).stat().st_size > 16 * 1024 * 1024:
        raise ValueError("oversized advisory input")
    document = ezdxf.readfile(source)
    events = {}
    for handle, kind, index, code, value in PATTERN.findall(Path(projection).read_text()):
        events.setdefault(handle.upper(), []).append((int(kind), int(index), int(code), int(value, 16)))
    if not events or len(events) > 1000:
        raise ValueError("missing/oversized map census")
    results = []
    for handle, values in events.items():
        obj = document.entitydb[handle]
        if obj.dxftype() != "CELLSTYLEMAP":
            raise ValueError("wrong source resource family")
        tags = flatten(obj.xtags)
        if not tags or tags[0] != (0, "CELLSTYLEMAP"):
            raise ValueError("unexpected source map framing")
        results.append({"handle": handle, **compare(tags[1:], values)})
    return {"schemaVersion": 1, "status": "SCOPED_MAP_REFERENCE_SPANS_MATCH", "qualificationEligible": False,
            "ezdxfVersion": ezdxf.__version__, "maps": results,
            "sourceSha256": hashlib.sha256(Path(source).read_bytes()).hexdigest(),
            "projectionSha256": hashlib.sha256(Path(projection).read_bytes()).hexdigest(),
            "limits": __doc__.split("\n\n", 1)[1].strip()}


def self_test():
    tags = [(5, "C120"), (102, "{ACAD_REACTORS"), (330, "C110"), (102, "}"),
            (330, "C110"), (340, "C101"), (1, "CELLSTYLE_BEGIN"), (90, 101),
            (1001, "APP"), (1003, "LAYER")]
    valid = [(0, 0, 5, 0xC120), (2, 2, 330, 0xC110), (1, 4, 330, 0xC110),
             (4, 5, 340, 0xC101), (6, 7, 90, 101), (8, 8, 1001, 0), (9, 9, 1003, 0)]
    assert compare(tags, valid) == {"handleSlots": 4, "localDefinitions": 1, "referenceEvents": 7}
    bad = [[], valid[:-1], valid + [valid[-1]], valid[::-1],
           [*valid[:4], (6, 7, 90, 102), *valid[5:]], [*valid[:4], (6, 6, 90, 101), *valid[5:]],
           [valid[0], *valid[2:]], [(0, 0, 5, 0x10000C120), *valid[1:]],
           [(10, 0, 90, 0xC120), *valid[1:]]]
    for fault in bad:
        try:
            compare(tags, fault)
        except ValueError:
            continue
        raise AssertionError("corrupt map projection passed")
    from types import SimpleNamespace
    tag = lambda c, v: SimpleNamespace(code=c, value=v)
    xtags = SimpleNamespace(subclasses=[[tag(5, "A"), tag(102, 0)]],
                            appdata=[[tag(102, "{APP"), tag(330, "B"), tag(102, "}")]], xdata=[])
    assert flatten(xtags) == [(5, "A"), (102, "{APP"), (330, "B"), (102, "}")]
    print("map reference observer: one positive, nine corruptions, appData expansion; no eligibility")


if __name__ == "__main__":
    if sys.flags.optimize:
        raise ValueError("optimized Python cannot run evidence checks")
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--source")
    parser.add_argument("--projection")
    args = parser.parse_args()
    if args.self_test:
        self_test()
    elif args.source and args.projection:
        print(json.dumps(observe(args.source, args.projection), indent=2, allow_nan=False))
    else:
        parser.error("use --self-test or --source DXF --projection CXX_LOG")
