#!/usr/bin/env python3
"""Bounded cross-kernel ray checks on the exact source-associated 2E1 SAB.

The cq-acis parser and OCCT STEP bridge are shared with earlier observations;
this is not an independent SAB decoder or source-placement qualification.
Only the cited planar profile and exact local tool binaries are admitted.
"""
import argparse
import hashlib
import importlib.metadata
import itertools
import json
import math
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

from audit_ac1027_planar_sab_geometry import audit_face, inside
from verify_acis218_sab_cqacis import bounded_read

SAB_SHA = '5ca4756434d068b932ac6e0100e5ff07eae36c7eed29a43367eeff8113cca0ec'
PINNED = {
    'bin/step-g': '63dc66a0e16972d9e85c5e35bc637d23624b0c6d1043b886c792fa9e0da9573b',
    'bin/rtshot': '628ac78da8e835cd9e0f314e478aefad5d5150f2f864b2c2ac80df1c18e8373e',
    'bin/mged': '2941b6a41de8f52fbbf87ae21ab6643fe4bdd3f2a61d26276a72bbfa7acc0191',
    'lib/libbrep.20.dylib': '96bb69e20fd4c01031a0436e4bcdae8dfcf5c91aa867449325328c4de6be8d8c',
    'lib/libopenNURBS.2010.04.95.dylib': '22e0be53452778ad4c5f3b106078468324831b0f7d002418151182e0adc273b0',
    'lib/librt.20.dylib': 'a25ffd4805bb616ea6c53511ea3749f756afa2e8c6dc186c2c9488a810b0e249',
    'lib/libstepcore.2.dylib': 'a1d6a4fe946ffb13665d2ecc019bd0e05adde73eecc896c0fd23ec0b3dcb1237',
    'lib/libsteputils.2.dylib': 'ed963559208e37fc091cebb270674c291eb7dd3756d981430fcbddaf2555277e',
    'lib/libstepeditor.2.dylib': 'b7066c7968a534040a1db539bab41dd08e8077cb1ebea1f01240f19f8b076205',
    'lib/libstepdai.2.dylib': 'd86fbe02a9c860a45eda229fc7104d57e28fdfe2f64b9a31fcd03ddfb6418a10',
    'lib/libbu.20.dylib': '747cfce363008999c23e0487c956f7c4b17bda8e8c2ae4ef8e424b34f76dda6c',
    'lib/libbn.20.dylib': '301e8f2dd9088a964aa58a30364f3576bcf0ab73e0f62f12ac2121449c68e652',
    'lib/libp2t.1.dylib': '59948f7c95ff976b9e948dd424f0974b646f6da00b6f3bd6d3129fc208c33088',
    'lib/libexpress.2.dylib': '83cb41d98cf30d401f0ef0aa5298026e1fc7e7f23a426f62f996c59578567744',
    'lib/libwdb.20.dylib': 'c7f4dd5b6a00be61b14ae58a783ac96ed7133fa84c4669d95d6a89af2c895eb2',
    'lib/libvds.1.dylib': '076a5074537e7f5ce12643527796d84c8b89b1c9c72b601eb4a0e65a8cadb477',
    'lib/libsysv.20.dylib': '0eeb765e9797a82d762d392b7613d8988c71c2d75004fc9a9d0915aaaa7f6d79',
}
def sha(data): return hashlib.sha256(data).hexdigest()
def require(ok, message):
    if not ok: raise ValueError(message)
def digest(path): return sha(Path(path).read_bytes())
def close(a, b, tol=1e-6): return math.dist(a, b) <= tol


