#!/usr/bin/env python3
"""Author and independently inspect the documented AC1015 compound witness.

No drawing is checked in. `author <file.dxf>` uses ezdxf 1.4.5. Convert the
result using ODA File Converter to ACAD2000 DWG, then run `verify <file.dwg>
[dwgread]` with GNU LibreDWG 0.14. The fixed authored handles and ODA source
profile are intentional; a producer that renumbers or legitimately encodes
other link roles needs a separately reviewed verifier/profile.
"""
import json
import subprocess
import sys
import tempfile
from pathlib import Path


def author(path):
    import ezdxf
    if ezdxf.__version__ != "1.4.5":
        raise SystemExit("author requires ezdxf 1.4.5")
    drawing = ezdxf.new("R2000", setup=True)
    block = drawing.blocks.new("RESEARCH_CHAIN")
    block.add_line((0, 0), (2, 0))
    block.add_attdef("FIRST", insert=(0, 0), text="D1", height=.2)
    block.add_attdef("SECOND", insert=(0, 1), text="D2", height=.2)
    model = drawing.modelspace()
    model.add_line((0, 0), (1, 0))
    insert = model.add_blockref("RESEARCH_CHAIN", (3, 4))
    insert.add_auto_attribs({"FIRST": "ONE", "SECOND": "TWO"})
    model.add_circle((7, 8), 2)
    insert = model.add_blockref("RESEARCH_CHAIN", (9, 10))
    insert.add_attrib("SINGLE", "THREE", (9, 10))
    model.add_line((11, 12), (13, 14))
    model.add_polyline2d([(15, 0), (16, 2), (18, 0)])
    model.add_polyline3d([(20, 1, 2), (21, 2, 3), (22, 3, 4)])
    model.add_line((23, 24), (25, 26))
    drawing.saveas(path)
    print("Authored", path)


def verify(path, dwgread):
    if Path(path).read_bytes()[:6] != b"AC1015":
        raise SystemExit("expected an AC1015 DWG")
    with tempfile.TemporaryDirectory() as tmp:
        output = Path(tmp) / "decoded.json"
        subprocess.run([dwgread, "-O", "JSON", "-o", str(output), path], check=True)
        document = json.loads(output.read_text())
    objects = document["OBJECTS"]
    entries = {o["handle"][2]: o for o in objects if o.get("handle")}

    def h(value, code, target, *, raw=None):
        assert value[0] == code and value[3] == target, value
        if raw is not None:
            assert value[2] == raw, value

    def entity(handle, kind, mode=2):
        result = entries[handle]
        assert result["entity"] == kind and result["entmode"] == mode, result
        return result

    # Top-level entity order deliberately skips every compound child/SEQEND.
    model = [(0x90, "LINE"), (0x91, "INSERT"), (0x95, "CIRCLE"),
             (0x96, "INSERT"), (0x99, "LINE"), (0x9A, "POLYLINE_2D"),
             (0x9F, "POLYLINE_3D"), (0xA4, "LINE")]
    prev_codes = [4, 8, 12, 8, 12, 8, 12, 12]
    next_codes = [6, 10, 6, 10, 6, 10, 10, 4]
    for index, (handle, kind) in enumerate(model):
        obj = entity(handle, kind)
        assert obj["nolinks"] == 0
        h(obj["prev_entity"], prev_codes[index], model[index-1][0] if index else 0)
        h(obj["next_entity"], next_codes[index], model[index+1][0] if index+1 < len(model) else 0)
    headers = [o for o in objects if o.get("object") == "BLOCK_HEADER"
               and o.get("name") == "*Model_Space"]
    assert len(headers) == 1
    h(headers[0]["first_entity"], 4, 0x90)
    h(headers[0]["last_entity"], 4, 0xA4)

    compounds = [
        (0x91, "INSERT", "ATTRIB", [0x93, 0x94], 0x92),
        (0x96, "INSERT", "ATTRIB", [0x98], 0x97),
        (0x9A, "POLYLINE_2D", "VERTEX_2D", [0x9C, 0x9D, 0x9E], 0x9B),
        (0x9F, "POLYLINE_3D", "VERTEX_3D", [0xA1, 0xA2, 0xA3], 0xA0),
    ]
    for owner, kind, child_kind, children, terminator in compounds:
        parent = entity(owner, kind)
        prefix = "attrib" if kind == "INSERT" else "vertex"
        if kind == "INSERT":
            assert parent["has_attribs"] == 1
        h(parent["first_" + prefix], 4, children[0])
        h(parent["last_" + prefix], 4, children[-1])
        h(parent["seqend"], 3, terminator)
        seqend = entity(terminator, "SEQEND", 0)
        h(seqend["ownerhandle"], 4, owner)
        assert seqend["nolinks"] == 0
        h(seqend["prev_entity"], 4, 0)
        h(seqend["next_entity"], 4, 0)
        for index, handle in enumerate(children):
            child = entity(handle, child_kind, 0)
            h(child["ownerhandle"], 4, owner)
            if len(children) == 3 and index == 1:
                assert child["nolinks"] == 1
                assert "prev_entity" not in child and "next_entity" not in child
                assert children[index-1] == handle-1 and children[index+1] == handle+1
            else:
                assert child["nolinks"] == 0
                h(child["prev_entity"], 8 if index else 4,
                  children[index-1] if index else 0)
                h(child["next_entity"], 6 if index+1 < len(children) else 4,
                  children[index+1] if index+1 < len(children) else 0)
    print("AC1015 INSERT/POLYLINE compound chain PASS")


def main():
    if len(sys.argv) == 3 and sys.argv[1] == "author":
        author(sys.argv[2])
    elif len(sys.argv) in (3, 4) and sys.argv[1] == "verify":
        verify(sys.argv[2], sys.argv[3] if len(sys.argv) == 4 else "dwgread")
    else:
        raise SystemExit("usage: audit_ac1015_compound_entity_chains.py author <out.dxf> | verify <source.dwg> [dwgread]")


if __name__ == "__main__":
    main()
