import argparse
import ezdxf
from pathlib import Path
parser=argparse.ArgumentParser(); parser.add_argument('output_dir',type=Path); root=parser.parse_args().output_dir
if ezdxf.__version__ != '1.4.5': raise SystemExit('requires ezdxf 1.4.5')
root.mkdir(parents=True,exist_ok=True)
if any((root/(m+'.dxf')).exists() for m in ('line','bulge','width','closed','tilt','rich','constant')): raise SystemExit('refusing to overwrite source')
for mode in ('line','bulge','width','closed','tilt','rich','constant'):
 d=ezdxf.new('R2007');m=d.modelspace()
 if mode=='line':m.add_line((1,2,3),(4,5,6))
 else:
  a=[0,0,0.,0.,0.];b=[5,0,0.,0.,0.];c=[5,3,0.,0.,0.]
  if mode in ('bulge','rich'): a[4]=.25;b[4]=-.5
  if mode in ('width','rich'): a[2:4]=[.2,.4];b[2:4]=[.4,.1]
  attrs={}
  if mode in ('closed','rich'):attrs['flags']=129
  if mode in ('tilt','rich'):attrs.update(elevation=2,thickness=.5,extrusion=(0,1,0))
  if mode=='constant':attrs['const_width']=.6
  m.add_lwpolyline([a,b,c],format='xyseb',close=mode in ('closed','rich'),dxfattribs=attrs)
 d.saveas(root/(mode+'.dxf'))
