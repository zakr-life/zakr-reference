/**
 * sessionSummary.js — cloud-side independent factuality check for
 * clinician-facing session-summary text, Addendum 2 §C.
 *
 * CANONICAL IMPLEMENTATION: `models/nlg_rationale/verifier.py`. That
 * module is the actual constrained slot-filling generator's verifier —
 * generation and verification both happen in Python, at the pipeline
 * level, before any summary text reaches this boundary. This file does
 * NOT reimplement natural-language generation, and it does not
 * reimplement the Python verifier by calling into it or shelling out to
 * it — it independently implements the SAME CONCEPT (re-check every
 * claimed field of an already-generated sentence against the structured
 * source record, without re-parsing the sentence's English text) for a
 * JS-side use case: a cloud reporting/portal code path that receives an
 * already-generated session-summary object (produced upstream by the
 * Python generator, or by any future generator that emits the same
 * `{text, claims}` shape) and needs its own defense-in-depth check
 * before that summary is shown to a clinician or written to storage,
 * without a round-trip back into the Python pipeline.
 *
 * This mirrors verifier.py's reasoning almost exactly:
 * - `verifySentenceClaims` checks a sentence's declared `claims` list
 *   against `sourceRecord` — it never inspects `sentence.text`.
 * - A sentence with no claims is refused, not trusted by omission.
 * - A claimed field absent from `sourceRecord`, or a claimed value that
 *   doesn't match the actual value, makes the sentence ungrounded.
 * - This is a second, independently-implemented code path (different
 *   language, different module, no shared state with the generator) —
 *   defense in depth, not a rubber stamp — but per verifier.py's own
 *   "Honest limits" section, it checks the DECLARED claims list against
 *   the record, not the English prose itself; it cannot catch a
 *   generator bug where the prose contradicts its own correctly-claimed
 *   value outside a templated slot.
 *
 * Addendum 2 §C's non-claim applies here too: this is not a
 * general-purpose hallucination detector for arbitrary generated text —
 * it only ever checks a `{field, assertedValue}` claims list against a
 * structured record with named fields, exactly the shape
 * `models/nlg_rationale/grammar.py` produces.
 */

'use strict';

const FLOAT_TOLERANCE = 1e-9;

/**
 * Compares one claimed value against the actual value from the source
 * record. `bool` is checked first and explicitly for the same reason
 * verifier.py does: without it, a claimed boolean could be mistaken for
 * matching a numeric 1/0 that isn't actually the same field semantics.
 * JS doesn't have Python's "bool is an int subclass" issue at the
 * language level, but `typeof` is still checked explicitly so a claimed
 * `true`/`false` never silently matches an actual `1`/`0`.
 */
function valuesMatch(assertedValue, actualValue) {
  if (typeof assertedValue === 'boolean' || typeof actualValue === 'boolean') {
    return assertedValue === actualValue;
  }
  if (typeof assertedValue === 'number' && typeof actualValue === 'number') {
    return Math.abs(assertedValue - actualValue) <= FLOAT_TOLERANCE;
  }
  return assertedValue === actualValue;
}

/**
 * Independently re-checks one generated sentence's claims against the
 * structured source record.
 *
 * @param {{text: string, claims: Array<{field: string, assertedValue: *}>}} sentence
 * @param {object} sourceRecord — the structured session-summary record
 *   (or adaptation-decision record) the sentence claims to describe.
 * @returns {{isGrounded: boolean, reason: string}}
 */
function verifySentenceClaims(sentence, sourceRecord) {
  const claims = sentence && sentence.claims;
  if (!claims || claims.length === 0) {
    return { isGrounded: false, reason: 'sentence carries no machine-readable claims to verify' };
  }
  if (sourceRecord === null || typeof sourceRecord !== 'object') {
    return { isGrounded: false, reason: 'sourceRecord must be an object' };
  }

  for (const claim of claims) {
    if (!Object.prototype.hasOwnProperty.call(sourceRecord, claim.field)) {
      return {
        isGrounded: false,
        reason: `claimed field '${claim.field}' is not present in sourceRecord at all`,
      };
    }
    const actualValue = sourceRecord[claim.field];
    if (!valuesMatch(claim.assertedValue, actualValue)) {
      return {
        isGrounded: false,
        reason: `claimed field '${claim.field}' asserts ${JSON.stringify(claim.assertedValue)} `
          + `but sourceRecord has ${JSON.stringify(actualValue)}`,
      };
    }
  }

  return { isGrounded: true, reason: `all ${claims.length} claimed field(s) verified against sourceRecord` };
}

/**
 * Batch form over an already-generated session-summary object.
 *
 * @param {{sentences: Array<{text: string, claims: Array}>}} summaryObj
 * @param {object} sourceRecord
 * @returns {{allGrounded: boolean, totalSentences: number, ungroundedCount: number,
 *   perSentence: Array<{isGrounded: boolean, reason: string}>}}
 *   `allGrounded` is `false` for an empty `sentences` array on purpose
 *   (a summary with zero checkable sentences is not evidence of
 *   groundedness — a vacuous pass would misrepresent what was actually
 *   verified), mirroring `hallucination_gate.py`'s "zero sentences is a
 *   failure, not a vacuous pass" rule.
 */
function verifySessionSummaryClaims(summaryObj, sourceRecord) {
  const sentences = (summaryObj && summaryObj.sentences) || [];
  const perSentence = sentences.map((sentence) => verifySentenceClaims(sentence, sourceRecord));
  const ungroundedCount = perSentence.filter((result) => !result.isGrounded).length;

  return {
    allGrounded: sentences.length > 0 && ungroundedCount === 0,
    totalSentences: sentences.length,
    ungroundedCount,
    perSentence,
  };
}

module.exports = {
  verifySessionSummaryClaims,
  verifySentenceClaims,
  valuesMatch,
};
