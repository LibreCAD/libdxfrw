#!/usr/bin/env python3
import argparse,ezdxf
p=argparse.ArgumentParser();p.add_argument('output');p.add_argument('--tilt',action='store_true');a=p.parse_args();d=ezdxf.new('R2013');v=d.views.new('ANGLECHECK');v.dxf.view_twist=45.;v.dxf.height=10.;v.dxf.width=15.;v.dxf.direction=(1.,2.,3.) if a.tilt else (0.,0.,1.);v.dxf.target=(4.,5.,6.) if a.tilt else (0.,0.,0.);d.modelspace().add_line((0,0),(10,10));d.saveas(a.output)
