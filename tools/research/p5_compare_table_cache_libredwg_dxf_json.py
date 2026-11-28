import argparse
import json
import sys

def pairs(path):
    lines=open(path, encoding='utf-8').read().splitlines()
    if len(lines)%2: raise SystemExit('odd DXF line count')
    return [(int(lines[i].strip()), lines[i+1].strip()) for i in range(0,len(lines),2)]

def records(path):
    ps=pairs(path); out=[]; section=''; i=0; in_t1=False; inside=False; current=None
    while i<len(ps):
        c,v=ps[i]
        if c==0 and v=='SECTION' and i+1<len(ps) and ps[i+1][0]==2:
            section=ps[i+1][1]; i+=2; continue
        if c==0 and v=='ENDSEC':
            if current: out.append(current); current=None
            section=''; in_t1=False; i+=1; continue
        if section=='BLOCKS' and c==0 and v=='BLOCK':
            if current: out.append(current); current=None
            j=i+1
            while j<len(ps) and ps[j][0]!=0:
                if ps[j][0]==2 and ps[j][1]=='*T1': in_t1=True
                j+=1
            inside=in_t1; i+=1; continue
        if section=='BLOCKS' and c==0 and v=='ENDBLK':
            if current: out.append(current); current=None
            if inside: break
            in_t1=False; inside=False; i+=1; continue
        if inside and c==0:
            if current: out.append(current)
            current={'type':v,'groups':[]}
            i+=1; continue
        if current is not None:
            current['groups'].append((c,v))
        i+=1
    return [r for r in out if r and r['type'] in ('MTEXT','LINE')]

def vals(r, code): return [v for c,v in r['groups'] if c==code]
def xyz(r, code):
    def one(n):
        a=vals(r,n); return float(a[-1]) if a else 0.0
    return (one(code),one(code+10),one(code+20))
def near(a,b): return abs(a-b)<=1e-9*max(1.,abs(a),abs(b))
def vecnear(a,b): return all(near(x,y) for x,y in zip(a,b))

parser=argparse.ArgumentParser(description="Compare selected TABLE-cache DXF primitive fields with LibreDWG JSON readback.")
parser.add_argument("dxf", help="isolated ASCII DXF emitted by libdxfrw")
parser.add_argument("json", help="LibreDWG dwgread JSON produced from that DXF via dxf2dwg")
args=parser.parse_args()
src=records(args.dxf)
peer=json.load(open(args.json, encoding="utf-8"))['OBJECTS']
peers=[o for o in peer if isinstance(o,dict) and o.get('entmode')==3 and o.get('entity') in ('MTEXT','LINE')]
if len(src)!=15 or len(peers)!=15: raise SystemExit(f'count mismatch dxf={len(src)} peer={len(peers)}')
checks=0
for idx,(a,b) in enumerate(zip(src,peers)):
    if a['type']!=b['entity']: raise SystemExit(f'kind/order mismatch {idx}: {a["type"]} {b["entity"]}')
    if a['type']=='LINE':
        start=xyz(a,10); end=xyz(a,11)
        if not vecnear(start,b['start']) or not vecnear(end,b['end']):
            raise SystemExit(f'LINE geometry mismatch {idx}: {start}/{end} vs {b["start"]}/{b["end"]}')
        checks+=2
    else:
        insertion=xyz(a,10)
        direction=xyz(a,11)
        text=''.join(vals(a,3))+''.join(vals(a,1))
        if not vecnear(insertion,b['ins_pt']): raise SystemExit(f'MTEXT insertion mismatch {idx}: {insertion} {b["ins_pt"]}')
        if not vecnear(direction,b['x_axis_dir']): raise SystemExit(f'MTEXT direction mismatch {idx}: {direction} {b["x_axis_dir"]}')
        if text!=b['text']: raise SystemExit(f'MTEXT text mismatch {idx}: {text!r} {b["text"]!r}')
        if int(vals(a,71)[-1])!=b['attachment'] or int(vals(a,72)[-1])!=b['flow_dir']:
            raise SystemExit(f'MTEXT layout mismatch {idx}')
        checks+=5
print(f'PASS: {checks} ordered selected field comparisons agree between emitted ASCII DXF and LibreDWG decoded output')
print('Excluded from comparison: MTEXT group-40/41 semantics, native text extents/rendering, source graph and whole-drawing closure.')
