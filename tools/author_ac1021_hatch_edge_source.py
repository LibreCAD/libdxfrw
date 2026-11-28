import ezdxf
from pathlib import Path
root=Path('/private/tmp/ac1021-hatch-edges/source-in')
for mode in ('ellipse','arc','linear'):
 d=ezdxf.new('R2007');m=d.modelspace();h=m.add_hatch();p=h.paths.add_edge_path(flags=1)
 if mode=='ellipse':p.add_ellipse(center=(3,4),major_axis=(4,0),ratio=.5,start_angle=0,end_angle=360,ccw=True)
 elif mode=='arc':p.add_arc(center=(3,4),radius=2,start_angle=0,end_angle=360,ccw=True)
 else:
  pts=[(0,0),(5,0),(5,3),(0,3)]
  for a,b in zip(pts,pts[1:]+pts[:1]):p.add_line(a,b)
 m.add_line((10,11,0),(12,13,0));d.saveas(root/(mode+'.dxf'))
