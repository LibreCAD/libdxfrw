#!/usr/bin/env python3
"""Verify a pinned AC1018 TABLE descendant source walk against peer/DXF output.

Bodies must be captured from the specified drawing's actual raw callbacks; this
script checks their pinned digests but cannot independently attest the capture.
It never modifies or embeds a drawing and does not authorize output or support.
"""
import argparse
import hashlib
import json
import pathlib
import struct
from p5_ac1018_table_xrecord_source_verify import Cursor

SOURCE = '1ba45d704ed7cc846ea4d6d08c7c78062f8090842ebc9e9a4cdefcb44ea6ab2a'
PEER = '23d17c79abb050743822552f1fb9e0db644ad43a2e9fca76db6c40a99bf4db5b'
DXF = '2c182a1f735af71a52cae7c5406e29ffecfc1b63d06eadf5114a7540f67879f3'
HASHES = {
 0xb9:'5caa9921eb02acc3efc94c6d71f90c9bc400dbb7969eb95c4dadf142064630f2',
 0xba:'bee29eb5b223a434febedbbd3939631821489a8a60022d3ea0ce878d47868a3c',
 0xbd:'1fc357e038d9e51ab8be68efe4a54f0333dfc3d5e3a4e0c03f22d55607e95ea3',
 0xbe:'15363bce84acd60729bd48a3e120029c65cd29e95f88a1c9148dd625826c3ff3',
 0xbf:'09f39a0c1f3238dd6b8777574724bf3e050993b86f96fa94e12655d837df7b48',
 0xc0:'6173c96ae8e0b3020702408453e47da957c4b5510eba7c999e992bf2d86ecada',
 0xc1:'b06d127e646ce1a66f850f2d53388a017b5e71e4903d633a94299f8e1f47f2ee',
 0xc2:'adcf1794a681df4d08c498039cd2eea0843bbc53ac0caf4a11c849e13fe6dc99',
 0xc3:'6c67c04ed00b488b7947b00396be966dff1fda851c5c90e7c8281f8f2d059da4',
 0xc4:'2eebdef56a93d8ac4db915ab20187bd7dd0e279990c22a5ffd1707a766a231b0',
 0xc5:'b46f4a5817d48372a33500c1eb556bae5a9988e0b9b62f338fdd87352f3cb197',
 0xc6:'27e780cea8040a9a2a69a0162a8db70d3c57aaa14c5a872becaa2c1e5569e88b',
}

def require(value, message):
    if not value:
        raise ValueError(message)

def load(path, expected):
    data=path.read_bytes()
    require(hashlib.sha256(data).hexdigest()==expected, f'hash mismatch: {path}')
    return data

def body_start(data, objtype, handle, end):
    c=Cursor(data)
    require(c.short()==objtype, 'unexpected type')
    require(int.from_bytes(c.raw(4),'little')==end, 'unexpected body end')
    h=c.handle()
    require((h['role'],h['value'])==(0,handle), 'unexpected identity')
    require(c.short()==0 and c.long()==0 and c.bits(1)==1, 'unexpected common shell')
    require(c.position==71, 'unexpected prefix end')
    return c

def bd(c):
    tag=c.bits(2)
    if tag==0: return struct.unpack('<d', c.raw(8))[0]
    require(tag in (1,2), 'reserved BD')
    return 1.0 if tag==1 else 0.0

def tail(c, expected):
    got=[c.handle() for _ in expected]
    require([(v['role'],v['value']) for v in got]==expected, 'handle tail mismatch')
    remainder=len(c.data)*8-c.position
    require(remainder<8 and c.bits(remainder)==0, 'bad padding')
    return got

def records(path):
    lines=path.read_text(encoding='utf-8').splitlines()
    require(len(lines)%2==0,'incomplete DXF pair')
    out=[]; row=[]
    for code,value in zip(lines[::2],lines[1::2]):
        pair=(code.strip(),value.strip())
        if pair[0]=='0' and row:
            out.append(row); row=[]
        row.append(pair)
    if row:out.append(row)
    return out

