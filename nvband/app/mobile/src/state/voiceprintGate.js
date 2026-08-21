/**
 * voiceprintGate.js — App-side enrollment-state tracking and capture-
 * request gating for voiceprint, mirroring
 * firmware/core1_inference_radio/audio/mic_power_gate.c's rule at the
 * app layer.
 *
 * This is deliberately a SEPARATE, independent implementation of the
 * same underlying safety/privacy policy (not a shared library imported
 * by both sides) — same philosophy as
 * app/mobile/src/ble/attestationGate.js mirroring
 * firmware/core1_inference_radio/ble/attestation.c: the app can be
 * compromised or buggy, so firmware's own gate
 * (`nvband_mic_gate_request_capture()`, only reachable during a
 * user-initiated request window) is the actual authoritative check. This
 * module exists so the app's UI never even attempts to trigger a mic
 * capture outside an explicit, foregrounded, user-initiated enrollment
 * or verification screen — and so this repo has a second, independent
 * test of the SAME "gated, session-scoped, no ambient/always-listening"
 * invariant (Addendum 2 §B), on both firmware and app sides. A
 * regression in one without the other is exactly the kind of drift this
 * duplication is meant to catch during review.
 */

const EnrollmentState = Object.freeze({
  NOT_ENROLLED: 'NOT_ENROLLED',
  ENROLLING: 'ENROLLING',
  ENROLLED: 'ENROLLED',
  FAILED: 'FAILED',
});

/**
 * The set of app-foreground states capture may be requested from. Only
 * the two explicit, user-initiated voiceprint screens qualify — never
 * BACKGROUND (app not in the foreground at all) and never
 * IDLE_FOREGROUND (app open, but the user hasn't navigated to a
 * voiceprint action). There is no "always listening" state in this enum
 * by construction.
 */
const CaptureRequestAppState = Object.freeze({
  VOICE_ENROLLMENT_ACTIVE: 'VOICE_ENROLLMENT_ACTIVE',
  VOICE_VERIFICATION_ACTIVE: 'VOICE_VERIFICATION_ACTIVE',
  IDLE_FOREGROUND: 'IDLE_FOREGROUND',
  BACKGROUND: 'BACKGROUND',
});

/**
 * Mirrors the firmware rule: a mic-capture request may only be issued
 * while the app is actively on an explicit voiceprint enrollment or
 * verification screen. Everything else — including simply having the
 * app open on some other screen — is refused.
 */
function requestCaptureAllowed(currentAppState) {
  return (
    currentAppState === CaptureRequestAppState.VOICE_ENROLLMENT_ACTIVE ||
    currentAppState === CaptureRequestAppState.VOICE_VERIFICATION_ACTIVE
  );
}

const ENROLLMENT_EVENTS = Object.freeze({
  START: 'START',
  UTTERANCE_CAPTURED: 'UTTERANCE_CAPTURED',
  SUCCEEDED: 'SUCCEEDED',
  FAILED: 'FAILED',
  RETRY: 'RETRY',
  RESET: 'RESET',
});

/**
 * A small, explicit transition table (never an ad hoc if/else chain a
 * caller could bypass) mirroring the discipline of
 * app/mobile/src/state/sessionStateMachine.js. An event with no defined
 * transition from the current state is a no-op (returns current
 * unchanged), never an implicit fall-through to some other state.
 */
const TRANSITIONS = Object.freeze({
  [EnrollmentState.NOT_ENROLLED]: {
    [ENROLLMENT_EVENTS.START]: EnrollmentState.ENROLLING,
  },
  [EnrollmentState.ENROLLING]: {
    [ENROLLMENT_EVENTS.UTTERANCE_CAPTURED]: EnrollmentState.ENROLLING,
    [ENROLLMENT_EVENTS.SUCCEEDED]: EnrollmentState.ENROLLED,
    [ENROLLMENT_EVENTS.FAILED]: EnrollmentState.FAILED,
  },
  [EnrollmentState.ENROLLED]: {
    [ENROLLMENT_EVENTS.RESET]: EnrollmentState.NOT_ENROLLED,
  },
  [EnrollmentState.FAILED]: {
    [ENROLLMENT_EVENTS.RETRY]: EnrollmentState.ENROLLING,
    [ENROLLMENT_EVENTS.RESET]: EnrollmentState.NOT_ENROLLED,
  },
});

function nextEnrollmentState(currentState, event) {
  const row = TRANSITIONS[currentState];
  if (!row || !(event in row)) {
    return currentState; // undefined transition: no-op, never a guess
  }
  return row[event];
}

module.exports = {
  EnrollmentState,
  CaptureRequestAppState,
  ENROLLMENT_EVENTS,
  requestCaptureAllowed,
  nextEnrollmentState,
};
