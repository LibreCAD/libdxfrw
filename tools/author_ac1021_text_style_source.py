#!/usr/bin/env python3
"""Author the sampled custom-style tilted TEXT input; ezdxf metadata may vary."""
import argparse
import ezdxf
if ezdxf.__version__ != '1.4.5': raise SystemExit('requires ezdxf 1.4.5')
p=argparse.ArgumentParser(description=__doc__);p.add_argument('output');a=p.parse_args()
d=ezdxf.new('R2007');d.styles.new('CUSTOM',dxfattribs={'font':'arial.ttf'})
d.modelspace().add_text('HELLO',dxfattribs={'style':'CUSTOM','insert':(3,4,2),'align_point':(6,7,2),'extrusion':(0,1,0),'height':2,'rotation':30,'width':1.25,'oblique':15,'text_generation_flag':2,'halign':2,'valign':2});d.saveas(a.output)
