#!/usr/bin/env python3
"""Hash-bound selected LAYER color-order audit; no whole-file graph claim."""
import argparse
import hashlib
from pathlib import Path


def require(value, message):
    if not value: raise ValueError(message)


def digest(path):
    require(path.is_file() and path.stat().st_size < 64*1024*1024, 'missing/oversized')
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    import ezdxf
    from ezdxf.lldxf.tagger import ascii_tags_loader, binary_tags_loader
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('source','producer','reordered','ascii','binary'):
        p.add_argument('--'+name,type=Path,required=True)
        p.add_argument('--'+name+'-sha256',required=True)
    for name in ('black_input','black_output'):
        p.add_argument('--'+name.replace('_','-'),type=Path)
        p.add_argument('--'+name.replace('_','-')+'-sha256')
    a=p.parse_args();require(ezdxf.__version__=='1.4.5','ezdxf version')
    black_args=(a.black_input,a.black_input_sha256,a.black_output,a.black_output_sha256)
    require(all(black_args) or not any(black_args),'black inputs require paths and hashes')
    records={}
    for name in ('source','producer','reordered','ascii','binary'):
        path=getattr(a,name).resolve(strict=True);sha=getattr(a,name+'_sha256')
        require(digest(path)==sha,name+': hash')
        data=path.read_bytes()
        if data.startswith(b'AutoCAD Binary DXF'):tags=list(binary_tags_loader(data))
        else:
            with path.open() as stream:tags=list(ascii_tags_loader(stream))
        rec=[]
        for i,t in enumerate(tags):
            if t.code!=0 or t.value!='LAYER':continue
            j=next((j for j in range(i+1,len(tags)) if tags[j].code==0),len(tags))
            row=tags[i:j]
            if any(t.code==2 and str(t.value).strip()=='COLORCHECK' for t in row):rec.append(row)
        require(len(rec)==1,name+': layer count')
        record=[(t.code,str(t.value).strip()) for t in rec[0]];records[name]=record
        aci=[int(v) for c,v in record if c==62];rgb=[int(v) for c,v in record if c==420]
        require(aci==[-148] and rgb==[1193046],name+': color fields')
        doc=ezdxf.readfile(path);layer=doc.layers.get('COLORCHECK');audit=doc.audit()
        require(layer.is_off() and layer.dxf.true_color==1193046,name+': typed color')
        require(not audit.errors and not audit.fixes,name+': audit')
        require(digest(path)==sha,name+': changed')
    require(records['source']==records['producer'],'producer projection differs')
    ordered=records['producer'];swapped=list(ordered)
    first=next(i for i,(c,_) in enumerate(swapped) if c==62)
    second=next(i for i,(c,_) in enumerate(swapped) if c==420)
    require(second==first+1,'unexpected source order')
    swapped[first],swapped[second]=swapped[second],swapped[first]
    require(swapped==records['reordered'],'reordered source changed more than field order')
    if all(black_args):
        for path,sha,aci in ((a.black_input,a.black_input_sha256,7),(a.black_output,a.black_output_sha256,18)):
            path=path.resolve(strict=True);require(digest(path)==sha,'black hash')
            doc=ezdxf.readfile(path);layer=doc.layers.get('BLACKCHECK');audit=doc.audit()
            require(layer.dxf.color==aci and layer.dxf.hasattr('true_color') and layer.dxf.true_color==0,'explicit black')
            require(not audit.errors and not audit.fixes,'black audit')
            require(digest(path)==sha,'black file changed')
        print('PASS: selected ODA black source and returned DXF retain explicit zero RGB')
    print('PASS: selected ODA layer, reordered projection and ASCII/Binary outputs preserve signed ACI and RGB')


if __name__=='__main__':main()
