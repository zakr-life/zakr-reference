"""Traces: Task 2 phase-locked-loop scheduling (CLAUDE.md §4)."""
import math
import os
import sys
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
from phase_locked_loop import estimate_phase, schedule_phase_locked_burst  # noqa: E402


def make_sine(freq_hz, sample_rate_hz, n_samples, phase0=0.0):
    return [math.sin(2 * math.pi * freq_hz * (i / sample_rate_hz) + phase0)
            for i in range(n_samples)]


class TestPhaseEstimation(unittest.TestCase):
    def test_clean_sine_estimates_frequency_accurately(self):
        sample_rate = 250.0
        signal = make_sine(6.0, sample_rate, 500)  # theta-band-ish
        est = estimate_phase(signal, sample_rate)
        self.assertIsNotNone(est)
        self.assertAlmostEqual(est.instantaneous_freq_hz, 6.0, delta=0.5)
        self.assertGreater(est.confidence, 0.8)

    def test_too_short_signal_returns_none(self):
        self.assertIsNone(estimate_phase([1.0, -1.0], 250.0))

    def test_zero_sample_rate_returns_none(self):
        self.assertIsNone(estimate_phase([1.0, -1.0, 1.0, -1.0], 0.0))

    def test_flat_signal_returns_none(self):
        self.assertIsNone(estimate_phase([0.0] * 100, 250.0))


class TestScheduling(unittest.TestCase):
    def test_low_confidence_produces_no_request(self):
        sample_rate = 250.0
        # Irregular signal -> low confidence.
        import random
        rng = random.Random(1)
        noisy = [rng.uniform(-1, 1) for _ in range(200)]
        est = estimate_phase(noisy, sample_rate)
        req = schedule_phase_locked_burst(est, now_us=0, min_confidence=0.9)
        self.assertIsNone(req)

    def test_high_confidence_produces_valid_request(self):
        sample_rate = 250.0
        signal = make_sine(6.0, sample_rate, 500)
        est = estimate_phase(signal, sample_rate)
        req = schedule_phase_locked_burst(est, now_us=1_000_000)
        self.assertIsNotNone(req)
        self.assertGreaterEqual(req.scheduled_start_time_us, 1_000_000)
        self.assertGreater(req.duration_us, 0)

    def test_none_phase_estimate_produces_no_request(self):
        req = schedule_phase_locked_burst(None, now_us=0)
        self.assertIsNone(req)


if __name__ == "__main__":
    unittest.main()
