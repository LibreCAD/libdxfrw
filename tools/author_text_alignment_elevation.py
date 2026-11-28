#!/usr/bin/env python3
"""Author non-fixture TEXT elevation inputs; requires ezdxf."""
import pathlib
import sys
import ezdxf

out=pathlib.Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=True)
for name,z,normal in [('same',3.,(0.,0.,1.)),('mismatch',7.,(0.,0.,1.)),
                      ('tilted-same',3.,(0.,1.,0.)),('tilted-mismatch',7.,(0.,1.,0.))]:
    d=ezdxf.new('R2013')
    d.modelspace().add_text('WITNESS',dxfattribs={'insert':(1.,2.,3.),
        'align_point':(4.,5.,z),'height':2.5,'halign':1,'valign':2,
        'rotation':30.,'extrusion':normal})
    d.saveas(out/(name+'.dxf'))
