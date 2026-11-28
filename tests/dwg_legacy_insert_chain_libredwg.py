#!/usr/bin/env python3
"""Independently inspect generated AC1015 empty-block attributed INSERTs."""
import json
import math
import subprocess
import sys
import tempfile
from pathlib import Path


def main():
    if len(sys.argv) not in (3,4) or (len(sys.argv)==4 and sys.argv[3] not in ("rotated", "group", "uniform", "xunit", "nonuniform", "fractional")):
        raise SystemExit("usage: dwg_legacy_insert_chain_libredwg.py <dwgread> <generated.dwg> [rotated|group|uniform|xunit|nonuniform|fractional]")
    rotated=len(sys.argv)==4 and sys.argv[3]=="rotated"
    group=len(sys.argv)==4 and sys.argv[3]=="group"
    scales={"uniform":[2.,2.,2.],"xunit":[1.,2.,3.],"nonuniform":[2.,3.,4.],"fractional":[.5,1.25,2.75]}
    expected_scale=scales.get(sys.argv[3],[1.,1.,1.]) if len(sys.argv)==4 else [1.,1.,1.]
    with tempfile.TemporaryDirectory() as tmp:
        output=Path(tmp)/"out.json"
        subprocess.run([sys.argv[1],"-O","JSON","-o",str(output),sys.argv[2]],check=True)
        document=json.loads(output.read_text())
    objects=document["OBJECTS"]
    by_handle={o["handle"][2]:o for o in objects if o.get("handle")}
    def ref(value,code,target):
        assert value[0]==code and value[3]==target,value
    if group:
        obj=by_handle[0xF20]
        assert obj["object"]=="GROUP" and len(obj["groups"])==1
        ref(obj["ownerhandle"],4,0xD);ref(obj["groups"][0],5,0x451)
    block=by_handle[0xD00]
    assert block["object"]=="BLOCK_HEADER" and block["name"]=="EMPTY_CHAIN"
    assert block["base_pt"]==[0.,0.,0.] and block["hasattrs"]==0
    ref(block["block_entity"],3,0xD10);ref(block["endblk_entity"],3,0xD01)
    ref(block["first_entity"],4,0);ref(block["last_entity"],4,0)
    assert len(block["inserts"])==2
    ref(block["inserts"][0],4,0x451);ref(block["inserts"][1],4,0x777)
    for h,kind in ((0xD10,"BLOCK"),(0xD01,"ENDBLK")):
        boundary=by_handle[h]
        assert boundary["entity"]==kind and boundary["entmode"]==0 and boundary["nolinks"]==0
        ref(boundary["ownerhandle"],4,0xD00);ref(boundary["prev_entity"],4,0);ref(boundary["next_entity"],4,0)
    model=[(0x130,"LINE"),(0x451,"INSERT"),(0x999,"CIRCLE"),(0x777,"INSERT"),(0xB00,"LINE")]
    block_model=[o for o in objects if o.get("object")=="BLOCK_HEADER" and o.get("name")=="*Model_Space"]
    assert len(block_model)==1
    ref(block_model[0]["first_entity"],4,model[0][0]);ref(block_model[0]["last_entity"],4,model[-1][0])
    for i,(h,kind) in enumerate(model):
        obj=by_handle[h];assert obj["entity"]==kind and obj["entmode"]==2 and obj["nolinks"]==0
        ref(obj["prev_entity"],4,model[i-1][0] if i else 0)
        ref(obj["next_entity"],4,model[i+1][0] if i+1<len(model) else 0)
    for parent,attrs,seq in ((0x451,[0xA10,0xA02],0xA50),(0x777,[0xC10],0xC50)):
        obj=by_handle[parent];assert obj["has_attribs"]==1
        assert obj["scale"] == (expected_scale if parent==0x451 else [1.,1.,1.])
        assert math.isclose(obj.get("rotation",0.),math.pi/6 if rotated and parent==0x451 else 0.,abs_tol=1e-9)
        ref(obj["block_header"],5,0xD00);ref(obj["first_attrib"],4,attrs[0])
        ref(obj["last_attrib"],4,attrs[-1]);ref(obj["seqend"],3,seq)
        for i,h in enumerate(attrs):
            child=by_handle[h];assert child["entity"]=="ATTRIB" and child["entmode"]==0 and child["nolinks"]==0
            ref(child["ownerhandle"],4,parent);ref(child["prev_entity"],4,attrs[i-1] if i else 0)
            ref(child["next_entity"],4,attrs[i+1] if i+1<len(attrs) else 0)
        footer=by_handle[seq];assert footer["entity"]=="SEQEND" and footer["entmode"]==0 and footer["nolinks"]==0
        ref(footer["ownerhandle"],4,parent);ref(footer["prev_entity"],4,0);ref(footer["next_entity"],4,0)
    expected=((0xA10,"FIRST","ONE",[3.,4.],.5,15.),
              (0xA02,"SECOND","TWO",[4.,5.],.75,25.),
              (0xC10,"SINGLE","THREE",[9.,10.],.6,0.))
    for h,tag,text,point,height,angle in expected:
        obj=by_handle[h];assert obj["tag"]==tag and obj["text_value"]==text
        assert obj["ins_pt"]==point and math.isclose(obj["height"],height,abs_tol=1e-9)
        assert math.isclose(obj.get("rotation",0.),math.radians(angle),abs_tol=1e-9)
    print("LibreDWG AC1015 empty-block attributed INSERT chain PASS")


if __name__=="__main__":main()