def record(rows, typ, handle):
    matches=[r for r in rows if r[0]==('0',typ) and ('5',handle) in r]
    require(len(matches)==1, f'expected unique {typ} {handle}')
    return matches[0]

def xrecord(data,h,expected_text):
    c=Cursor(data)
    require(c.short()==79, 'child XRECORD type')
    end=int.from_bytes(c.raw(4),'little'); own=c.handle()
    require(own['value']==h and own['role']==0,'child identity')
    require(c.short()==0 and c.long()==0 and c.bits(1)==1, 'child common shell')
    size=c.long(); payload=c.raw(size)
    require(c.short()==1 and c.position==end,'child cloning or end')
    handles=tail(c,[(4,0xbd)])
    pos=0; values=[]; pages=[]
    while pos<len(payload):
        require(pos+2<=len(payload),'truncated XRECORD code')
        code=int.from_bytes(payload[pos:pos+2],'little');pos+=2
        if code in (90,93,94):
            require(pos+4<=len(payload),'truncated XRECORD integer')
            value=int.from_bytes(payload[pos:pos+4],'little',signed=True);pos+=4
        else:
            require(code in (1,300,302) and pos+3<=len(payload),'unexpected string code')
            n=int.from_bytes(payload[pos:pos+2],'little');page=payload[pos+2];pos+=3
            require(pos+n<=len(payload),'truncated XRECORD string')
            value=payload[pos:pos+n].decode('ascii');pos+=n;pages.append(page)
        values.append([code,value])
    require(all(p==45 for p in pages),'unexpected per-value codepage')
    if expected_text is None:
        expected=[[93,7],[90,0],[94,0],[300,''],[302,'']]
    else:
        expected=[[93,4],[90,4],[1,expected_text],[94,0],[300,''],[302,expected_text]]
    require(values==expected,'child payload mismatch')
    return {'bodyEnd':end,'owner':handles[0],'pairs':values,'stringCodepages':pages}

