import os
import sys
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))

from evaluate_verification import (
    cosine_similarity, run_evaluation,
    assert_no_subject_data_reused_across_enrollment_and_impostor_role,
)


class TestCosineSimilarity(unittest.TestCase):
    def test_identical_vectors(self):
        a = [1.0, 2.0, 3.0]
        self.assertAlmostEqual(cosine_similarity(a, a), 1.0, places=6)

    def test_degenerate_vector_fails_closed(self):
        zero = [0.0, 0.0, 0.0]
        a = [1.0, 2.0, 3.0]
        self.assertEqual(cosine_similarity(zero, a), 0.0)


class TestLeakageGuard(unittest.TestCase):
    def test_same_subject_raises(self):
        with self.assertRaises(AssertionError):
            assert_no_subject_data_reused_across_enrollment_and_impostor_role("A", "A")

    def test_different_subject_ok(self):
        assert_no_subject_data_reused_across_enrollment_and_impostor_role("A", "B")


class TestRunEvaluation(unittest.TestCase):
    def test_produces_bounded_measured_rates(self):
        result = run_evaluation()
        self.assertEqual(result["data_provenance"], "synthetic")
        self.assertGreater(result["genuine_trials"], 0)
        self.assertGreater(result["impostor_trials"], 0)
        self.assertGreaterEqual(result["false_reject_rate"], 0.0)
        self.assertLessEqual(result["false_reject_rate"], 1.0)
        self.assertGreaterEqual(result["false_accept_rate"], 0.0)
        self.assertLessEqual(result["false_accept_rate"], 1.0)


if __name__ == "__main__":
    unittest.main()
