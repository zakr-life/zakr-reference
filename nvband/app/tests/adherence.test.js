const { test } = require('node:test');
const assert = require('node:assert/strict');
const { computeAdherence } = require('../mobile/src/state/adherence');

test('charged but never worn is flagged distinctly, not just "fine"', () => {
  const samples = Array.from({ length: 10 }, (_, i) => ({
    timestampMs: i * 1000,
    imuMotionDetected: false,
    isCharging: true,
  }));
  const r = computeAdherence(samples, 0, 9000);
  assert.equal(r.wornFraction, 0);
  assert.equal(r.chargedFraction, 1);
  assert.match(r.summary, /charged.*little sign of being worn/);
});

test('good adherence when worn majority of window', () => {
  const samples = Array.from({ length: 10 }, (_, i) => ({
    timestampMs: i * 1000,
    imuMotionDetected: i < 7,
    isCharging: false,
  }));
  const r = computeAdherence(samples, 0, 9000);
  assert.equal(r.wornFraction, 0.7);
  assert.match(r.summary, /good adherence/);
});

test('empty window reports no data rather than fabricating a fraction', () => {
  const r = computeAdherence([], 0, 1000);
  assert.equal(r.wornFraction, null);
  assert.equal(r.sampleCount, 0);
});

test('samples outside window are excluded', () => {
  const samples = [
    { timestampMs: -500, imuMotionDetected: true, isCharging: false },
    { timestampMs: 500, imuMotionDetected: false, isCharging: false },
  ];
  const r = computeAdherence(samples, 0, 1000);
  assert.equal(r.sampleCount, 1);
  assert.equal(r.wornFraction, 0);
});
