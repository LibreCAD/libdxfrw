#!/usr/bin/env python3
"""Walk one pinned CRC-captured AC1018 TABLE entity common handle prefix.

Input body hash binds this observation but does not itself authenticate source
or validate CRC. Capture must use the separately documented instrument/read.
No graph closure or output support is established.
"""
import argparse
import hashlib
import json
from pathlib import Path
from p5_ac1018_table_xrecord_source_verify import Cursor, require
from p5_ac1018_table_descendants_verify import SOURCE, PEER, load

BODY='d79e300c1fde2031114023ab665149688e4d8236bb5ff2f591f387c7baca93a2'

def inspect(data):
    require(len(data)==2287,'body size')
    c=Cursor(data)
    require(c.short()==513 and c.position==18,'class')
    end=int.from_bytes(c.raw(4),'little')
    require(end==18156 and end < len(data)*8,'data-end boundary')
    ident=c.handle()
    require((ident['role'],ident['size'],ident['value'],c.position)==(0,1,0x9c,66),'self handle')
    require(c.short()==0 and c.position==68,'EED')
    require(c.bits(1)==1,'proxy flag')
    graphic_size=int.from_bytes(c.raw(4),'little')
    require(graphic_size==2104 and c.position==101,'proxy size')
    # Validate the complete proxy span without interpreting cached geometry.
    require(graphic_size <= (end-c.position)//8,'proxy escapes object data')
    c.raw(graphic_size)
    require(c.position==16933,'proxy end')
    mode=c.bits(2);reactors=c.long();missing=c.bits(1)
    require((mode,reactors,missing,c.position)==(2,0,0,16938),'entity handle selectors')
    # ENC 0x0100 has no 0x40 AcDbColor reference; later common fields do
    # not precede the xdictionary H, but pin them before interpreting layer.
    require(c.short()==256 and c.position==16940,'entity color')
    require(c.bits(2)==1,'linetype scale')
    require(c.bits(2)==0 and c.bits(2)==0,'linetype/plotstyle flags')
    require(c.short()==0 and c.bits(8)==29 and c.position==16956,'visibility/lineweight')
    c.position=end
    xdic=c.handle();layer=c.handle();block=c.handle();style=c.handle()
    require((xdic['role'],xdic['size'],xdic['value'],xdic['startBit'],xdic['endBit'])==(3,1,0xbb,18156,18172),'xdictionary H3')
    require([(x['role'],x['size'],x['value']) for x in (layer,block,style)]==[(5,1,0x10),(5,1,0x9d),(5,1,0x6b)],'following handle sequence')
    return {'type':513,'handle':ident,'dataEndBit':end,'eedBytes':0,'proxyFlag':1,'proxyDataStartBit':101,'proxyBytes':graphic_size,'proxyDataEndBit':16933,'entmode':mode,'reactors':reactors,'xdictionaryMissing':missing,'xdictionary':xdic,'followingHandles':[layer,block,style]}

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--source',type=Path,required=True);p.add_argument('--body',type=Path,required=True)
    p.add_argument('--peer-json',type=Path,required=True);p.add_argument('--self-test',action='store_true')
    a=p.parse_args();load(a.source,SOURCE);data=load(a.body,BODY);peer=json.loads(load(a.peer_json,PEER));result=inspect(data)
    objs=[o for o in peer['OBJECTS'] if o.get('handle',[])[-1:]==[0x9c]]
    require(len(objs)==1,'peer source object')
    o=objs[0]
    require(o['dxfname']=='ACAD_TABLE' and o['type']==513 and o['bitsize']==18156 and o['size']==2287,'peer source identity')
    require((o['preview_exists'],o['preview_size'],o['entmode'],o['is_xdic_missing'])==(1,2104,2,0),'peer selectors')
    require(o['xdicobjhandle']==[3,1,0xbb,0xbb] and o['layer'][-1]==0x10 and o['block_header'][-1]==0x9d and o['tablestyle'][-1]==0x6b,'peer handles')
    if a.self_test:
        for damaged in (data[:-1], data[:18156//8]+bytes([data[18156//8]^(1 << (7-18156%8))])+data[18156//8+1:]):
            try: inspect(damaged)
            except ValueError: continue
            raise ValueError('damaged input accepted')
    print(json.dumps(result,indent=2))
if __name__=='__main__':main()
