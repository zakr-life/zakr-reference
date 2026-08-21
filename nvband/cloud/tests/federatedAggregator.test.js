const { test } = require('node:test');
const assert = require('node:assert/strict');
const {
  DEFAULT_MIN_COHORT_SIZE,
  validateDeltaSubmission, isCohortReadyToAggregate, aggregateRound,
} = require('../federated/aggregator');

function delta(scale, numClasses = 3, numFeatures = 20) {
  return {
    weights: Array.from({ length: numClasses }, () => new Array(numFeatures).fill(scale)),
    bias: new Array(numClasses).fill(scale),
  };
}

test('validateDeltaSubmission accepts a small, well-formed delta', () => {
  const result = validateDeltaSubmission(delta(0.01));
  assert.equal(result.accepted, true);
});

test('validateDeltaSubmission rejects an out-of-bound-norm delta', () => {
  const result = validateDeltaSubmission(delta(50.0));
  assert.equal(result.accepted, false);
  assert.match(result.reason, /exceeds plausible bound/);
});

test('validateDeltaSubmission rejects malformed deltas', () => {
  assert.equal(validateDeltaSubmission(null).accepted, false);
  assert.equal(validateDeltaSubmission({ weights: [], bias: [] }).accepted, false);
  assert.equal(validateDeltaSubmission({ weights: [[1, 2]], bias: [1, 2] }).accepted, false); // bias/weights length mismatch
  assert.equal(validateDeltaSubmission({ weights: [[1, NaN]], bias: [1] }).accepted, false);
  assert.equal(validateDeltaSubmission({ weights: [[1, 2], [1]], bias: [1, 1] }).accepted, false); // ragged rows
});

test('isCohortReadyToAggregate respects the minimum cohort size', () => {
  assert.equal(isCohortReadyToAggregate(DEFAULT_MIN_COHORT_SIZE - 1), false);
  assert.equal(isCohortReadyToAggregate(DEFAULT_MIN_COHORT_SIZE), true);
  assert.equal(isCohortReadyToAggregate(DEFAULT_MIN_COHORT_SIZE + 5), true);
});

test('aggregateRound refuses below the minimum cohort, as a defense-in-depth re-check', () => {
  const deltas = Array.from({ length: DEFAULT_MIN_COHORT_SIZE - 1 }, () => delta(0.1));
  const result = aggregateRound(deltas);
  assert.equal(result.aggregate, null);
  assert.match(result.reason, /cohort too small/);
});

test('aggregateRound refuses below minimum even if caller skipped isCohortReadyToAggregate', () => {
  // Simulates a caller bug: never checked readiness, calls straight through.
  const deltas = [delta(0.1), delta(0.2)];
  const result = aggregateRound(deltas);
  assert.equal(result.aggregate, null);
});

test('aggregateRound accepts and averages a valid cohort', () => {
  const deltas = [
    ...Array.from({ length: DEFAULT_MIN_COHORT_SIZE - 1 }, () => delta(1.0)),
    delta(3.0),
  ];
  const result = aggregateRound(deltas);
  assert.notEqual(result.aggregate, null);
  assert.ok(Math.abs(result.aggregate.weights[0][0] - (1.0 * (DEFAULT_MIN_COHORT_SIZE - 1) + 3.0) / DEFAULT_MIN_COHORT_SIZE) < 1e-9);
});

test('aggregateRound result has a bounded/sane magnitude for bounded inputs', () => {
  const deltas = Array.from({ length: DEFAULT_MIN_COHORT_SIZE }, () => delta(0.5));
  const result = aggregateRound(deltas);
  const norm = Math.sqrt(
    result.aggregate.weights.flat().reduce((s, v) => s + v * v, 0) +
    result.aggregate.bias.reduce((s, v) => s + v * v, 0)
  );
  assert.ok(norm <= Math.sqrt(3 * 20 + 3) * 0.5 + 1e-6);
});

test('aggregateRound refuses a malformed delta even within an otherwise-large cohort', () => {
  const deltas = Array.from({ length: DEFAULT_MIN_COHORT_SIZE - 1 }, () => delta(0.1));
  deltas.push({ weights: [[NaN]], bias: [1] });
  const result = aggregateRound(deltas);
  assert.equal(result.aggregate, null);
  assert.match(result.reason, /malformed/);
});
