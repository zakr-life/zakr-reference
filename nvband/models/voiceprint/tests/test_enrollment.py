"""Traces: Addendum 2 §B enrollment protocol ("3 spoken utterances ...
averaged into one template")."""
import os
import sys
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
from enrollment import enroll_template, REQUIRED_ENROLLMENT_UTTERANCES  # noqa: E402
from synthetic_voice import FEATURE_DIM  # noqa: E402


class TestEnrollment(unittest.TestCase):
    def test_requires_exactly_three_utterances(self):
        self.assertEqual(REQUIRED_ENROLLMENT_UTTERANCES, 3)

    def test_averages_elementwise(self):
        u1 = [1.0] * FEATURE_DIM
        u2 = [2.0] * FEATURE_DIM
        u3 = [3.0] * FEATURE_DIM
        template = enroll_template([u1, u2, u3])
        self.assertEqual(len(template), FEATURE_DIM)
        for v in template:
            self.assertAlmostEqual(v, 2.0)

    def test_averages_per_dimension_independently(self):
        u1 = [0.0] * FEATURE_DIM
        u2 = [0.0] * FEATURE_DIM
        u3 = [0.0] * FEATURE_DIM
        u1[0] = 3.0
        u2[0] = 6.0
        u3[0] = 9.0
        template = enroll_template([u1, u2, u3])
        self.assertAlmostEqual(template[0], 6.0)
        self.assertAlmostEqual(template[1], 0.0)

    def test_rejects_too_few_utterances(self):
        u = [0.0] * FEATURE_DIM
        with self.assertRaises(ValueError):
            enroll_template([u, u])

    def test_rejects_too_many_utterances(self):
        u = [0.0] * FEATURE_DIM
        with self.assertRaises(ValueError):
            enroll_template([u, u, u, u])

    def test_rejects_empty_list(self):
        with self.assertRaises(ValueError):
            enroll_template([])

    def test_rejects_wrong_length_vector(self):
        good = [0.0] * FEATURE_DIM
        bad = [0.0] * (FEATURE_DIM - 1)
        with self.assertRaises(ValueError):
            enroll_template([good, good, bad])


if __name__ == "__main__":
    unittest.main()
