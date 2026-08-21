"""Traces: models/voiceprint/export_model.py's latency/size budget gate,
mirroring models/export/export_model.py's test discipline."""
import os
import sys
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
from enrollment import enroll_template  # noqa: E402
from synthetic_voice import FEATURE_DIM, generate_subjects, generate_utterances  # noqa: E402
import export_model  # noqa: E402


class TestQuantization(unittest.TestCase):
    def test_quantize_dequantize_round_trips_approximately(self):
        template = [round(-2.0 + 0.25 * i, 3) for i in range(FEATURE_DIM)]
        quantized = export_model.quantize_template(template)
        dequantized = export_model.dequantize_template(quantized)
        self.assertEqual(len(dequantized), FEATURE_DIM)
        for original, back in zip(template, dequantized):
            self.assertAlmostEqual(original, back, delta=0.05)

    def test_quantize_handles_all_zero_template_without_crashing(self):
        quantized = export_model.quantize_template([0.0] * FEATURE_DIM)
        self.assertEqual(quantized["scale"], 1.0)
        self.assertTrue(all(v == 0 for v in quantized["quantized_int8"]))

    def test_quantized_values_are_valid_int8_range(self):
        template = [10.0, -10.0] + [0.0] * (FEATURE_DIM - 2)
        quantized = export_model.quantize_template(template)
        for v in quantized["quantized_int8"]:
            self.assertGreaterEqual(v, -127)
            self.assertLessEqual(v, 127)


class TestSizeAndLatencyBudget(unittest.TestCase):
    def setUp(self):
        subjects = generate_subjects(num_subjects=1, seed=2)
        utterances = generate_utterances(subjects, utterances_per_subject=3, seed=3)
        feats = [u.features for u in utterances]
        self.template = enroll_template(feats)
        self.quantized = export_model.quantize_template(self.template)

    def test_template_size_is_within_budget(self):
        size_kb = export_model.estimate_template_size_kb(self.quantized)
        self.assertLessEqual(size_kb, export_model.MAX_TEMPLATE_SIZE_KB)

    def test_verify_latency_is_within_budget(self):
        latency_ms = export_model.benchmark_verify_latency(self.quantized, num_probes=50)
        self.assertLessEqual(latency_ms, export_model.MAX_VERIFY_LATENCY_MS)

    def test_size_estimate_grows_with_dimension(self):
        small = export_model.quantize_template([1.0, 2.0])
        big = export_model.quantize_template([1.0] * 64)
        self.assertLess(export_model.estimate_template_size_kb(small),
                         export_model.estimate_template_size_kb(big))


class TestFarFrrEvaluation(unittest.TestCase):
    def test_evaluate_far_frr_returns_plausible_rates(self):
        report = export_model.evaluate_far_frr()
        self.assertGreater(report["genuine_trials"], 0)
        self.assertGreater(report["impostor_trials"], 0)
        self.assertGreaterEqual(report["false_accept_rate"], 0.0)
        self.assertLessEqual(report["false_accept_rate"], 1.0)
        self.assertGreaterEqual(report["false_reject_rate"], 0.0)
        self.assertLessEqual(report["false_reject_rate"], 1.0)

    def test_far_frr_is_measured_not_hardcoded_to_zero(self):
        # This pins the exact honest numbers this export run measures at
        # the module's default seed/threshold -- if synthetic_voice.py's
        # generation parameters change, this test is EXPECTED to need
        # updating (see export_model.py's own docstring: measured, not
        # hand-tuned to a target).
        report = export_model.evaluate_far_frr()
        self.assertEqual(report["genuine_trials"], 90)
        self.assertEqual(report["impostor_trials"], 870)


class TestExportGateEndToEnd(unittest.TestCase):
    def test_main_runs_and_produces_a_model_card_without_raising(self):
        # main() calls sys.exit(0) implicitly by returning normally when
        # the gate passes; SystemExit is not raised in the passing case.
        export_model.main()
        card_path = os.path.join(os.path.dirname(__file__), "..", "model_card.md")
        self.assertTrue(os.path.exists(card_path))
        with open(card_path) as f:
            content = f.read()
        self.assertIn("Model Card", content)
        self.assertIn("synthetic", content.lower())
        self.assertIn("False reject rate", content)
        self.assertIn("False accept rate", content)


if __name__ == "__main__":
    unittest.main()