def classify_step(path, output, expected_faces, expected_holes):
    """Classify source STEP loops via geometry, not their original ordering."""
    raw = bounded_read(path, 5 * 1024 * 1024)
    text = raw.decode('ascii')
    records = {int(k): v.strip() for k, v in re.findall(r'#(\d+)\s*=\s*(.*?);', text, re.S)}
    require(len(records) <= 1000, 'too many STEP records')
    def match(i, pattern):
        m = re.fullmatch(pattern, records[i], re.S)
        require(m is not None, f'unexpected STEP record #{i}')
        return m
    def point(i):
        m = match(i, r"CARTESIAN_POINT\('',\((.*?)\)\)")
        p = tuple(float(x) for x in m.group(1).split(','))
        require(len(p) == 3 and all(math.isfinite(x) for x in p), 'bad point')
        return p
    def vertex(i): return point(int(match(i, r"VERTEX_POINT\('',#(\d+)\)").group(1)))
    def ring(bound):
        m = match(bound, r"FACE_BOUND\('',#(\d+),\.(T|F)\.\)")
        orient = m.group(2)
        loop = match(int(m.group(1)), r"EDGE_LOOP\('',\((.*?)\)\)")
        refs = [int(x) for x in re.findall(r'#(\d+)', loop.group(1))]
        require(3 <= len(refs) <= 1000, 'invalid edge count')
        edges = []
        for ref in refs:
            edge = match(ref, r"ORIENTED_EDGE\('',\*,\*,#(\d+),\.(T|F)\.\)")
            curve = match(int(edge.group(1)), r"EDGE_CURVE\('',#(\d+),#(\d+),#(\d+),\.(T|F)\.\)")
            require(records[int(curve.group(3))].startswith('LINE('), 'non-line edge')
            a, b = vertex(int(curve.group(1))), vertex(int(curve.group(2)))
            edges.append((a, b) if edge.group(2) == 'T' else (b, a))
        require(all(close(a[1], b[0], 1e-7) for a, b in zip(edges, edges[1:] + edges[:1])), 'open ring')
        pts = [a for a, _ in edges]
        return pts if orient == 'T' else list(reversed(pts))
    outers, top, holes = [], None, 0
    faces = [(i, v) for i, v in records.items() if v.startswith('ADVANCED_FACE(')]
    require(len(faces) == expected_faces, 'unexpected face count')
    for i, _ in faces:
        face = match(i, r"ADVANCED_FACE\('',\((.*?)\),#(\d+),\.(T|F)\.\)")
        refs = [int(x) for x in re.findall(r'#(\d+)', face.group(1))]
        require(1 <= len(refs) <= 2, 'unsupported loop count')
        plane = match(int(face.group(2)), r"PLANE\('',#(\d+)\)")
        place = match(int(plane.group(1)), r"AXIS2_PLACEMENT_3D\('',#(\d+),#(\d+),#(\d+)\)")
        origin = point(int(place.group(1)))
        normal = tuple(float(x) for x in match(int(place.group(2)), r"DIRECTION\('',\((.*?)\)\)").group(1).split(','))
        require(len(normal) == 3 and all(math.isfinite(x) for x in normal), 'bad normal')
        if face.group(3) == 'F': normal = tuple(-v for v in normal)
        loops = [ring(x) for x in refs]
        def signed(pts):
            p = pts[0]
            return math.fsum(sum((a[(j+1)%3]-p[(j+1)%3])*(b[(j+2)%3]-p[(j+2)%3])
                                 - (a[(j+2)%3]-p[(j+2)%3])*(b[(j+1)%3]-p[(j+1)%3])
                                 for a,b in zip(pts,pts[1:]+pts[:1])) * normal[j] for j in range(3))/2
        signs = [signed(p) for p in loops]
        require(sum(v > 1e-6 for v in signs) == 1 and all(abs(v) > 1e-6 for v in signs), 'ambiguous winding')
        outer_index = next(j for j, v in enumerate(signs) if v > 1e-6)
        ordered = [loops[outer_index]] + [loops[j] for j in range(len(loops)) if j != outer_index]
        # Checks planarity, simple loops, winding, strict nesting and intersections.
        audit_face(ordered, normal, origin, ordered[0][0])
        outers.append(refs[outer_index]); holes += len(ordered)-1
        if len(ordered) == 2 and close(normal, (0.,0.,1.), 1e-8):
            require(top is None, 'multiple top faces'); top = ordered
    require(holes == expected_holes and len(set(outers)) == expected_faces, 'unexpected topology')
    for ident in outers:
        text, count = re.subn(r'(#'+str(ident)+r'\s*=\s*)FACE_BOUND\(', r'\1FACE_OUTER_BOUND(', text, count=1)
        require(count == 1, 'cannot classify face bound')
    output.write_text(text, encoding='ascii')
    return top, {'face_count': len(faces), 'hole_count': holes, 'outer_bounds': outers,
                 'before_sha256': sha(raw), 'after_sha256': digest(output)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sab', required=True, type=Path)
    parser.add_argument('--brlcad-root', required=True, type=Path)
    parser.add_argument('--work-dir', required=True, type=Path)
    args = parser.parse_args()
    raw = bounded_read(args.sab, 1024 * 1024)
    require(sha(raw) == SAB_SHA, 'unexpected SAB source hash')
    for name, expected in PINNED.items(): require(digest(args.brlcad_root / name) == expected, f'unexpected BRL-CAD {name}')
    require(importlib.metadata.version('cq-acis') == '0.3.12' and importlib.metadata.version('cadquery') == '2.8.0'
            and importlib.metadata.version('cadquery-ocp') == '7.9.3.1.1', 'unexpected bridge versions')
    import cq_acis
    import cq_acis._native as native
    import cq_acis.cadquery as bridge
    import cadquery as cq
    import cadquery.occ_impl.shapes as shapes
    import OCP.OCP as ocp
    modules = {Path(native.__file__): '28456eb65642c94169b35c7ab437658f91d8208db62b6cd030c63e24035a22b2',
               Path(bridge.__file__): '3b5f4429dcb152aaca999a1d4627542eb0406a868dc03a8a5cdd8abdf262f323',
               Path(shapes.__file__): '6f6a9136b45177bd9cb00ed1706f90078aab04ea2b1a7eb26ab5b46aa43f6c94',
               Path(ocp.__file__): '827fa23f8169d10d13b91d8808dd5d3306b6f3e5af1d5b7d6ddc61ce7617299a'}
    for path, expected in modules.items(): require(digest(path) == expected, f'unexpected module {path.name}')
    args.work_dir.mkdir(parents=True, exist_ok=True)
    work = Path(tempfile.mkdtemp(prefix='p7-brlcad-', dir=args.work_dir)).resolve()
    root = work / 'brlcad'; root.symlink_to(args.brlcad_root.resolve(), target_is_directory=True)
    require(':' not in str(root), 'scratch path contains DYLD separator')
    env = os.environ.copy(); env['DYLD_LIBRARY_PATH'] = str(root / 'lib'); env['BRLCAD_ROOT'] = str(root)
    def run(cmd):
        r = subprocess.run([str(x) for x in cmd], env=env, cwd=work, capture_output=True, text=True, timeout=60, check=False)
        text = r.stdout + r.stderr
        require(r.returncode == 0, f'{cmd[0]} exit {r.returncode}: {text[-1000:]}')
        require(not re.search(r'(?i)\b(?:warning|error):| is not valid', text), f'diagnostic: {text[-1000:]}')
        return text
    model = cq.Workplane().box(2,3,4).val()
    require(abs(model.Volume()-24) < 1e-9 and abs(model.Area()-52) < 1e-9, 'cube control')
    solid = cq_acis.to_cadquery(cq_acis.parse_sab_model(raw, source_id='2e1.sab'))
    require(len(solid.vals()) == 1, 'expected exactly one converted shape')
    shape = solid.val()
    results = {}
    for name, obj, faces, holes in [('cube', model, 6, 0), ('solid', shape, 8, 2)]:
        step, normalized, gfile = (work / (name + ext) for ext in ('.step', '-outer.step', '.g'))
        obj.exportStep(str(step), write_pcurves=False)
        top, info = classify_step(step, normalized, faces, holes)
        log = run([root/'bin/step-g', '-o', gfile, normalized])
        require(gfile.is_file() and gfile.stat().st_size > 0, 'no database')
        info['brlcad_database_sha256'] = digest(gfile)
        report = run([root/'bin/mged', '-c', gfile, "brep {''0.s} info"])
        for key, number in [('faces', faces), ('loops', faces+holes), ('edges', 12 if name=='cube' else 18), ('vertices', 8 if name=='cube' else 12)]:
            require(re.search(r'(?m)^'+key+r':\s*'+str(number)+r'\s*$', report), f'missing {name} {key} count')
        info['step_g_log_sha256'] = sha(log.encode()); info['mged_info_sha256'] = sha(report.encode())
        results[name] = info
        if name == 'solid': require(top is not None, 'missing top-with-hole'); top_loops = top
    def ray(name, point, hit, distances=None):
        log = run([root/'bin/rtshot', '-R','0.001','-n','1','-c','1', '-p',str(point[0]),str(point[1]),'10', '-d','0','0','-1', work/(name+'.g'), "''0.r"])
        observed = '--- Hit region ' in log
        require(observed == hit and (hit or 'missed' in log), f'{name} unexpected ray')
        if hit:
            a = re.search(r'InHIT dist=([0-9.eE+-]+)', log); b = re.search(r'OutHIT dist=([0-9.eE+-]+)', log)
            require(a and b and abs(float(a.group(1))-distances[0])<1e-6 and abs(float(b.group(1))-distances[1])<1e-6, f'{name} ray depth')
        return {'point': list(point), 'hit': hit, 'distances': list(distances) if hit else None, 'output_sha256': sha(log.encode())}
    cube_rays = [ray('cube',(0.,0.),True,(8.,12.)), ray('cube',(3.,0.),False)]
    outer, inner = top_loops
    require(len(outer) == len(inner) == 3, 'expected triangular rings')
    pairing = min(itertools.permutations(inner), key=lambda p: sum(math.dist(a,b) for a,b in zip(outer,p)))
    o2, i2 = [p[:2] for p in outer], [p[:2] for p in inner]
    def dist_segment(p,a,b):
        dx,dy = b[0]-a[0],b[1]-a[1]; t=max(0.,min(1.,((p[0]-a[0])*dx+(p[1]-a[1])*dy)/(dx*dx+dy*dy)))
        return math.hypot(p[0]-a[0]-t*dx,p[1]-a[1]-t*dy)
    def margin(p): return min(dist_segment(p,a,b) for ring in (o2,i2) for a,b in zip(ring,ring[1:]+ring[:1]))
    rays = []
    for i in range(3):
        j = (i+1)%3
        for t in (.35,.65):
            p = tuple(((1-t)*outer[i][k]+t*outer[j][k]+(1-t)*pairing[i][k]+t*pairing[j][k])/2 for k in (0,1))
            require(inside(p,o2) and not inside(p,i2) and margin(p)>0.01, 'unsafe rim sample')
            row=ray('solid',p,True,(6.,10.)); row['boundary_margin']=margin(p); row['role']='rim'; rays.append(row)
    hole = tuple(sum(p[k] for p in inner)/3 for k in (0,1))
    require(inside(hole,o2) and inside(hole,i2) and margin(hole)>0.01, 'unsafe hole sample')
    row=ray('solid',hole,False); row['boundary_margin']=margin(hole); row['role']='hole'; rays.append(row)
    outside = (5000.,15000.)
    require(not inside(outside,o2), 'unsafe exterior sample')
    row=ray('solid',outside,False); row['boundary_margin']=margin(outside); row['role']='outside'; rays.append(row)
    print(json.dumps({'qualification_eligible':False,'source_sha256':SAB_SHA,'bridge':'cq-acis 0.3.12 / OCCT, STEP without pcurves',
                      'kernel':'BRL-CAD 7.24 STEPcode/openNURBS/librt','module_sha256':{str(k):v for k,v in modules.items()},
                      'brlcad_sha256':PINNED,'scratch':str(work),'shapes':results,'cube_rays':cube_rays,'source_rays':rays,
                      'limits':['shared SAB parser and OCCT STEP bridge','unknown source history and DWG placement','only eight sampled rays; no source-native volume or support claim']},indent=2))

if __name__ == '__main__': main()
