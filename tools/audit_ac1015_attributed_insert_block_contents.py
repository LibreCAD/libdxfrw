#!/usr/bin/env python3
"""Create and inspect a scratch AC1015 attributed INSERT with block geometry.

Author a DXF with ezdxf 1.4.5, convert it to ACAD2000 DWG and back using
ODA File Converter, then inspect the physical DWG with LibreDWG and the DXF
with ezdxf. All DWG/DXF outputs are scratch artifacts.
"""
import json
import math
import subprocess
import sys
import tempfile
from pathlib import Path


def author(path, singleton=False):
    import ezdxf
    assert ezdxf.__version__ == "1.4.5"
    doc = ezdxf.new("R2000", setup=True)
    block = doc.blocks.new("CONTENT_CHAIN")
    model = doc.modelspace()
    # Interleave allocations so the block chain is not inferred from handle order.
    block.add_line((1, 2, 3), (4, 5, 6))
    model.add_line((11, 12, 13), (14, 15, 16))
    if not singleton:
        block.add_circle((7, 8, 9), 2.5)
    model.add_circle((17, 18, 0), 3.5)
    if not singleton:
        block.add_arc((21, 22, 23), 4.5, 20, 110)
    insert = model.add_blockref("CONTENT_CHAIN", (3, 4, 0), dxfattribs={"rotation": 30})
    insert.add_attrib("FIRST", "ONE", (3, 4, 0), dxfattribs={"height": .5, "rotation": 15})
    insert.add_attrib("SECOND", "TWO", (4, 5, 0), dxfattribs={"height": .75, "rotation": 25})
    model.add_line((31, 32, 0), (33, 34, 0))
    doc.saveas(path)
    print("Authored", path)


