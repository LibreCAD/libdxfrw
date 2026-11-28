"""Optional independent source-reference span check, never support eligibility.

Compare the C++ --inspect reference events against ezdxf's independently loaded
TABLECONTENT tags. Schema roles are NOT independently decoded here: only event
code, scalar identity, exact flat tag position, and complete handle-slot coverage
are corroborated. No target resolution, rich-field/output or modeler proof.
"""

import argparse
import hashlib
import json
from pathlib import Path
import re


PATTERN = re.compile(r"^source-reference kind=(\d+) index=(\d+) code=(\d+) value=([0-9a-f]+)$", re.M)
CODES = {0: 5, 1: 330, 2: 340, 3: 340, 4: 340, 5: 340, 6: 340,
         7: 330, 8: 340, 9: 330, 10: 330, 11: 90}


def compare(tags, events):
    if not events or len(tags) > 65536 or len(events) > len(tags):
        raise ValueError("empty/oversized reference projection")
    seen = set()
    local_ids = 0
    previous = -1
    for kind, index, code, value in events:
        if kind not in CODES or code != CODES[kind] or not 0 <= value <= 0xFFFFFFFF:
            raise ValueError("unknown/mismatched schema role")
        if index <= previous or index >= len(tags) or tags[index][0] != code:
            raise ValueError("duplicate/reordered/misplaced reference span")
        previous = index
        observed = tags[index][1]
        if kind == 11:
            if type(observed) is not int or observed != value:
                raise ValueError("style-local integer identity differs")
            local_ids += 1
        else:
            if type(observed) is not str or int(observed, 16) != value:
                raise ValueError("handle identity differs")
            seen.add(index)
    handles = {i for i, (code, _) in enumerate(tags)
               if code == 5 or 320 <= code <= 369 or 390 <= code <= 399
               or code in (480, 481, 1005)}
    if handles != seen:
        raise ValueError("source contains an omitted or invented handle span")
    return {"handleSlots": len(handles), "localStyleIdSlots": local_ids,
            "referenceEvents": len(events)}


def observe(source, projection):
    import ezdxf
    document = ezdxf.readfile(source)
    objects = [obj for obj in document.objects if obj.dxftype() == "TABLECONTENT"]
    if len(objects) != 1:
        raise ValueError("observer requires exactly one source TABLECONTENT")
    obj = objects[0]
    if obj.xtags.appdata or obj.xtags.xdata:
        raise ValueError("observer has no extended common-metadata profile")
    tags = []
    for subclass in obj.xtags.subclasses:
        for tag in subclass:
            # ezdxf compiles XYZ components into DXFVertex independently.
            if hasattr(tag, "dxftags"):
                tags.extend((t.code, t.value) for t in tag.dxftags())
            else:
                tags.append((tag.code, tag.value))
    if not tags or tags[0] != (0, "TABLECONTENT"):
        raise ValueError("unexpected source framing")
    events = [(int(k), int(i), int(c), int(v, 16))
              for k, i, c, v in PATTERN.findall(Path(projection).read_text())]
    result = compare(tags[1:], events)
    return {"schemaVersion": 1, "qualificationEligible": False,
            "status": "SCOPED_CONTENT_REFERENCE_SPANS_MATCH", "ezdxfVersion": ezdxf.__version__,
            "sourceSha256": hashlib.sha256(Path(source).read_bytes()).hexdigest(),
            "projectionSha256": hashlib.sha256(Path(projection).read_bytes()).hexdigest(),
            "contentHandle": obj.dxf.handle, **result,
            "limits": "Exact flat span/code/scalar and complete handle coverage only; role labels, target classes, rich content, output and modeler semantics are not independently qualified"}


def self_test():
    tags = [(5, "A0"), (330, "B0"), (100, "AcDbTableContent"), (90, 101), (340, "C100")]
    valid = [(0, 0, 5, 0xA0), (1, 1, 330, 0xB0), (11, 3, 90, 101), (4, 4, 340, 0xC100)]
    assert compare(tags, valid) == {"handleSlots": 3, "localStyleIdSlots": 1, "referenceEvents": 4}
    bad = [[], valid[:-1], valid + [valid[-1]], valid[::-1],
           [valid[0], (1, 2, 330, 0xB0), *valid[2:]],
           [*valid[:2], (11, 3, 90, 102), valid[3]],
           [*valid[:3], (4, 4, 340, 0xC101)],
           [*valid[:3], (99, 4, 340, 0xC100)],
           [(0, 0, 5, 0x1000000A0), *valid[1:]]]
    for fault in bad:
        try:
            compare(tags, fault)
        except ValueError:
            continue
        raise AssertionError("corrupt projection passed")
    print("content reference observer: one positive and nine corruptions; no eligibility")


if __name__ == "__main__":
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
