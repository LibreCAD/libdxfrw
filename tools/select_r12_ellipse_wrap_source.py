#!/usr/bin/env python3
import argparse,hashlib
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--source',required=True);p.add_argument('--sha256',required=True);p.add_argument('--output',required=True);a=p.parse_args()
data=Path(a.source).read_bytes()
if hashlib.sha256(data).hexdigest()!=a.sha256:raise SystemExit('source hash mismatch')
lines=data.splitlines()
if len(lines)%2:raise SystemExit('malformed group pairs')
pairs=list(zip(lines[::2],lines[1::2])); starts=[i for i,(c,v) in enumerate(pairs) if c.strip()==b'0' and v.strip()==b'ELLIPSE']
if len(starts)!=1:raise SystemExit('unexpected ellipse count')
i=starts[0];j=i+1
while j<len(pairs) and pairs[j][0].strip()!=b'0':j+=1
entity=pairs[i:j]
# Keep the exact selected physical group values/order; only the surrounding
# document is synthesized to isolate the supported consumer profile.
head=[(b'0',b'SECTION'),(b'2',b'HEADER'),(b'9',b'$ACADVER'),(b'1',b'AC1027'),(b'0',b'ENDSEC'),(b'0',b'SECTION'),(b'2',b'ENTITIES')]
tail=[(b'0',b'ENDSEC'),(b'0',b'EOF')]
Path(a.output).write_bytes(b'\n'.join(x for pair in head+entity+tail for x in pair)+b'\n')
