#!/usr/bin/env python3
import sys
from pathlib import Path
import ezdxf
p = sys.argv[1]
if Path(p).exists():
    raise SystemExit("refusing to overwrite an existing file")
doc = ezdxf.new('R2007')
m = doc.modelspace()
m.add_line((1,2,3),(4,5,6))
m.add_point((3,4,5))
m.add_circle((7,8,2), 2.5)
m.add_circle((7,8,2), 2.5, dxfattribs={'extrusion':(0,1,0),'thickness':1.25})
m.add_circle((7,8,2), 2.5, dxfattribs={'extrusion':(0,0,-1),'thickness':1.25})
m.add_arc((10,11,3),4,20,115)
m.add_arc((10,11,3),4,20,115,dxfattribs={'extrusion':(0,1,0),'thickness':0.5})
m.add_arc((10,11,3),4,20,115,dxfattribs={'extrusion':(0,0,-1),'thickness':0.5})
doc.saveas(p)
print('ezdxf',ezdxf.__version__)
