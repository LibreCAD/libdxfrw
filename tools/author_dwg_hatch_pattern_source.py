#!/usr/bin/env python3
import argparse,ezdxf
p=argparse.ArgumentParser();p.add_argument('output');p.add_argument('--mpolygon',action='store_true');a=p.parse_args()
d=ezdxf.new('R2013');m=d.modelspace();h=m.add_mpolygon() if a.mpolygon else m.add_hatch();h.paths.add_polyline_path([(0,0),(5,0),(5,3),(0,3)],is_closed=True);h.set_pattern_fill('ANSI31',color=7,angle=30.,scale=1.);d.saveas(a.output)
