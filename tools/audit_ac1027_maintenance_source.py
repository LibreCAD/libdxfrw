#!/usr/bin/env python3
"""Check one hash-bound AutoCAD 2013 source profile and selected ODA readback.

The source-to-DXF converter join is an externally observed step, not inferred
from hashes. No source fixture is included or required by the unit tests.
"""
import argparse
import hashlib
import json
import math
import re
import tempfile
from pathlib import Path
from bounded_process import run_bounded


def require(value,message):
    if not value: raise ValueError(message)


def digest(path):
    require(path.is_file() and path.stat().st_size <= 128*1024*1024,'input missing or oversized')
    return hashlib.sha256(path.read_bytes()).hexdigest()


def source_metadata(data):
    require(len(data)>18 and data[:6]==b'AC1027','source revision')
    require(tuple(data[i] for i in (11,12,17,18))==(8,3,31,8),'source metadata differs')


def source_aux_trace(trace):
    pos=trace.find('AuxHeader (123)');require(pos>=0,'source AuxHeader missing')
    section=trace[pos:pos+1600]
    for pattern in (r'(?m)^dwg_version: 0x1f \[RSx',r'(?m)^maint_version: 0x8 \[RSx',
                    r'(?m)^dwg_version_1: 0x16 \[RSx',r'(?m)^maint_version_1: 0x2e \[RSx',
                    r'(?m)^dwg_version_2: 0x16 \[RSx',r'(?m)^maint_version_2: 0x2e \[RSx'):
        require(re.search(pattern,section) is not None,'source AuxHeader field differs')


def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('source','dxf','dwgread'):
        p.add_argument('--'+name,type=Path,required=True);p.add_argument('--'+name+'-sha256',required=True)
    a=p.parse_args();names=('source','dxf','dwgread')
    for name in names:require(digest(getattr(a,name))==getattr(a,name+'_sha256').lower(),name+' hash')
    source_metadata(a.source.read_bytes());version=run_bounded([str(a.dwgread),'--version'],5,4096)
    require(version.returncode==0 and version.stdout.strip()==b'dwgread 0.14','reader version')
    with tempfile.TemporaryDirectory(prefix='ac1027-maint-source-') as temp:
        target=Path(temp)/'decoded.json';run=run_bounded([str(a.dwgread),'-v9','-O','JSON','-o',str(target),str(a.source)],30,8*1024*1024)
        require(run.returncode==0 and target.is_file(),'source DWG decode')
        trace=(run.stdout+run.stderr).decode('utf-8','replace');source_aux_trace(trace)
        require(re.search(r'(?m)^crc32 => 0x[0-9a-fA-F]+ \(verified\)$', trace) is not None,
                'source file-header CRC not observed')
        doc=json.loads(target.read_text());info=doc.get('AppInfo',{})
        require(doc['FILEHEADER']['version']=='AC1027' and info.get('version')=='19.0.55.0.0'
                and 'AutoCAD' in info.get('product_info',''),'unexpected source AppInfo')
        lines=[x for x in doc['OBJECTS'] if x.get('entity')=='LINE' and x.get('entmode')==2]
        require(len(lines)==1 and lines[0].get('handle',[0,0,0])[2]==0x1ca,'source LINE inventory')
        native=lines[0]
    import ezdxf
    require(ezdxf.__version__=='1.4.5','ezdxf version')
    output=ezdxf.readfile(a.dxf);audit=output.audit();model=list(output.modelspace())
    require(output.dxfversion=='AC1027' and not audit.errors and not audit.fixes
            and len(model)==1 and model[0].dxftype()=='LINE'
            and int(model[0].dxf.handle,16)==native['handle'][2],'source DXF inventory/audit')
    for output_point,native_point in ((model[0].dxf.start,native['start']),(model[0].dxf.end,native['end'])):
        require(all(math.isclose(output_point[i],native_point[i],rel_tol=1e-12,abs_tol=1e-12)
                    for i in range(3)),'source LINE coordinates')
    for name in names:require(digest(getattr(a,name))==getattr(a,name+'_sha256').lower(),name+' changed')
    print('source metadata/AuxHeader/AppInfo and selected ODA LINE: PASS; conversion lineage external')

if __name__=='__main__':main()
