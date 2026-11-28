#!/usr/bin/env python3
"""Walk one pinned AC1018 TABLECONTENT body and cross-check ODA DXF.

The ODA-created DWG and ODA conversion are from the same vendor. Raw-callback
capture is a provenance boundary, and this is not an output-support test.
"""
import argparse, json, struct
from pathlib import Path
from p5_ac1018_table_xrecord_source_verify import Cursor
from p5_ac1018_table_descendants_verify import SOURCE, PEER, DXF, HASHES, load, body_start

if not __debug__:
    raise RuntimeError('Do not run this verifier with Python optimizations')
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--source',type=Path,required=True)
parser.add_argument('--peer-json',type=Path,required=True)
parser.add_argument('--dxf',type=Path,required=True)
parser.add_argument('--body-dir',type=Path,required=True)
args=parser.parse_args()
load(args.source,SOURCE); load(args.peer_json,PEER); load(args.dxf,DXF)
b=load(args.body_dir/'p5-desc-body-185.bin',HASHES[0xb9])
c=body_start(b,514,0xb9,13313);h=Cursor(b);h.position=13313
text_fields=[]; handle_fields=[]

def handle(n):
 x=h.handle()
 if x['role'] != (4 if n=='parent' else 5):raise ValueError((n,x))
 handle_fields.append({'field':n,**x})
 return x

def bd():
 t=c.bits(2)
 if t==0:return struct.unpack('<d',c.raw(8))[0]
 if t==1:return 1.
 if t==2:return 0.
 raise ValueError(('BD',c.position))
def tv():
 start=c.position;n=c.short()
 if n>1000:raise ValueError(('TV count',n,c.position))
 raw=c.raw(n);text_fields.append({'startBit':start,'endBit':c.position,'bytesHex':raw.hex()})
 return raw.decode('ascii').rstrip('\x00')
def cmc():
 idx=c.short(); rgb=c.long();flags=c.bits(8)
 if flags&1:tv()
 if flags&2:tv()
 return {'index':idx,'rgb':hex(rgb & 0xffffffff),'flags':flags}
def fmt():
 start=c.position;v=[c.long(),c.long(),c.long(),c.long(),tv(),bd(),bd(),c.long(),cmc()];hr=handle('textstyle');v.append(bd());return {'start':start,'end':c.position,'properties':v,'textstyle':hr}
def style():
 start=c.position;t=c.long();flags=c.short();v={'start':start,'type':t,'flag':flags}
 if flags:
  if flags!=1:raise ValueError(('styleflag',flags,c.position))
  v.update({'override':c.long(),'merge':c.long(),'background':cmc(),'layout':c.long(),'format':fmt()})
  m=c.short()
  if m:raise ValueError(('unexpected margins',m,c.position))
  v['margins']=[]
  n=c.long();v['border_count']=n
  if n>6:raise ValueError(('borders',n,c.position))
  borders=[]
  for i in range(n):
   pos=c.position;edge=c.long()
   if edge:
    z=[c.long(),c.long(),cmc(),c.long(),c.long(),bd()]; ref=handle('border-linetype')
   else:z=[];ref=None
   borders.append({'start':pos,'end':c.position,'edge':edge,'fields':z,'lineType':ref})
  v['borders']=borders
 v['end']=c.position;return v

def value():
 t=c.long()
 if t==0:v=c.long()
 elif t==1:v=c.long()
 elif t==2:v=bd()
 elif t==4:v=tv()
 else:raise ValueError(('value type',t,c.position))
 return [t,v]
def customs():
 n=c.long()
 if n>30:raise ValueError(('custom count',n,c.position))
 return [[tv(),value()] for _ in range(n)]
def content():
 t=c.long()
 if t!=1:raise ValueError(('content type',t,c.position))
 v=value();n=c.long()
 if n:raise ValueError(('attrs',n,c.position))
 present=c.short()
 if present not in (0,1):raise ValueError(('format presence',present,c.position))
 f=fmt() if present else None
 return {'type':t,'value':v,'format':f}
