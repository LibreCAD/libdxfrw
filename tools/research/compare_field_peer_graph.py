#!/usr/bin/env python3
"""Compare a LibreDWG minJSON FIELD inventory with an ASCII DXF peer export.

This measures decoded agreement; it does not prove source wire spans, lossless
export, evaluator behavior, or authority of either implementation.
"""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def records(path):
    lines = path.read_text(encoding="utf-8").splitlines()
    if len(lines) % 2:
        raise ValueError("truncated DXF group pair")
    result = []
    for index in range(0, len(lines), 2):
        code, value = int(lines[index].strip()), lines[index + 1]
        if code == 0:
            result.append((value.strip(), []))
        elif not result:
            raise ValueError("DXF value without a record")
        else:
            result[-1][1].append((code, value))
    return result


def one(groups, code):
    values = [value for actual, value in groups if actual == code]
    if len(values) != 1:
        raise ValueError("missing or duplicated group %s" % code)
    return values[0]


def object_handle(groups):
    return int(one(groups, 5), 16)


def field_owner(groups):
    depth = 0
    owners = []
    for code, value in groups:
        if code == 100:
            break
        if code == 102:
            if value.startswith("{"):
                depth += 1
            elif value == "}":
                depth -= 1
                if depth < 0:
                    raise ValueError("unbalanced application group")
        elif code == 330 and depth == 0:
            owners.append(int(value, 16))
    if depth or len(owners) != 1:
        raise ValueError("ambiguous field owner")
    return owners[0]


def field_body(groups):
    positions = [i for i, (code, value) in enumerate(groups)
                 if code == 100 and value == "AcDbField"]
    if len(positions) != 1:
        raise ValueError("missing or duplicated AcDbField subclass")
    return groups[positions[0] + 1:]


def children(body):
    result = []
    current = None
    for code, value in body:
        if code == 6:
            if current is not None:
                raise ValueError("unclosed child value")
            current = {"key": value}
        elif code == 7:
            if current is not None:
                raise ValueError("unclosed child before FIELD root value")
            return result
        elif code == 304 and current is not None:
            if value != "ACVALUE_END":
                raise ValueError("unexpected child value terminator")
            result.append(current)
            current = None
        elif current is not None and code in (90, 91, 93, 94, 1, 140, 300, 302):
            if code in current:
                raise ValueError("duplicated child-value group %s" % code)
            current[code] = value
    raise ValueError("FIELD missing root value marker")


