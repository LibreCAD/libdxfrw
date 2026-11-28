#!/usr/bin/env python3
"""Verify a pinned AC1018 TABLESTYLE/CELLSTYLEMAP graph and selected fields.

Raw callback captures do not by themselves prove their source association; ODA
DXF is same-vendor evidence, and no output/support qualification is implied.
"""
import argparse
import json
import struct
from pathlib import Path
from p5_ac1018_table_xrecord_source_verify import Cursor
from p5_ac1018_table_descendants_verify import SOURCE, PEER, DXF, load, records, record

if not __debug__:
    raise RuntimeError('Run without Python optimizations')
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--source',type=Path,required=True)
parser.add_argument('--peer-json',type=Path,required=True)
parser.add_argument('--dxf',type=Path,required=True)
parser.add_argument('--body-dir',type=Path,required=True)
a=parser.parse_args()
load(a.source,SOURCE);peer=json.loads(load(a.peer_json,PEER));load(a.dxf,DXF)
hashes={0x6a:'541100cb6a7f419ca7922ed5fef42c522ccbfb73242e51e96cfa1e0ea15977e0',
        0x6b:'8fc864c19c3588fd7167908650b39d7ec369cdeb9508b1337da1873cc5c8abda',
        0xdb:'fbad2c3da87264281c944cf5ac9983442b3d7e9e910c19b48ea67dfa307beed0',
        0xdc:'e73c19f8102c18c0b3ea6761d4b74062874cde5abe2355e9ac5a87ca9d11ffa9',
        0xdd:'c015cfeffa6352247050f29f29a355cf171473865199f4f29bfea54d4a3e2fb4'}
b={h:load(a.body_dir/f'body-{h}.bin',digest) for h,digest in hashes.items()}
dx=records(a.dxf);objects=peer['OBJECTS']

def common(data,typ,end,ident,reactors,missing):
    c=Cursor(data);assert c.short()==typ and int.from_bytes(c.raw(4),'little')==end
    own=c.handle();assert (own['role'],own['value'])==(0,ident)
    assert c.short()==0 and c.long()==reactors and c.bits(1)==missing
    return c

def tail(data,end,expected):
    c=Cursor(data,end);out=[c.handle() for _ in expected]
    assert [(x['role'],x['value']) for x in out]==expected
    rem=len(data)*8-c.position;assert rem<8 and c.bits(rem)==0
    return out

def tv(c):
    n=c.short();assert n<=1024
    return c.raw(n).decode('ascii').rstrip('\x00')
def bd(c):
    tag=c.bits(2)
    if tag==0:return struct.unpack('<d',c.raw(8))[0]
    assert tag in (1,2)
    return 1.0 if tag==1 else 0.0
def color(c):
    index=c.short();rgb=c.long() & 0xffffffff;flags=c.bits(8)
    assert index==0 and flags==0 and rgb in (0xc1000000,0xc8000000)
    return rgb

def source_dict(h, end, count, clone, owner, names, expected_tail):
    c=common(b[h],42,end,h,1 if h==0x6a else 0,1)
    assert c.long()==count and c.short()==clone and c.bits(8)==owner
    observed=[tv(c) for _ in range(count)]
    assert observed==names and c.position==end
    return tail(b[h],end,expected_tail)

dictA=source_dict(0x6a,173,1,1,0,['Standard'],[(4,0xc),(4,0xc),(2,0x6b)])
dictB=source_dict(0xdb,607,2,1,1,['ACAD_XREC_ROUNDTRIP','ACAD_ROUNDTRIP_2008_TABLESTYLE_CELLSTYLEMAP'],[(4,0x6b),(2,0xdc),(2,0xdd)])
# ODA DXF lists the same dictionary pairs in a different physical order.
xd=record(dx,'DICTIONARY','12B')
assert xd==[('0','DICTIONARY'),('5','12B'),('330','6B'),('100','AcDbDictionary'),('280','1'),('281','1'),('3','ACAD_ROUNDTRIP_2008_TABLESTYLE_CELLSTYLEMAP'),('360','12D'),('3','ACAD_XREC_ROUNDTRIP'),('360','12C')]

# Ancillary style XRECORD 0xdc: observed RS/RC legacy strings; no broad
# interpretation of its codepage byte or XRECORD ordering is inferred.
c=common(b[0xdc],79,803,0xdc,1,1);count=c.long();start=c.position
assert count==89;payload=c.raw(count);assert c.short()==1 and c.position==803
xdata=[];pos=0
while pos<len(payload):
    group=int.from_bytes(payload[pos:pos+2],'little');pos+=2
    if group in (90,91,92,93,94,95):
        value=int.from_bytes(payload[pos:pos+4],'little',signed=True);pos+=4
    else:
        assert group in (1,2,3,102) and pos+3<=len(payload)
        n=int.from_bytes(payload[pos:pos+2],'little');page=payload[pos+2];pos+=3
        assert page==45 and pos+n<=len(payload)
        value=payload[pos:pos+n].decode('ascii');pos+=n
    xdata.append([group,value])