def cell():
 start=c.position;flags=c.long();tooltip=tv();customData=c.long();items=customs();linked=c.long()
 if linked:raise ValueError(('linked',linked,c.position))
 n=c.long()
 if n>5:raise ValueError(('content count',n,c.position))
 v=[content() for _ in range(n)];s=style();sid=c.long();unknown=c.long()
 if unknown:raise ValueError(('unknown',unknown,c.position))
 return {'start':start,'end':c.position,'flags':flags,'tooltip':tooltip,'customData':customData,'customs':items,'contents':v,'style':s,'styleId':sid}
owner=handle('parent');title=tv();descr=tv();ncols=c.long();assert (title,descr,ncols)==('','',3) and owner['value']==0xbc
cols=[]
for i in range(ncols):
 start=c.position;nm=tv();data=c.long();items=customs();s=style();sid=c.long();width=bd();cols.append({'start':start,'end':c.position,'name':nm,'customData':data,'items':items,'style':s,'styleId':sid,'width':width})
nrows=c.long(); rows=[]
for i in range(nrows):
 start=c.position;n=c.long();cells=[cell() for j in range(n)];data=c.long();items=customs();s=style();sid=c.long();height=bd();rows.append({'start':start,'end':c.position,'cells':cells,'customData':data,'items':items,'style':s,'styleId':sid,'height':height})
fieldrefs=c.long();assert fieldrefs==0 and c.position==13039
for i in range(fieldrefs):handle('fieldref')
overall=style();merged=c.long();assert merged==1 and c.position==13297
ranges=[[c.long(),c.long(),c.long(),c.long()] for _ in range(merged)]
stylehandle=handle('tablestyle')
assert ranges==[[0,0,0,2]] and c.position==13313 and h.position==13745
assert stylehandle['value']==0x6b and len(handle_fields)==52
assert (owner['size'],stylehandle['size'])==(1,1)
assert all((x['size'],x['value'])==(0,0) for x in handle_fields[1:-1])
assert sum(x['field']=='textstyle' for x in handle_fields)==17
assert sum(x['field']=='border-linetype' for x in handle_fields)==33
assert len(b)*8-h.position==7 and h.bits(7)==0
p={'columns':cols,'rows':rows,'overallStyle':overall,'styleHandle':stylehandle,'end':c.position,'hend':13745}
assert [(x['start'],x['end'],x['width']) for x in cols]==[(85,171,152.),(171,257,152.),(257,343,152.)]
assert [(x['start'],x['end'],x['height']) for x in rows]==[(353,3485,11.),(3485,8261,9.),(8261,13037,9.)]


lines=args.dxf.read_text().splitlines();pairs=[(x.strip(),y.strip()) for x,y in zip(lines[::2],lines[1::2])];on=False;rec=[]
for k,v in pairs:
 if k=='0':
  if on:break
  on=v=='TABLECONTENT'
 if on:rec.append((k,v))

def block(seq,begin,end,unique=True):
 starts=[i for i,x in enumerate(seq) if x[1]==begin]
 assert starts and (not unique or len(starts)==1),(begin,len(starts))
 i=starts[0]; js=[j for j in range(i+1,len(seq)) if seq[j][1]==end];assert js,(begin,end)
 return seq[i+1:js[0]]
def norm(data):
 out=[]
 for k,v in data:
  if k in {'40','140','144'}:out.append((int(k),float(v)))
  elif k in {'90','91','92','93','94','95','170','171','62'}:out.append((int(k),int(v)))
  elif k=='340':out.append((340,int(v,16)))
  else:out.append((int(k),v))
 return out
def color(raw):
 assert raw['flags']==0 and raw['index']==0
 return {'0xc1000000':0,'0xc8000000':257}[raw['rgb']]
