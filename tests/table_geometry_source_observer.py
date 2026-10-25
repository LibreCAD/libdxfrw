"""Optional independent TABLEGEOMETRY source comparison; never a support certificate.

The regular self-test needs only Python. External observation requires ezdxf;
drawings, the C++ --inspect JSON and optional pinned LibreDWG JSON stay local.
No input is edited and no external executable is launched by this observer.
"""

import argparse
import hashlib
import json
import math
import sys
from pathlib import Path


def decode(tags, handle, owner):
    position = 0

    def take(code):
        nonlocal position
        if position >= len(tags) or tags[position][0] != code:
            raise ValueError(f"missing/reordered geometry group {code}")
        value = tags[position][1]
        position += 1
        return value

    def integer(code, low=-(2**31), high=2**31 - 1):
        value = take(code)
        if type(value) is not int or not low <= value <= high:
            raise ValueError(f"out-of-domain geometry integer {code}")
        return value

    def real(code):
        value = take(code)
        if type(value) not in (int, float) or not math.isfinite(value):
            raise ValueError(f"non-finite geometry real {code}")
        return value

    def point(code):
        value = list(take(code))  # ezdxf independently groups the XYZ tuple.
        if len(value) != 3 or not all(math.isfinite(v) for v in value):
            raise ValueError(f"incomplete geometry point {code}")
        return value

    if take(100) != "AcDbTableGeometry":
        raise ValueError("wrong geometry subclass")
    rows, columns, count = integer(90, 0, 5000), integer(91, 0, 5000), integer(92, 0, 10000)
    if rows * columns != count:
        raise ValueError("geometry cell product differs")
    cells = []
    total_contents = 0
    for _ in range(count):
        cell = {"flags": integer(93), "widthWithGap": real(40), "heightWithGap": real(41)}
        cell["softPointer"] = int(take(330), 16)
        content_count = integer(94, 0, 10000 - total_contents)
        total_contents += content_count
        cell["contents"] = [point(10) + point(11) + [real(c) for c in (43, 44, 45, 46)]
                            + [integer(95)] for _ in range(content_count)]
        cells.append(cell)
    if position != len(tags):
        raise ValueError("unknown geometry tail")
    return {"handle": int(handle, 16), "owner": int(owner, 16),
            "rows": rows, "columns": columns, "cells": cells}


def peer_payload(record):
    cells = []
    for cell in record["cells"]:
        contents = [g["dist_top_left"] + g["dist_center"]
                    + [g[k] for k in ("content_width", "content_height", "width", "height", "unknown")]
                    for g in cell.get("geometry", [])]
        cells.append({"flags": cell["geom_data_flag"], "widthWithGap": cell["width_w_gap"],
                      "heightWithGap": cell["height_w_gap"],
                      "softPointer": cell["tablegeometry"][-1], "contents": contents})
    return {"rows": record["numrows"], "columns": record["numcols"], "cells": cells}


def differences(left, right, path=""):
    if isinstance(left, dict) and isinstance(right, dict) and left.keys() == right.keys():
        return [item for key in left for item in differences(left[key], right[key], f"{path}/{key}")]
    if isinstance(left, list) and isinstance(right, list) and len(left) == len(right):
        return [item for i, (a, b) in enumerate(zip(left, right))
                for item in differences(a, b, f"{path}/{i}")]
    return [] if left == right else [{"path": path, "dxf": left, "peer": right}]


