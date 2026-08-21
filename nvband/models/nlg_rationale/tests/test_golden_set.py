"""Traces: Addendum 2 §C golden_set.py — synthetic record generation for
the hallucination-rate CI gate."""
import os
import sys
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", ".."))
from nlg_rationale.golden_set import build_golden_set, DEFAULT_GOLDEN_SET_SIZE  # noqa: E402
from nlg_rationale.record_schema import validate_record  # noqa: E402
from nlg_rationale.grammar import generate  # noqa: E402
from nlg_rationale.verifier import verify_sentence  # noqa: E402


class TestBuildGoldenSet(unittest.TestCase):
    def test_default_size(self):
        entries = build_golden_set()
        self.assertEqual(len(entries), DEFAULT_GOLDEN_SET_SIZE)

    def test_deterministic_across_runs(self):
        a = build_golden_set(n=50, seed=42)
        b = build_golden_set(n=50, seed=42)
        self.assertEqual(a, b)

    def test_different_seed_changes_random_portion(self):
        a = build_golden_set(n=50, seed=1)
        b = build_golden_set(n=50, seed=2)
        self.assertNotEqual(a, b)

    def test_spans_both_record_types(self):
        entries = build_golden_set()
        types = {e["record_type"] for e in entries}
        self.assertEqual(types, {"adaptation_decision", "session_summary"})

    def test_includes_a_missing_optional_field_case(self):
        entries = build_golden_set()
        summaries = [e for e in entries if e["record_type"] == "session_summary"]
        self.assertTrue(any("adherence_pct" not in e["data"] for e in summaries))
        self.assertTrue(any("adherence_pct" in e["data"] for e in summaries))

    def test_includes_both_adaptation_directions(self):
        entries = build_golden_set()
        adaptations = [e["data"] for e in entries
                       if e["record_type"] == "adaptation_decision"]
        self.assertTrue(any(d["response_delta"] > 0 for d in adaptations))
        self.assertTrue(any(d["response_delta"] < 0 for d in adaptations))

    def test_includes_deliberately_invalid_records(self):
        """The golden set must include at least one record per type that
        FAILS validation, to exercise the refuse-to-generate path."""
        entries = build_golden_set()
        invalid_count = sum(
            1 for e in entries
            if not validate_record(e["record_type"], e["data"])[0]
        )
        self.assertGreaterEqual(invalid_count, 2)

    def test_n_below_handcrafted_count_raises(self):
        with self.assertRaises(ValueError):
            build_golden_set(n=1)


class TestGoldenSetEndToEnd(unittest.TestCase):
    """Runs actual generation + verification over the whole golden set —
    the "runs end to end" requirement, distinct from
    test_hallucination_gate.py's threshold-logic unit tests."""

    def test_every_valid_record_generates_and_verifies_grounded_sentences(self):
        entries = build_golden_set()
        total_sentences = 0
        ungrounded = 0
        for entry in entries:
            sentences, error = generate(entry["record_type"], entry["data"])
            is_meant_to_be_valid = validate_record(entry["record_type"],
                                                     entry["data"])[0]
            if not is_meant_to_be_valid:
                self.assertIsNone(sentences, entry)
                continue
            self.assertIsNotNone(sentences, f"{entry['note']}: {error}")
            for s in sentences:
                total_sentences += 1
                is_grounded, reason = verify_sentence(s, entry["data"])
                if not is_grounded:
                    ungrounded += 1
        self.assertGreater(total_sentences, 0)
        self.assertEqual(ungrounded, 0,
                          "the constrained generator produced an ungrounded "
                          "sentence on the golden set — see reason strings above")


if __name__ == "__main__":
    unittest.main()
