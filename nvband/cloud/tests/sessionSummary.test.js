/**
 * Traces: Addendum 2 §C — cloud-side independent factuality check,
 * mirroring the Python verifier's test cases
 * (models/nlg_rationale/tests/test_verifier.py). THIS FILE is the JS
 * side's most important test: it proves verifySentenceClaims() actually
 * catches a corrupted claim rather than rubber-stamping everything.
 */
const { test } = require('node:test');
const assert = require('node:assert/strict');
const {
  verifySessionSummaryClaims,
  verifySentenceClaims,
  valuesMatch,
} = require('../reporting/sessionSummary');

const SOURCE_RECORD = {
  sessionId: 'S1',
  durationS: 600.0,
  numStimulationEvents: 12,
  numAdaptations: 12,
  avgConfidence: 0.87,
  adherencePct: 95.0,
};

function sentence(claims) {
  return { text: 'irrelevant to verification — see module docstring', claims };
}

test('valuesMatch: a boolean never falsely matches numeric 1/0', () => {
  assert.equal(valuesMatch(true, 1), false);
  assert.equal(valuesMatch(false, 0), false);
  assert.equal(valuesMatch(true, true), true);
});

test('valuesMatch: floats compare within tight tolerance', () => {
  assert.equal(valuesMatch(0.87, 0.87 + 1e-12), true);
  assert.equal(valuesMatch(0.87, 0.99), false);
});

test('verifySentenceClaims: a single correct claim is grounded', () => {
  const s = sentence([{ field: 'sessionId', assertedValue: 'S1' }]);
  const { isGrounded } = verifySentenceClaims(s, SOURCE_RECORD);
  assert.equal(isGrounded, true);
});

test('verifySentenceClaims: multiple correct claims are grounded', () => {
  const s = sentence([
    { field: 'durationS', assertedValue: 600.0 },
    { field: 'numAdaptations', assertedValue: 12 },
  ]);
  const { isGrounded } = verifySentenceClaims(s, SOURCE_RECORD);
  assert.equal(isGrounded, true);
});

test('verifySentenceClaims: a deliberately corrupted numeric claim is caught', () => {
  // Real avgConfidence is 0.87; this sentence claims 0.42.
  const s = sentence([{ field: 'avgConfidence', assertedValue: 0.42 }]);
  const { isGrounded, reason } = verifySentenceClaims(s, SOURCE_RECORD);
  assert.equal(isGrounded, false);
  assert.match(reason, /avgConfidence/);
});

test('verifySentenceClaims: a corrupted string claim is caught', () => {
  const s = sentence([{ field: 'sessionId', assertedValue: 'WRONG-SESSION' }]);
  const { isGrounded } = verifySentenceClaims(s, SOURCE_RECORD);
  assert.equal(isGrounded, false);
});

test('verifySentenceClaims: a claim naming a field absent from sourceRecord is caught', () => {
  const s = sentence([{ field: 'nonexistentField', assertedValue: 42 }]);
  const { isGrounded, reason } = verifySentenceClaims(s, SOURCE_RECORD);
  assert.equal(isGrounded, false);
  assert.match(reason, /nonexistentField/);
});

test('verifySentenceClaims: one bad claim among good ones still fails the sentence', () => {
  const s = sentence([
    { field: 'sessionId', assertedValue: 'S1' },       // correct
    { field: 'numAdaptations', assertedValue: 999 },    // corrupted
  ]);
  const { isGrounded } = verifySentenceClaims(s, SOURCE_RECORD);
  assert.equal(isGrounded, false);
});

test('verifySentenceClaims: a sentence with no claims at all is not trusted', () => {
  const { isGrounded, reason } = verifySentenceClaims(sentence([]), SOURCE_RECORD);
  assert.equal(isGrounded, false);
  assert.match(reason, /no machine-readable claims/);
});

test('verifySentenceClaims: a stale/mismatched sourceRecord fails an otherwise-correct claim', () => {
  const correctlyClaimedSentence = sentence([{ field: 'sessionId', assertedValue: 'S1' }]);
  const staleRecord = { ...SOURCE_RECORD, sessionId: 'S2' };
  const { isGrounded } = verifySentenceClaims(correctlyClaimedSentence, staleRecord);
  assert.equal(isGrounded, false);
});

test('verifySessionSummaryClaims: a fully-grounded summary reports allGrounded true', () => {
  const summary = {
    sentences: [
      sentence([{ field: 'sessionId', assertedValue: 'S1' }]),
      sentence([{ field: 'durationS', assertedValue: 600.0 }]),
    ],
  };
  const result = verifySessionSummaryClaims(summary, SOURCE_RECORD);
  assert.equal(result.allGrounded, true);
  assert.equal(result.totalSentences, 2);
  assert.equal(result.ungroundedCount, 0);
});

test('verifySessionSummaryClaims: one ungrounded sentence flips allGrounded false and is counted', () => {
  const summary = {
    sentences: [
      sentence([{ field: 'sessionId', assertedValue: 'S1' }]),          // good
      sentence([{ field: 'avgConfidence', assertedValue: 0.01 }]),      // corrupted
    ],
  };
  const result = verifySessionSummaryClaims(summary, SOURCE_RECORD);
  assert.equal(result.allGrounded, false);
  assert.equal(result.ungroundedCount, 1);
  assert.equal(result.perSentence[0].isGrounded, true);
  assert.equal(result.perSentence[1].isGrounded, false);
});

test('verifySessionSummaryClaims: an empty sentence list is not a vacuous pass', () => {
  const result = verifySessionSummaryClaims({ sentences: [] }, SOURCE_RECORD);
  assert.equal(result.allGrounded, false);
  assert.equal(result.totalSentences, 0);
});

test('verifySessionSummaryClaims: a missing sentences array is handled the same as empty', () => {
  const result = verifySessionSummaryClaims({}, SOURCE_RECORD);
  assert.equal(result.allGrounded, false);
  assert.equal(result.totalSentences, 0);
});
