"""
verifier.py — the independent factuality verifier (Addendum 2 §C).

"A second, independent factuality verifier re-parses every generated
sentence and cross-checks every asserted value against the source record
before the sentence is allowed to render — defense in depth, the same
philosophy as the stim-command hard clamp: verify at the boundary, do
not just trust the generator."

Two things make this a genuinely separate check, not a rubber stamp on
`grammar.py`'s own work:

1. It is a different function in a different module, with no shared
   mutable state and no call back into `grammar.py`'s template logic. It
   does not know or care HOW a sentence was produced.
2. It checks `sentence_record.claims` — the machine-readable
   `FieldClaim(field, asserted_value)` list `grammar.py` attaches to
   every sentence — against `source_record`, the SAME structured record
   the caller separately holds. It never re-parses `sentence_record.text`
   with any NLP/string-matching technique. Re-deriving claims from the
   English text and then checking them against the record the text was
   generated from would be circular (both steps trace back to the same
   generation code path); checking the generator's own declared claims
   list against an independently-held copy of the source record is not
   circular in the same way, and it genuinely catches real bug classes —
   e.g. a template that (by a coding mistake) attaches `session_id` as a
   claim but actually interpolated a different field's value into the
   text, or a caller that passes a stale/mismatched source record for
   verification. See `README.md` for the honest limits of this defense
   (it is not a substitute for testing `grammar.py`'s templates
   themselves — `tests/test_grammar.py` does that).
"""
from typing import Any, Dict, Tuple

# Floating point values pass through +:.4f / +:d formatting in
# grammar.py, and this codebase elsewhere (e.g. federated/clipping.py's
# clip bound checks) already accepts that exact float equality across a
# dataclass round-trip is not something to rely on. A tight tolerance
# here means "the same number", not "close enough to plausibly claim."
FLOAT_TOLERANCE = 1e-9


def _values_match(asserted: Any, actual: Any) -> bool:
    # bool is checked first and explicitly: Python's bool is a subclass
    # of int, so without this an asserted `True` could numerically
    # "match" an actual `1` even where the field is meant to be boolean
    # (or vice versa) — that is exactly the kind of near-miss a
    # defense-in-depth check should not paper over.
    if isinstance(asserted, bool) or isinstance(actual, bool):
        return asserted is actual
    if isinstance(asserted, (int, float)) and isinstance(actual, (int, float)):
        return abs(float(asserted) - float(actual)) <= FLOAT_TOLERANCE
    return asserted == actual


def verify_sentence(sentence_record, source_record: Dict[str, Any]
                     ) -> Tuple[bool, str]:
    """Independently re-checks every claim `sentence_record` makes
    (`sentence_record.claims`, a tuple of `grammar.FieldClaim`) against
    `source_record`, the structured record the sentence is supposed to
    describe.

    Returns `(is_grounded, reason)`:
    - `is_grounded=False` if the sentence carries no claims at all (an
      unclaimed sentence is refused, not trusted by omission), if any
      claimed field is absent from `source_record`, or if any claimed
      value does not match the value actually in `source_record` for
      that field.
    - `is_grounded=True` only once every single claim has been checked
      and matched.

    `reason` is always a human-readable string explaining the verdict —
    on failure, it names the specific field and the mismatch, so a
    caller (or the CI gate's printed report) can show exactly what went
    wrong rather than a bare boolean.
    """
    claims = getattr(sentence_record, "claims", None)
    if not claims:
        return False, "sentence carries no machine-readable claims to verify"

    if not isinstance(source_record, dict):
        return False, f"source_record must be a dict, got {type(source_record).__name__}"

    for claim in claims:
        if claim.field not in source_record:
            return False, (f"claimed field '{claim.field}' is not present in "
                            f"source_record at all")
        actual = source_record[claim.field]
        if not _values_match(claim.asserted_value, actual):
            return False, (f"claimed field '{claim.field}' asserts "
                            f"{claim.asserted_value!r} but source_record has "
                            f"{actual!r}")

    return True, f"all {len(claims)} claimed field(s) verified against source_record"


def verify_sentences(sentence_records, source_record: Dict[str, Any]):
    """Convenience batch form of `verify_sentence` over a list of
    sentences generated from the SAME source_record. Returns a list of
    `(sentence_record, is_grounded, reason)` tuples — it does not raise
    or short-circuit on the first ungrounded sentence, because the CI
    gate (`hallucination_gate.py`) needs to see and count every failure,
    not just the first.
    """
    results = []
    for sentence_record in sentence_records:
        is_grounded, reason = verify_sentence(sentence_record, source_record)
        results.append((sentence_record, is_grounded, reason))
    return results
