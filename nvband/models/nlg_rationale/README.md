# models/nlg_rationale/

Implements Addendum 2 §C ("Bounded-rationale generation — target ≤0.5%
ungrounded-claim rate"). Pure Python 3 standard library — no numpy/
TensorFlow/sklearn, no external NLP library, same discipline as the rest
of `models/`:

```
python3 -m unittest discover -s tests -p 'test_*.py' -v
python3 hallucination_gate.py
```

54 unit tests, plus the gate script itself, which is a real,
runnable CI check — not documentation asserting a number.

## What this is NOT (read this before reading anything else here)

**This is not a general-purpose language model, and this directory does
not claim to have solved hallucination in one.** Addendum 2 §C states
this explicitly, and it is repeated here on purpose, in close to the
same words, because it is the single most important thing to understand
about why the ≤0.5% target is achievable and honestly reportable at all:

> This design achieves a low, verifiable ungrounded-claim rate *because*
> generation is constrained and independently checked — it is not
> evidence that hallucination in an unconstrained/general-purpose
> language model has been "solved" or reduced to 0.5%, and no claim to
> that effect should ever be made from this codebase.

If you are looking at this directory as a template for "how ZAKR gets
hallucination down to 0.5% in an LLM," that is a misreading — there is
no LLM here. Read on for what actually exists.

## What this actually is

A **constrained, slot-filling generator** with an **independent
factuality verifier**, for exactly two structured record types produced
elsewhere in this repo (or a record of the same shape, e.g. from a
future feature):

1. `AdaptationDecisionRecord` — mirrors, field-for-field,
   `models/training/adaptation.py`'s `AdaptationDecision` dataclass
   (minus its own `rationale` string, which is generated text, not a
   citable source field — see `record_schema.py`'s docstring for why).
2. `SessionSummaryRecord` — a new record type for the clinician-facing
   session-summary feature this addendum names (session duration,
   stimulation-event count, adaptation count, average model confidence,
   optional caregiver adherence percentage).

**Why a hallucination is structurally hard to produce here:** every
sentence comes from one of a small, fixed set of Python f-string
templates in `grammar.py`. Every value that appears in a template's text
is read directly out of a record that was validated against
`record_schema.py` *first* — there is no code path where the generator
invents a number, defaults a missing field, or pulls a value from
anywhere other than the record it was handed. Alongside the text, every
sentence carries a machine-readable `claims` list
(`FieldClaim(field, asserted_value)`) naming exactly which record fields
it states values for.

`verifier.py` is a second, independent module that re-checks every claim
in that list against the source record — not by re-parsing the English
text (which would be circular: both the text and any re-derived claims
would trace back to the same generation code), but by comparing the
generator's own declared claims against an independently-held copy of
the structured record. This is defense in depth in the same spirit as
the stim-command hard clamp (CLAUDE.md §0.1): verify at the boundary, do
not just trust the producer. It genuinely catches real bug classes — a
template that claims the wrong field, or a caller that verifies against
a stale/mismatched record (see `tests/test_verifier.py`, in particular
`test_stale_source_record_causes_correct_generator_output_to_fail`) —
even though it cannot catch every conceivable bug (see "Honest limits"
below).

## Layout

- `record_schema.py` — the two record types, and `validate_record()`,
  which the generator calls before producing anything. Refuses (does not
  default) a record missing a required field.
- `grammar.py` — the fixed template set. `generate(record_type, data)`
  returns `(sentences, None)` on success or `(None, error_message)` if
  the record is invalid. Never returns a partial/best-effort result.
- `verifier.py` — `verify_sentence(sentence_record, source_record) ->
  (is_grounded, reason)`, the independent factuality check described
  above.
- `golden_set.py` — `build_golden_set()`: 200 deterministic synthetic
  records (fixed seed) spanning both record types, both adaptation
  directions, boundary/clamped values, the optional `adherence_pct`
  field both present and absent, and a handful of deliberately INVALID
  records to exercise the refuse-to-generate path.
- `hallucination_gate.py` — the CI gate: runs every golden-set record
  through `grammar.generate()` + `verifier.verify_sentence()`, computes
  `measured_rate = ungrounded_sentences / total_sentences`, prints a
  report, and `sys.exit(1)` if the measured rate exceeds
  `MAX_UNGROUNDED_CLAIM_RATE = 0.005` — mirroring
  `models/export/export_model.py`'s "compute a real metric, fail the
  build if it violates budget" pattern.
- `tests/` — see below.

## The measured rate (read STATUS.md's honesty discipline first)

`hallucination_gate.py` is a script you run, not a number typed into
this file. As of this pass, running it against the 200-record golden set
prints:

```
golden-set records: 200 (2 correctly refused as invalid)
sentences generated: 738
ungrounded sentences: 0
measured ungrounded-claim rate: 0.0000% (budget: 0.50%)

hallucination gate PASSED (measured rate within budget)
```

**0% on this golden set is the expected outcome of the architecture, not
a lucky draw worth being suspicious of.** The generator cannot assert a
value it wasn't handed from an already-validated record — there is no
code path for it to do otherwise. The only way this number could be
nonzero is a genuine bug (a template claiming the wrong field, a stale
comparison), which is exactly the class of defect this gate exists to
catch before it ships. This number will change if the golden set, the
templates, or the verifier change — re-run the script; do not copy this
block forward as a permanent claim.

## Honest limits of this defense in depth

- The verifier checks the generator's **declared claims** against the
  source record. It does not independently re-derive what the English
  text says by parsing it — Addendum 2 §C is explicit that doing so
  would be circular (the re-derivation and the original generation both
  trace back to the same code). So this defends against claims-list bugs
  (wrong field name, stale record) but does **not** defend against a
  template whose English wording contradicts its own correctly-claimed
  value (e.g. a typo in surrounding prose that doesn't touch a `{...}`
  slot at all) — that class of bug is `grammar.py`'s own correctness,
  covered by `tests/test_grammar.py`, not `verifier.py`'s job.
- This generator only ever describes the two record types in
  `record_schema.py`. It is not a mechanism for summarizing arbitrary
  text, EEG waveforms, or anything not already reduced to a structured
  record with named fields.
- Nothing here is a clinical or diagnostic claim about the content of
  the sentences themselves (e.g. no claim that "confidence 0.87" implies
  any particular clinical meaning) — it is a claim only about
  traceability: every asserted value in a generated sentence traces to a
  named field in the source record.

## Tests

- `tests/test_record_schema.py` — required/optional fields, type
  checking (including the bool-is-not-an-int gotcha), missing vs.
  explicit-None handling.
- `tests/test_grammar.py` — generation refuses (does not default) a
  record missing a required field; every claim traces to the record;
  the optional `adherence_pct` sentence is included/omitted correctly.
- `tests/test_verifier.py` — **the most important file here.** Proves
  the verifier actually flags a deliberately-corrupted claim (wrong
  numeric value, wrong string, wrong boolean, a claim naming a
  nonexistent field, a stale source record) rather than being a rubber
  stamp, using hand-built sentences that don't depend on `grammar.py`
  being correct.
- `tests/test_golden_set.py` — golden-set construction properties, plus
  an actual end-to-end generation+verification run over the whole set.
- `tests/test_hallucination_gate.py` — the gate's threshold decision
  (`gate_would_pass()`) unit-tested against hand-built report dicts,
  including an injected fake high failure rate proving `main()` calls
  `sys.exit(1)` — kept separate from a second test class that runs
  `run_gate()` for real, unmocked.

## Related

- `cloud/reporting/sessionSummary.js` mirrors this module's verifier
  CONCEPT in JS, for the cloud-side session-summary reporting path — see
  that file's docstring, which cross-references `verifier.py` as the
  canonical implementation.
- `models/training/adaptation.py` is the source of the
  `AdaptationDecisionRecord` field shape (unmodified by this pass — see
  the top-level task constraints; this directory only reads that shape,
  it does not touch `adaptation.py`).
