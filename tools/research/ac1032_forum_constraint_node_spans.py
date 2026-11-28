#!/usr/bin/env python3
"""Walk 13 selected constraint-node bodies from the hash-bound AC1032 sample.

The source DWG and libdxfrw-decompressed OBJECTS section are hash-checked.
Body/handle intervals and candidate field order were obtained by instrumenting
ACadRust 0.6.8. The separate Python primitive walk checks every interval and
compares selected geometry and dependency edges to a hash-bound LibreDWG JSON.
This does not independently validate decompression, node semantics, status or
registry flags, or authorize output of the containing graph.
"""
import argparse
import json
import math
import struct

from ac1032_forum_assocnetwork_spans import Bits, OBJECTS_HASH, SOURCE_HASH, checked
from ac1032_forum_constraint_registry_spans import CLASSES, INDEXES

PEER_HASH = "1ad1aa1677c56ff001ef676be3c810a7cd2c5b18c94e5be2fa1f426f62d14b87"
# Candidate intervals are frame-relative: start, common end, body end,
# followed by handle-stream start and end. The peer order is not normative.
SPANS = [
    (429,487,1026,4679,4703), (1026,1064,1118,4703,4711),
    (1118,1166,1170,4711,4711), (1170,1208,1262,4711,4719),
    (1262,1310,1314,4719,4719), (1314,1382,1934,4719,4743),
    (1934,1972,2026,4743,4751), (2026,2074,2078,4751,4751),
    (2078,2116,2170,4751,4759), (2170,2218,2222,4759,4759),
    (2222,2260,2314,4759,4767), (2314,2362,2366,4767,4767),
    (2366,2414,2418,4767,4767),
]


def signed_bl(bits):
    value = bits.bitshort_or_long("BL")
    return value if value < 0x80000000 else value - 0x100000000


def bitdouble(bits):
    code = (bits.one() << 1) | bits.one()
    if code == 0:
        value = struct.unpack("<d", bytes(bits.byte() for _ in range(8)))[0]
        if not math.isfinite(value):
            raise ValueError("non-finite bitdouble")
        return value
    if code == 1:
        return 1.0
    if code == 2:
        return 0.0
    raise ValueError("invalid bitdouble code")


def vector(bits):
    return [bitdouble(bits) for _ in range(3)]


