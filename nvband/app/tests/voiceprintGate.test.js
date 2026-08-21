const { test } = require('node:test');
const assert = require('node:assert/strict');
const {
  EnrollmentState, CaptureRequestAppState, ENROLLMENT_EVENTS,
  requestCaptureAllowed, nextEnrollmentState,
} = require('../mobile/src/state/voiceprintGate');

test('capture is allowed only during an explicit voiceprint screen', () => {
  assert.equal(requestCaptureAllowed(CaptureRequestAppState.VOICE_ENROLLMENT_ACTIVE), true);
  assert.equal(requestCaptureAllowed(CaptureRequestAppState.VOICE_VERIFICATION_ACTIVE), true);
});

test('capture is refused when the app is merely open on some other screen', () => {
  assert.equal(requestCaptureAllowed(CaptureRequestAppState.IDLE_FOREGROUND), false);
});

test('capture is refused when the app is backgrounded -- no ambient listening', () => {
  assert.equal(requestCaptureAllowed(CaptureRequestAppState.BACKGROUND), false);
});

test('capture is refused for an unrecognized/undefined app state, never default-allow', () => {
  assert.equal(requestCaptureAllowed('SOME_UNKNOWN_STATE'), false);
  assert.equal(requestCaptureAllowed(undefined), false);
  assert.equal(requestCaptureAllowed(null), false);
});

test('enrollment: NOT_ENROLLED -> ENROLLING on START', () => {
  const next = nextEnrollmentState(EnrollmentState.NOT_ENROLLED, ENROLLMENT_EVENTS.START);
  assert.equal(next, EnrollmentState.ENROLLING);
});

test('enrollment: ENROLLING -> ENROLLED on SUCCEEDED', () => {
  const next = nextEnrollmentState(EnrollmentState.ENROLLING, ENROLLMENT_EVENTS.SUCCEEDED);
  assert.equal(next, EnrollmentState.ENROLLED);
});

test('enrollment: ENROLLING -> FAILED on FAILED', () => {
  const next = nextEnrollmentState(EnrollmentState.ENROLLING, ENROLLMENT_EVENTS.FAILED);
  assert.equal(next, EnrollmentState.FAILED);
});

test('enrollment: FAILED -> ENROLLING on RETRY', () => {
  const next = nextEnrollmentState(EnrollmentState.FAILED, ENROLLMENT_EVENTS.RETRY);
  assert.equal(next, EnrollmentState.ENROLLING);
});

test('enrollment: ENROLLED -> NOT_ENROLLED on RESET', () => {
  const next = nextEnrollmentState(EnrollmentState.ENROLLED, ENROLLMENT_EVENTS.RESET);
  assert.equal(next, EnrollmentState.NOT_ENROLLED);
});

test('enrollment: undefined transitions are a no-op, never an implicit guess', () => {
  assert.equal(
    nextEnrollmentState(EnrollmentState.NOT_ENROLLED, ENROLLMENT_EVENTS.SUCCEEDED),
    EnrollmentState.NOT_ENROLLED
  );
  assert.equal(
    nextEnrollmentState(EnrollmentState.ENROLLED, ENROLLMENT_EVENTS.START),
    EnrollmentState.ENROLLED
  );
  assert.equal(
    nextEnrollmentState('SOME_UNKNOWN_STATE', ENROLLMENT_EVENTS.START),
    'SOME_UNKNOWN_STATE'
  );
});

test('enrollment: multiple UTTERANCE_CAPTURED events stay in ENROLLING', () => {
  let state = nextEnrollmentState(EnrollmentState.NOT_ENROLLED, ENROLLMENT_EVENTS.START);
  state = nextEnrollmentState(state, ENROLLMENT_EVENTS.UTTERANCE_CAPTURED);
  state = nextEnrollmentState(state, ENROLLMENT_EVENTS.UTTERANCE_CAPTURED);
  assert.equal(state, EnrollmentState.ENROLLING);
});
