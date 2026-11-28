#!/usr/bin/env python3
"""Audit a selected PDF-underlay producer DXF and selected adapter output."""
import argparse
import hashlib
import math
import ezdxf

p=argparse.ArgumentParser()
p.add_argument('--source', required=True)
p.add_argument('--source-sha256', required=True)
p.add_argument('--selected', required=True)
p.add_argument('--selected-sha256', required=True)
p.add_argument('--normal', choices=('default','tilted'), required=True)
a=p.parse_args()
for path,digest in ((a.source,a.source_sha256),(a.selected,a.selected_sha256)):
    if hashlib.sha256(open(path,'rb').read()).hexdigest()!=digest:
        raise SystemExit('digest mismatch: '+path)
expected=(0.,0.,1.) if a.normal=='default' else (1.,2.,3.)
for path in (a.source,a.selected):
    doc=ezdxf.readfile(path)
    if doc.dxfversion not in ('AC1024','AC1027'): raise SystemExit('unexpected DXF revision')
    entries=[e for e in doc.modelspace() if e.dxftype() in ('PDFREFERENCE','PDFUNDERLAY')]
    if len(entries)!=1: raise SystemExit('unexpected PDF entity inventory')
    e=entries[0]
    if not math.isclose(e.dxf.rotation,45.,abs_tol=1e-10): raise SystemExit('rotation mismatch')
    if tuple(e.dxf.insert)!=(1.,2.,3.) or tuple(e.dxf.extrusion)!=expected: raise SystemExit('placement mismatch')
    if len(doc.audit().errors): raise SystemExit('ezdxf audit errors')
print('PDF underlay selected placement and rotation verified')
