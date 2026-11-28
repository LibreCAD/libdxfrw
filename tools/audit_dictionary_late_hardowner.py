#!/usr/bin/env python3
"""Check an ODA-returned DICTIONARY and its controlled late-280 roundtrip."""
import argparse
import hashlib
from pathlib import Path
import ezdxf

def require(test, message):
    if not test:
        raise ValueError(message)

def check_hash(path, expected):
    require(hashlib.sha256(path.read_bytes()).hexdigest() == expected, f'hash mismatch: {path}')

def tags(path):
    text = path.read_text().splitlines()
    require(len(text) % 2 == 0, f'odd group lines: {path}')
    return [(text[i].strip(), text[i+1].strip()) for i in range(0, len(text), 2)]

def record_start(a):
    for i,(c,v) in enumerate(a):
        if c == '0' and v == 'DICTIONARY':
            j=i+1
            while j < len(a) and a[j][0] != '0':
                j+=1
            if ('5','78') in a[i:j]:
                return i,j
    raise ValueError('dictionary 78 missing')

def graph(path):
    doc=ezdxf.readfile(path)
    require(not doc.audit().errors, f'audit errors: {path}')
    rows=[o for o in doc.objects.query('DICTIONARY') if o.dxf.handle=='78' and 'ACAD_XREC_ROUNDTRIP' in o]
    require(len(rows)==1,'dictionary key multiplicity')
    obj=rows[0]; x=obj['ACAD_XREC_ROUNDTRIP']
    require(obj.dxf.handle=='78' and obj.dxf.owner=='70' and obj.dxf.hard_owned==1,'dictionary fields')
    require(x.dxftype()=='XRECORD' and x.dxf.owner=='78','xrecord ownership')
    require([(e.code,e.value) for e in x.tags]==[(102,'DISPLAYNAME'),(1,'Imperial24'),(102,'FLAGS'),(90,0)],'xrecord payload')
    return x.dxf.handle

def main():
    p=argparse.ArgumentParser()
    for name in ('source','edited','returned'):
        p.add_argument('--'+name,type=Path,required=True); p.add_argument('--'+name+'-sha256',required=True)
    a=p.parse_args()
    require(ezdxf.__version__=='1.4.5','expected ezdxf 1.4.5')
    for name in ('source','edited','returned'): check_hash(getattr(a,name),getattr(a,name+'_sha256'))
    src,edited=tags(a.source),tags(a.edited)
    i,j=record_start(src); row=src[i:j]
    require(row.count(('280','1'))==1 and row.count(('360','83'))==1,'unexpected source fields')
    row.remove(('280','1')); row.insert(row.index(('360','83'))+1,('280','1'))
    require(src[:i]+row+src[j:]==edited,'edited file differs beyond one group relocation')
    before=graph(a.source); moved=graph(a.edited); after=graph(a.returned)
    require(before==moved=='83' and after=='88','unexpected observed handles')
    print('PASS: exact 280 relocation, ODA roundtrip dictionary, XRECORD owner and payload')
if __name__=='__main__': main()
