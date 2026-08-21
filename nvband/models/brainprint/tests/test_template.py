import os
import sys
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))

from template import epoch_to_features, build_template, TEMPLATE_LEN


class TestEpochToFeatures(unittest.TestCase):
    def test_normalizes_per_channel(self):
        bp = [[10, 10, 10, 10, 10], [4, 0, 0, 0, 0], [1, 1, 1, 1, 1], [2, 2, 2, 2, 2]]
        feat = epoch_to_features(bp)
        self.assertEqual(len(feat), TEMPLATE_LEN)
        self.assertAlmostEqual(sum(feat[0:5]), 1.0, places=6)
        self.assertAlmostEqual(feat[5], 1.0, places=6)  # ch1 all in band0

    def test_rejects_flat_channel(self):
        bp = [[0, 0, 0, 0, 0], [1, 1, 1, 1, 1], [1, 1, 1, 1, 1], [1, 1, 1, 1, 1]]
        with self.assertRaises(ValueError):
            epoch_to_features(bp)

    def test_rejects_negative_power(self):
        bp = [[-1, 1, 1, 1, 1], [1, 1, 1, 1, 1], [1, 1, 1, 1, 1], [1, 1, 1, 1, 1]]
        with self.assertRaises(ValueError):
            epoch_to_features(bp)

    def test_rejects_wrong_shape(self):
        with self.assertRaises(ValueError):
            epoch_to_features([[1, 1, 1, 1, 1]])  # only 1 channel


class TestBuildTemplate(unittest.TestCase):
    def test_averages(self):
        f = [0.05 * i for i in range(1, TEMPLATE_LEN + 1)]
        epochs = [f, f, f]
        template = build_template(epochs)
        self.assertAlmostEqual(template[0], f[0], places=6)

    def test_enforces_epoch_bounds(self):
        f = [0.05] * TEMPLATE_LEN
        with self.assertRaises(ValueError):
            build_template([f, f])  # below default min of 3
        with self.assertRaises(ValueError):
            build_template([f] * 20)  # above default max of 16

    def test_rejects_wrong_length_feature(self):
        with self.assertRaises(ValueError):
            build_template([[0.1, 0.2], [0.1, 0.2], [0.1, 0.2]])


if __name__ == "__main__":
    unittest.main()
