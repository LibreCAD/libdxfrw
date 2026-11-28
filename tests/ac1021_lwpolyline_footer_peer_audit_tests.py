#!/usr/bin/env python3
"""Fixture-free negative checks for the LWPOLYLINE footer peer audit."""
import copy
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from audit_ac1021_lwpolyline_footer import check_dwg_polyline


class Checks(unittest.TestCase):
    def setUp(self):
        self.indexed = {18: {'name': '0'}}
        self.value = {'entmode': 2, 'layer': [5, 1, 18, 18], 'flag': 827,
                      'points': [[0., 0.], [5., 0.], [5., 3.]],
                      'bulges': [.25, -.5, 0.],
                      'widths': [{'start': .2, 'end': .4}, {'start': .4, 'end': .1}],
                      'elevation': 2., 'thickness': .5, 'extrusion': [0., 1., 0.]}

    def test_graph_and_flags(self):
        check_dwg_polyline(self.value, self.indexed, 'rich')
        for key, value in [('entmode', 1), ('layer', [4, 1, 18, 18]), ('flag', 315)]:
            bad = copy.deepcopy(self.value); bad[key] = value
            with self.assertRaises((ValueError, KeyError)):
                check_dwg_polyline(bad, self.indexed, 'rich')

    def test_vertex_fields(self):
        changes = [('points', [[0., 0.], [5., 0.], [5., 4.]]),
                   ('bulges', [.25, .5, 0.]),
                   ('widths', [{'start': .2, 'end': .4}, {'start': .4, 'end': .2}]),
                   ('widths', [{'start': .2, 'end': .4}, {'start': .4, 'end': .1}, {'start': .1, 'end': 0.}])]
        for key, value in changes:
            bad = copy.deepcopy(self.value); bad[key] = value
            with self.assertRaises((ValueError, KeyError)):
                check_dwg_polyline(bad, self.indexed, 'rich')

    def test_elevation_and_normal(self):
        for key, value in [('elevation', 0.), ('thickness', 0.), ('extrusion', [0., 0., 1.])]:
            bad = copy.deepcopy(self.value); bad[key] = value
            with self.assertRaises((ValueError, KeyError)):
                check_dwg_polyline(bad, self.indexed, 'rich')


if __name__ == '__main__':
    unittest.main()
