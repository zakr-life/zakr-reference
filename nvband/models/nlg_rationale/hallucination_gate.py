"""
hallucination_gate.py — CI gate for the bounded-rationale generator
(Addendum 2 §C), mirroring `models/export/export_model.py`'s
"compute a real metric, print it, fail the build if it violates a named
budget" pattern.

"The CI gate fails the build if the measured rate on the golden set
exceeds the 0.5% target."

This script runs every record in `golden_set.py` through
`grammar.generate()` and then `verifier.verify_sentence()` on every
sentence that generation produces, counts how many of those sentences
are ungrounded (the verifier said `is_grounded=False`), and computes:

    measured_rate = ungrounded_sentences / total_sentences_generated

exactly as `MAX_UNGROUNDED_CLAIM_RATE` below is defined against. It then
prints the measured number and exits non-zero if it exceeds budget —
STATUS.md's honesty discipline requires that number come from an actual
run of this script, never be hand-typed into documentation as an
assumed constant. Run it with:

    python3 models/nlg_rationale/hallucination_gate.py

Why 0% (or very close to it) on the golden set is the EXPECTED outcome,
not a lucky result to be suspicious of: this generator is structurally
incapable of asserting a claim that isn't a direct field read out of an
already-schema-validated record (see `grammar.py`'s docstring). The only
way a sentence could fail verification here is a genuine bug — a
template attaching the wrong field name to a claim, or a claim carrying
a stale/miscomputed value — which is exactly the class of bug this gate
exists to catch before it ships, not evidence that "hallucination" in
the general, free-generation-LLM sense has been solved. See
`README.md`'s "Explicit non-claim" section, which repeats Addendum 2
§C's own words on this almost verbatim, on purpose.
"""
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
from nlg_rationale.golden_set import build_golden_set  # noqa: E402
from nlg_rationale.grammar import generate  # noqa: E402
from nlg_rationale.verifier import verify_sentence  # noqa: E402

# Addendum 2 §C: "target ≤0.5% ungrounded-claim rate."
MAX_UNGROUNDED_CLAIM_RATE = 0.005


def run_gate(golden_records=None) -> dict:
    """Runs generation + verification over every golden-set record.
    Returns a report dict (never raises, never exits) so both `main()`
    and the unit tests can drive it without process-exit side effects."""
    if golden_records is None:
        golden_records = build_golden_set()

    total_records = len(golden_records)
    refused_records = 0
    total_sentences = 0
    ungrounded_sentences = 0
    failures = []  # list of (record_note, sentence_text, reason)

    for entry in golden_records:
        sentences, error = generate(entry["record_type"], entry["data"])
        if sentences is None:
            # A correctly-refused invalid record is the SAFE outcome
            # (see golden_set.py's handcrafted "INVALID:" entries) — it
            # is counted separately, never folded into the ungrounded
            # rate, because refusing to generate is not the same failure
            # mode as generating an ungrounded claim.
            refused_records += 1
            continue

        for sentence in sentences:
            total_sentences += 1
            is_grounded, reason = verify_sentence(sentence, entry["data"])
            if not is_grounded:
                ungrounded_sentences += 1
                failures.append((entry.get("note", entry["record_type"]),
                                  sentence.text, reason))

    rate = (ungrounded_sentences / total_sentences) if total_sentences else None

    return {
        "total_records": total_records,
        "refused_records": refused_records,
        "total_sentences": total_sentences,
        "ungrounded_sentences": ungrounded_sentences,
        "rate": rate,
        "failures": failures,
    }


def gate_would_pass(report: dict) -> bool:
    """The gate's threshold decision, isolated from I/O and process exit
    so it can be unit-tested directly against a hand-built report dict
    (see tests/test_hallucination_gate.py) as well as exercised for real
    by `main()`.

    A `rate` of `None` (zero sentences generated at all) is treated as a
    FAILURE, not a vacuous pass — a golden set that produced no sentences
    to check would make "0% ungrounded" a meaningless, misleading number,
    exactly the kind of unearned claim STATUS.md's honesty discipline
    rules out.
    """
    if report["rate"] is None:
        return False
    return report["rate"] <= MAX_UNGROUNDED_CLAIM_RATE


def _print_report(report: dict) -> None:
    print(f"golden-set records: {report['total_records']} "
          f"({report['refused_records']} correctly refused as invalid)")
    print(f"sentences generated: {report['total_sentences']}")
    print(f"ungrounded sentences: {report['ungrounded_sentences']}")
    if report["rate"] is None:
        print("measured ungrounded-claim rate: N/A (zero sentences generated)")
    else:
        print(f"measured ungrounded-claim rate: {report['rate'] * 100:.4f}% "
              f"(budget: {MAX_UNGROUNDED_CLAIM_RATE * 100:.2f}%)")
    if report["failures"]:
        print("\nungrounded sentences (first 10 shown):")
        for note, text, reason in report["failures"][:10]:
            print(f"  - [{note}] {text!r}\n    reason: {reason}")


def main():
    report = run_gate()
    _print_report(report)

    if not gate_would_pass(report):
        print("\nHALLUCINATION GATE FAILED: measured ungrounded-claim rate "
              f"exceeds the {MAX_UNGROUNDED_CLAIM_RATE * 100:.2f}% budget "
              "(or zero sentences were generated at all).")
        sys.exit(1)

    print("\nhallucination gate PASSED (measured rate within budget)")


if __name__ == "__main__":
    main()
