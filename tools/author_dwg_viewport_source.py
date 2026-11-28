#!/usr/bin/env python3
import argparse,ezdxf
p=argparse.ArgumentParser();p.add_argument('output');p.add_argument('--tilt',action='store_true');a=p.parse_args();d=ezdxf.new('R2013');d.modelspace().add_line((0,0),(10,10));v=d.layouts.get('Layout1').add_viewport((5,5),(10,8),(0,0),10);v.dxf.snap_angle=30.;v.dxf.view_twist_angle=45.;v.dxf.view_direction_vector=(1.,2.,3.) if a.tilt else (0.,0.,1.);v.dxf.view_target_point=(4.,5.,6.) if a.tilt else (0.,0.,0.);d.saveas(a.output)
