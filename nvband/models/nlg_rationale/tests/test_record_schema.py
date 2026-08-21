"""Traces: Addendum 2 §C record_schema.py — the shape of records the
generator is allowed to consume."""
import os
import sys
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", ".."))
from nlg_rationale.record_schema import (  # noqa: E402
    validate_record, required_field_names, optional_field_names,
    AdaptationDecisionRecord, SessionSummaryRecord,
)

VALID_ADAPTATION = {
    "session_id": "S1", "burst_id": 1, "timestamp_us": 1000,
    "response_delta": 0.3, "proposed_current_delta_mA": 0.015,
    "proposed_timing_delta_us": 0, "clamped_current": False,
    "clamped_timing": False,
}

VALID_SESSION_SUMMARY = {
    "session_id": "S1", "start_timestamp_us": 1000, "duration_s": 600.0,
    "num_stimulation_events": 12, "num_adaptations": 12,
    "avg_confidence": 0.87, "adherence_pct": 95.0,
}


class TestValidateRecordAdaptation(unittest.TestCase):
    def test_valid_record_passes(self):
        ok, errors = validate_record("adaptation_decision", VALID_ADAPTATION)
        self.assertTrue(ok, errors)
        self.assertEqual(errors, [])

    def test_missing_required_field_is_rejected(self):
        data = dict(VALID_ADAPTATION)
        del data["response_delta"]
        ok, errors = validate_record("adaptation_decision", data)
        self.assertFalse(ok)
        self.assertTrue(any("response_delta" in e for e in errors))

    def test_required_field_present_but_none_is_rejected(self):
        """None is not a stand-in for 'missing' — a record where a
        required field is explicitly null must also be refused, not
        treated as 'field technically present'."""
        data = dict(VALID_ADAPTATION)
        data["burst_id"] = None
        ok, errors = validate_record("adaptation_decision", data)
        self.assertFalse(ok)
        self.assertTrue(any("burst_id" in e for e in errors))

    def test_wrong_type_is_rejected(self):
        data = dict(VALID_ADAPTATION)
        data["response_delta"] = "not-a-number"
        ok, errors = validate_record("adaptation_decision", data)
        self.assertFalse(ok)
        self.assertTrue(any("response_delta" in e for e in errors))

    def test_bool_is_not_silently_accepted_as_int(self):
        """Python's bool is an int subclass; the schema must not let a
        boolean satisfy an int-typed field (burst_id)."""
        data = dict(VALID_ADAPTATION)
        data["burst_id"] = True
        ok, errors = validate_record("adaptation_decision", data)
        self.assertFalse(ok)

    def test_unknown_field_is_rejected(self):
        data = dict(VALID_ADAPTATION)
        data["not_a_real_field"] = 1
        ok, errors = validate_record("adaptation_decision", data)
        self.assertFalse(ok)
        self.assertTrue(any("not_a_real_field" in e for e in errors))

    def test_all_adaptation_fields_are_required(self):
        self.assertEqual(
            set(required_field_names(AdaptationDecisionRecord)),
            set(VALID_ADAPTATION.keys()),
        )
        self.assertEqual(optional_field_names(AdaptationDecisionRecord), [])


class TestValidateRecordSessionSummary(unittest.TestCase):
    def test_valid_record_with_adherence_passes(self):
        ok, errors = validate_record("session_summary", VALID_SESSION_SUMMARY)
        self.assertTrue(ok, errors)

    def test_valid_record_without_optional_adherence_passes(self):
        data = dict(VALID_SESSION_SUMMARY)
        del data["adherence_pct"]
        ok, errors = validate_record("session_summary", data)
        self.assertTrue(ok, errors)

    def test_missing_required_avg_confidence_is_rejected(self):
        data = dict(VALID_SESSION_SUMMARY)
        del data["avg_confidence"]
        ok, errors = validate_record("session_summary", data)
        self.assertFalse(ok)

    def test_adherence_pct_is_the_only_optional_field(self):
        self.assertEqual(optional_field_names(SessionSummaryRecord),
                          ["adherence_pct"])


class TestValidateRecordGeneral(unittest.TestCase):
    def test_unknown_record_type_is_rejected(self):
        ok, errors = validate_record("not_a_real_type", {})
        self.assertFalse(ok)

    def test_non_dict_data_is_rejected(self):
        ok, errors = validate_record("adaptation_decision", "not a dict")
        self.assertFalse(ok)


if __name__ == "__main__":
    unittest.main()