assert xdata==[[102,'ACAD_ROUNDTRIP_PRE2007_TABLESTYLE'],[90,4],[91,0],[1,''],[92,4],[93,0],[2,''],[94,4],[95,0],[3,'']]
xTail=tail(b[0xdc],803,[(4,0xdb),(4,0xdb)])
xPeer=[o for o in objects if o.get('object')=='XRECORD' and o.get('handle',[])[-1:]==[0xdc]]
assert len(xPeer)==1 and xPeer[0]['xdata']==xdata and xPeer[0]['ownerhandle'][-1]==0xdb
xDxf=record(dx,'XRECORD','12C');assert xDxf[-len(xdata):]==[(str(k),str(v)) for k,v in xdata]

# Independently walk the observed 2004 backsave body and separated handle tail.
# ODA v5.4.1 introduces CELLSTYLEMAP in 2008; this is a producer profile only.
c=common(b[0xdd],508,5287,0xdd,1,1)
h=Cursor(b[0xdd],5287)
roles=[]
def cell_handle(role,value):
    item=h.handle(); assert (item['role'],item['value'])==(role,value)
    roles.append(item);return item
cell_handle(4,0xdb);cell_handle(4,0xdb)
assert c.long()==3 and c.position==89
styles=[]
for expected_id,expected_class,expected_name,expected_merge,expected_align,expected_height in [
        (3,2,'_DATA',0,2,4.5),(1,1,'_TITLE',32768,5,6.0),(2,1,'_HEADER',0,5,4.5)]:
    begin=c.position
    assert c.long()==5 and c.short()==1
    assert c.long()==0 and c.long()==expected_merge
    assert color(c)==0xc8000000 and c.long()==1
    assert [c.long(),c.long(),c.long(),c.long()]==[0,0,4,0]
    assert tv(c)=='' and bd(c)==0 and bd(c)==1 and c.long()==expected_align
    assert color(c)==0xc1000000
    cell_handle(5,0x11);assert bd(c)==expected_height
    assert c.short()==1 and [bd(c) for _ in range(6)]==[1.5]*6
    assert c.long()==6
    for edge in [1,2,4,8,16,32]:
        assert c.long()==edge and c.long()==0 and c.long()==1
        assert color(c)==0xc1000000 and c.long()==-2 and c.long()==0 and bd(c)==0.045
        cell_handle(5,0x14)
    assert c.long()==expected_id and c.long()==expected_class and tv(c)==expected_name
    styles.append({'id':expected_id,'class':expected_class,'name':expected_name,'merge':expected_merge,
                   'alignment':expected_align,'height':expected_height,'start':begin,'end':c.position})
assert c.position==5287 and h.position==5655 and h.bits(1)==0
assert [(s['start'],s['end']) for s in styles]==[(89,1803),(1803,3557),(3557,5287)]

maps=[o for o in objects if o.get('object')=='CELLSTYLEMAP' and o.get('handle',[])[-1:]==[0xdd]]
assert len(maps)==1 and maps[0]['ownerhandle'][-1]==0xdb and len(maps[0]['cells'])==3
for s,p in zip(styles,maps[0]['cells']):
    assert (p['id'],p['type'],p['name'])==(s['id'],s['class'],s['name'])
    assert (p['cellstyle.type'],p['cellstyle.data_flags'],p['cellstyle.property_override_flags'],
            p['cellstyle.merge_flags'],p['cellstyle.bg_color']['rgb'],p['cellstyle.content_layout'])==(5,1,0,s['merge'],'c8000000',1)
    pre='cellstyle.content_format.'
    assert [p[pre+k] for k in ['property_override_flags','property_flags','value_data_type','value_unit_type','value_format_string','rotation','block_scale','cell_alignment']]==[0,0,4,0,'',0,1,s['alignment']]
    assert p[pre+'content_color']['rgb']=='c1000000' and p[pre+'text_style'][-1]==0x11 and p[pre+'text_height']==s['height']
    assert [p['cellstyle.'+k] for k in ['vert_margin','horiz_margin','bottom_margin','right_margin','margin_horiz_spacing','margin_vert_spacing']]==[1.5]*6
    assert p['cellstyle.num_borders']==len(p['cellstyle.borders'])==6
    for edge,bor in zip([1,2,4,8,16,32],p['cellstyle.borders']):
        assert [bor[k] for k in ['index_mask','border_overrides','border_type','linewt','visible','double_line_spacing']]==[edge,0,1,-2,0,0.045]
        assert bor['color']['rgb']=='c1000000' and bor['ltype'][-1]==0x14

# DXF matching is by stable style ID, not physical order. Values 0 and 257
# are observed output values; this does not establish general CMC semantics.
rec=record(dx,'CELLSTYLEMAP','12D')
starts=[i for i,x in enumerate(rec) if x[1]=='TABLEFORMAT_BEGIN'];assert len(starts)==3
def block(seq,start,end):
    i=next(i for i,x in enumerate(seq) if x[1]==start)
    j=next(j for j in range(i+1,len(seq)) if seq[j][1]==end)
    return seq[i+1:j]
