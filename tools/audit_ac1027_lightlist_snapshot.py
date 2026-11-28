#!/usr/bin/env python3
"""Audit the exact selected LIGHTLIST record, without claiming whole-graph output."""
import argparse
import hashlib
from pathlib import Path


def require(value, message):
    if not value:
        raise ValueError(message)


def digest(path):
    require(path.is_file() and path.stat().st_size < 64 * 1024 * 1024, 'missing or oversized artifact')
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    import ezdxf
    from ezdxf.lldxf.tagger import ascii_tags_loader, binary_tags_loader
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('source','selected','ascii','binary'):
        p.add_argument('--'+name,type=Path,required=True)
        p.add_argument('--'+name+'-sha256',required=True)
    args=p.parse_args()
    require(ezdxf.__version__=='1.4.5','unexpected ezdxf version')
    expected=[(0,'LIGHTLIST'),(5,'F000'),(102,'{ACAD_REACTORS'),(330,'30'),
              (102,'}'),(330,'30'),(100,'AcDbLightList'),(90,'1'),(90,'1'),(5,'2F'),(1,'LOCAL_LIGHT')]
    for name in ('source','selected','ascii','binary'):
        path=getattr(args,name).resolve(strict=True); sha=getattr(args,name+'_sha256')
        require(digest(path)==sha,name+': hash mismatch')
        raw=path.read_bytes()
        if raw.startswith(b'AutoCAD Binary DXF'):
            tags=list(binary_tags_loader(raw))
        else:
            with path.open() as stream:tags=list(ascii_tags_loader(stream))
        starts=[i for i,t in enumerate(tags) if t.code==0 and t.value=='LIGHTLIST']
        require(len(starts)==1,name+': LIGHTLIST inventory')
        i=starts[0];j=next((j for j in range(i+1,len(tags)) if tags[j].code==0),len(tags))
        record=[(t.code,str(t.value).strip()) for t in tags[i:j]]
        require(record==expected,name+': selected record differs')
        require(digest(path)==sha,name+': artifact changed')
    print('PASS: ODA source, selected and ASCII/Binary LIGHTLIST tag records match')


if __name__=='__main__':
    main()
