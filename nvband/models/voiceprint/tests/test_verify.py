"""Traces: Addendum 2 §B ("1:1 verification ... never 1:N identification/
surveillance against a population")."""
import os
import sys
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
from synthetic_voice import generate_subjects, generate_utterances, FEATURE_DIM  # noqa: E402
from enrollment import enroll_template  # noqa: E402
from verify import cosine_similarity, verify_1to1, DEFAULT_VERIFICATION_THRESHOLD  # noqa: E402
import verify as verify_module  # noqa: E402


class TestCosineSimilarity(unittest.TestCase):
    def test_identical_vectors_have_similarity_one(self):
        v = [1.0, 2.0, 3.0]
        self.assertAlmostEqual(cosine_similarity(v, v), 1.0)

    def test_orthogonal_vectors_have_similarity_zero(self):
        self.assertAlmostEqual(cosine_similarity([1.0, 0.0], [0.0, 1.0]), 0.0)

    def test_opposite_vectors_have_similarity_negative_one(self):
        self.assertAlmostEqual(cosine_similarity([1.0, 1.0], [-1.0, -1.0]), -1.0)

    def test_zero_vector_is_defined_as_zero_similarity_not_a_crash(self):
        self.assertEqual(cosine_similarity([0.0, 0.0], [1.0, 1.0]), 0.0)

    def test_rejects_mismatched_lengths(self):
        with self.assertRaises(ValueError):
            cosine_similarity([1.0, 2.0], [1.0])


class TestVerify1to1(unittest.TestCase):
    def setUp(self):
        self.subjects = generate_subjects(num_subjects=8, seed=555)
        self.utterances = generate_utterances(self.subjects, utterances_per_subject=5, seed=556)
        self.by_subject = {}
        for u in self.utterances:
            self.by_subject.setdefault(u.subject_id, []).append(u.features)

    def test_same_subject_probe_matches_above_threshold(self):
        subject_id = next(iter(self.by_subject))
        feats = self.by_subject[subject_id]
        template = enroll_template(feats[:3])
        genuine_probe = feats[3]

        result = verify_1to1(template, genuine_probe)

        self.assertGreaterEqual(result["similarity"], DEFAULT_VERIFICATION_THRESHOLD)
        self.assertTrue(result["match"])

    def test_different_subject_probe_is_below_threshold(self):
        subject_ids = list(self.by_subject.keys())
        template = enroll_template(self.by_subject[subject_ids[0]][:3])
        impostor_probe = self.by_subject[subject_ids[1]][0]

        result = verify_1to1(template, impostor_probe)

        self.assertLess(result["similarity"], DEFAULT_VERIFICATION_THRESHOLD)
        self.assertFalse(result["match"])

    def test_result_reports_the_threshold_used(self):
        subject_id = next(iter(self.by_subject))
        template = enroll_template(self.by_subject[subject_id][:3])
        probe = self.by_subject[subject_id][3]
        result = verify_1to1(template, probe, threshold=0.999)
        self.assertEqual(result["threshold"], 0.999)
        self.assertFalse(result["match"])  # 0.999 is an unreasonably strict bar

    def test_rejects_wrong_dimensional_vectors(self):
        with self.assertRaises(ValueError):
            verify_1to1([0.0] * (FEATURE_DIM - 1), [0.0] * FEATURE_DIM)


class TestNoOneToManySearchFunctionExists(unittest.TestCase):
    """Structural test of the Addendum 2 §B invariant: this module must
    never grow a 1:N identification/search function."""

    def test_verify_module_exposes_no_multi_template_search_function(self):
        suspicious_names = [
            name for name in dir(verify_module)
            if any(keyword in name.lower() for keyword in
                   ("identify", "search", "rank", "match_all", "find_speaker"))
        ]
        self.assertEqual(suspicious_names, [],
                          f"found suspicious 1:N-shaped function name(s): {suspicious_names}")


if __name__ == "__main__":
    unittest.main()
