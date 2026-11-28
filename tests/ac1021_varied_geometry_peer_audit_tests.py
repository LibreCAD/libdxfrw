#!/usr/bin/env python3
"""Fixture-free negative controls for the AC1021 varied-geometry peer audit."""
import copy
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from audit_ac1021_varied_geometry_peer import check_dwg_entity


class GeometryChecks(unittest.TestCase):
    def setUp(self):
        self.objects = {18: {'name': '0'}, 19: {'name': 'STANDARD'}}
        self.common = {'entmode': 2, 'layer': [5, 1, 18, 18]}

    def check_changed(self, obj, changes):
        check_dwg_entity(obj, self.objects)
        for key, value in changes:
            bad = copy.deepcopy(obj); bad[key] = value
            with self.assertRaises((ValueError, KeyError)):
                check_dwg_entity(bad, self.objects)

    def test_line(self):
        obj = dict(self.common, entity='LINE', start=[1.,2.,3.], end=[4.,5.,6.])
        self.check_changed(obj, [('start',[1.,2.,0.]), ('end',[4.,5.,7.]),
                                 ('layer',[4,1,18,18]), ('entmode',1)])

    def test_ellipse(self):
        obj = dict(self.common, entity='ELLIPSE', center=[2.,3.,4.], sm_axis=[4.,0.,0.],
                   extrusion=[0.,0.,1.], axis_ratio=.5, start_angle=.2, end_angle=2.4)
        self.check_changed(obj, [('center',[2.,3.,0.]), ('sm_axis',[4.,1.,0.]),
                                 ('axis_ratio',.7), ('start_angle',.3),
                                 ('end_angle',2.5), ('extrusion',[0.,1.,0.])])

    def test_polyline(self):
        obj = dict(self.common, entity='LWPOLYLINE', points=[[0.,0.],[5.,0.],[5.,3.]],
                   flag=8, elevation=2., bulges=[])
        self.check_changed(obj, [('points',[[0.,0.],[5.,0.],[5.,4.]]), ('flag',0x208),
                                 ('elevation',3.), ('bulges',[.1])])

    def test_text(self):
        obj = dict(self.common, entity='TEXT', ins_pt=[3.,4.], elevation=0., height=2.,
                   rotation=0.5235987755982988, text_value='HELLO', style=[5,1,19,19])
        self.check_changed(obj, [('text_value','BYE'), ('height',3.), ('rotation',0.),
                                 ('style',[4,1,19,19]), ('horiz_alignment',1),
                                 ('extrusion',[0.,1.,0.])])
        missing = copy.deepcopy(obj); del missing['height']
        with self.assertRaises(KeyError):
            check_dwg_entity(missing, self.objects)
        obj['style']=[5,1,19,19]; self.objects[19]['name']='Standard'
        check_dwg_entity(obj, self.objects)


if __name__ == '__main__':
    unittest.main()
