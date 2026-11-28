#!/usr/bin/env python3
"""Hash-bound audit of a local AC1032 frame-one raster producer witness."""
import argparse
import hashlib
from pathlib import Path
from bounded_process import run_bounded

EXPECTED = {
    'input_dxf': 'ebb8fd284c6180bf75d5d1383667ba0582beda9bf194ed95bb677213654cf5e7',
    'dwg': '416a7c99d9ae13ec5e5c289fd8fd9fe895262c89fde292fa4e8a2a216abe83a7',
    'output_dxf': 'bff9585b16bb445e84f160b3fa47079c50413a10362d05ce9367ea1bac7592e3',
}
def require(ok, message):
    if not ok: raise ValueError(message)
def digest(path):
    require(path.is_file() and path.stat().st_size < 64*1024*1024, 'file absent or too large')
    return hashlib.sha256(path.read_bytes()).hexdigest()
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('input_dxf','dwg','output_dxf','probe','dwgread'):
        parser.add_argument('--'+name.replace('_','-'),type=Path,required=True)
    args=parser.parse_args(); paths={k:getattr(args,k).resolve(strict=True) for k in EXPECTED}
    for name,path in paths.items(): require(digest(path)==EXPECTED[name],name+' hash mismatch')
    import ezdxf
    require(ezdxf.__version__=='1.4.5','ezdxf version')
    for name,version in (('input_dxf','AC1024'),('output_dxf','AC1032')):
        doc=ezdxf.readfile(paths[name]); record=doc.rootdict.get('ACAD_IMAGE_VARS')
        require(doc.dxfversion==version and record is not None and
                (record.dxf.frame,record.dxf.quality,record.dxf.units)==(1,1,3),name+' raster')
    reader=args.dwgread.resolve(strict=True)
    ver=run_bounded([str(reader),'--version'],5,4096)
    require(ver.returncode==0 and ver.stdout.strip()==b'dwgread 0.14','LibreDWG version')
    trace=run_bounded([str(reader),'-v9',str(paths['dwg'])],30,8*1024*1024)
    require(trace.returncode==0,'LibreDWG read')
    text=(trace.stdout+trace.stderr).decode('utf8','replace')
    a=text.find('Decode object RASTERVARIABLES');b=text.find('< Next object:',a)
    require(a>=0 and b>a,'raster trace')
    span=text[a:b]
    for piece in ('bitsize: 73','handle: 0.1.2F','num_reactors: 1','is_xdic_missing: 1','has_ds_data: 0',
                  'ownerhandle: (4.1.A)','reactors[0]: (4.1.A)','class_version: 0',
                  'image_frame: 1','image_quality: 1','units: 3','crc: 9262','9262 == 9262'):
        require(piece in span,'missing peer observation: '+piece)
    result=run_bounded([str(args.probe.resolve(strict=True)),str(paths['dwg'])],30,1024*1024)
    require(result.returncode==0 and b'AC1032 frame=1 RASTERVARIABLES source proof: PASS' in result.stdout,'local probe')
    for name,path in paths.items():require(digest(path)==EXPECTED[name],name+' changed')
    print('AC1032 physical frame-one RASTERVARIABLES source audit PASS')
if __name__=='__main__':main()
