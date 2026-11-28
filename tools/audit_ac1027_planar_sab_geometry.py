#!/usr/bin/env python3
"""Independent arithmetic audit of the three known AC1027 planar SAB payloads.

The optional cq-acis parser supplies source entities and references; geometry
below uses only Python vector/polygon arithmetic, not an OCCT geometry kernel.
The profile is deliberately exact and hash-bound.  It does not establish source
placement, units, native ACIS truth, or the meaning of retained raw attributes.
"""
from __future__ import annotations
import argparse
from collections import Counter
import hashlib
from importlib.metadata import version
import json
import math
from pathlib import Path
import sys
from verify_acis218_sab_cqacis import bounded_read

EXPECTED = {
 '176': '596d37aaf81ebe9662bd22ba199bce876403dec8da71dd3287b0255adb354782',
 '37d': 'fabe3e9804c44472907987ea96409b30b7db35575a34688430b0157d1b6c5566',
 '2e1': '5ca4756434d068b932ac6e0100e5ff07eae36c7eed29a43367eeff8113cca0ec',
}
TOL = 1e-6

def require(ok, message):
    if not ok: raise ValueError(message)

def sub(a,b): return tuple(x-y for x,y in zip(a,b))
def dot(a,b): return math.fsum(x*y for x,y in zip(a,b))
def cross(a,b): return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])
def vec(v): return (v.x,v.y,v.z)
def norm(a): return math.sqrt(dot(a,a))
def is_zero_ref(ref): return ref.index == -1

def orient(a,b,c): return (b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0])
def on_segment(a,b,p):
    return abs(orient(a,b,p)) <= TOL and all(min(a[i],b[i])-TOL <= p[i] <= max(a[i],b[i])+TOL for i in (0,1))
def intersects(a,b,c,d):
    v=(orient(a,b,c),orient(a,b,d),orient(c,d,a),orient(c,d,b))
    if (v[0]>TOL and v[1]<-TOL or v[0]<-TOL and v[1]>TOL) and (v[2]>TOL and v[3]<-TOL or v[2]<-TOL and v[3]>TOL): return True
    return any(abs(v[i])<=TOL and on_segment(*args) for i,args in enumerate(((a,b,c),(a,b,d),(c,d,a),(c,d,b))))
def inside(point, polygon):
    result=False
    for a,b in zip(polygon,polygon[1:]+polygon[:1]):
        require(not on_segment(a,b,point), 'hole touches boundary')
        if (a[1]>point[1]) != (b[1]>point[1]):
            if point[0] < (b[0]-a[0])*(point[1]-a[1])/(b[1]-a[1])+a[0]: result=not result
    return result

def audit_face(loops, normal, origin, anchor):
    """Return signed-loop area and volume terms for one planar oriented face."""
    require(loops and len(loops)<=2, 'unsupported face loop count')
    require(all(math.isfinite(x) for x in normal+origin+anchor), 'nonfinite plane')
    require(abs(norm(normal)-1.0)<1e-9, 'non-unit plane normal')
    axis=max(range(3),key=lambda i: abs(normal[i])); dims=[i for i in range(3) if i!=axis]
    flat=[]; signed=[]; volume=[]; moments=[[],[],[]]
    for pts in loops:
        require(3<=len(pts)<=1000, 'invalid ring size')
        require(all(all(math.isfinite(x) for x in p) and abs(dot(sub(p,origin),normal))<=TOL for p in pts), 'nonfinite or nonplanar ring')
        xy=[(p[dims[0]]-origin[dims[0]],p[dims[1]]-origin[dims[1]]) for p in pts]
        for i in range(len(xy)):
            require(norm(sub(pts[i],pts[(i+1)%len(pts)]))>TOL, 'degenerate edge')
            for j in range(i+1,len(xy)):
                if j==i+1 or (i==0 and j==len(xy)-1): continue
                require(not intersects(xy[i],xy[(i+1)%len(xy)],xy[j],xy[(j+1)%len(xy)]), 'self-intersecting ring')
        flat.append(xy)
        pivot=pts[0]
        signed.append(math.fsum(dot(cross(sub(pts[i],pivot),sub(pts[i+1],pivot)),normal)/2 for i in range(1,len(pts)-1)))
        tetra=[(dot(sub(pivot,anchor),cross(sub(pts[i],anchor),sub(pts[i+1],anchor)))/6, pts[i],pts[i+1]) for i in range(1,len(pts)-1)]
        volume.append(math.fsum(t[0] for t in tetra))
        for v,p,q in tetra:
            for k in range(3):moments[k].append(v*(anchor[k]+pivot[k]+p[k]+q[k])/4)
    require(signed[0]>TOL and all(x < -TOL for x in signed[1:]), 'outer/hole winding mismatch')
    if len(flat)==2:
        a,b=flat
        require(all(inside(p,a) for p in b), 'hole outside outer ring')
        require(all(not intersects(p,q,r,s) for p,q in zip(a,a[1:]+a[:1]) for r,s in zip(b,b[1:]+b[:1])), 'intersecting rings')
    return math.fsum(signed),math.fsum(volume),tuple(math.fsum(values) for values in moments)

