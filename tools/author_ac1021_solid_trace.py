#!/usr/bin/env python3
"""Author sampled SOLID/TRACE controls; DXF metadata and hashes may vary."""
import argparse
from pathlib import Path
import ezdxf
if ezdxf.__version__!='1.4.5':raise SystemExit('requires ezdxf 1.4.5')
p=argparse.ArgumentParser(description=__doc__);p.add_argument('output',type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True)
points=[(1,2,3),(4,2,3),(1,4,3),(4,4,3)]
for entity in ('solid','trace'):
 for normal in ('default','tilt'):
  for thickness in ('zero','thick'):
   doc=ezdxf.new('R2007');model=doc.modelspace();kwargs={'extrusion':(0,1,0) if normal=='tilt' else (0,0,1),'thickness':.5 if thickness=='thick' else 0.}
   getattr(model,'add_'+entity)(points,dxfattribs=kwargs);model.add_line((10,11,0),(12,13,0));doc.saveas(a.output/f'{entity}-{normal}-{thickness}.dxf')
