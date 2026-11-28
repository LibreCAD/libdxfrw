#!/usr/bin/env python3
"""Check generated bounded AC1015 POLYLINE chains through GNU LibreDWG."""
import json
import subprocess
import sys
import tempfile
from pathlib import Path


def main():
    if len(sys.argv) != 4 or sys.argv[3] not in ("2d", "3d", "2d-two", "3d-two", "multiple", "rich"):
        raise SystemExit("usage: dwg_legacy_polyline_chain_libredwg.py <dwgread> <drawing.dwg> <2d|3d|2d-two|3d-two|multiple|rich>")
    mode = sys.argv[3]
    with tempfile.TemporaryDirectory() as tmp:
        target = Path(tmp) / "out.json"
        command=[sys.argv[1]] + (["-v4"] if mode=="rich" else []) + ["-O", "JSON", "-o", str(target), sys.argv[2]]
        result=subprocess.run(command,check=True,capture_output=True,text=True)
        trace=result.stdout+result.stderr
        document = json.loads(target.read_text())
    objects=document["OBJECTS"]
    by_handle={o["handle"][2]:o for o in objects if o.get("handle")}
    if mode == "multiple":
        model=[0x451,0x777]
        block=[o for o in objects if o.get("object")=="BLOCK_HEADER" and o.get("name")=="*Model_Space"]
        assert len(block)==1 and block[0]["first_entity"][3]==model[0] and block[0]["last_entity"][3]==model[-1]
        for i,(parent,kind,vertices,seq) in enumerate(((0x451,"2D",[0xA10,0xA02],0xA50),(0x777,"3D",[0xC11,0xC00],0xC50))):
            obj=by_handle[parent]
            assert obj["entity"]=="POLYLINE_"+kind and obj["entmode"]==2 and obj["nolinks"]==0
            assert obj["prev_entity"][0]==4 and obj["prev_entity"][3]==(model[i-1] if i else 0)
            assert obj["next_entity"][0]==4 and obj["next_entity"][3]==(model[i+1] if i+1<len(model) else 0)
            assert obj["first_vertex"][0]==4 and obj["first_vertex"][3]==vertices[0]
            assert obj["last_vertex"][0]==4 and obj["last_vertex"][3]==vertices[-1]
            assert obj["seqend"][0]==3 and obj["seqend"][3]==seq
            for index,handle in enumerate(vertices):
                child=by_handle[handle]
                assert child["entity"]=="VERTEX_"+kind and child["ownerhandle"][0]==4 and child["ownerhandle"][3]==parent
                assert child["prev_entity"][0]==4 and child["prev_entity"][3]==(vertices[index-1] if index else 0)
                assert child["next_entity"][0]==4 and child["next_entity"][3]==(vertices[index+1] if index+1<len(vertices) else 0)
            footer=by_handle[seq]
            assert footer["entity"]=="SEQEND" and footer["ownerhandle"][0]==4 and footer["ownerhandle"][3]==parent
        print("LibreDWG AC1015 multiple POLYLINE chains PASS")
        return
    model=[0x130,0x451,0x999]; vertices=[0xA10,0xA02] if mode.endswith("-two") else [0xA10,0xA02,0xA20]
    block=[o for o in objects if o.get("object")=="BLOCK_HEADER" and o.get("name")=="*Model_Space"]
    assert len(block)==1 and block[0]["first_entity"][3]==model[0] and block[0]["last_entity"][3]==model[-1]
    for i,h in enumerate(model):
        item=by_handle[h]
        assert item["entmode"]==2 and item["nolinks"]==0
        assert item["prev_entity"][0]==4 and item["prev_entity"][3]==(model[i-1] if i else 0)
        assert item["next_entity"][0]==4 and item["next_entity"][3]==(model[i+1] if i+1<len(model) else 0)
    poly=by_handle[0x451]
    assert poly["entity"]==("POLYLINE_3D" if mode.startswith("3d") else "POLYLINE_2D")
    assert poly["first_vertex"][0]==4 and poly["first_vertex"][3]==vertices[0]
    assert poly["last_vertex"][0]==4 and poly["last_vertex"][3]==vertices[-1]
    assert poly["seqend"][0]==3 and poly["seqend"][3]==0xA50
    for i,h in enumerate(vertices):
        vertex=by_handle[h]
        assert vertex["entity"]==("VERTEX_3D" if mode.startswith("3d") else "VERTEX_2D")
        assert vertex["entmode"]==0 and vertex["ownerhandle"][0]==4 and vertex["ownerhandle"][3]==0x451
        assert vertex["nolinks"]==0
        assert vertex["prev_entity"][0]==4 and vertex["prev_entity"][3]==(vertices[i-1] if i else 0)
        assert vertex["next_entity"][0]==4 and vertex["next_entity"][3]==(vertices[i+1] if i+1<len(vertices) else 0)
        assert vertex["point"]==[float(i),float(i*2),float(i+1) if mode.startswith("3d") else 0.0]
    if mode == "rich":
        assert poly["start_width"] == .5 and poly["end_width"] == .75 and poly["thickness"] == .25
        vertex=by_handle[vertices[0]]
        assert vertex["bulge"] == .25 and vertex["tangent_dir"] == .75
        # GNU LibreDWG 0.14 JSON omits VERTEX_2D widths; inspect the exact
        # matching object's independent -v4 decode instead of assuming 0.
        chunks=[chunk for chunk in trace.split("==========================================")
                if "Decode entity VERTEX_2D" in chunk and "handle: 0.2.A10 [H 5]" in chunk]
        assert len(chunks)==1 and "start_width: 0.2 [BD 40]" in chunks[0]
        assert "end_width: 0.4 [BD 41]" in chunks[0]
    seq=by_handle[0xA50]
    assert seq["entity"]=="SEQEND" and seq["entmode"]==0
    assert seq["ownerhandle"][0]==4 and seq["ownerhandle"][3]==0x451
    assert seq["prev_entity"][0]==4 and seq["prev_entity"][3]==0
    assert seq["next_entity"][0]==4 and seq["next_entity"][3]==0
    print("LibreDWG AC1015 bounded POLYLINE chain PASS", mode)


if __name__=="__main__":
    main()
