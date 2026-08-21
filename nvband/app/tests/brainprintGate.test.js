const { test } = require('node:test');
const assert = require('node:assert/strict');
const {
  EnrollmentState, GateResult, initialEnrollmentState, reduceEnrollment,
  evaluateBrainprintGate,
} = require('../mobile/src/state/brainprintGate');

test('enrollment starts NOT_ENROLLED and transitions on events', () => {
  let s = initialEnrollmentState();
  assert.equal(s.enrollment, EnrollmentState.NOT_ENROLLED);
  s = reduceEnrollment(s, { type: 'ENROLLMENT_STARTED' });
  assert.equal(s.enrollment, EnrollmentState.ENROLLING);
  s = reduceEnrollment(s, { type: 'ENROLLMENT_SUCCEEDED' });
  assert.equal(s.enrollment, EnrollmentState.ENROLLED);
});

test('failed enrollment is a distinct state, and reset returns to NOT_ENROLLED', () => {
  let s = initialEnrollmentState();
  s = reduceEnrollment(s, { type: 'ENROLLMENT_STARTED' });
  s = reduceEnrollment(s, { type: 'ENROLLMENT_FAILED' });
  assert.equal(s.enrollment, EnrollmentState.ENROLLMENT_FAILED);
  s = reduceEnrollment(s, { type: 'ENROLLMENT_RESET' });
  assert.equal(s.enrollment, EnrollmentState.NOT_ENROLLED);
});

test('no-lockout invariant: match yields MATCH, everything else yields FALLBACK, never anything else', () => {
  assert.equal(evaluateBrainprintGate(true), GateResult.MATCH);
  assert.equal(evaluateBrainprintGate(false), GateResult.FALLBACK);
  // there are only ever two possible outcomes
  assert.deepEqual(Object.values(GateResult).sort(), ['FALLBACK', 'MATCH']);
});

test('repeated mismatches never escalate beyond FALLBACK (no accumulating lockout state)', () => {
  for (let i = 0; i < 1000; i++) {
    assert.equal(evaluateBrainprintGate(false), GateResult.FALLBACK);
  }
});

test('unknown event leaves enrollment state unchanged', () => {
  const s = initialEnrollmentState();
  const s2 = reduceEnrollment(s, { type: 'NOT_A_REAL_EVENT' });
  assert.deepEqual(s, s2);
});
