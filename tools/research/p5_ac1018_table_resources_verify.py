#!/usr/bin/env python3
"""Walk pinned AC1018 STYLE/LTYPE/APPID resources and controls.

Body hashes are observations, not self-authenticating provenance or CRC proof.
The source was captured by a separate instrumented reader. Same-vendor DXF is
only producer self-consistency; no output or font semantics are certified.
"""
import argparse
import json
import struct
from pathlib import Path
from p5_ac1018_table_xrecord_source_verify import Cursor,require
from p5_ac1018_table_descendants_verify import SOURCE,PEER,DXF,load,records,record

HASHES={3:'058bff0a93b443957f9acadb4913cb6f1eaed3933ad1a3d21ae7b16939e863aa',
5:'e63d8d1ac76d14b69e7a729e1529dea47d234de71a7621457fa70a2bb1cd13dc',
9:'3d4d00b5fc0944207670d015a443b682d8b7bb3db62239c18a1729c660524ed6',
17:'c7cb2baebcbdd967cc4616950956f60d79858fffd2cb8ecbe782d90d094c4a63',
18:'163fe361c36c9fd050085fa8c354b33663f3a2b11cac772367e9e6aca798c6c1',
20:'c823f7cb044ebda1893574a97715d25698f9f9b41f7980cdcb50c9368b3fabdf',
21:'13bbf6c9821b3f9594b951ee1c8074c087bb424363be7ab734aeb7a557b483eb',
22:'216398bfdbfd64f18fbb8ef5af3a43b4610b77cf64943b56ebbc8214482ba8e7',
205:'c00225334eb43b6081e4543c4e61b8e7360c103c41cc1ff7250501680a5cf6e9',
215:'4180eaf906210c0bbe66d37ef565b785ac89e750b5d30378c8f2ae3c4e4195c5'}
if not __debug__:raise RuntimeError('run without optimization')
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source',type=Path,required=True);p.add_argument('--peer-json',type=Path,required=True);p.add_argument('--dxf',type=Path,required=True);p.add_argument('--body-dir',type=Path,required=True);p.add_argument('--self-test',action='store_true');a=p.parse_args()
load(a.source,SOURCE);peer=json.loads(load(a.peer_json,PEER));load(a.dxf,DXF)
body={k:load(a.body_dir/('p5-table-resource-%d.bin'%k),v) for k,v in HASHES.items()}
dx=records(a.dxf)

def common(data,kind,ident,end,eed=False):
 c=Cursor(data);assert c.short()==kind and int.from_bytes(c.raw(4),'little')==end
 h=c.handle();assert (h['role'],h['size'],h['value'])==(0,1,ident)
 if eed:
  assert c.short()==14;appid=c.handle();assert (appid['role'],appid['size'],appid['value'],appid['startBit'],appid['endBit'])==(5,1,18,68,84)
  assert c.raw(14)==bytes.fromhex('0005002d417269616c4700000000') and c.position==196
  assert c.short()==0
 else:assert c.short()==0
 assert c.long()==0 and c.bits(1)==1
 return c

def tail(c,expected):
 out=[c.handle() for _ in expected]
 assert [(x['role'],x['value']) for x in out]==expected
 rem=len(c.data)*8-c.position;assert rem<8 and c.bits(rem)==0
 return out

def text(c,expected):
 start=c.position;n=c.short();dataStart=c.position
 assert n==len(expected);assert c.raw(n)==expected.encode('ascii')
 return {'lengthStartBit':start,'dataStartBit':dataStart,'endBit':c.position,'value':expected}
def bd(c,expected):
 tag=c.bits(2)
 if tag==0:value=struct.unpack('<d',c.raw(8))[0]
 else:
  assert tag in (1,2);value=1.0 if tag==1 else 0.0
 assert value==expected

def find(ident,kind):
 found=[o for o in peer['OBJECTS'] if o.get('object')==kind and o.get('handle',[])[-1:]==[ident]]
 assert len(found)==1;return found[0]

def appid_row(ident,name,end):
 c=common(body[ident],67,ident,end)
 name_field=text(c,name)
 flag_start=c.position
 xref_ref=c.bits(1);xref_index_start=c.position;xref_index=c.short()
 dependency_start=c.position;dependency=c.bits(1)
 unknown_start=c.position;unknown=c.bits(8)
 flags=(xref_ref,xref_index,dependency,unknown)
 assert flags==(1,0,0,0) and c.position==end
 out=tail(c,[(4,9),(5,0)])
 row=find(ident,'APPID')
 assert row['name']==name and row['ownerhandle'][-1]==9
 assert (row['is_xref_ref'],row['is_xref_resolved'],row['is_xref_dep'],row['unknown'])==(1,0,0,0)
 return {'nameField':name_field,'xrefReferenceSpan':[flag_start,flag_start+1],
         'xrefIndexSpan':[xref_index_start,dependency_start],
         'dependencyFlagSpan':[dependency_start,dependency_start+1],
         'unknownRcSpan':[unknown_start,unknown_start+8],
         'bodyEndBit':end,'xrefFlags':flags,'tail':out,
         'paddingBits':len(body[ident])*8-out[-1]['endBit']}

