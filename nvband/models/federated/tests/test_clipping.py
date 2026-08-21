import os
import sys
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))

from clipping import clip_delta, add_calibrated_noise, delta_l2_norm


def _delta(scale):
    return {"weights": [[scale, scale], [scale, scale]], "bias": [scale, scale]}


class TestClipDelta(unittest.TestCase):
    def test_small_delta_passes_through_unchanged(self):
        d = _delta(0.01)
        clipped = clip_delta(d, max_l2_norm=10.0)
        self.assertEqual(clipped, d)

    def test_large_delta_is_bounded(self):
        d = _delta(100.0)
        clipped = clip_delta(d, max_l2_norm=1.0)
        self.assertAlmostEqual(delta_l2_norm(clipped), 1.0, places=5)

    def test_never_scales_up(self):
        d = _delta(0.001)
        clipped = clip_delta(d, max_l2_norm=1.0)
        self.assertLessEqual(delta_l2_norm(clipped), delta_l2_norm(d) + 1e-9)

    def test_all_zero_delta_stays_zero(self):
        d = _delta(0.0)
        clipped = clip_delta(d, max_l2_norm=1.0)
        self.assertEqual(delta_l2_norm(clipped), 0.0)

    def test_rejects_non_positive_bound(self):
        with self.assertRaises(ValueError):
            clip_delta(_delta(1.0), max_l2_norm=0.0)


class TestNoise(unittest.TestCase):
    def test_zero_scale_is_a_no_op(self):
        d = _delta(1.0)
        noised = add_calibrated_noise(d, noise_scale=0.0)
        self.assertEqual(noised, d)

    def test_positive_scale_changes_values_but_stays_bounded(self):
        d = _delta(1.0)
        noised = add_calibrated_noise(d, noise_scale=0.05, seed=1)
        self.assertNotEqual(noised, d)
        # bounded magnitude check: noise shouldn't blow up an already-small delta
        self.assertLess(delta_l2_norm(noised), delta_l2_norm(d) + 2.0)

    def test_rejects_negative_scale(self):
        with self.assertRaises(ValueError):
            add_calibrated_noise(_delta(1.0), noise_scale=-0.1)


if __name__ == "__main__":
    unittest.main()
