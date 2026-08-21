import os
import sys
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "..", "training"))

from softmax_classifier import SoftmaxClassifier
from federated_round import federated_average, run_round, _simulate_device_batch
from clipping import delta_l2_norm


def _toy_model(num_features=20, num_classes=3):
    return SoftmaxClassifier(num_features, num_classes).to_dict()


def _delta(scale, num_features=20, num_classes=3):
    return {"weights": [[scale] * num_features for _ in range(num_classes)],
            "bias": [scale] * num_classes}


class TestFederatedAverage(unittest.TestCase):
    def test_averages_multiple_deltas(self):
        deltas = [_delta(1.0), _delta(3.0)]
        avg = federated_average(deltas)
        self.assertAlmostEqual(avg["weights"][0][0], 2.0, places=9)
        self.assertAlmostEqual(avg["bias"][0], 2.0, places=9)

    def test_refuses_empty_list(self):
        with self.assertRaises(ValueError):
            federated_average([])

    def test_average_of_clipped_deltas_is_itself_bounded(self):
        from clipping import clip_delta
        deltas = [clip_delta(_delta(50.0), max_l2_norm=1.0) for _ in range(5)]
        avg = federated_average(deltas)
        # mean of same-magnitude clipped deltas should not exceed the per-delta bound
        self.assertLessEqual(delta_l2_norm(avg), 1.0 + 1e-6)


class TestSimulateDeviceBatch(unittest.TestCase):
    def test_produces_matched_length_X_y(self):
        X, y = _simulate_device_batch(seed=1)
        self.assertEqual(len(X), len(y))
        self.assertGreater(len(X), 0)


class TestRunRound(unittest.TestCase):
    def test_produces_a_model_shaped_candidate(self):
        model = _toy_model()
        candidate = run_round(model, num_devices=4)
        self.assertEqual(candidate["num_features"], model["num_features"])
        self.assertEqual(candidate["num_classes"], model["num_classes"])

    def test_candidate_moves_a_bounded_amount_from_global(self):
        model = _toy_model()
        candidate = run_round(model, num_devices=6, max_delta_l2_norm=0.5)
        moved = delta_l2_norm({
            "weights": [[candidate["weights"][c][j] - model["weights"][c][j]
                         for j in range(model["num_features"])]
                        for c in range(model["num_classes"])],
            "bias": [candidate["bias"][c] - model["bias"][c] for c in range(model["num_classes"])],
        })
        # averaging clipped deltas each bounded at 0.5 should not exceed that bound
        self.assertLessEqual(moved, 0.5 + 1e-6)


if __name__ == "__main__":
    unittest.main()
