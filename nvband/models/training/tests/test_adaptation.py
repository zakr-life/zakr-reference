"""Traces: Task 3 closed-loop adaptation + explainability logging (CLAUDE.md §4)."""
import os
import sys
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
from adaptation import (decide_adaptation, PostStimResponse, AdaptationLog,  # noqa: E402
                         MAX_CURRENT_DELTA_PER_ADAPTATION_MA,
                         MAX_TIMING_DELTA_PER_ADAPTATION_US)


class TestAdaptationDecision(unittest.TestCase):
    def test_positive_response_increases_current(self):
        r = PostStimResponse("S1", 1, pre_stim_band_power=1.0,
                              post_stim_band_power=2.0, timestamp_us=1000)
        d = decide_adaptation(r)
        self.assertGreater(d.proposed_current_delta_mA, 0)
        self.assertIn("delta=+1.0000", d.rationale)

    def test_negative_response_decreases_current_and_probes_timing(self):
        r = PostStimResponse("S1", 2, pre_stim_band_power=2.0,
                              post_stim_band_power=1.0, timestamp_us=2000)
        d = decide_adaptation(r)
        self.assertLess(d.proposed_current_delta_mA, 0)
        self.assertGreater(d.proposed_timing_delta_us, 0)

    def test_extreme_response_clamped_at_soft_bound(self):
        r = PostStimResponse("S1", 3, pre_stim_band_power=0.0,
                              post_stim_band_power=1000.0, timestamp_us=3000)
        d = decide_adaptation(r)
        self.assertTrue(d.clamped_current)
        self.assertLessEqual(abs(d.proposed_current_delta_mA),
                              MAX_CURRENT_DELTA_PER_ADAPTATION_MA + 1e-9)
        self.assertIn("CLAMPED", d.rationale)

    def test_rationale_is_reconstructible_from_record_alone(self):
        """Explainability requirement: the decision record's own fields
        (response_delta, proposed_*) must be consistent with what
        `rationale` states, without needing any other context."""
        r = PostStimResponse("S1", 4, pre_stim_band_power=0.5,
                              post_stim_band_power=0.8, timestamp_us=4000)
        d = decide_adaptation(r)
        self.assertAlmostEqual(d.response_delta, 0.3, places=6)
        self.assertIn(f"{d.response_delta:+.4f}", d.rationale)

    def test_timing_delta_never_exceeds_soft_bound(self):
        r = PostStimResponse("S1", 5, pre_stim_band_power=5.0,
                              post_stim_band_power=0.0, timestamp_us=5000)
        d = decide_adaptation(r)
        self.assertLessEqual(abs(d.proposed_timing_delta_us),
                              MAX_TIMING_DELTA_PER_ADAPTATION_US)


class TestAdaptationLog(unittest.TestCase):
    def test_log_preserves_every_decision_for_clinician_review(self):
        log = AdaptationLog()
        for i in range(5):
            r = PostStimResponse("S1", i, pre_stim_band_power=1.0,
                                  post_stim_band_power=1.0 + i * 0.1,
                                  timestamp_us=i * 1000)
            log.record(decide_adaptation(r))
        self.assertEqual(len(log.decisions), 5)
        as_list = log.to_list()
        self.assertEqual(len(as_list), 5)
        self.assertTrue(all("rationale" in d for d in as_list))


if __name__ == "__main__":
    unittest.main()
