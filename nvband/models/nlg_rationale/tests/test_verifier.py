"""Traces: Addendum 2 §C verifier.py — the independent factuality
verifier. THIS is the most important test file in this directory: it
proves the verifier actually catches a mismatch between a claimed value
and the source record, rather than being a rubber stamp that always
returns True. Every corruption test below constructs the "generated
sentence" by hand (NOT via grammar.generate()) specifically so this test
does not depend on grammar.py being correct — it tests verifier.py in
isolation, as an independent check."""
import os
import sys
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", ".."))
from nlg_rationale.grammar import FieldClaim, GeneratedSentence, generate  # noqa: E402
from nlg_rationale.verifier import verify_sentence, verify_sentences  # noqa: E402

SOURCE_RECORD = {
    "session_id": "S1", "burst_id": 9, "timestamp_us": 9000,
    "response_delta": 0.42, "proposed_current_delta_mA": 0.021,
    "proposed_timing_delta_us": 0, "clamped_current": False,
    "clamped_timing": False,
}


def _sentence(claims):
    return GeneratedSentence(text="irrelevant to verification — see docstring",
                              template_id="test.manual", claims=tuple(claims))


class TestVerifySentenceAcceptsGroundedClaims(unittest.TestCase):
    def test_single_correct_claim_is_grounded(self):
        s = _sentence([FieldClaim("burst_id", 9)])
        is_grounded, reason = verify_sentence(s, SOURCE_RECORD)
        self.assertTrue(is_grounded, reason)

    def test_multiple_correct_claims_are_grounded(self):
        s = _sentence([
            FieldClaim("session_id", "S1"),
            FieldClaim("response_delta", 0.42),
            FieldClaim("clamped_current", False),
        ])
        is_grounded, reason = verify_sentence(s, SOURCE_RECORD)
        self.assertTrue(is_grounded, reason)

    def test_float_claim_within_tight_tolerance_is_grounded(self):
        s = _sentence([FieldClaim("response_delta", 0.42 + 1e-12)])
        is_grounded, _ = verify_sentence(s, SOURCE_RECORD)
        self.assertTrue(is_grounded)


class TestVerifySentenceCatchesUngroundedClaims(unittest.TestCase):
    """The core defense-in-depth test: a sentence whose claimed value
    does not match the source record must be flagged, independent of
    whatever produced it."""

    def test_wrong_numeric_value_is_caught(self):
        # Deliberately corrupted: the real response_delta is 0.42, this
        # sentence claims 0.99.
        s = _sentence([FieldClaim("response_delta", 0.99)])
        is_grounded, reason = verify_sentence(s, SOURCE_RECORD)
        self.assertFalse(is_grounded)
        self.assertIn("response_delta", reason)

    def test_wrong_string_value_is_caught(self):
        s = _sentence([FieldClaim("session_id", "WRONG-SESSION")])
        is_grounded, reason = verify_sentence(s, SOURCE_RECORD)
        self.assertFalse(is_grounded)

    def test_wrong_boolean_value_is_caught(self):
        s = _sentence([FieldClaim("clamped_current", True)])  # actual is False
        is_grounded, reason = verify_sentence(s, SOURCE_RECORD)
        self.assertFalse(is_grounded)

    def test_bool_does_not_falsely_match_numeric_1_or_0(self):
        """Regression guard: Python's bool is an int subclass, so a naive
        `==` check would let True "match" 1 or False "match" 0 even when
        the field's actual value is a plain int, not a boolean. A claim
        of `clamped_current=True` (bool) must not be considered grounded
        against a hypothetical actual value of the int `1` unless it
        really is the boolean True."""
        record = dict(SOURCE_RECORD)
        record["clamped_current"] = 1  # deliberately an int, not a bool
        s = _sentence([FieldClaim("clamped_current", True)])
        is_grounded, reason = verify_sentence(s, record)
        self.assertFalse(is_grounded)

    def test_claim_referencing_field_not_in_source_record_is_caught(self):
        """Simulates a generator bug: a claim naming a field that isn't
        even part of the source record at all (e.g. a stale/renamed
        field, or a claim built against the wrong record)."""
        s = _sentence([FieldClaim("nonexistent_field", 42)])
        is_grounded, reason = verify_sentence(s, SOURCE_RECORD)
        self.assertFalse(is_grounded)
        self.assertIn("nonexistent_field", reason)

    def test_one_bad_claim_among_good_ones_still_fails_the_whole_sentence(self):
        s = _sentence([
            FieldClaim("session_id", "S1"),           # correct
            FieldClaim("response_delta", -999.0),      # corrupted
        ])
        is_grounded, reason = verify_sentence(s, SOURCE_RECORD)
        self.assertFalse(is_grounded)

    def test_sentence_with_no_claims_at_all_is_not_trusted(self):
        s = _sentence([])
        is_grounded, reason = verify_sentence(s, SOURCE_RECORD)
        self.assertFalse(is_grounded)

    def test_stale_source_record_causes_correct_generator_output_to_fail(self):
        """Simulates a caller passing the WRONG (stale/mismatched) source
        record for verification — a real generated sentence, verified
        against a record it was not actually generated from, must not be
        rubber-stamped as grounded just because it came from grammar.py."""
        sentences, error = generate("adaptation_decision", SOURCE_RECORD)
        self.assertIsNone(error)
        stale_record = dict(SOURCE_RECORD)
        stale_record["burst_id"] = 999  # a different session's data
        burst_sentence = next(s for s in sentences
                               if s.template_id == "adaptation.response_delta.v1")
        is_grounded, reason = verify_sentence(burst_sentence, stale_record)
        self.assertFalse(is_grounded)


class TestVerifySentencesBatch(unittest.TestCase):
    def test_batch_verification_counts_each_sentence_independently(self):
        good = _sentence([FieldClaim("burst_id", 9)])
        bad = _sentence([FieldClaim("burst_id", 12345)])
        results = verify_sentences([good, bad], SOURCE_RECORD)
        self.assertEqual(len(results), 2)
        self.assertTrue(results[0][1])
        self.assertFalse(results[1][1])

    def test_real_generator_output_is_fully_grounded_against_its_own_record(self):
        """End-to-end sanity: sentences generated from a valid record and
        verified against THAT SAME record must all be grounded."""
        sentences, error = generate("adaptation_decision", SOURCE_RECORD)
        self.assertIsNone(error)
        results = verify_sentences(sentences, SOURCE_RECORD)
        self.assertTrue(all(is_grounded for _, is_grounded, _ in results),
                         [r for r in results if not r[1]])


if __name__ == "__main__":
    unittest.main()
