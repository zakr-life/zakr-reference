"""Traces: Addendum 2 §C grammar.py — constrained slot-filling
generation. Most important property tested here: the generator REFUSES
to generate rather than substitute a default when a required field is
missing."""
import os
import sys
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", ".."))
from nlg_rationale.grammar import generate, FieldClaim  # noqa: E402

VALID_ADAPTATION = {
    "session_id": "S1", "burst_id": 7, "timestamp_us": 7000,
    "response_delta": 0.6, "proposed_current_delta_mA": 0.03,
    "proposed_timing_delta_us": 0, "clamped_current": False,
    "clamped_timing": False,
}

VALID_SESSION_SUMMARY = {
    "session_id": "S2", "start_timestamp_us": 2000, "duration_s": 300.0,
    "num_stimulation_events": 6, "num_adaptations": 6,
    "avg_confidence": 0.72, "adherence_pct": 88.5,
}


class TestGenerateRefusesInvalidRecords(unittest.TestCase):
    def test_missing_required_field_is_refused_not_defaulted(self):
        data = dict(VALID_ADAPTATION)
        del data["response_delta"]
        sentences, error = generate("adaptation_decision", data)
        self.assertIsNone(sentences)
        self.assertIsNotNone(error)
        self.assertIn("response_delta", error)

    def test_missing_required_session_summary_field_is_refused(self):
        data = dict(VALID_SESSION_SUMMARY)
        del data["num_stimulation_events"]
        sentences, error = generate("session_summary", data)
        self.assertIsNone(sentences)
        self.assertIn("num_stimulation_events", error)

    def test_unknown_record_type_is_refused(self):
        sentences, error = generate("nonexistent_type", {})
        self.assertIsNone(sentences)
        self.assertIsNotNone(error)


class TestGenerateAdaptationSentences(unittest.TestCase):
    def test_generates_sentences_with_claims_traceable_to_the_record(self):
        sentences, error = generate("adaptation_decision", VALID_ADAPTATION)
        self.assertIsNone(error)
        self.assertGreater(len(sentences), 0)
        for s in sentences:
            self.assertTrue(s.claims, f"sentence {s.text!r} has no claims")
            for claim in s.claims:
                self.assertIsInstance(claim, FieldClaim)
                # Every claimed value must equal the actual value on the
                # source record it was generated from (the generator's
                # own internal consistency — independent verification of
                # this same property against a SEPARATE code path is
                # verifier.py's job, tested in test_verifier.py).
                self.assertEqual(claim.asserted_value,
                                  VALID_ADAPTATION[claim.field])

    def test_response_delta_direction_reflected_in_current_sign(self):
        sentences, _ = generate("adaptation_decision", VALID_ADAPTATION)
        current_sentence = next(s for s in sentences
                                 if s.template_id == "adaptation.current_delta.v1")
        self.assertIn("+0.0300", current_sentence.text)

    def test_clamped_flag_produces_clamp_note_in_text(self):
        data = dict(VALID_ADAPTATION)
        data["clamped_current"] = True
        sentences, _ = generate("adaptation_decision", data)
        current_sentence = next(s for s in sentences
                                 if s.template_id == "adaptation.current_delta.v1")
        self.assertIn("clamped", current_sentence.text.lower())

    def test_no_clamp_note_when_not_clamped(self):
        sentences, _ = generate("adaptation_decision", VALID_ADAPTATION)
        current_sentence = next(s for s in sentences
                                 if s.template_id == "adaptation.current_delta.v1")
        self.assertNotIn("clamped", current_sentence.text.lower())


class TestGenerateSessionSummarySentences(unittest.TestCase):
    def test_adherence_sentence_present_when_field_present(self):
        sentences, _ = generate("session_summary", VALID_SESSION_SUMMARY)
        ids = [s.template_id for s in sentences]
        self.assertIn("session_summary.adherence.v1", ids)

    def test_adherence_sentence_absent_when_field_absent(self):
        """The generator must not invent an adherence figure when the
        record legitimately doesn't have one — it must omit the sentence
        entirely, not fabricate a placeholder value."""
        data = dict(VALID_SESSION_SUMMARY)
        del data["adherence_pct"]
        sentences, error = generate("session_summary", data)
        self.assertIsNone(error)
        ids = [s.template_id for s in sentences]
        self.assertNotIn("session_summary.adherence.v1", ids)

    def test_all_claims_traceable_to_record(self):
        sentences, _ = generate("session_summary", VALID_SESSION_SUMMARY)
        for s in sentences:
            for claim in s.claims:
                self.assertEqual(claim.asserted_value,
                                  VALID_SESSION_SUMMARY[claim.field])


if __name__ == "__main__":
    unittest.main()
