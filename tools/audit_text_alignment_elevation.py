#!/usr/bin/env python3
"""Hash-bound selected TEXT field comparison; requires ezdxf and LibreDWG."""
import argparse
import hashlib
import json
import pathlib
import subprocess
import tempfile
import ezdxf
from ezdxf.math import OCS

def require(value,message):
    if not value: raise ValueError(message)

def checked(path,digest):
    data=path.read_bytes()
    require(hashlib.sha256(data).hexdigest()==digest,'SHA-256 mismatch: '+str(path))

def text_from_dxf(path):
    doc=ezdxf.readfile(path)
    model=list(doc.modelspace())
    require(len(model)==1 and model[0].dxftype()=='TEXT','wrong entity inventory')
    audit=doc.audit();require(not audit.errors and not audit.fixes,'ezdxf audit failure')
    return model[0]

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--input',type=pathlib.Path,required=True);p.add_argument('--input-sha',required=True)
    p.add_argument('--dwg',type=pathlib.Path,required=True);p.add_argument('--dwg-sha',required=True)
    p.add_argument('--output',type=pathlib.Path,required=True);p.add_argument('--output-sha',required=True)
    p.add_argument('--dwgread',type=pathlib.Path,required=True)
    p.add_argument('--mode',choices=['same','mismatch','tilted-same','tilted-mismatch','generated-loss'],required=True)
    a=p.parse_args()
    for path,digest in ((a.input,a.input_sha),(a.dwg,a.dwg_sha),(a.output,a.output_sha)): checked(path,digest)
    source=text_from_dxf(a.input);output=text_from_dxf(a.output)
    tilted=a.mode.startswith('tilted-'); normal=(0.,1.,0.) if tilted else (0.,0.,1.)
    mismatch='mismatch' in a.mode or a.mode=='generated-loss'
    expected=3. if a.mode in ('same','tilted-same','generated-loss') else 7.
    for t in (source,output):
        require(t.dxf.text=='WITNESS' and t.dxf.halign==1 and t.dxf.valign==2 and t.dxf.height==2.5 and t.dxf.rotation==30.,'TEXT scalar mismatch')
        require(tuple(t.dxf.extrusion)==normal,'normal mismatch')
    require(tuple(source.dxf.insert)==(1.,2.,3.) and tuple(source.dxf.align_point)==(4.,5.,7. if mismatch else 3.),'source points mismatch')
    require(tuple(output.dxf.insert)==(1.,2.,expected) and tuple(output.dxf.align_point)==(4.,5.,expected),'output points mismatch')
    world=tuple(OCS(normal).to_wcs(output.dxf.align_point))
    require(world==((-4.,expected,5.) if tilted else (4.,5.,expected)),'alignment WCS mismatch')
    with tempfile.TemporaryDirectory() as td:
        path=pathlib.Path(td)/'read.json'
        subprocess.run([str(a.dwgread),'-O','JSON','-o',str(path),str(a.dwg)],check=True,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
        data=json.loads(path.read_text());rows=[x for x in data['OBJECTS'] if x.get('entity')=='TEXT']
        require(len(rows)==1,'DWG TEXT inventory mismatch');t=rows[0]
        require(t['elevation']==expected and t['ins_pt']==[1.,2.] and t['alignment_pt']==[4.,5.] and t['horiz_alignment']==1 and t['vert_alignment']==2,'DWG TEXT fields mismatch')
    print(a.mode,'selected source/DWG/output PASS; alignment WCS',world)

if __name__=='__main__': main()
