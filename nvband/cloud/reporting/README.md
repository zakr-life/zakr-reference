# cloud/reporting/

Implements the cloud-side half of Addendum 2 §C ("Bounded-rationale
generation"). Node.js standard library only (`node:test`,
`node:assert/strict`) — no framework dependency, matching the rest of
`cloud/`'s framework-agnostic policy (see `cloud/README.md`).

```
node --test ../tests/sessionSummary.test.js
```

14 tests.

## What this is

**Not a generator.** The actual constrained slot-filling generator and
its canonical independent verifier are Python, and live in
`models/nlg_rationale/` (`grammar.py` / `verifier.py`) — see that
directory's `README.md` for why the architecture (fixed templates bound
to named fields of a validated record, checked by a second independent
module) is what makes an honestly reported ≤0.5% ungrounded-claim rate
achievable at all, and for the explicit non-claim that this is not a
general-purpose language model.

`sessionSummary.js` implements the SAME verifier CONCEPT, independently,
in JS: `verifySessionSummaryClaims(summaryObj, sourceRecord)` re-checks
every claimed field of an already-generated session-summary object
against the structured source record it is supposed to describe — the
defense-in-depth check a cloud reporting/portal code path needs before
showing a summary to a clinician or persisting it, without a round-trip
back into the Python pipeline for every read. It never re-parses the
summary's English text (that would be circular, per Addendum 2 §C — see
`sessionSummary.js`'s module docstring), and it never generates text
itself.

## Layout

- `sessionSummary.js` — `verifySessionSummaryClaims()` (batch, over a
  `{sentences: [...]}` summary object) and `verifySentenceClaims()`
  (single sentence). Cross-references `models/nlg_rationale/verifier.py`
  as the canonical implementation this mirrors — see the header comment
  for exactly how the two match and where they intentionally diverge
  (language-level `typeof` checks instead of Python's bool-is-an-int
  guard, otherwise the same logic).
- (tests live at `cloud/tests/sessionSummary.test.js`, matching this
  repo's existing convention of a single flat `cloud/tests/` directory
  for every cloud module's tests — see `cloud/tests/deidentify.test.js`
  for the pattern this follows.)

## What this deliberately does not do

- No natural-language generation of any kind. If a summary object never
  reaches this module, this module has nothing to check — it is a check
  on already-produced output, not a producer.
- No network call, no shelling out to Python, no dependency on
  `models/nlg_rationale/` being reachable at runtime. The two
  implementations are independent by construction (defense in depth
  means the JS check must keep working even if the Python side has a
  bug specific to its language/runtime).
- No claim that this — or the Python side — has solved hallucination for
  unconstrained/general-purpose generated text. See
  `models/nlg_rationale/README.md`'s "What this is NOT" section, which
  applies here without modification.
