import os
import sys
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "..", "training"))

from softmax_classifier import SoftmaxClassifier
from local_update import compute_local_delta, apply_delta


def _toy_model(num_features=4, num_classes=3):
    m = SoftmaxClassifier(num_features, num_classes)
    return m.to_dict()


def _toy_batch(num_features=4, n=10):
    X = [[float(i % 3), float((i + 1) % 3), 0.5, 1.0] for i in range(n)]
    y = [i % 3 for i in range(n)]
    return X, y


class TestComputeLocalDelta(unittest.TestCase):
    def test_returns_delta_shaped_like_model(self):
        model = _toy_model()
        X, y = _toy_batch()
        delta = compute_local_delta(model, X, y)
        self.assertEqual(len(delta["weights"]), model["num_classes"])
        self.assertEqual(len(delta["weights"][0]), model["num_features"])
        self.assertEqual(len(delta["bias"]), model["num_classes"])

    def test_does_not_mutate_input_model(self):
        model = _toy_model()
        import copy
        before = copy.deepcopy(model)
        X, y = _toy_batch()
        compute_local_delta(model, X, y)
        self.assertEqual(model, before)

    def test_rejects_empty_batch(self):
        model = _toy_model()
        with self.assertRaises(ValueError):
            compute_local_delta(model, [], [])

    def test_rejects_mismatched_lengths(self):
        model = _toy_model()
        X, y = _toy_batch()
        with self.assertRaises(ValueError):
            compute_local_delta(model, X, y[:-1])

    def test_nonzero_batch_produces_a_nonzero_delta(self):
        model = _toy_model()
        X, y = _toy_batch()
        delta = compute_local_delta(model, X, y, epochs=3)
        moved = sum(abs(v) for row in delta["weights"] for v in row) + sum(abs(v) for v in delta["bias"])
        self.assertGreater(moved, 0.0)


class TestApplyDelta(unittest.TestCase):
    def test_apply_then_diff_recovers_delta(self):
        model = _toy_model()
        X, y = _toy_batch()
        delta = compute_local_delta(model, X, y)
        updated = apply_delta(model, delta)
        for c in range(model["num_classes"]):
            for j in range(model["num_features"]):
                self.assertAlmostEqual(
                    updated["weights"][c][j] - model["weights"][c][j],
                    delta["weights"][c][j], places=9)

    def test_apply_delta_does_not_mutate_input(self):
        model = _toy_model()
        import copy
        before = copy.deepcopy(model)
        zero_delta = {"weights": [[0.0] * model["num_features"]] * model["num_classes"],
                      "bias": [0.0] * model["num_classes"]}
        apply_delta(model, zero_delta)
        self.assertEqual(model, before)


if __name__ == "__main__":
    unittest.main()
