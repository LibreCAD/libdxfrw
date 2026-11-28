#!/usr/bin/env python3
"""Author three scratch LIGHT graphs with distinct group-421 values."""
import argparse,subprocess,sys
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__);p.add_argument('directory',type=Path);a=p.parse_args();root=a.directory
if root.exists():raise ValueError('output directory must not exist')
root.mkdir(parents=True);seed=root/'seed.dxf'
script=Path(__file__).with_name('lightlist_physical_graph_probe.py')
subprocess.run([sys.executable,str(script),str(seed)],check=True,timeout=30)
lines=seed.read_text().splitlines();tags=list(zip(lines[::2],lines[1::2]));seed.unlink()
i=next(i for i,(code,value) in enumerate(tags) if code.strip()=='0' and value=='LIGHT')
j=next(j for j in range(i+1,len(tags)) if tags[j][0].strip()=='0')
k=next(k for k in range(i,j) if tags[k][0].strip()=='100' and tags[k][1]=='AcDbLight')
for name,value in [('black',0),('rgb',1193046),('white',16777215)]:
 out=tags[:k+1]+[(' 63','7'),('421',str(value))]+tags[k+1:]
 (root/(name+'.dxf')).write_text(''.join(c+'\n'+v+'\n' for c,v in out))
