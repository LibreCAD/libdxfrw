#!/usr/bin/env python3
import argparse
import hashlib
import math
from pathlib import Path
import ezdxf


def check(ok, reason):
    if not ok:
        raise SystemExit(reason)


def verify(path, digest):
    check(hashlib.sha256(Path(path).read_bytes()).hexdigest()==digest, 'hash mismatch')


def main():
    p=argparse.ArgumentParser()
    p.add_argument('--source',required=True);p.add_argument('--source-sha256',required=True)
    p.add_argument('--output',required=True);p.add_argument('--output-sha256',required=True)
    p.add_argument('--mode',choices=('corrected','legacy'),required=True)
    a=p.parse_args();verify(a.source,a.source_sha256);verify(a.output,a.output_sha256)
    src=ezdxf.readfile(a.source);out=ezdxf.readfile(a.output)
    check(src.dxfversion=='AC1027' and out.dxfversion=='AC1009', 'revision mismatch')
    se=list(src.modelspace().query('ELLIPSE'));oe=list(out.modelspace().query('POLYLINE'))
    check(len(se)==1 and len(oe)==1,'wrong entity inventory')
    ellipse=se[0]; vertices=[tuple(v.dxf.location) for v in oe[0].vertices]
    check(abs(ellipse.dxf.start_param-5.5)<1e-12 and abs(ellipse.dxf.end_param-(.5+2*math.pi))<1e-12,'source arc mismatch')
    src_audit=src.audit();out_audit=out.audit()
    check(not src_audit.errors and not src_audit.fixes and not out_audit.errors and not out_audit.fixes,'DXF audit error')
    if a.mode=='corrected':
        check(len(vertices)==28,'wrong corrected vertex count')
        reference=list(ellipse.vertices(ellipse.params(len(vertices))))
        check(all(math.dist(x,y)<1e-10 for x,y in zip(vertices,reference)), 'polyline differs from sampled CCW ellipse')
    else:
        check(len(vertices)==103 and vertices[len(vertices)//2][0]<0.,'legacy behavior changed')
    print(a.mode,'R12 vertex geometry checked against ODA source')


if __name__=='__main__':main()