control_result={}
for ident,kind,count,links in [(3,52,1,[(4,0),(2,17)]),(5,56,1,[(4,0),(2,22),(3,20),(3,21)]),(9,66,3,[(4,0),(2,18),(2,205),(2,215)])]:
 c=common(body[ident],kind,ident,73);assert c.position==63 and c.long()==count and c.position==73
 control_result[ident]=tail(c,links)
s=find(3,'STYLE_CONTROL');assert [x[-1] for x in s['entries']]==[17]
l=find(5,'LTYPE_CONTROL');assert [x[-1] for x in l['entries']]==[22] and l['byblock'][-1]==20 and l['bylayer'][-1]==21
app=find(9,'APPID_CONTROL');assert [x[-1] for x in app['entries']]==[18,205,215]

appid_sibling_results={
 205:appid_row(205,'ACAD_DSTYLE_DIMJOGGED_JOGANGLE_SYMBOL',381),
 215:appid_row(215,'ACAD_MLEADERVER',205),
}

c=common(body[17],53,17,445,True);assert c.position==201;text(c,'Standard')
assert (c.bits(1),c.short(),c.bits(1),c.bits(1),c.bits(1))==(1,0,0,0,0)
bd(c,0.0);bd(c,1.0);bd(c,0.0);assert c.bits(8)==0;bd(c,2.5);text(c,'arial.ttf');text(c,'');assert c.position==445
style_tail=tail(c,[(4,3),(5,0)])
s=find(17,'STYLE');assert s['name']=='Standard' and s['ownerhandle'][-1]==3 and s['xref'][-1]==0
assert (s['is_xref_ref'],s['is_xref_resolved'],s['is_xref_dep'],s['is_shape'],s['is_vertical'])==(1,0,0,0,0)
assert (s['text_size'],s['width_factor'],s['oblique_angle'],s['generation'],s['last_height'],s['font_file'],s['bigfont_file'])==(0.,1.,0.,0,2.5,'arial.ttf','')
assert s['eed']==[{'size':14,'handle':[5,1,18],'code':0,'value':'Arial'},{'code':71,'value':0}]

c=common(body[18],67,18,117);text(c,'ACAD');assert (c.bits(1),c.short(),c.bits(1),c.bits(8),c.position)==(1,0,0,0,117)
appid_tail=tail(c,[(4,9),(5,0)]);app=find(18,'APPID');assert app['name']=='ACAD' and app['ownerhandle'][-1]==9 and app['xref'][-1]==0 and app['unknown']==0

ltype_tails={}
for ident,name,desc,end in [(20,'ByBlock','',2201),(21,'ByLayer','',2201),(22,'Continuous','Solid line',2313)]:
 c=common(body[ident],57,ident,end);text(c,name);assert (c.bits(1),c.short(),c.bits(1))==(1,0,0);text(c,desc);bd(c,0.0)
 assert c.bits(8)==65 and c.bits(8)==0 and c.raw(256)==bytes(256) and c.position==end
 ltype_tails[ident]=tail(c,[(4,5),(5,0)])
 x=find(ident,'LTYPE');assert x['name']==name and x['ownerhandle'][-1]==5 and x['xref'][-1]==0
 assert (x['description'],x['pattern_len'],x['alignment'],x['numdashes'])==(desc,0.,65,0) and x['strings_area']=='00'*256

# ODA 2004 DXF uses the same handles for these records; record only its
# same-vendor projection, not independent rendering or codepage meaning.
style=record(dx,'STYLE','11');assert style[-3:]==[('1001','ACAD'),('1000','Arial'),('1071','0')]
assert [('2','Standard'),('70','0'),('40','0.0'),('41','1.0'),('50','0.0'),('71','0'),('42','2.5'),('3','arial.ttf'),('4','')]==style[5:14]
assert ('2','ACAD') in record(dx,'APPID','12')
for ident,name,desc in [('14','ByBlock',''),('15','ByLayer',''),('16','Continuous','Solid line')]:
 r=record(dx,'LTYPE',ident);assert [('2',name),('70','0'),('3',desc),('72','65'),('73','0'),('40','0.0')]==r[-6:]
if a.self_test:
 damaged=bytearray(body[17]);damaged[68//8]^=1<<(7-68%8)
 try:common(bytes(damaged),53,17,445,True)
 except (ValueError,AssertionError):pass
 else:raise ValueError('mutated APPID role accepted')
 damaged=bytearray(body[205]);damaged[381//8]^=1<<(7-381%8)
 try:tail(Cursor(damaged,381),[(4,9),(5,0)])
 except (ValueError,AssertionError):pass
 else:raise ValueError('mutated APPID_CONTROL owner role accepted')
print(json.dumps({'result':'pass','controls':control_result,'styleTail':style_tail,'appidTail':appid_tail,'ltypeTails':ltype_tails,
 'appidSiblings':appid_sibling_results,
 'styleEed':{'bytes':'0005002d417269616c4700000000','appid':18,'string':'Arial','code71':0,'codepageRaw':'002d'},
 'limitations':'caller body hashes do not attest origin or CRC; ODA DXF is same-vendor; font and full graph semantics remain open'},indent=2))