def verify(source, output, dwgread, singleton=False):
    import ezdxf
    assert ezdxf.__version__ == "1.4.5"
    assert Path(source).read_bytes()[:6] == b"AC1015"
    with tempfile.TemporaryDirectory() as tmp:
        result = Path(tmp) / "source.json"
        subprocess.run([dwgread, "-O", "JSON", "-o", str(result), source], check=True)
        doc = json.loads(result.read_text())
    objects = doc["OBJECTS"]
    by_handle = {o["handle"][2]: o for o in objects if o.get("handle")}
    def ref(value, code, target):
        assert value[0] == code and value[3] == target, value
    block = by_handle[0x8A]
    assert block["object"] == "BLOCK_HEADER" and block["name"] == "CONTENT_CHAIN"
    ref(block["block_entity"], 3, 0x8B); ref(block["endblk_entity"], 3, 0x8C)
    if singleton:
        ref(block["first_entity"], 4, 0x8D); ref(block["last_entity"], 4, 0x8D)
        obj = by_handle[0x8D]
        assert obj["entity"] == "LINE" and obj["entmode"] == 0 and obj["nolinks"] == 0
        ref(obj["ownerhandle"], 4, 0x8A)
        ref(obj["prev_entity"], 4, 0); ref(obj["next_entity"], 4, 0)
        out = ezdxf.readfile(output)
        entities = list(out.blocks.get("CONTENT_CHAIN"))
        assert len(entities) == 1 and entities[0].dxftype() == "LINE"
        assert tuple(entities[0].dxf.start) == (1, 2, 3) and tuple(entities[0].dxf.end) == (4, 5, 6)
        inserts = [e for e in out.modelspace() if e.dxftype() == "INSERT"]
        assert len(inserts) == 1 and inserts[0].dxf.name == "CONTENT_CHAIN"
        assert [(a.dxf.tag, a.dxf.text) for a in inserts[0].attribs] == [("FIRST", "ONE"), ("SECOND", "TWO")]
        print("ODA source singleton block and attributed INSERT PASS")
        return
    ref(block["first_entity"], 4, 0x8D); ref(block["last_entity"], 4, 0x91)
    assert len(block["inserts"]) == 1
    ref(block["inserts"][0], 4, 0x92)
    for handle, kind in ((0x8B, "BLOCK"), (0x8C, "ENDBLK")):
        obj = by_handle[handle]
        assert obj["entity"] == kind and obj["entmode"] == 0 and obj["nolinks"] == 0
        ref(obj["ownerhandle"], 4, 0x8A)
        ref(obj["prev_entity"], 4, 0); ref(obj["next_entity"], 4, 0)
    contents = [(0x8D, "LINE"), (0x8F, "CIRCLE"), (0x91, "ARC")]
    for index, (handle, kind) in enumerate(contents):
        obj = by_handle[handle]
        assert obj["entity"] == kind and obj["entmode"] == 0 and obj["nolinks"] == 0
        ref(obj["ownerhandle"], 4, 0x8A)
        ref(obj["prev_entity"], 12 if index else 4, contents[index-1][0] if index else 0)
        ref(obj["next_entity"], 10 if index+1 < len(contents) else 4,
            contents[index+1][0] if index+1 < len(contents) else 0)
    model = [(0x8E, "LINE"), (0x90, "CIRCLE"), (0x92, "INSERT"), (0x96, "LINE")]
    for index, (handle, kind) in enumerate(model):
        obj = by_handle[handle]
        assert obj["entity"] == kind and obj["entmode"] == 2 and obj["nolinks"] == 0
        ref(obj["prev_entity"], 12 if index else 4, model[index-1][0] if index else 0)
        ref(obj["next_entity"], 10 if index+1 < len(model) else 4,
            model[index+1][0] if index+1 < len(model) else 0)
    insert = by_handle[0x92]
    ref(insert["block_header"], 5, 0x8A)
    ref(insert["first_attrib"], 4, 0x94); ref(insert["last_attrib"], 4, 0x95)
    ref(insert["seqend"], 3, 0x93)
    for handle, kind, prev, nxt, prevcode, nextcode in (
            (0x94, "ATTRIB", 0, 0x95, 4, 6), (0x95, "ATTRIB", 0x94, 0, 8, 4),
            (0x93, "SEQEND", 0, 0, 4, 4)):
        obj = by_handle[handle]
        assert obj["entity"] == kind and obj["entmode"] == 0 and obj["nolinks"] == 0
        ref(obj["ownerhandle"], 4, 0x92)
        ref(obj["prev_entity"], prevcode, prev); ref(obj["next_entity"], nextcode, nxt)
    out = ezdxf.readfile(output)
    entities = list(out.blocks.get("CONTENT_CHAIN"))
    assert [e.dxftype() for e in entities] == ["LINE", "CIRCLE", "ARC"]
    line, circle, arc = entities
    assert tuple(line.dxf.start) == (1, 2, 3) and tuple(line.dxf.end) == (4, 5, 6)
    assert tuple(circle.dxf.center) == (7, 8, 9) and circle.dxf.radius == 2.5
    assert tuple(arc.dxf.center) == (21, 22, 23) and arc.dxf.radius == 4.5
    assert arc.dxf.start_angle == 20 and arc.dxf.end_angle == 110
    model_out = list(out.modelspace())
    assert [e.dxftype() for e in model_out] == ["LINE", "CIRCLE", "INSERT", "LINE"]
    placed = model_out[2]
    assert placed.dxf.name == "CONTENT_CHAIN" and placed.dxf.rotation == 30
    assert [(a.dxf.tag, a.dxf.text, a.dxf.height, a.dxf.rotation) for a in placed.attribs] == [
        ("FIRST", "ONE", .5, 15), ("SECOND", "TWO", .75, 25)]
    print("ODA source attributed INSERT and named-block geometry graph PASS")


def main():
    if len(sys.argv) == 3 and sys.argv[1] in ("author", "author-singleton"):
        author(sys.argv[2], sys.argv[1] == "author-singleton")
    elif len(sys.argv) in (4, 5) and sys.argv[1] in ("verify", "verify-singleton"):
        verify(sys.argv[2], sys.argv[3], sys.argv[4] if len(sys.argv) == 5 else "dwgread",
               sys.argv[1] == "verify-singleton")
    else:
        raise SystemExit("usage: audit_ac1015_attributed_insert_block_contents.py author <input.dxf> | verify <source.dwg> <roundtrip.dxf> [dwgread]")


if __name__ == "__main__":
    main()
