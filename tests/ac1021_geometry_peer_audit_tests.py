#!/usr/bin/env python3
"""Fixture-free geometry and OCS basis controls for the AC1021 peer audit."""
from pathlib import Path
import math
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from audit_ac1021_geometry_peer import (MODES, NEGATIVE, STANDARD, TILT, check_fields,
                                      close, match_expected, wcs)


class GeometryAuditTests(unittest.TestCase):
    def test_basis_and_expected_fields(self):
        self.assertEqual((7., 8., 2.), wcs((7., 8., 2.), STANDARD))
        self.assertEqual((-7., 2., 8.), wcs((7., 8., 2.), TILT))
        self.assertEqual((-7., 8., -2.), wcs((7., 8., 2.), NEGATIVE))
        check_fields("ARC", TILT, .5, (10., 11., 3.), 4., math.radians(20), math.radians(115))
        for values in (((10., 11., 4.), 4., 20., 115.),
                       ((10., 11., 3.), 5., 20., 115.),
                       ((10., 11., 3.), 4., 21., 115.),
                       ((10., 11., 3.), 4., 20., 114.)):
            with self.assertRaises(ValueError):
                check_fields("ARC", TILT, .5, values[0], values[1],
                             math.radians(values[2]), math.radians(values[3]))
        with self.assertRaises(ValueError):
            close((1., 2.), (1., 2., 3.), "short vector")

    def test_inventory_and_normal(self):
        line = {"kind": "LINE", "normal": STANDARD}
        self.assertEqual(1, len(match_expected([line], MODES["line"],
                         lambda x: x["kind"], lambda x: x["normal"])))
        for objects in ([], [line, line], [{"kind": "LINE", "normal": TILT}]):
            with self.assertRaises(ValueError):
                match_expected(objects, MODES["line"], lambda x: x["kind"], lambda x: x["normal"])


if __name__ == "__main__":
    unittest.main()