def fmt(raw,seq):
 d=norm(block(seq,'CONTENTFORMAT_BEGIN','CONTENTFORMAT_END'));v=raw['properties']
 expected=[(90,v[0]),(91,v[1]),(92,v[2]),(93,v[3]),(300,v[4]),(40,v[5]),(140,v[6]),(94,v[7]),(62,color(v[8])),(340,raw['textstyle']['value']),(144,v[9])]
 assert d==expected,(d,expected)
def verify_cell(src,dx):
 linked=block(dx,'LINKEDTABLEDATACELL_BEGIN','LINKEDTABLEDATACELL_END')
 assert linked[:3]==[('90',str(src['flags'])),('300',src['tooltip']),('91',str(src['customData']))]
 custom=block(linked,'DATAMAP_BEGIN','DATAMAP_END')
 actual=[('90',str(len(src['customs'])))]
 for key,(t,v) in src['customs']:
  actual += [('300',key),('301','DATAMAP_VALUE'),('90',str(t)),('140',str(v))]
 assert custom==actual,(custom,actual)
 count=next(i for i,x in enumerate(linked) if x[0]=='95')
 assert linked[count-1]==('92','0')
 assert int(linked[count][1])==len(src['contents'])
 if src['contents']:
  content=block(linked,'CELLCONTENT_BEGIN','CELLCONTENT_END');v=src['contents'][0]
  assert content==[('90',str(v['type'])),('300','VALUE'),('90',str(v['value'][0])),('1',v['value'][1]),('91','0')],content
  formatted=block(linked,'FORMATTEDCELLCONTENT_BEGIN','FORMATTEDCELLCONTENT_END')
  assert formatted[0]==('170','1')
  fmt(v['format'],formatted)
 sty=src['style'];outer=block(dx,'FORMATTEDTABLEDATACELL_BEGIN','FORMATTEDTABLEDATACELL_END'); s=block(outer,'TABLEFORMAT_BEGIN','TABLEFORMAT_END')
 expectedhead=[(90,sty['type']),(170,sty['flag']),(91,sty['override']),(92,sty['merge']),(62,color(sty['background'])),(93,sty['layout'])]
 assert norm(s[:6])==expectedhead,(s[:6],expectedhead)
 fmt(sty['format'],s)
 pos=next(i for i,x in enumerate(s) if x[1]=='CONTENTFORMAT_END')+1
 assert norm(s[pos:pos+2])==[(171,0),(94,sty['border_count'])]
 pos+=2
 for border in sty['borders']:
  assert norm([s[pos]])==[(95,border['edge'])];pos+=1
  g=block(s[pos:],'GRIDFORMAT_BEGIN','GRIDFORMAT_END',False);vals=border['fields']
  expected=[(90,vals[0]),(91,vals[1]),(62,color(vals[2])),(92,vals[3]),(340,border['lineType']['value']),(93,vals[4]),(40,vals[5])]
  assert norm(g)==expected,(norm(g),expected)
  pos+=next(i for i,x in enumerate(s[pos:]) if x[1]=='GRIDFORMAT_END')+1
 assert pos==len(s)
 cell=block(dx,'TABLECELL_BEGIN','TABLECELL_END');assert norm(cell)==[(90,src['styleId']),(91,0)]