def close_vec(left, right, tolerance=1e-8):
    return len(left) == len(right) and all(abs(a - b) <= tolerance for a, b in zip(left, right))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source_dwg")
    parser.add_argument("objects_bin")
    parser.add_argument("libredwg_minjson")
    args = parser.parse_args()
    checked(args.source_dwg, SOURCE_HASH)
    data = checked(args.objects_bin, OBJECTS_HASH)
    peer = json.loads(checked(args.libredwg_minjson, PEER_HASH))
    offset, size = 33150, 596
    if len(data) != 33803 or offset + size > len(data):
        raise ValueError("unexpected OBJECTS section or frame boundary")
    frame = data[offset:offset + size]
    result = []
    for node_id, (start, common_end, end, hstart, hend) in enumerate(SPANS, 1):
        body = Bits(frame, start, end)
        handles = Bits(frame, hstart, hend)
        parsed_id = signed_bl(body)
        count = signed_bl(body)
        if parsed_id != node_id or not 0 <= count <= 13:
            raise ValueError("unexpected node ID or connection count")
        connections = [signed_bl(body) for _ in range(count)]
        status = body.byte()
        if body.pos != common_end:
            raise ValueError("node common fields escaped their candidate span")
        class_name = CLASSES[INDEXES[node_id - 1] - 1]
        if class_name in ("AcPointCurveConstraint", "AcCenterPointConstraint", "AcTangentConstraint"):
            fields = {"owner": signed_bl(body), "implied": body.one(), "active": body.one()}
        elif class_name == "AcConstrainedImplicitPoint":
            ref = handles.handle()
            geometry_node = signed_bl(body)
            point = vector(body) if ref["raw_ref"] else None
            fields = {"handle": ref, "geometry_node": geometry_node, "point": point,
                      "type": body.byte(), "index": signed_bl(body), "curve": signed_bl(body)}
        elif class_name == "AcConstrainedBoundedLine":
            fields = {"handle": handles.handle(), "geometry_node": signed_bl(body),
                      "point": vector(body), "direction": vector(body), "ray": body.one(),
                      "start": vector(body), "end": vector(body)}
        elif class_name == "AcConstrainedArc":
            fields = {"handle": handles.handle(), "geometry_node": signed_bl(body),
                      "center": vector(body), "normal": vector(body), "direction": vector(body),
                      "radius": bitdouble(body), "start_parameter": bitdouble(body),
                      "end_parameter": bitdouble(body), "reserved": bitdouble(body),
                      "start": vector(body), "end": vector(body)}
        else:
            raise ValueError("unsupported candidate class")
        if body.pos != end or handles.pos != hend:
            raise ValueError("candidate class fields escaped source spans")
        result.append({"id": node_id, "class_name": class_name, "connections": connections,
                       "status_raw": status, "body_start": start, "common_end": common_end,
                       "body_end": end, "handle_start": hstart, "handle_end": hend,
                       "fields": fields})

    objects = {o["handle"][2]: o for o in peer["OBJECTS"]
               if isinstance(o.get("handle"), list) and len(o["handle"]) >= 3}
    line, arc, line_dep, arc_dep = (objects[h] for h in (0x2A3, 0x2A2, 0x2A9, 0x2AA))
    line_fields, arc_fields = result[0]["fields"], result[5]["fields"]
    if line.get("entity") != "LINE" or arc.get("entity") != "ARC":
        raise ValueError("peer geometry targets are not LINE and ARC")
    if line_fields["handle"] != {"code": 4, "size": 2, "raw_ref": 0x2A9} or arc_fields["handle"] != {"code": 4, "size": 2, "raw_ref": 0x2AA}:
        raise ValueError("geometry node dependency role mismatch")
    if not close_vec(line_fields["start"], line["start"]) or not close_vec(line_fields["end"], line["end"]):
        raise ValueError("line end points differ")
    if (not close_vec(arc_fields["center"], arc["center"])
            or not close_vec(arc_fields["normal"], arc["extrusion"])
            or abs(arc_fields["radius"] - arc["radius"]) > 1e-8):
        raise ValueError("arc center, normal or radius differs")
    for angle, key in ((arc["start_angle"], "start"), (arc["end_angle"], "end")):
        expected = [arc["center"][0] + arc["radius"] * math.cos(angle),
                    arc["center"][1] + arc["radius"] * math.sin(angle), arc["center"][2]]
        if not close_vec(arc_fields[key], expected):
            raise ValueError("trimmed arc point differs")
    for dependency, target in ((line_dep, 0x2A3), (arc_dep, 0x2A2)):
        if (dependency.get("object") != "ASSOCGEOMDEPENDENCY"
                or dependency["ownerhandle"][-1] != 0x2A8
                or dependency["assocdep.dep_on"][0] != 4
                or dependency["assocdep.dep_on"][-1] != target):
            raise ValueError("dependency owner or target differs")
    if (line["reactors"][0][0] != 4 or line["reactors"][0][-1] != 0x2A9
            or arc["reactors"][0][0] != 4 or arc["reactors"][0][-1] != 0x2AA):
        raise ValueError("geometry reactor backlink differs")
    print(json.dumps({"source_sha256": SOURCE_HASH, "objects_sha256": OBJECTS_HASH,
                      "libredwg_json_sha256": PEER_HASH, "nodes": result,
                      "selected_graph": "0x2A8 -> 0x2A9 -> LINE 0x2A3; 0x2A8 -> 0x2AA -> ARC 0x2A2",
                      "note": "Arc 0..2pi candidate curve domain differs from the ARC trim angles; compare trimmed end points."}, indent=2))


if __name__ == "__main__":
    main()