def norm(items):
    out=[]
    for k,v in items:
        if k in {'40','140','144'}:out.append((int(k),float(v)))
        elif k in {'90','91','92','93','94','95','170','171','62'}:out.append((int(k),int(v)))
        elif k=='340':out.append((340,int(v,16)))
        else:out.append((int(k),v))
    return out
byid={s['id']:s for s in styles};order=[]
for ix,start in enumerate(starts):
    part=rec[start:starts[ix+1] if ix+1<len(starts) else len(rec)]
    data=block(part,'TABLEFORMAT_BEGIN','TABLEFORMAT_END');ident=norm(block(part,'CELLSTYLE_BEGIN','CELLSTYLE_END'))
    ident_id=ident[0][1];s=byid[ident_id];order.append(ident_id)
    assert ident==[(90,ident_id),(91,s['class']),(300,s['name'])]
    assert norm(data[:6])==[(90,5),(170,1),(91,0),(92,s['merge']),(62,257),(93,1)]
    assert norm(block(data,'CONTENTFORMAT_BEGIN','CONTENTFORMAT_END'))==[(90,0),(91,0),(92,4),(93,0),(300,''),(40,0.0),(140,1.0),(94,s['alignment']),(62,0),(340,0x11),(144,s['height'])]
    assert norm(block(data,'CELLMARGIN_BEGIN','CELLMARGIN_END'))==[(40,1.5)]*6
    pos=next(i for i,x in enumerate(data) if x[1]=='CELLMARGIN_END')+1
    assert norm(data[pos:pos+1])==[(94,6)];pos+=1
    for edge in [1,2,4,8,16,32]:
        assert norm([data[pos]])==[(95,edge)];pos+=1
        assert norm(block(data[pos:],'GRIDFORMAT_BEGIN','GRIDFORMAT_END'))==[(90,0),(91,1),(62,0),(92,-2),(340,0x14),(93,0),(40,0.045)]
        pos+=next(i for i,x in enumerate(data[pos:]) if x[1]=='GRIDFORMAT_END')+1
    assert pos==len(data)
assert order==[1,2,3]

# TABLESTYLE body: legacy row style layout under ODA 20.4.101.1.
c=common(b[0x6b],505,1920,0x6b,2,0)
assert c.position==79 and tv(c)=='Standard' and c.short()==0 and c.short()==0
assert bd(c)==1.5 and bd(c)==1.5 and c.bits(1)==0 and c.bits(1)==0
for height,align in [(4.5,2),(6.0,5),(4.5,5)]:
    assert bd(c)==height and c.short()==align and color(c)==0xc1000000
    assert color(c)==0xc8000000 and c.bits(1)==0
    for _ in range(6):
        assert c.short()==65534 and c.bits(1)==1 and color(c)==0xc1000000
assert c.position==1920
styleTail=tail(b[0x6b],1920,[(4,0x6a),(4,0x6a),(4,0x9c),(3,0xdb),(5,0x11),(5,0x11),(5,0x11)])
peer_styles=[o for o in objects if o.get('object')=='TABLESTYLE' and o.get('handle',[])[-1:]==[0x6b]]
assert len(peer_styles)==1
ps=peer_styles[0]
assert ps['name']=='Standard' and ps['ownerhandle'][-1]==0x6a and ps['xdicobjhandle'][-1]==0xdb
assert [x[-1] for x in ps['reactors']]==[0x6a,0x9c]
assert [ps[k] for k in ['flow_direction','flags','horiz_cell_margin','vert_cell_margin','is_title_suppressed','is_header_suppressed']]==[0,0,1.5,1.5,0,0]
assert len(ps['rowstyles'])==3
for row,(height,align) in zip(ps['rowstyles'],[(4.5,2),(6.,5),(4.5,5)]):
    assert row['text_style'][-1]==0x11 and row['text_height']==height and row['text_alignment']==align
    assert row['text_color']['rgb']=='c1000000' and row['fill_color']['rgb']=='c8000000' and row['has_bgcolor']==0
    assert len(row['borders'])==6
    for border in row['borders']:
        assert border['linewt']==-2 and border['visible']==1 and border['color']['rgb']=='c1000000'
print(json.dumps({'result':'pass','dictionary_source_order':['ACAD_XREC_ROUNDTRIP','ACAD_ROUNDTRIP_2008_TABLESTYLE_CELLSTYLEMAP'],
                  'dictionary_oda_dxf_order':['ACAD_ROUNDTRIP_2008_TABLESTYLE_CELLSTYLEMAP','ACAD_XREC_ROUNDTRIP'],
                  'cell_style_source_order':[3,1,2],'cell_style_oda_dxf_order':order,
                  'cell_style_source_bit_spans':styles,'table_style_tail':styleTail,
                  'limitations':'pinned producer profile; raw callback provenance and CRC require separate source read; ODA DXF same-vendor; no support claim'},indent=2))