starts=[i for i,x in enumerate(rec) if x[1]=='LINKEDTABLEDATACELL_BEGIN']
assert len(starts)==9
for index,start in enumerate(starts):
 end=next(i for i in range(start,len(rec)) if rec[i][1]=='TABLECELL_END')+1
 verify_cell(p['rows'][index//3]['cells'][index%3],rec[start:end])
# Column and row wrappers are compared separately from nested cell records.
def chunks(seq,begin,end):
    return [block(seq[i:],begin,end,False) for i,x in enumerate(seq) if x[1]==begin]
column_linked=chunks(rec,'LINKEDTABLEDATACOLUMN_BEGIN','LINKEDTABLEDATACOLUMN_END')
column_style=chunks(rec,'FORMATTEDTABLEDATACOLUMN_BEGIN','FORMATTEDTABLEDATACOLUMN_END')
column_geom=chunks(rec,'TABLECOLUMN_BEGIN','TABLECOLUMN_END')
assert len(column_linked)==len(column_style)==len(column_geom)==3
assert rec[:8]==[('0','TABLECONTENT'),('5','111'),('330','114'),('100','AcDbLinkedData'),('1',''),('300',''),('100','AcDbLinkedTableData'),('90','3')]
for col,linked,st,geom in zip(cols,column_linked,column_style,column_geom):
    assert linked[:2]==[('300',col['name']),('91',str(col['customData']))]
    assert block(linked,'DATAMAP_BEGIN','DATAMAP_END')==[('90','0')]
    assert norm(block(st,'TABLEFORMAT_BEGIN','TABLEFORMAT_END'))==[(90,col['style']['type']),(170,col['style']['flag'])]
    assert norm(geom)==[(90,col['styleId']),(40,col['width'])]
row_styles=chunks(rec,'FORMATTEDTABLEDATAROW_BEGIN','FORMATTEDTABLEDATAROW_END')
row_geom=chunks(rec,'TABLEROW_BEGIN','TABLEROW_END')
row_linked=chunks(rec,'LINKEDTABLEDATAROW_BEGIN','LINKEDTABLEDATAROW_END')
assert len(row_styles)==len(row_geom)==len(row_linked)==3
first_row=next(i for i,x in enumerate(rec) if x[1]=='LINKEDTABLEDATAROW_BEGIN')
assert rec[first_row-2]==('91','3')
for row,linked,st,geom in zip(rows,row_linked,row_styles,row_geom):
    assert not row['items'] and row['customData']==0
    assert linked[0]==('90','3') and sum(x[1]=='LINKEDTABLEDATACELL_BEGIN' for x in linked)==3
    assert linked[-5:]==[('91','0'),('301','CUSTOMDATA'),('1','DATAMAP_BEGIN'),('90','0'),('309','DATAMAP_END')]
    assert norm(block(st,'TABLEFORMAT_BEGIN','TABLEFORMAT_END'))==[(90,row['style']['type']),(170,row['style']['flag'])]
    assert norm(geom)==[(90,row['styleId']),(40,row['height'])]
# The final TABLEFORMAT belongs to the formatted-table-data object.
style_starts=[i for i,x in enumerate(rec) if x==('100','AcDbFormattedTableData')]
assert len(style_starts)==1
assert rec[style_starts[0]-1]==('92','0')
final=rec[style_starts[0]:]
overall_block=block(final,'TABLEFORMAT_BEGIN','TABLEFORMAT_END')
expected=[(90,overall['type']),(170,overall['flag']),(91,overall['override']),(92,overall['merge']),(62,color(overall['background'])),(93,overall['layout'])]
assert norm(overall_block[:6])==expected
fmt(overall['format'],overall_block)
pos=next(i for i,x in enumerate(overall_block) if x[1]=='CONTENTFORMAT_END')+1
assert norm(overall_block[pos:])==[(171,0),(94,overall['border_count'])]
end=next(i for i,x in enumerate(final) if x[1]=='TABLEFORMAT_END')+1
assert norm(final[end:])==[(90,1),(91,0),(92,0),(93,0),(94,2),(100,'AcDbTableContent'),(340,0x6b)]
print(json.dumps({'status':'pass','sourceSha256':SOURCE,'bodySha256':HASHES[0xb9],
 'dataBitEnd':c.position,'handleBitEnd':13745,'textFields':text_fields,
 'handles':handle_fields,'columns':cols,'rows':rows,'overallStyle':overall,'mergedRanges':ranges},indent=2))
