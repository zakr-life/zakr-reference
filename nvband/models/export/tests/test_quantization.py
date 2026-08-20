"""Traces: int8 quantization correctness and export budget gate (CLAUDE.md §4)."""
import os
import sys
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "..", "training"))
from export_model import (quantize_symmetric_int8, quantize_model,  # noqa: E402
                           dequantized_predict, estimate_model_size_kb,
                           MAX_MODEL_SIZE_KB, MAX_INFERENCE_LATENCY_MS)
from softmax_classifier import SoftmaxClassifier  # noqa: E402


class TestQuantization(unittest.TestCase):
    def test_int8_range_never_exceeded(self):
        values = [-500.0, -1.0, 0.0, 0.5, 999.0]
        q, scale = quantize_symmetric_int8(values)
        self.assertTrue(all(-127 <= v <= 127 for v in q))
        self.assertGreater(scale, 0)

    def test_all_zero_values_handled_without_div_by_zero(self):
        q, scale = quantize_symmetric_int8([0.0, 0.0, 0.0])
        self.assertEqual(q, [0, 0, 0])
        self.assertEqual(scale, 1.0)

    def test_dequantized_roundtrip_close_to_original(self):
        values = [1.0, -2.5, 0.75, 3.3]
        q, scale = quantize_symmetric_int8(values)
        dequant = [v * scale for v in q]
        for orig, dq in zip(values, dequant):
            self.assertLess(abs(orig - dq), 0.05)

    def test_quantized_model_predictions_mostly_match_float_model(self):
        import random
        rng = random.Random(0)
        model = SoftmaxClassifier(num_features=4, num_classes=3)
        X = [[rng.uniform(0, 2) for _ in range(4)] for _ in range(80)]
        y = [rng.randrange(3) for _ in range(80)]
        model.train(X, y, epochs=30)

        quantized = quantize_model(model)

        agree = 0
        for x in X:
            float_pred = model.predict(x)
            quant_pred = dequantized_predict(quantized, x)
            if float_pred == quant_pred:
                agree += 1
        # Quantization should preserve the decision in the large majority
        # of cases on the data the model was actually trained on.
        self.assertGreater(agree / len(X), 0.85)

    def test_model_size_estimate_is_positive_and_reasonable(self):
        model = SoftmaxClassifier(num_features=20, num_classes=3)
        quantized = quantize_model(model)
        size_kb = estimate_model_size_kb(quantized)
        self.assertGreater(size_kb, 0)
        self.assertLess(size_kb, MAX_MODEL_SIZE_KB)  # this tiny model must fit

    def test_budgets_are_positive_named_constants(self):
        self.assertGreater(MAX_MODEL_SIZE_KB, 0)
        self.assertGreater(MAX_INFERENCE_LATENCY_MS, 0)


if __name__ == "__main__":
    unittest.main()
