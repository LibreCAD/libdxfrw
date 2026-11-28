#!/usr/bin/env python3
"""Audit source-bound SECTIONOBJECT count reordering and typed output."""
import argparse,hashlib,io
from pathlib import Path
import ezdxf
from ezdxf.lldxf.tagger import ascii_tags_loader,binary_tags_loader

def need(x,msg):
 if not x:raise ValueError(msg)
def load(path,expected):
 data=path.read_bytes();need(len(data)<64*1024*1024 and hashlib.sha256(data).hexdigest()==expected,'hash/size '+str(path));return data
def recs(data):
 tags=list(binary_tags_loader(data)) if data.startswith(b'AutoCAD Binary DXF') else list(ascii_tags_loader(io.StringIO(data.decode('ascii'))));out=[]
 for i,t in enumerate(tags):
  if t.code==0 and str(t.value).strip()=='SECTIONOBJECT':
   j=next((n for n in range(i+1,len(tags)) if tags[n].code==0),len(tags));out.append(tags[i:j])
 return out
def geometry(data):
 r=recs(data);need(len(r)==1,'section count');v=r[0];result=[]
 for count,codes in [(92,(11,21,31)),(93,(12,22,32))]:
  cs=[int(x.value) for x in v if x.code==count];need(len(cs)==1,'count field');groups=[[float(x.value) for x in v if x.code==c] for c in codes];need(all(len(g)==cs[0] for g in groups),'component count');result.append(list(zip(*groups)))
 return result
def main():
 p=argparse.ArgumentParser(description=__doc__)
 for n in ('source','reordered','projection','ascii','binary','authored','late_input','noz_input','nocount_input','oda_late','oda_noz','oda_nocount'):
  k=n.replace('_','-');p.add_argument('--'+k,type=Path,required=True);p.add_argument('--'+k+'-sha256',required=True)
 a=p.parse_args();need(ezdxf.__version__=='1.4.5','ezdxf version');data={}
 for n in ('source','reordered','projection','ascii','binary','authored','late_input','noz_input','nocount_input','oda_late','oda_noz','oda_nocount'):
  path=getattr(a,n).resolve(strict=True);sha=getattr(a,n+'_sha256');data[n]=load(path,sha)
  if n!='projection':
   doc=ezdxf.readfile(path);audit=doc.audit();need(not audit.errors and not audit.fixes,n+' audit')
  need(load(path,sha)==data[n],n+' changed')
 expected=[[(0.,0.,0.),(4.,0.,0.)],[(0.,1.,0.),(4.,1.,0.)]]
 for n in ('source','reordered','projection','ascii','binary','oda_noz'):need(geometry(data[n])==expected,n+' vertices')
 need(geometry(data['oda_late'])==[[],[]],'ODA late-count observation');need(geometry(data['oda_nocount'])==[[],[]],'ODA absent-count observation')
 def pairs(x):
  l=x.decode('ascii').splitlines();need(len(l)%2==0,'odd pairs');return list(zip(l[::2],l[1::2]))
 original=pairs(data['source']);changed=pairs(data['reordered']);matches=[]
 for i,(c,v) in enumerate(original):
  if c.strip()=='0' and v.strip()=='SECTIONOBJECT':matches.append((i,next((n for n in range(i+1,len(original)) if original[n][0].strip()=='0'),len(original))))
 need(len(matches)==1,'source section');i,j=matches[0];record=original[i:j]
 for count,component in [('92','31'),('93','32')]:
  ids=[k for k,(c,v) in enumerate(record) if c.strip()==count];need(len(ids)==1,'source count');item=record.pop(ids[0]);positions=[k for k,(c,v) in enumerate(record) if c.strip()==component];need(len(positions)==2,'source component');record.insert(positions[-1]+1,item)
 original[i:j]=record;need(original==changed,'extra reordered group mutation')
 authored=pairs(data['authored']);late=pairs(data['late_input']);noz=pairs(data['noz_input']);matches=[]
 for i,(c,v) in enumerate(authored):
  if c.strip()=='0' and v.strip()=='SECTIONOBJECT':matches.append((i,next((n for n in range(i+1,len(authored)) if authored[n][0].strip()=='0'),len(authored))))
 need(len(matches)==1,'authored section');i,j=matches[0];record=authored[i:j];lateRecord=list(record)
 for count,component in [('92','31'),('93','32')]:
  idx=[k for k,(c,v) in enumerate(lateRecord) if c.strip()==count];need(len(idx)==1,'authored count');item=lateRecord.pop(idx[0]);positions=[k for k,(c,v) in enumerate(lateRecord) if c.strip()==component];need(len(positions)==2,'authored component');lateRecord.insert(positions[-1]+1,item)
 need(authored[:i]+lateRecord+authored[j:]==late,'late control differs')
 body=next(k for k,(c,v) in enumerate(record) if c.strip()=='100' and v.strip()=='AcDbSection');nozRecord=record[:body]+[x for x in record[body:] if x[0].strip() not in ('31','32')]
 need(authored[:i]+nozRecord+authored[j:]==noz,'omitted-Z control differs')
 nocount=pairs(data['nocount_input']);countless=record[:body]+[x for x in record[body:] if x[0].strip() not in ('92','93')];need(authored[:i]+countless+authored[j:]==nocount,'absent-count control differs')
 projected=recs(data['projection']);selected=recs(data['reordered']);need(len(projected)==len(selected)==1 and [(x.code,str(x.value).strip()) for x in projected[0]]==[(x.code,str(x.value).strip()) for x in selected[0]],'selected source record mismatch')
 print('PASS: exact producer-record count movement, native ASCII/Binary geometry and bounded ODA controls')
if __name__=='__main__':main()
