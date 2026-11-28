#!/usr/bin/env python3
import argparse
from pathlib import Path
import ezdxf
p=argparse.ArgumentParser();p.add_argument('directory');a=p.parse_args()
if ezdxf.__version__!='1.4.5':raise SystemExit('expected ezdxf 1.4.5')
out=Path(a.directory);out.mkdir(parents=True,exist_ok=False)
for name,normal in [('default',(0,0,1)),('tilt',(0,1,0))]:
 d=ezdxf.new('R2013');d.modelspace().add_ellipse(center=(2,3,4),major_axis=(4,0,0),ratio=.5,start_param=5.5,end_param=.5,dxfattribs={'extrusion':normal});d.saveas(out/(name+'.dxf'))
