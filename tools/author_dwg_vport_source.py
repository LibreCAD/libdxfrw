#!/usr/bin/env python3
import argparse,ezdxf
p=argparse.ArgumentParser();p.add_argument('output');p.add_argument('--tilt',action='store_true');a=p.parse_args();d=ezdxf.new('R2013');v=d.viewports.get('*Active')[0];v.dxf.snap_rotation=30.;v.dxf.view_twist=45.
if a.tilt:v.dxf.direction=(1.,2.,3.);v.dxf.target=(4.,5.,6.)
d.modelspace().add_line((0,0),(10,10));d.saveas(a.output)
