#!/usr/bin/env python3
"""Hash-bound peer decode of the four R2013/R2018 HEADER defaults."""
import argparse
import hashlib
import json
import tempfile
from pathlib import Path
from bounded_process import run_bounded


def require(ok, message):
    if not ok:
        raise ValueError(message)


def digest(path):
    require(path.is_file() and path.stat().st_size <= 64 * 1024 * 1024,
            'missing or oversized DWG')
    return hashlib.sha256(path.read_bytes()).hexdigest()


def verify_document(doc, revision):
    require(doc.get('FILEHEADER', {}).get('version') == revision, 'revision')
    header=doc['HEADER']
    require(header.get('REQUIREDVERSIONS') == 0, 'required versions')
    require([header.get(k) for k in ('unit1_ratio','unit2_ratio','unit3_ratio','unit4_ratio')]
            == [412148564080.0, 1.0, 1.0, 1.0], 'opening HEADER defaults')
    handles=[x['handle'][2] for x in doc['OBJECTS']
             if isinstance(x.get('handle'),list) and len(x['handle'])>=3]
    seed=header.get('HANDSEED')
    require(handles and isinstance(seed,list) and len(seed)>=3
            and seed[2]>max(handles), 'HANDSEED does not cover emitted identities')
    lines=[x for x in doc['OBJECTS'] if x.get('entity')=='LINE']
    require(len(lines)==1 and lines[0].get('start')==[1.,2.,3.]
            and lines[0].get('end')==[4.,5.,6.], 'LINE geometry')


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--dwg',type=Path,required=True);p.add_argument('--dwg-sha256',required=True)
    p.add_argument('--version',choices=('AC1027','AC1032'),required=True)
    p.add_argument('--dwgread',type=Path,required=True)
    a=p.parse_args();dwg=a.dwg.resolve(strict=True);reader=a.dwgread.resolve(strict=True)
    v=run_bounded([str(reader),'--version'],5,4096)
    require(v.returncode==0 and v.stdout.strip()==b'dwgread 0.14','requires LibreDWG 0.14')
    require(digest(dwg)==a.dwg_sha256,'DWG hash mismatch')
    with tempfile.TemporaryDirectory(prefix='dwg-modern-header-') as tmp:
        out=Path(tmp)/'decoded.json'; result=run_bounded([str(reader),'-O','JSON','-o',str(out),str(dwg)],30,1024*1024)
        require(result.returncode==0 and out.is_file() and out.stat().st_size<=128*1024*1024,'decode failed')
        verify_document(json.loads(out.read_bytes()),a.version)
    require(digest(dwg)==a.dwg_sha256,'DWG changed')
    print(a.version,'HEADER defaults and LINE peer decode PASS')


if __name__=='__main__':main()