def observe(source, projection_path, peer_path=None):
    import ezdxf  # Optional independent implementation, not a project dependency.

    document = ezdxf.readfile(source)
    projection = json.loads(Path(projection_path).read_text())
    if projection.get("imported") is not True:
        raise ValueError("C++ source import failed")
    observed, wrappers = [], []
    for obj in document.objects:
        if obj.dxftype() != "TABLEGEOMETRY":
            continue
        if len(obj.xtags.subclasses) != 2 or obj.xtags.appdata or obj.xtags.xdata:
            raise ValueError("unqualified common geometry metadata")
        observed.append(decode(list(obj.xtags.subclasses[1]), obj.dxf.handle, obj.dxf.owner))
        wrapper = document.entitydb[obj.dxf.owner]
        if wrapper.dxftype() != "XRECORD":
            raise ValueError("sample cache wrapper is not XRECORD")
        tags = list(wrapper.tags)
        content_handles = [t.value for t in tags if t.code == 360]
        geometry_handles = [t.value for t in tags if t.code == 361]
        if (len(content_handles) != 1 or geometry_handles != [obj.dxf.handle]
                or not any(t.code == 102 and t.value == "ACAD_ROUNDTRIP_2008_TABLE_ENTITY" for t in tags)):
            raise ValueError("sample round-trip wrapper slots differ")
        content = document.entitydb[content_handles[0]]
        dictionary = document.entitydb[wrapper.dxf.owner]
        table = document.entitydb[dictionary.dxf.owner]
        if (content.dxftype() != "TABLECONTENT" or content.dxf.owner != obj.dxf.owner
                or dictionary.dxftype() != "DICTIONARY" or table.dxftype() != "ACAD_TABLE"
                or dictionary.get("ACAD_XREC_ROUNDTRIP") is not wrapper
                or table.get_extension_dict().dictionary is not dictionary):
            raise ValueError("sample cache owner/dictionary/table chain differs")
        wrappers.append({"xrecord": wrapper.dxf.handle, "dictionary": dictionary.dxf.handle,
                         "table": table.dxf.handle, "contentHardOwner360": content.dxf.handle,
                         "geometryHardOwner361": obj.dxf.handle})
    if not observed or projection["geometry"] != observed:
        raise ValueError("C++ fields differ from independent DXF source observation")
    report = {"schemaVersion": 1, "qualificationEligible": False, "status": "SCOPED_SOURCE_FIELDS_MATCH",
              "ezdxfVersion": ezdxf.__version__, "sourceVersion": document.dxfversion,
              "sourceSha256": hashlib.sha256(Path(source).read_bytes()).hexdigest(),
              "projectionSha256": hashlib.sha256(Path(projection_path).read_bytes()).hexdigest(),
              "geometryRecords": len(observed), "cellsCompared": sum(len(g["cells"]) for g in observed),
              "contentsCompared": sum(len(c["contents"]) for g in observed for c in g["cells"]),
              "roundtripWrappers": wrappers,
              "limits": "Same-source scalar/role corroboration, not cache correctness, full TABLECONTENT, native writer, geometry or platform qualification"}
    if peer_path:
        peer = json.loads(Path(peer_path).read_text())
        candidates = [peer_payload(o) for o in peer["OBJECTS"] if o.get("object") == "TABLEGEOMETRY"]
        selected = [{k: g[k] for k in ("rows", "columns", "cells")} for g in observed]
        delta = differences(selected, candidates)
        report["pairedPeerGeometryPayload"] = "MISMATCH" if delta else "MATCH_WITHOUT_IDENTITY_EQUIVALENCE"
        if delta:
            report["status"] = "SCOPED_DXF_SOURCE_FIELDS_MATCH_PEER_PAYLOAD_DIFFERS"
            report["peerDifferenceCount"] = len(delta)
            report["peerDifferences"] = delta[:16]
            report["peerLimit"] = "No tolerance applied; decimal serialization may differ, but native bit/value equivalence is not proved"
        report["peerJsonSha256"] = hashlib.sha256(Path(peer_path).read_bytes()).hexdigest()
    return report


def self_test():
    tags = [(100, "AcDbTableGeometry"), (90, 1), (91, 1), (92, 1), (93, -2),
            (40, 3.5), (41, 4.5), (330, "A1"), (94, 1), (10, (1, 2, 3)),
            (11, (4, 5, 6)), (43, 7.5), (44, 8.5), (45, 9.5), (46, 10.5), (95, -8)]
    value = decode(tags, "A0", "0")
    assert value["owner"] == 0 and value["cells"][0]["softPointer"] == 0xA1
    assert value["cells"][0]["contents"] == [[1, 2, 3, 4, 5, 6, 7.5, 8.5, 9.5, 10.5, -8]]
    assert differences([7.599999999999999], [7.6]) == [{"path": "/0", "dxf": 7.599999999999999, "peer": 7.6}]
    controls = [tags[:-1], tags + [(100, "future")], tags[:9] + tags[10:],
                tags[:9] + [(10, (1, 2))] + tags[10:], tags[:5] + [(40, math.inf)] + tags[6:],
                tags[:3] + [(92, 2)] + tags[4:], tags[:2] + [(91, -1)] + tags[3:],
                tags[:8] + [(94, 10001)] + tags[9:], tags[:-1] + [(95, 2**31)]]
    for bad in controls:
        try:
            decode(bad, "A0", "0")
        except (ValueError, TypeError):
            continue
        raise AssertionError("observer accepted corrupt geometry profile")
    print("table geometry observer: 1 exact positive, 9 corruption refusals; qualificationEligible=false")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--source")
    parser.add_argument("--projection")
    parser.add_argument("--peer-dwg-json")
    args = parser.parse_args()
    if args.self_test:
        self_test()
    elif args.source and args.projection:
        result = observe(args.source, args.projection, args.peer_dwg_json)
        print(json.dumps(result, indent=2, allow_nan=False))
        sys.exit(1 if result.get("pairedPeerGeometryPayload") == "MISMATCH" else 0)
    else:
        parser.error("use --self-test or --source DXF --projection JSON")
