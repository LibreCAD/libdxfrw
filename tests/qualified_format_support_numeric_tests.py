#!/usr/bin/env python3
"""Dependency-free numeric admission controls using real frozen evidence.

No authority, committed-file, route, or evidence-crosswalk function is mocked.
An invalid tolerance must fail its numeric check, while a valid changed number
must still fail the immutable evidence crosswalk. These are rejection-stage
controls, not proof that the baseline admitted a modified frozen claim.
"""

import copy
import hashlib
import math
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import check_qualified_format_support as support


class QualificationNumericTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.claims = support._read_json(support.DEFAULT_CLAIMS)
        cls.inputs = support._read_json(support.DEFAULT_INPUTS)
        cls.routes = support._read_json(support.DEFAULT_ROUTES)
        cls.matrix = support._read_json(support.DEFAULT_MATRIX)
        cls.bound_files = {
            support.DEFAULT_CLAIMS, support.DEFAULT_STATUS,
            support.DEFAULT_INPUTS, support.DEFAULT_ROUTES, support.DEFAULT_MATRIX,
        }
        for row in cls.claims["evidence"]:
            cls.bound_files.add(support.ROOT / row["artifactPath"])
            cls.bound_files.add(support.ROOT / row["checkerPath"])
        cls.original_hashes = cls.hashes()

    @classmethod
    def hashes(cls):
        return {str(path): hashlib.sha256(path.read_bytes()).hexdigest()
                for path in cls.bound_files}

    @classmethod
    def tearDownClass(cls):
        if cls.hashes() != cls.original_hashes:
            raise AssertionError("numeric controls changed frozen authority files")

    def validate(self, claims):
        return support.validate_claims(
            claims, root=support.ROOT, inputs=self.inputs,
            routes=self.routes, matrix=self.matrix, allow_draft=False,
        )

    def changed_tolerance(self, normalization, tolerance):
        claims = copy.deepcopy(self.claims)
        claim = next(row for row in claims["claims"]
                     if row["normalization"] == normalization)
        claim["tolerance"] = tolerance
        return claims

    def test_frozen_claims_still_validate(self):
        by_id, by_route = self.validate(self.claims)
        self.assertEqual(len(by_id), 15)
        self.assertEqual(len(by_route), 2)

    def test_json_numbers_are_finite_at_every_depth(self):
        invalid = [
            '{"number":1e999}', '{"number":-1e999}',
            '{"nested":{"array":[0,{"number":1e999}]}}',
            '{"nested":{"array":[0,{"number":-1e999}]}}',
            '{"number":NaN}', '{"number":Infinity}', '{"number":-Infinity}',
        ]
        with tempfile.TemporaryDirectory(prefix="qualification numeric ") as directory:
            path = Path(directory) / "input.json"
            for payload in invalid:
                with self.subTest(payload=payload):
                    path.write_text(payload, encoding="utf-8")
                    with self.assertRaisesRegex(support.SupportError, "non-finite JSON"):
                        support._read_json(path)
            path.write_text('{"zero":0,"tiny":1e-12,"nested":[-0.0,1.0,1e308]}',
                            encoding="utf-8")
            result = support._read_json(path)
            self.assertIs(type(result["zero"]), int)
            self.assertEqual(result["tiny"], 1e-12)
            self.assertEqual(result["nested"], [-0.0, 1.0, 1e308])
            self.assertLess(math.copysign(1.0, result["nested"][0]), 0)

    def test_json_duplicate_and_object_contracts_unchanged(self):
        with tempfile.TemporaryDirectory(prefix="qualification numeric ") as directory:
            path = Path(directory) / "input.json"
            for payload, error in [
                ('{"nested":{"key":0,"key":1}}', "duplicate JSON key"),
                ('[0,1]', "must contain one JSON object"),
            ]:
                with self.subTest(payload=payload):
                    path.write_text(payload, encoding="utf-8")
                    with self.assertRaisesRegex(support.SupportError, error):
                        support._read_json(path)

    def test_float_tolerance_invalid_values_reach_numeric_rejection(self):
        invalid = [False, True, float("nan"), float("inf"), -float("inf"),
                   -1, "1e-12", None, [], {}, 10 ** 400]
        for key in ("absolute", "relative"):
            for value in invalid:
                with self.subTest(key=key, value=repr(value)):
                    tolerance = {"absolute": 1e-12, "relative": 1e-12}
                    tolerance[key] = value
                    with self.assertRaisesRegex(support.SupportError, "invalid float tolerance"):
                        self.validate(self.changed_tolerance("floatTolerance", tolerance))

    def test_float_tolerance_shape_is_exact(self):
        for tolerance in (None, [], 0, {"absolute": 1e-12},
                          {"absolute": 1e-12, "relative": 1e-12, "extra": 0}):
            with self.subTest(tolerance=tolerance):
                with self.assertRaisesRegex(support.SupportError, "invalid float tolerance"):
                    self.validate(self.changed_tolerance("floatTolerance", tolerance))

    def test_finite_float_tolerance_mismatch_still_reaches_crosswalk(self):
        for key in ("absolute", "relative"):
            for value in (0, 0.0, 1.0, 1e308):
                with self.subTest(key=key, value=value):
                    tolerance = {"absolute": 1e-12, "relative": 1e-12}
                    tolerance[key] = value
                    with self.assertRaisesRegex(support.SupportError, "does not exactly crosswalk"):
                        self.validate(self.changed_tolerance("floatTolerance", tolerance))

    def test_scalar_tolerance_contracts_remain_finite_and_nonboolean(self):
        for normalization in ("finiteNumber", "degreesFromRadians"):
            for value in (False, True, float("nan"), float("inf"), -float("inf"),
                          -1, "0", None, [], {}, 10 ** 400):
                with self.subTest(normalization=normalization, value=repr(value)):
                    with self.assertRaisesRegex(support.SupportError, "invalid numeric tolerance"):
                        self.validate(self.changed_tolerance(normalization, value))

    def test_finite_scalar_mismatch_still_reaches_crosswalk(self):
        for normalization in ("finiteNumber", "degreesFromRadians"):
            original = next(row["tolerance"] for row in self.claims["claims"]
                            if row["normalization"] == normalization)
            for value in (0, 1e-12, 1.0):
                if value == original:
                    continue
                with self.subTest(normalization=normalization, value=value):
                    with self.assertRaisesRegex(support.SupportError, "does not exactly crosswalk"):
                        self.validate(self.changed_tolerance(normalization, value))


if __name__ == "__main__":
    unittest.main()
