#!/usr/bin/env python3
"""Create or verify a controlled ODA AC1018 XRECORD codepage-45 probe.

The XRECORD is authored locally in a pinned existing ODA-DXF source. DWG input,
output DXF and raw-callback capture are external scratch artifacts, not fixtures.
The producer and reader are the same vendor; output proves only that profile.
"""
import argparse
import hashlib
import json
from pathlib import Path
from p5_ac1018_table_xrecord_source_verify import Cursor

BASE='2c182a1f735af71a52cae7c5406e29ffecfc1b63d06eadf5114a7540f67879f3'
INPUT='0661a2661ecdc09f5b5b2d3fe4bc489f0817ee2fdf44b26b24bb470bee01c002'
DRAWING='39d94194e86473276b9a8bf3164e2f90c78cf67ac4d6a1afa4669f18479701ed'
BODY='5881e3e2498dcd5d0d6e2709a25340d00ce72d12fc4313d85f7e0315a5e853af'
OUTPUT='4fffb221e84714f1759fb98987531fc1875e694125577a7ca69edea5bda905d1'
PEER='86657ed2b301701a1769c3ca6d5b8b9f7bc7d2dfe02bcb9b75eda6a2d7e559f1'

def require(ok, message):
    if not ok: raise ValueError(message)

def pinned(path, digest):
    raw=path.read_bytes()
    require(hashlib.sha256(raw).hexdigest()==digest,f'hash mismatch: {path}')
    return raw

def pairs(raw, encoding):
    lines=raw.decode(encoding).splitlines()
    require(len(lines)%2==0,'odd number of DXF lines')
    return list(zip(lines[::2],lines[1::2]))

def record(items, typ, handle):
    rows=[];current=[]
    for k,v in items:
        entry=(k.strip(),v.strip())
        if entry[0]=='0' and current:rows.append(current);current=[]
        current.append(entry)
    if current:rows.append(current)
    found=[r for r in rows if r[0]==('0',typ) and ('5',handle) in r]
    require(len(found)==1, f'expected unique {typ} {handle}')
    return found[0]

def author(args):
    lines=pairs(pinned(args.base_dxf,BASE),'utf-8')
    nod=[]
    for i,(k,v) in enumerate(lines[:-2]):
        if k.strip()=='0' and v.strip()=='DICTIONARY' and lines[i+1][1].strip()=='C' and lines[i+2][1].strip()=='0':nod.append(i)
    require(len(nod)==1,'named-object root not unique')
    pos=nod[0]+1
    while lines[pos][0].strip()!='0':pos+=1
    lines[pos:pos]=[('  3','CODEPAGE_PROBE'),('350','7FFE')]
    entries=[('  0','XRECORD'),('  5','7FFE'),('330','C'),('100','AcDbXrecord'),('280','1'),
             ('  1','café—€'),('  2',r'\U+0416'),('  3',r'\U+6F22')]
    lines[pos+2:pos+2]=entries
    data=('\r\n'.join(value for pair in lines for value in pair)+'\r\n').encode('cp1252')
    require(hashlib.sha256(data).hexdigest()==INPUT,'authored input digest mismatch')
    with args.output.open('xb') as target:target.write(data)
    print('PASS authored input',INPUT)

def verify(args):
    source=pinned(args.input_dxf,INPUT)
    dwg=pinned(args.dwg,DRAWING)
    body=pinned(args.body,BODY)
    output=pinned(args.output_dxf,OUTPUT)
    peer=json.loads(pinned(args.peer_json,PEER))
    require(dwg[:6]==b'AC1018' and int.from_bytes(dwg[0x13:0x15],'little')==30,'DWG revision/file codepage mismatch')
    require(b'caf\xe9\x97\x80' in source,'input CP1252 probe bytes missing')
    original=record(pairs(source,'cp1252'),'XRECORD','7FFE')
    require(original[-3:]==[('1','café—€'),('2',r'\U+0416'),('3',r'\U+6F22')],'authored XRECORD mismatch')
    c=Cursor(body)
    typ=c.short();end=int.from_bytes(c.raw(4),'little');identity=c.handle()
    require(typ==79 and end==331 and identity['role']==0 and identity['value']==0x7ffe,'XRECORD prefix mismatch')
    require(c.short()==0 and c.long()==0 and c.bits(1)==1,'unexpected common shell')
    size=c.long();require(size==30,'data size mismatch');data=c.raw(size)
    expected=[(1,bytes.fromhex('636166c3a9e28094c280')),(2,bytes.fromhex('d096')),(3,bytes.fromhex('e6bca2'))]
    pos=0;values=[]
    for expected_code,expected_bytes in expected:
        require(pos+5<=len(data),'truncated XRECORD pair')
        code=int.from_bytes(data[pos:pos+2],'little');n=int.from_bytes(data[pos+2:pos+4],'little');page=data[pos+4];pos+=5
        value=data[pos:pos+n];pos+=n
        require((code,page,value)==(expected_code,45,expected_bytes),'XRECORD source value mismatch')
        values.append({'group':code,'codepage':page,'rawHex':value.hex(),'utf8':value.decode('utf-8')})
    require(pos==len(data) and c.short()==1 and c.position==end,'data or clone boundary mismatch')
    owner=c.handle();require((owner['role'],owner['value'])==(4,0xc),'owner role mismatch')
    pad=len(body)*8-c.position
    require(pad<8 and c.bits(pad)==0,'trailing padding mismatch')
    roundtrip=record(pairs(output,'utf-8'),'XRECORD','7FFE')
    projected=[(str(v['group']),v['utf8']) for v in values]
    require(roundtrip[-3:]==projected,'ODA output does not match UTF-8 projection')
    require(values[0]['utf8']=='café—\u0080' and original[-3][1]=='café—€','expected euro mismatch absent')
    records=[x for x in peer['OBJECTS'] if x.get('object')=='XRECORD' and x.get('handle',[])[-1:]==[0x7ffe]]
    require(len(records)==1 and records[0].get('ownerhandle',[])[-1:]==[0xc],'peer identity mismatch')
    observed=records[0]['xdata']
    require([x[0] for x in observed]==[1,2,3] and observed[1][1]!=values[1]['utf8'] and observed[2][1]!=values[2]['utf8'],'peer decoding differs from expected observation')
    print(json.dumps({'status':'pass','drawingSha256':DRAWING,'fileCodepage':30,'sourceBodyEndBit':end,
      'sourceOwner':owner,'values':values,'odaOutput':projected,'authoredEuro':'€',
      'odaObservedEuro':'\\u0080','libredwg014Projection':observed,
      'scope':'one ODA producer/consumer; no cross-vendor semantic qualification'},indent=2,ensure_ascii=True))

parser=argparse.ArgumentParser(description=__doc__)
sub=parser.add_subparsers(dest='command',required=True)
a=sub.add_parser('author');a.add_argument('--base-dxf',type=Path,required=True);a.add_argument('--output',type=Path,required=True);a.set_defaults(fn=author)
v=sub.add_parser('verify');v.add_argument('--input-dxf',type=Path,required=True);v.add_argument('--dwg',type=Path,required=True);v.add_argument('--body',type=Path,required=True);v.add_argument('--output-dxf',type=Path,required=True);v.add_argument('--peer-json',type=Path,required=True);v.set_defaults(fn=verify)
args=parser.parse_args();args.fn(args)
