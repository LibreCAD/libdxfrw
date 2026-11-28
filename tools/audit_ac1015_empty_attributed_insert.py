#!/usr/bin/env python3
"""Author and independently inspect an empty-block AC1015 INSERT witness.

`author <file.dxf>` requires ezdxf 1.4.5. Convert to ACAD2000 DWG with ODA,
then convert that DWG back to DXF. `verify <source.dwg> <roundtrip.dxf>
[dwgread]` checks the witnessed ownership graph with GNU LibreDWG 0.14 and
the retained fields with ezdxf. These generated drawings are scratch only.
"""
import json
import math
import subprocess
import sys
import tempfile
from pathlib import Path


def author(path, rotated=False):
    import ezdxf
    if ezdxf.__version__ != "1.4.5":
        raise SystemExit("author requires ezdxf 1.4.5")
    drawing=ezdxf.new("R2000",setup=True)
    drawing.blocks.new("EMPTY_CHAIN")
    model=drawing.modelspace()
    model.add_line((1,2,3),(4,5,6))
    first=model.add_blockref("EMPTY_CHAIN",(3,4,0))
    if rotated: first.dxf.rotation=30.
    first.add_attrib("FIRST","ONE",(3,4,0),dxfattribs={"height":.5,"rotation":15})
    first.add_attrib("SECOND","TWO",(4,5,0),dxfattribs={"height":.75,"rotation":25})
    model.add_circle((7,8),2.5)
    second=model.add_blockref("EMPTY_CHAIN",(9,10,0))
    second.add_attrib("SINGLE","THREE",(9,10,0),dxfattribs={"height":.6})
    model.add_line((11,12),(13,14))
    drawing.saveas(path)
    print("Authored",path)


def verify(source, output, dwgread, rotated=False):
    import ezdxf
    if ezdxf.__version__ != "1.4.5":
        raise SystemExit("verify requires ezdxf 1.4.5")
    if Path(source).read_bytes()[:6]!=b"AC1015":
        raise SystemExit("expected AC1015 source")
    with tempfile.TemporaryDirectory() as tmp:
        out=Path(tmp)/"source.json"
        subprocess.run([dwgread,"-O","JSON","-o",str(out),source],check=True)
        doc=json.loads(out.read_text())
    objects=doc["OBJECTS"]
    by_handle={o["handle"][2]:o for o in objects if o.get("handle")}
    def ref(value,code,target):
        assert value[0]==code and value[3]==target,value
    def ent(handle,kind,mode):
        obj=by_handle[handle]
        assert obj["entity"]==kind and obj["entmode"]==mode and obj["nolinks"]==0,obj
        return obj
    block=by_handle[0x8A]
    assert block["object"]=="BLOCK_HEADER" and block["name"]=="EMPTY_CHAIN"
    assert block["hasattrs"]==0 and block["base_pt"]==[0.,0.,0.]
    ref(block["block_entity"],3,0x8B);ref(block["endblk_entity"],3,0x8C)
    ref(block["first_entity"],4,0);ref(block["last_entity"],4,0)
    assert len(block["inserts"])==2
    ref(block["inserts"][0],4,0x8E);ref(block["inserts"][1],4,0x93)
    for handle,kind in ((0x8B,"BLOCK"),(0x8C,"ENDBLK")):
        boundary=ent(handle,kind,0);ref(boundary["ownerhandle"],4,0x8A)
        ref(boundary["prev_entity"],4,0);ref(boundary["next_entity"],4,0)
    model=[(0x8D,"LINE"),(0x8E,"INSERT"),(0x92,"CIRCLE"),(0x93,"INSERT"),(0x96,"LINE")]
    for i,(handle,kind) in enumerate(model):
        obj=ent(handle,kind,2)
        assert obj["prev_entity"][3]==(model[i-1][0] if i else 0)
        assert obj["next_entity"][3]==(model[i+1][0] if i+1<len(model) else 0)
    for parent,children,footer in ((0x8E,[0x90,0x91],0x8F),(0x93,[0x95],0x94)):
        insert=by_handle[parent]
        expected_rotation=math.radians(30.) if rotated and parent==0x8E else 0.
        assert math.isclose(insert.get("rotation",0.),expected_rotation,abs_tol=1e-9)
        assert insert["has_attribs"]==1
        ref(insert["block_header"],5,0x8A)
        ref(insert["first_attrib"],4,children[0]);ref(insert["last_attrib"],4,children[-1])
        ref(insert["seqend"],3,footer)
        end=ent(footer,"SEQEND",0);ref(end["ownerhandle"],4,parent)
        ref(end["prev_entity"],4,0);ref(end["next_entity"],4,0)
        for i,handle in enumerate(children):
            child=ent(handle,"ATTRIB",0);ref(child["ownerhandle"],4,parent)
            ref(child["prev_entity"],8 if i else 4,children[i-1] if i else 0)
            ref(child["next_entity"],6 if i+1<len(children) else 4,children[i+1] if i+1<len(children) else 0)
    result=ezdxf.readfile(output)
    assert len(result.blocks.get("EMPTY_CHAIN"))==0
    inserts=[e for e in result.modelspace() if e.dxftype()=="INSERT"]
    assert len(inserts)==2 and all(e.dxf.name=="EMPTY_CHAIN" for e in inserts)
    assert math.isclose(inserts[0].dxf.rotation,30. if rotated else 0.,abs_tol=1e-9)
    expected=[[('FIRST','ONE',(3.,4.,0.),.5,15.),('SECOND','TWO',(4.,5.,0.),.75,25.)],
              [('SINGLE','THREE',(9.,10.,0.),.6,0.)]]
    for insert, wanted in zip(inserts,expected):
        assert len(insert.attribs)==len(wanted)
        for attribute,(tag,text,point,height,rotation) in zip(insert.attribs,wanted):
            assert attribute.dxf.tag==tag and attribute.dxf.text==text
            assert all(math.isclose(x,y,abs_tol=1e-9) for x,y in zip(attribute.dxf.insert,point))
            assert math.isclose(attribute.dxf.height,height,abs_tol=1e-9)
            assert math.isclose(attribute.dxf.rotation,rotation,abs_tol=1e-9)
    print("ODA source empty-block ATTRIB/SEQEND graph and roundtrip PASS")


def main():
    if len(sys.argv)==3 and sys.argv[1] in ("author","author-rotated"):
        author(sys.argv[2],sys.argv[1]=="author-rotated")
    elif len(sys.argv) in (4,5) and sys.argv[1] in ("verify","verify-rotated"):
        verify(sys.argv[2],sys.argv[3],sys.argv[4] if len(sys.argv)==5 else "dwgread",sys.argv[1]=="verify-rotated")
    else:raise SystemExit("usage: audit_ac1015_empty_attributed_insert.py author|author-rotated <input.dxf> | verify|verify-rotated <source.dwg> <output.dxf> [dwgread]")


if __name__=="__main__":main()