def source_observation(identity, raw):
    from cq_acis import parse_sab_model
    model=parse_sab_model(raw,source_id=identity)
    es=model.entities
    supported={'RawEntity','BodyEntity','LumpEntity','ShellEntity','FaceEntity','LoopEntity',
               'CoedgeEntity','EdgeEntity','VertexEntity','StraightCurveEntity',
               'PointEntity','PlaneSurfaceEntity'}
    require(all(type(e).__name__ in supported for e in es), 'unsupported entity type')
    require(all(not hasattr(e,'pattern') or is_zero_ref(e.pattern) for e in es), 'pattern outside profile')
    by_index={e.raw.index if hasattr(e,'raw') else e.index:e for e in es}
    require(len(by_index)==len(es), 'duplicate entity identity')
    def resolve(ref, expected=None):
        require(ref.index in by_index, 'missing entity reference')
        value=by_index[ref.index]
        if expected: require(type(value).__name__==expected, f'expected {expected}')
        return value
    raw_rows=[e for e in es if type(e).__name__=='RawEntity']
    raw_names=Counter(e.type_name for e in raw_rows)
    require(set(raw_names)<= {'asmheader','persubent-acadSolidHistory-attrib'}, 'unknown raw record type')
    require(len(model.bodies())==1 and sum(type(e).__name__=='LumpEntity' for e in es)==1
            and sum(type(e).__name__=='ShellEntity' for e in es)==1, 'body/lump/shell census mismatch')
    body=model.bodies()[0]
    require(is_zero_ref(body.transform) and is_zero_ref(body.wire), 'body transform or wire unsupported')
    lump=resolve(body.lump,'LumpEntity'); require(is_zero_ref(lump.next_lump) and resolve(lump.body)==body, 'lump graph mismatch')
    shell=resolve(lump.shell,'ShellEntity');require(is_zero_ref(shell.next_shell) and is_zero_ref(shell.subshell) and is_zero_ref(shell.wire) and resolve(shell.lump)==lump, 'shell graph mismatch')
    face=resolve(shell.face,'FaceEntity'); faces=[]; seen_faces=set()
    while face is not None:
        require(face.raw.index not in seen_faces and len(seen_faces)<1000, 'face cycle')
        require(resolve(face.shell)==shell and is_zero_ref(face.subshell) and not face.reversed
                and face.double_sided == (identity != '2e1'),
                'face owner or orientation outside profile')
        seen_faces.add(face.raw.index);faces.append(face)
        face=None if is_zero_ref(face.next_face) else resolve(face.next_face,'FaceEntity')
    require(seen_faces=={e.raw.index for e in es if type(e).__name__=='FaceEntity'}, 'unaccounted face')
    allpoints=[vec(e.location) for e in es if type(e).__name__=='PointEntity']
    require(allpoints, 'no points'); anchor=allpoints[0]; edge_directions={}; edge_faces={}; used_points=set(); used_vertices=set(); used_curves=set(); used_planes=set(); all_loops=set(); all_coedges=set(); area_terms=[]; volume_terms=[]; moment_terms=[[],[],[]]; loop_count=0
    for face in faces:
        plane=resolve(face.surface,'PlaneSurfaceEntity'); used_planes.add(plane.raw.index); n=vec(plane.normal); origin=vec(plane.origin); loop=resolve(face.loop,'LoopEntity'); loops=[]; seen_loops=set()
        while loop is not None:
            require(loop.raw.index not in seen_loops and resolve(loop.face)==face, 'loop cycle or owner mismatch')
            seen_loops.add(loop.raw.index); require(loop.raw.index not in all_loops, 'shared loop'); all_loops.add(loop.raw.index); loop_count+=1; coedge=resolve(loop.coedge,'CoedgeEntity'); first=coedge.raw.index; seen_coedges=set(); ring=[]; directions=[]
            while True:
                require(coedge.raw.index not in seen_coedges and len(seen_coedges)<1000 and resolve(coedge.loop)==loop, 'coedge cycle or owner mismatch')
                seen_coedges.add(coedge.raw.index); require(coedge.raw.index not in all_coedges, 'shared coedge'); all_coedges.add(coedge.raw.index); edge=resolve(coedge.edge,'EdgeEntity'); curve=resolve(edge.curve,'StraightCurveEntity'); used_curves.add(curve.raw.index)
                require(is_zero_ref(coedge.pcurve), 'non-null pcurve outside profile')
                if identity=='2e1':
                    partner=resolve(coedge.partner_coedge,'CoedgeEntity')
                    require(resolve(partner.partner_coedge)==coedge and resolve(partner.edge)==edge, 'coedge partner mismatch')
                else:require(is_zero_ref(coedge.partner_coedge), 'unexpected REGION partner')
                start=resolve(edge.start_vertex,'VertexEntity'); end=resolve(edge.end_vertex,'VertexEntity')
                if coedge.reversed:start,end=end,start
                pnt=resolve(start.point,'PointEntity'); p=vec(pnt.location); q=vec(resolve(end.point,'PointEntity').location)
                direction=vec(curve.direction); require(all(math.isfinite(x) for x in direction+vec(curve.origin)) and norm(direction)>TOL, 'invalid line')
                require(norm(cross(sub(p,vec(curve.origin)),direction))<=TOL*max(1.0,norm(direction)) and norm(cross(sub(q,vec(curve.origin)),direction))<=TOL*max(1.0,norm(direction)), 'edge endpoints off line')
                used_points.add(pnt.raw.index); used_vertices.add(start.raw.index); ring.append(p); directions.append((start.raw.index,end.raw.index)); edge_directions.setdefault(edge.raw.index,[]).append((start.raw.index,end.raw.index)); edge_faces.setdefault(edge.raw.index,set()).add(face.raw.index)
                following=resolve(coedge.next_coedge,'CoedgeEntity'); require(resolve(following.previous_coedge)==coedge, 'nonreciprocal coedge')
                coedge=following
                if coedge.raw.index==first:break
            require(all(directions[i][1]==directions[(i+1)%len(directions)][0] for i in range(len(directions))), 'disconnected coedge ring')
            loops.append(ring);loop=None if is_zero_ref(loop.next_loop) else resolve(loop.next_loop,'LoopEntity')
        a,v,moments=audit_face(loops,n,origin,anchor);area_terms.append(a);volume_terms.append(v)
        for i in range(3):moment_terms[i].append(moments[i])
    require(used_points=={e.raw.index for e in es if type(e).__name__=='PointEntity'}, 'unaccounted point')
    require(used_vertices=={e.raw.index for e in es if type(e).__name__=='VertexEntity'}, 'unaccounted vertex')
    require(all_loops=={e.raw.index for e in es if type(e).__name__=='LoopEntity'}, 'unaccounted loop')
    require(all_coedges=={e.raw.index for e in es if type(e).__name__=='CoedgeEntity'}, 'unaccounted coedge')
    require(used_curves=={e.raw.index for e in es if type(e).__name__=='StraightCurveEntity'}, 'unaccounted curve')
    require(used_planes=={e.raw.index for e in es if type(e).__name__=='PlaneSurfaceEntity'}, 'unaccounted plane')
    require(set(edge_directions)=={e.raw.index for e in es if type(e).__name__=='EdgeEntity'}, 'unaccounted edge')
    closed=all(len(v)==2 and len(edge_faces[k])==2 and v[0]==(v[1][1],v[1][0]) for k,v in edge_directions.items())
    if identity=='2e1': require(closed, 'solid edge incidence is not closed and opposed')
    else: require(all(len(v)==1 for v in edge_directions.values()), 'REGION boundary incidence mismatch')
    volume=math.fsum(volume_terms) if closed else None
    if closed: require(volume>TOL, 'non-positive solid volume')
    centroid=[math.fsum(t)/volume for t in moment_terms] if closed else None
    return {'handle':identity.upper(),'sha256':hashlib.sha256(raw).hexdigest(), 'face_count':len(faces), 'loop_count':loop_count, 'edge_count':len(edge_directions), 'vertex_count':len(used_vertices), 'closed_opposed_edges':closed, 'bounds':[[min(p[i] for p in allpoints) for i in range(3)],[max(p[i] for p in allpoints) for i in range(3)]], 'area':math.fsum(area_terms), 'oriented_volume':volume, 'centroid':centroid, 'parser_units_mm':model.metadata.units_mm, 'parser_body_transform':str(body.transform), 'raw_record_types':dict(sorted(raw_names.items()))}