def compare(minjson, dxf):
    objects = json.loads(minjson.read_text(encoding="utf-8"))["OBJECTS"]
    source_fields = [obj for obj in objects if obj.get("object") == "FIELD"]
    source = {obj["handle"][-1]: obj for obj in source_fields}
    source_lists = [obj for obj in objects if obj.get("object") == "FIELDLIST"]
    if len(source_fields) != len(source) or len(source_lists) != 1:
        raise ValueError("ambiguous source FIELD inventory")
    all_records = records(dxf)
    object_records = []
    section = None
    for name, groups in all_records:
        if name == "SECTION":
            if not groups or groups[0][0] != 2:
                raise ValueError("SECTION missing name")
            section = groups[0][1]
        elif name == "ENDSEC":
            section = None
        elif section == "OBJECTS":
            object_records.append((name, groups))
    output_fields = [groups for name, groups in object_records if name == "FIELD"]
    output = {object_handle(groups): groups for groups in output_fields}
    output_lists = [groups for name, groups in object_records if name == "FIELDLIST"]
    if len(output_fields) != len(output) or len(output_lists) != 1:
        raise ValueError("ambiguous peer FIELD inventory")
    source_list = [value[-1] for value in source_lists[0]["fields"]]
    list_groups = output_lists[0]
    count_index = next(i for i, (code, _) in enumerate(list_groups) if code == 90)
    declared_count = int(list_groups[count_index][1])
    peer_list = [int(value, 16) for code, value in list_groups[count_index + 1:] if code == 330]
    if declared_count != len(peer_list):
        raise ValueError("peer FIELDLIST count mismatch")
    mismatches = Counter()
    examples = {}
    matched_children = 0
    missing_child_arrays = 0
    missing_arrays_with_peer_children = 0
    compared_child_fields = Counter()
    evaluators = Counter()

    def check(name, handle, left, right):
        if left != right:
            mismatches[name] += 1
            examples.setdefault(name, "0x%X" % handle)

    for handle in sorted(source.keys() & output.keys()):
        field, groups = source[handle], output[handle]
        body = field_body(groups)
        evaluators[field["id"]] += 1
        check("owner", handle, field["ownerhandle"][-1], field_owner(groups))
        prefix_end = next(i for i, (code, _) in enumerate(body) if code == 90)
        prefix = body[:prefix_end]
        check("evaluator", handle, field["id"], one(prefix, 1))
        one(prefix, 2)
        check("expression", handle, field["code"],
              "".join(value for code, value in prefix if code in (2, 3)))
        end = next(i for i, (code, _) in enumerate(body) if code == 97)
        check("child_field_edges", handle, [x[-1] for x in field.get("childs", [])],
              [int(value, 16) for code, value in body[:end] if code == 360])
        check("object_edges", handle, [x[-1] for x in field.get("objects", [])],
              [int(value, 16) for code, value in body if code == 331])
        for name, code in (("evaluation_option", 91), ("filing_option", 92),
                           ("field_state", 94), ("evaluation_status", 95),
                           ("evaluation_error_code", 96), ("evaluation_error_msg", 300)):
            peer = next(value for actual, value in body if actual == code)
            check(name, handle, field[name], peer if code == 300 else int(peer))
        decoded = field.get("childval", [])
        exported = children(body)
        if "childval" in field:
            check("child_value_count", handle, len(decoded), len(exported))
        else:
            missing_child_arrays += 1
            if exported:
                missing_arrays_with_peer_children += 1
        matched_children += min(len(decoded), len(exported))
        for left, right in zip(decoded, exported):
            for name, code, convert in (("key", "key", str), ("value.format_flags", 93, int),
                                        ("value.data_type", 90, int), ("value.unit_type", 94, int),
                                        ("value.format_string", 300, str), ("value.value_string", 302, str),
                                        ("value.data_string", 1, str), ("value.data_long", 91, int),
                                        ("value.data_double", 140, float)):
                if name in left and code in right:
                    compared_child_fields[name] += 1
                    check("child." + name, handle, left[name], convert(right[code]))

    return {
        "minjsonSha256": sha256(minjson), "peerDxfSha256": sha256(dxf),
        "sourceFields": len(source), "peerFields": len(output),
        "sameFieldHandles": set(source) == set(output),
        "fieldListHandlesEqual": source_lists[0]["handle"][-1] == object_handle(list_groups),
        "fieldListOwnersEqual": source_lists[0]["ownerhandle"][-1] == field_owner(list_groups),
        "sourceFieldListMembers": len(source_list), "peerFieldListMembers": len(peer_list),
        "sourceFieldListMatchesFieldInventory": set(source_list) == set(source),
        "peerFieldListMatchesFieldInventory": set(peer_list) == set(output),
        "orderedFieldListsEqual": source_list == peer_list,
        "sourceFieldListUnique": len(set(source_list)) == len(source_list),
        "sourceChildFieldEdges": sum(len(x.get("childs", [])) for x in source.values()),
        "sourceObjectEdges": sum(len(x.get("objects", [])) for x in source.values()),
        "evaluatorCounts": dict(sorted(evaluators.items())),
        "pairedChildValues": matched_children,
        "sourceUnreportedChildArrays": missing_child_arrays,
        "unreportedArraysWithPeerChildren": missing_arrays_with_peer_children,
        "comparedChildMembers": dict(sorted(compared_child_fields.items())),
        "mismatches": dict(sorted(mismatches.items())), "firstMismatchHandles": examples,
        "limitations": "Decoded peer comparison only; no wire-schema, native validity, or lossless-output claim.",
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("minjson", type=Path)
    parser.add_argument("peer_dxf", type=Path)
    args = parser.parse_args()
    print(json.dumps(compare(args.minjson, args.peer_dxf), indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
