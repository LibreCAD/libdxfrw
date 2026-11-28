#!/usr/bin/env python3
"""Author the seven standard dimension subtype geometry used by the probe."""
import argparse, ezdxf
p=argparse.ArgumentParser();p.add_argument('output');a=p.parse_args()
d=ezdxf.new('R2013');m=d.modelspace()
values=[m.add_linear_dim(base=(2,4),p1=(0,0),p2=(5,0),angle=30),m.add_aligned_dim(p1=(0,0),p2=(5,0),distance=2),m.add_radius_dim(center=(3,3),radius=2,angle=30),m.add_diameter_dim(center=(3,3),radius=2,angle=30),m.add_angular_dim_3p(base=(4,4),center=(0,0),p1=(5,0),p2=(0,5)),m.add_angular_dim_2l(base=(4,4),line1=((0,0),(5,0)),line2=((0,0),(0,5))),m.add_ordinate_x_dim((3,2),(2,2))]
for entry in values:
 entry.render();entry.dimension.dxf.text_rotation=45;entry.dimension.dxf.horizontal_direction=15
values[1].dimension.dxf.dimtype=33
values[0].dimension.dxf.oblique_angle=20;values[1].dimension.dxf.oblique_angle=25
d.saveas(a.output)