def main():
    a=argparse.ArgumentParser(description=__doc__)
    a.add_argument('--source',type=pathlib.Path,required=True)
    a.add_argument('--peer-json',type=pathlib.Path,required=True)
    a.add_argument('--dxf',type=pathlib.Path,required=True)
    a.add_argument('--body-dir',type=pathlib.Path,required=True,
                   help='directory containing p5-desc-body-<decimal handle>.bin callback captures')
    o=a.parse_args()
    load(o.source,SOURCE); peer=json.loads(load(o.peer_json,PEER));load(o.dxf,DXF)
    bodies={h:load(o.body_dir/f'p5-desc-body-{h}.bin',sha) for h,sha in HASHES.items()}
    dx=records(o.dxf)
    c=body_start(bodies[0xba],515,0xba,173)
    require((c.long(),c.long(),c.long())==(3,3,9),'geometry dimensions')
    geometry=[]
    for _ in range(9): geometry.append([c.long(),bd(c),bd(c),c.long()])
    require(geometry==[[0,0.,0.,0]]*9 and c.position==173,'geometry cells')
    geom_tail=tail(c,[(4,0xbc)]+[(4,0)]*9)
    geom=record(dx,'TABLEGEOMETRY','112')
    require(geom[2:7]==[('330','114'),('100','AcDbTableGeometry'),('90','3'),('91','3'),('92','9')], 'DXF geometry prefix')
    require(geom[7:]==[('93','0'),('40','0.0'),('41','0.0'),('330','0'),('94','0')]*9, 'DXF geometry cells')
    objects=peer['OBJECTS']
    for name,handle in [('TABLECONTENT',0xb9),('TABLEGEOMETRY',0xba),('DATATABLE',0xbd)]:
        matches=[x for x in objects if x.get('object')==name and x.get('handle',[])[-1:]==[handle]]
        require(len(matches)==1 and matches[0].get('ownerhandle',[])[-1:]==[0xbc], 'peer descendant identity/owner')
        if name=='TABLEGEOMETRY':
            require(matches[0].get('numrows')==3 and matches[0].get('numcols')==3, 'peer geometry dimensions')
    c=body_start(bodies[0xbd],517,0xbd,877)
    require((c.short(),c.long(),c.long(),c.short())==(2,4,9,0),'DATATABLE prefix')
    require((c.long(),c.short())==(2,0),'column 0')
    doubles=[bd(c) for _ in range(9)]
    require((c.long(),c.short())==(1,0),'column 1')
    first=[c.long() for _ in range(9)]
    require((c.long(),c.short())==(1,0),'column 2')
    second=[c.long() for _ in range(9)]
    require((c.long(),c.short())==(6,0) and c.position==877,'column 3')
    require(doubles==[1579.,0.,0.,49.,50.,51.,52.,53.,54.] and first==[262144,0,0,262144,262144,262144,262144,262144,262144] and second==[0]*9,'column values')
    dtable_tail=tail(c,[(4,0xbc)]+[(3,h) for h in range(0xbe,0xc7)])
    table=record(dx,'DATATABLE','115')
    expected=[('0','DATATABLE'),('5','115'),('330','114'),('100','AcDbDataTable'),('70','2'),('90','4'),('91','9'),('1',''),('92','2'),('2','')]
    expected += [('40',str(x)) for x in doubles]+[('92','1'),('2','')]
    expected += [('93',str(x)) for x in first]+[('92','1'),('2','')]
    expected += [('93',str(x)) for x in second]+[('92','6'),('2','')]
    expected += [('360',format(h,'X')) for h in range(0x116,0x11f)]
    require(table==expected,'DATATABLE DXF projection')
    children={}
    for h in range(0xbe,0xc7):
        label='Title' if h==0xbe else None if h in (0xbf,0xc0) else str(h-0xc0)
        observed=xrecord(bodies[h],h,label); children[hex(h)]=observed
        m=[r for r in objects if r.get('object')=='XRECORD' and r.get('handle',[])[-1:]==[h]]
        require(len(m)==1 and m[0].get('ownerhandle',[])[-1:]==[0xbd] and m[0].get('xdata')==observed['pairs'],'LibreDWG child mismatch')
        dxrec=record(dx,'XRECORD',format(0x116+h-0xbe,'X'))
        require(dxrec[2:5]==[('330','115'),('100','AcDbXrecord'),('280','1')], 'DXF child prefix')
        require(dxrec[5:]==[(str(k),str(v)) for k,v in observed['pairs']], 'DXF child payload')
    c=body_start(bodies[0xb9],514,0xb9,13313)
    require((c.short(),c.short(),c.long(),c.short(),c.long())==(0,0,3,0,0),'TABLECONTENT prefix')
    c.position=13313
    content_tail=tail(c,[(4,0xbc)]+[(5,0)]*50+[(5,0x6b)])
    content=record(dx,'TABLECONTENT','111')
    require(('330','114') in content and ('340','6B') in content,'TABLECONTENT DXF references')
    require(any(x.get('object')=='TABLESTYLE' and x.get('handle',[])[-1:]==[0x6b] for x in objects),'TABLESTYLE target')
    print(json.dumps({'status':'pass','sourceSha256':SOURCE,'dxfSha256':DXF,
      'geometry':{'bodyEnd':173,'cells':len(geometry),'handleTail':geom_tail},
      'datatable':{'bodyEnd':877,'doubles':doubles,'integers':first,'zeroIntegers':second,'handleTail':dtable_tail},
      'children':children,'tableContent':{'bodyEnd':13313,'prefixEnd':89,'handleTailCount':len(content_tail),'unqualifiedNullReferences':50,'lastHardPointer':content_tail[-1]}},indent=2))

if __name__=='__main__':main()
