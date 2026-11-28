#!/usr/bin/env python3
"""Hash-bind LibreDWG and raw ODA DXF evidence for a selected GROUP vertex."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile

PROFILES = {
    "source2d": {"parent": 0x2f, "vertex": 0x32, "group": 0x34, "group_owner": 0xC,
                 "vertices": [(0x31, (0., 0., 0.)), (0x32, (5., 0., 0.)), (0x33, (5., 3., 0.))], "three": False},
    "generated2d": {"parent": 0x451, "vertex": 0x501, "group": 0x900, "group_owner": 0xD,
                    "vertices": [(0x501, (1., 2., 0.)), (0x502, (3., 4., 0.))], "three": False},
    "generated3d": {"parent": 0x451, "vertex": 0x501, "group": 0x900, "group_owner": 0xD,
                    "vertices": [(0x501, (1., 2., 0.)), (0x502, (3., 4., 1.))], "three": True},
}


def require(condition, message):
    if not condition:
        raise ValueError(message)


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def chunks(text):
    lines = text.splitlines()
    require(len(lines) % 2 == 0, "odd DXF tag lines")
    current = []
    for index in range(0, len(lines), 2):
        tag = (lines[index].strip(), lines[index + 1].strip())
        if tag[0] == "0" and current:
            yield current
            current = []
        current.append(tag)
    if current:
        yield current


def one(values, message):
    require(len(values) == 1, message)
    return values[0]


def owner_and_reactors(record):
    owners = []; reactors = []; in_reactors = False
    for code, value in record:
        if code == "102" and value == "{ACAD_REACTORS": in_reactors = True
        elif code == "102" and value == "}": in_reactors = False
        elif code == "330": (reactors if in_reactors else owners).append(value)
    return owners, reactors


def validate_dxf(text, profile):
    records = list(chunks(text))
    def entity(kind, handle):
        return one([r for r in records if r[0] == ("0", kind)
                    and ("5", format(handle, "X")) in r], f"missing/duplicate {kind} {handle:X}")
    group = entity("GROUP", profile["group"])
    require([v for c, v in group if c == "340"] == [format(profile["vertex"], "X")], "wrong GROUP target")
    require(owner_and_reactors(group) == ([format(profile["group_owner"], "X")],
                                          [format(profile["group_owner"], "X")]), "wrong GROUP owner/reactor")
    parent = entity("POLYLINE", profile["parent"])
    flag = [int(v) for c, v in parent if c == "70"]
    require((flag[0] if flag else 0) & 8 == (8 if profile["three"] else 0), "wrong POLYLINE dimension")
    for handle, point in profile["vertices"]:
        v = entity("VERTEX", handle)
        owners, reactors = owner_and_reactors(v)
        require(owners == [format(profile["parent"], "X")], "wrong VERTEX owner")
        require(reactors == ([format(profile["group"], "X")] if handle == profile["vertex"] else []), "wrong VERTEX reactor")
        coords = tuple(float(one([v for c, v in v if c == code], "missing/duplicate coordinate")) for code in ("10", "20", "30"))
        require(coords == point, "wrong VERTEX coordinates")


def validate_peer(data, profile):
    objects = data["OBJECTS"]
    def obj(kind, handle, field):
        return one([o for o in objects if o.get(field) == kind and o.get("handle", [None])[-1] == handle], f"missing peer {kind}")
    group = obj("GROUP", profile["group"], "object")
    require([r[-1] for r in group.get("groups", [])] == [profile["vertex"]], "peer GROUP target")
    require(all(r[0] == 5 for r in group.get("groups", [])), "peer GROUP role")
    require(group["ownerhandle"][0] == 4 and group["ownerhandle"][-1] == profile["group_owner"]
            and group.get("is_xdic_missing") == 1
            and group.get("reactors") == [[4, 1, profile["group_owner"], profile["group_owner"]]], "peer GROUP common metadata")
    kind = "POLYLINE_3D" if profile["three"] else "POLYLINE_2D"
    poly = obj(kind, profile["parent"], "entity")
    identities = [profile["group"], profile["parent"], poly["seqend"][-1]]
    identities.extend(handle for handle, _ in profile["vertices"])
    require(len(set(identities)) == len(identities), "graph identity collision")
    for handle in identities:
        require(sum(1 for entry in objects if entry.get("handle", [None])[-1] == handle) == 1,
                "missing/duplicate global object identity")
    require([r[-1] for r in poly.get("vertex", [])] == [x[0] for x in profile["vertices"]], "peer child order")
    require(all(r[0] == 3 for r in poly.get("vertex", [])), "peer child ownership role")
    vkind = "VERTEX_3D" if profile["three"] else "VERTEX_2D"
    for handle, point in profile["vertices"]:
        vertex = obj(vkind, handle, "entity")
        require(tuple(vertex["point"]) == point, "peer coordinates")
        require(vertex["ownerhandle"][0] == 4 and vertex["ownerhandle"][-1] == profile["parent"], "peer owner")
        reactors = vertex.get("reactors", [])
        require([r[-1] for r in reactors] == ([profile["group"]] if handle == profile["vertex"] else [])
                and all(r[0] == 4 for r in reactors), "peer reactor")
    seqend = obj("SEQEND", poly["seqend"][-1], "entity")
    require(poly["seqend"][0] == 3 and seqend["ownerhandle"][0] == 4
            and seqend["ownerhandle"][-1] == profile["parent"], "peer SEQEND ownership")
    model = one([o for o in objects if o.get("object") == "BLOCK_HEADER" and o.get("name", "").upper() == "*MODEL_SPACE"], "peer model block")
    require([r[-1] for r in model.get("entities", [])] == [profile["parent"]], "peer block membership")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("profile", choices=PROFILES)
    ap.add_argument("dwg"); ap.add_argument("dxf")
    ap.add_argument("--dwg-sha256", required=True); ap.add_argument("--dxf-sha256", required=True)
    ap.add_argument("--dwgread", default="dwgread")
    args = ap.parse_args()
    expected = (args.dwg_sha256.lower(), args.dxf_sha256.lower())
    require((digest(args.dwg), digest(args.dxf)) == expected, "artifact digest mismatch")
    require(Path(args.dwg).read_bytes()[:6] == b"AC1021", "not AC1021")
    validate_dxf(Path(args.dxf).read_text(encoding="utf-8"), PROFILES[args.profile])
    with tempfile.TemporaryDirectory(prefix="ac1021-group-peer-") as tmp:
        out = Path(tmp) / "peer.json"
        subprocess.run([args.dwgread, "-O", "JSON", "-o", str(out), args.dwg],
                       check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        validate_peer(json.loads(out.read_text()), PROFILES[args.profile])
    require((digest(args.dwg), digest(args.dxf)) == expected, "artifact changed during audit")
    print(f"{args.profile}: LibreDWG and raw ODA DXF GROUP/vertex graph PASS")


if __name__ == "__main__":
    main()