def self_test():
    n=(0.,0.,1.); o=(0.,0.,0.)
    outer=[(0.,0.,0.),(4.,0.,0.),(4.,3.,0.),(0.,3.,0.)]
    hole=[(1.,1.,0.),(1.,2.,0.),(2.,2.,0.),(2.,1.,0.)]
    area,volume,_=audit_face([outer,hole],n,o,o)
    require(abs(area-11.)<1e-12 and volume==0., 'polygon/hole area self-test')
    outside=[(5.,1.,0.),(5.,2.,0.),(6.,2.,0.),(6.,1.,0.)]
    nonplanar=[(0.,0.,0.),(4.,0.,0.),(4.,3.,1.),(0.,3.,0.)]
    for rings in ([outer,list(reversed(hole))], [[outer[0],outer[2],outer[1],outer[3]]], [outer,outside], [nonplanar]):
        try:audit_face(rings,n,o,o)
        except ValueError:continue
        raise ValueError('malformed winding or bowtie accepted')
    cube=[([(0.,0.,0.),(0.,1.,0.),(1.,1.,0.),(1.,0.,0.)],(0.,0.,-1.)), ([(0.,0.,1.),(1.,0.,1.),(1.,1.,1.),(0.,1.,1.)],(0.,0.,1.)), ([(0.,0.,0.),(1.,0.,0.),(1.,0.,1.),(0.,0.,1.)],(0.,-1.,0.)), ([(1.,0.,0.),(1.,1.,0.),(1.,1.,1.),(1.,0.,1.)],(1.,0.,0.)), ([(1.,1.,0.),(0.,1.,0.),(0.,1.,1.),(1.,1.,1.)],(0.,1.,0.)), ([(0.,1.,0.),(0.,0.,0.),(0.,0.,1.),(0.,1.,1.)],(-1.,0.,0.))]
    vals=[audit_face([p],n,p[0],(0.,0.,0.)) for p,n in cube]
    require(abs(math.fsum(x[0] for x in vals)-6.)<1e-12 and abs(math.fsum(x[1] for x in vals)-1.)<1e-12,'cube area/volume self-test')
    require(all(abs(math.fsum(x[2][i] for x in vals)-0.5)<1e-12 for i in range(3)),'cube centroid self-test')
    shift=(12345.,-9876.,54.); shifted=[([tuple(p[i]+shift[i] for i in range(3)) for p in pts],normal) for pts,normal in cube]
    vals=[audit_face([pts],normal,pts[0],shift) for pts,normal in shifted]
    require(abs(math.fsum(x[0] for x in vals)-6.)<1e-12 and abs(math.fsum(x[1] for x in vals)-1.)<1e-12,'translated cube self-test')
    require(all(abs(math.fsum(x[2][i] for x in vals)-(shift[i]+0.5))<1e-9 for i in range(3)),'translated cube centroid self-test')
    print('PASS: analytic cube, hole, reversed hole and self-intersection controls')

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--self-test',action='store_true');parser.add_argument('--sab-dir',type=Path);args=parser.parse_args()
    if args.self_test:self_test();return 0
    require(args.sab_dir is not None,'--sab-dir required');require(version('cq-acis')=='0.3.12','expected cq-acis 0.3.12')
    raw={}
    for identity,digest in EXPECTED.items():
        data=bounded_read(args.sab_dir/(identity+'.sab'),65536);require(hashlib.sha256(data).hexdigest()==digest,'payload digest mismatch');raw[identity]=data
    print(json.dumps({'qualification_eligible':False,'method':'cq-acis parsed records + independent Python analytic geometry; not an independent source decoder','entities':[source_observation(k,v) for k,v in raw.items()]},indent=2,allow_nan=False))
    return 0
if __name__=='__main__':
    try:sys.exit(main())
    except (OSError,ValueError,ImportError) as exc:print('ERROR:',exc,file=sys.stderr);sys.exit(1)
