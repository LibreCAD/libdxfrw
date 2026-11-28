#!/usr/bin/env python3
"""Inspect scratch AC1015 block contents and attributed INSERT with LibreDWG."""
import json
import math
import subprocess
import sys
import tempfile
from pathlib import Path


def main():
    if len(sys.argv) not in (3, 4) or (len(sys.argv) == 4 and sys.argv[3] not in ("singleton", "scaled", "tilted", "negativez")):
        raise SystemExit("usage: dwg_legacy_block_content_chain_libredwg.py <dwgread> <file.dwg> [singleton|scaled|tilted|negativez]")
    singleton = len(sys.argv) == 4 and sys.argv[3] == "singleton"
    mode = sys.argv[3] if len(sys.argv) == 4 else "normal"
    scaled = mode in ("scaled", "tilted", "negativez")
    with tempfile.TemporaryDirectory() as tmp:
        path = Path(tmp) / "out.json"
        subprocess.run([sys.argv[1], "-O", "JSON", "-o", str(path), sys.argv[2]], check=True)
        objects = json.loads(path.read_text())["OBJECTS"]
    by_handle = {o["handle"][2]: o for o in objects if o.get("handle")}
    def ref(value, code, target):
        assert value[0] == code and value[3] == target, value
    block = by_handle[0xD00]
    assert block["object"] == "BLOCK_HEADER" and block["name"] == "CONTENT_CHAIN"
    ref(block["block_entity"], 3, 0xD10); ref(block["endblk_entity"], 3, 0xD01)
    ref(block["first_entity"], 4, 0x901); ref(block["last_entity"], 4, 0x901 if singleton else 0xA22)
    assert len(block["inserts"]) == 1
    ref(block["inserts"][0], 4, 0x451)
    for handle, kind in ((0xD10, "BLOCK"), (0xD01, "ENDBLK")):
        boundary = by_handle[handle]
        assert boundary["entity"] == kind and boundary["entmode"] == 0
        ref(boundary["ownerhandle"], 4, 0xD00)
        ref(boundary["prev_entity"], 4, 0); ref(boundary["next_entity"], 4, 0)
    model = [(0x130, "LINE"), (0x451, "INSERT"), (0x999, "CIRCLE")]
    model_headers = [o for o in objects if o.get("object") == "BLOCK_HEADER" and o.get("name") == "*Model_Space"]
    assert len(model_headers) == 1
    ref(model_headers[0]["first_entity"], 4, model[0][0]); ref(model_headers[0]["last_entity"], 4, model[-1][0])
    for i, (handle, kind) in enumerate(model):
        entity = by_handle[handle]
        assert entity["entity"] == kind and entity["entmode"] == 2 and entity["nolinks"] == 0
        ref(entity["prev_entity"], 4, model[i-1][0] if i else 0)
        ref(entity["next_entity"], 4, model[i+1][0] if i+1 < len(model) else 0)
    content = [(0x901, "LINE")] if singleton else [(0x901, "LINE"), (0x550, "CIRCLE"), (0xA22, "ARC")]
    for i, (handle, kind) in enumerate(content):
        obj = by_handle[handle]
        assert obj["entity"] == kind and obj["entmode"] == 0 and obj["nolinks"] == 0
        ref(obj["ownerhandle"], 4, 0xD00)
        ref(obj["prev_entity"], 4, content[i-1][0] if i else 0)
        ref(obj["next_entity"], 4, content[i+1][0] if i+1 < len(content) else 0)
    assert by_handle[0x901]["start"] == [1., 2., 3.] and by_handle[0x901]["end"] == [4., 5., 6.]
    if not singleton:
        assert by_handle[0x550]["center"] == [7., 8., 9.] and by_handle[0x550]["radius"] == 2.5
        arc = by_handle[0xA22]
        assert arc["center"] == [21., 22., 23.] and arc["radius"] == 4.5
        assert math.isclose(arc["start_angle"], math.radians(20), abs_tol=1e-9)
        assert math.isclose(arc["end_angle"], math.radians(110), abs_tol=1e-9)
    insert = by_handle[0x451]
    assert insert["entity"] == "INSERT" and insert["has_attribs"] == 1
    assert insert["scale"] == ([2., 3., 4.] if scaled else [1., 1., 1.])
    assert insert.get("extrusion", [0., 0., 1.]) == {
        "tilted": [0., 1., 0.], "negativez": [0., 0., -1.]}.get(mode, [0., 0., 1.])
    assert math.isclose(insert.get("rotation", 0.), math.pi/6 if scaled else 0., abs_tol=1e-9)
    ref(insert["block_header"], 5, 0xD00)
    ref(insert["first_attrib"], 4, 0xA10); ref(insert["last_attrib"], 4, 0xA10)
    ref(insert["seqend"], 3, 0xA50)
    for handle, kind in ((0xA10, "ATTRIB"), (0xA50, "SEQEND")):
        obj = by_handle[handle]
        assert obj["entity"] == kind and obj["entmode"] == 0
        ref(obj["ownerhandle"], 4, 0x451)
        ref(obj["prev_entity"], 4, 0); ref(obj["next_entity"], 4, 0)
    attr = by_handle[0xA10]
    assert attr["tag"] == "FIRST" and attr["text_value"] == "ONE" and attr["height"] == .5
    assert attr["ins_pt"] == [3., 4.] and math.isclose(attr.get("rotation", 0.), 0., abs_tol=1e-9)
    print("LibreDWG AC1015 named-block content chain PASS")


if __name__ == "__main__":
    main()
