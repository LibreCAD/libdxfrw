#!/usr/bin/env python3
"""Walk candidate node registry and class names in the hash-pinned forum DWG.

This checks raw BS/BL/B/TU values on independently bounded Python cursors;
the decoded input section comes from libdxfrw and is not independent of its
page decoder. Field roles follow the ACadRust peer candidate, not ODA schema.
"""
import argparse
import json
from ac1032_forum_assocnetwork_spans import Bits, OBJECTS_HASH, SOURCE_HASH, checked

CLASSES = ["AcConstrainedBoundedLine", "AcConstrainedImplicitPoint",
           "AcPointCurveConstraint", "AcConstrainedArc",
           "AcCenterPointConstraint", "AcTangentConstraint"]
INDEXES = [1, 2, 3, 2, 3, 4, 2, 3, 2, 3, 2, 5, 6]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source_dwg")
    parser.add_argument("objects_bin")
    args = parser.parse_args()
    checked(args.source_dwg, SOURCE_HASH)
    data = checked(args.objects_bin, OBJECTS_HASH)
    offset, size = 33150, 596
    if len(data) != 33803 or offset + size > len(data):
        raise ValueError("invalid decoded source section or frame bounds")
    frame = data[offset:offset + size]
    body = Bits(frame, 121, 2418)
    text = Bits(frame, 2418, 4558)
    fields = []

    def field(name, kind, expected):
        start = body.pos
        value = body.one() if kind == "B" else body.bitshort_or_long("BL")
        if value != expected:
            raise ValueError("unexpected candidate registry field: " + name)
        fields.append({"field": name, "kind": kind, "start_bit": start,
                       "end_bit": body.pos, "value": value})

    field("next_node_id_candidate", "BL", 13)
    field("root_id_candidate", "BL", 0)
    field("root_connection_count_candidate", "BL", 0)
    field("root_flag_candidate", "B", 0)
    field("class_type_count_candidate", "BL", 6)
    strings = []
    for name in CLASSES:
        start = text.pos
        count = text.bitshort_or_long("BS")
        if count <= 0 or count > 128 or count * 16 > text.end - text.pos:
            raise ValueError("invalid bounded class string length")
        value = bytes(text.byte() for _ in range(count * 2)).decode("utf-16-le")
        if value != name:
            raise ValueError("unexpected candidate class name")
        strings.append({"start_bit": start, "end_bit": text.pos,
                        "units": count, "value": value})
    if text.pos != 4558:
        raise ValueError("unexpected class-string suffix")
    field("registered_count_candidate", "BL", 13)
    registry = []
    for index, class_index in enumerate(INDEXES, 1):
        start = body.pos
        flag = body.one()
        index_value = body.bitshort_or_long("BL")
        node_id = body.bitshort_or_long("BL")
        if (flag, index_value, node_id) != (0, class_index, index):
            raise ValueError("unexpected candidate registry tuple")
        registry.append({"start_bit": start, "end_bit": body.pos,
                         "flag": flag, "class_index": index_value,
                         "node_id": node_id, "class_name": CLASSES[index_value - 1]})
    if body.pos != 429:
        raise ValueError("unexpected candidate registry boundary")
    print(json.dumps({"source_sha256": SOURCE_HASH, "objects_sha256": OBJECTS_HASH,
                      "handle": "0x2A8", "fields": fields, "strings": strings,
                      "registry": registry}, indent=2))


if __name__ == "__main__":
    main()
