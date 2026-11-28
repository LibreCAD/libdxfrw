#!/usr/bin/env python3
"""Author sampled MTEXT rotation controls; DXF metadata and hashes can vary."""
import argparse
from pathlib import Path
import ezdxf
if ezdxf.__version__ != '1.4.5':raise SystemExit('requires ezdxf 1.4.5')
p=argparse.ArgumentParser(description=__doc__);p.add_argument('output',type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True)
for name,normal in [('default',(0,0,1)),('tilt',(0,1,0)),('negative',(0,0,-1)),('nonunit',(0,2,0))]:
 d=ezdxf.new('R2007');d.modelspace().add_mtext('HELLO',dxfattribs={'insert':(3,4,2),'extrusion':normal,'char_height':2.,'width':10.,'rotation':30.,'attachment_point':1,'flow_direction':1});d.saveas(a.output/(name+'.dxf'))
lines=(a.output/'tilt.dxf').read_text().splitlines();records=[(lines[i],lines[i+1]) for i in range(0,len(lines),2)]
start=next(i for i,(c,v) in enumerate(records) if c.strip()=='0' and v=='MTEXT');end=next(i for i in range(start+1,len(records)) if records[i][0].strip()=='0');idx=next(i for i in range(start,end) if records[i][0].strip()=='50');direction=[(' 11','0.0'),(' 21','0.0'),(' 31','1.0')]
for name,at in [('direction_then_rotation',idx),('rotation_then_direction',idx+1)]:
 (a.output/(name+'.dxf')).write_text(''.join(c+'\n'+v+'\n' for c,v in records[:at]+direction+records[at:]))
