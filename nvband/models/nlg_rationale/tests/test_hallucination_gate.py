"""Traces: Addendum 2 §C hallucination_gate.py — the CI gate script,
mirroring the style of `models/export/tests/test_quantization.py`
testing `export_model.py`'s budget-gate logic.

Two DISTINCT things are tested here, deliberately kept separate:
1. `gate_would_pass()`'s threshold decision, unit-tested against
   hand-built report dicts (including an injected fake high failure
   rate) — no real generation/verification runs for these.
2. `run_gate()` actually executed for real against the real golden set
   — the "runs end to end, for real" case, separate from #1.
"""
import os
import sys
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", ".."))
from nlg_rationale.hallucination_gate import (  # noqa: E402
    run_gate, gate_would_pass, MAX_UNGROUNDED_CLAIM_RATE, main,
)


class TestGateThresholdLogicInjected(unittest.TestCase):
    """Pure threshold-logic tests against hand-built report dicts — none
    of these run real generation or verification."""

    def test_zero_rate_passes(self):
        report = {"rate": 0.0, "total_sentences": 100, "ungrounded_sentences": 0}
        self.assertTrue(gate_would_pass(report))

    def test_rate_exactly_at_budget_passes(self):
        report = {"rate": MAX_UNGROUNDED_CLAIM_RATE, "total_sentences": 1000,
                   "ungrounded_sentences": 5}
        self.assertTrue(gate_would_pass(report))

    def test_injected_high_failure_rate_fails_the_gate(self):
        """Directly exercises the failure path without running a single
        real generation: a fabricated report claiming a 10% ungrounded
        rate must make the gate refuse to pass."""
        report = {"rate": 0.10, "total_sentences": 100, "ungrounded_sentences": 10}
        self.assertFalse(gate_would_pass(report))

    def test_rate_just_over_budget_fails(self):
        report = {"rate": MAX_UNGROUNDED_CLAIM_RATE + 1e-6,
                   "total_sentences": 1_000_000, "ungrounded_sentences": 5001}
        self.assertFalse(gate_would_pass(report))

    def test_zero_sentences_generated_is_treated_as_failure_not_vacuous_pass(self):
        report = {"rate": None, "total_sentences": 0, "ungrounded_sentences": 0}
        self.assertFalse(gate_would_pass(report))


class TestMainExitsNonZeroOnInjectedFailure(unittest.TestCase):
    """Confirms `main()` itself calls `sys.exit(1)` when the (mocked)
    report is a failure — separate from actually running the pipeline
    for real."""

    def test_main_exits_nonzero_when_run_gate_reports_high_failure(self):
        from unittest.mock import patch
        fake_report = {
            "total_records": 10, "refused_records": 0,
            "total_sentences": 10, "ungrounded_sentences": 10,
            "rate": 1.0, "failures": [("fake", "fake sentence", "fake reason")],
        }
        with patch("nlg_rationale.hallucination_gate.run_gate",
                   return_value=fake_report):
            with self.assertRaises(SystemExit) as ctx:
                main()
            self.assertEqual(ctx.exception.code, 1)

    def test_main_does_not_exit_when_run_gate_reports_success(self):
        from unittest.mock import patch
        fake_report = {
            "total_records": 10, "refused_records": 0,
            "total_sentences": 10, "ungrounded_sentences": 0,
            "rate": 0.0, "failures": [],
        }
        with patch("nlg_rationale.hallucination_gate.run_gate",
                   return_value=fake_report):
            try:
                main()
            except SystemExit:
                self.fail("main() should not sys.exit() on a passing report")


class TestRunGateForReal(unittest.TestCase):
    """Actually runs generation + verification over the real golden set
    — no mocking. This is the "runs end to end" case, distinct from the
    injected-failure tests above."""

    def test_real_run_produces_a_measured_rate_within_budget(self):
        report = run_gate()
        self.assertGreater(report["total_sentences"], 0)
        self.assertIsNotNone(report["rate"])
        self.assertTrue(gate_would_pass(report),
                         f"real measured rate {report['rate']} exceeded budget; "
                         f"failures: {report['failures']}")

    def test_real_run_counts_the_handcrafted_invalid_records_as_refused(self):
        report = run_gate()
        self.assertGreaterEqual(report["refused_records"], 2)


if __name__ == "__main__":
    unittest.main()
