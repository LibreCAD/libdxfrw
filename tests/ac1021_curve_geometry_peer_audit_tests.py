#!/usr/bin/env python3
"""Fixture-free negative controls for the AC1021 curve peer audit."""
import copy
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from audit_ac1021_curve_geometry_peer import check_dwg_entity


class CurveChecks(unittest.TestCase):
    def setUp(self):
        self.objects={18:{'name':'0'}}
        self.common={'entmode':2,'layer':[5,1,18,18],'handle':[0,1,54]}

    def check_changed(self,obj,changes):
        check_dwg_entity(obj,self.objects,3)
        for key,value in changes:
            bad=copy.deepcopy(obj); bad[key]=value
            with self.assertRaises((ValueError,KeyError)):
                check_dwg_entity(bad,self.objects,3)

    def test_spline(self):
        obj=dict(self.common,entity='SPLINE',scenario=1,degree=2,rational=0,weighted=0,
                 knots=[0,0,0,1,1,1],ctrl_pts=[{'x':0,'y':0,'z':0},{'x':2,'y':4,'z':0},{'x':5,'y':0,'z':0}])
        self.check_changed(obj,[('degree',3),('weighted',1),('knots',[0,0,0,1,1,.9]),('ctrl_pts',[])])

    def test_hatch(self):
        path={'flag':2,'closed':1,'polyline_paths':[{'point':[0,0]},{'point':[5,0]},
              {'point':[5,3]},{'point':[0,3]}],'boundary_handles':[]}
        obj=dict(self.common,entity='HATCH',name='SOLID',is_solid_fill=1,is_associative=0,paths=[path])
        self.check_changed(obj,[('is_solid_fill',0),('is_associative',1),('paths',[])])
        bad=copy.deepcopy(obj);bad['paths'][0]['polyline_paths'][2]['point']=[6,3]
        with self.assertRaises(ValueError):check_dwg_entity(bad,self.objects,3)

    def test_polyline(self):
        obj=dict(self.common,entity='POLYLINE_2D',vertex=[[3,1,55,55],[3,1,56,56],[3,1,57,57]],seqend=[3,1,58,58])
        for i,point in enumerate(((0,0,0),(5,0,0),(5,3,0)),55):
            self.objects[i]={'entity':'VERTEX_2D','entmode':0,'ownerhandle':[4,1,54,54],'point':point}
        self.objects[58]={'entity':'SEQEND','entmode':0,'ownerhandle':[4,1,54,54]}
        check_dwg_entity(obj,self.objects,3)
        with self.assertRaises(ValueError):check_dwg_entity(obj,self.objects,5)
        self.check_changed(obj,[('seqend',[5,1,58,58]),('vertex',obj['vertex'][:2])])
        self.objects[56]['ownerhandle']=[4,1,55,55]
        with self.assertRaises(ValueError):check_dwg_entity(obj,self.objects,3)


if __name__=='__main__':unittest.main()
