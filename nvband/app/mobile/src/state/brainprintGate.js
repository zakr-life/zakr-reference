/**
 * brainprintGate.js — App-side mirror of the firmware brainprint
 * auth-gate no-lockout invariant.
 *
 * Addendum 2 §A. Deliberately duplicated from
 * firmware/core1_inference_radio/biometric/brainprint_auth_gate.c rather
 * than shared, matching this repo's existing philosophy of independently
 * testing the same safety/privacy invariant on both firmware and app
 * sides (see attestation.c / attestationGate.js) — a regression in one
 * without the other is a review-catchable drift, not a shared single
 * point of failure.
 */

/** @typedef {'NOT_ENROLLED'|'ENROLLING'|'ENROLLED'|'ENROLLMENT_FAILED'} BrainprintEnrollmentState */

const EnrollmentState = Object.freeze({
  NOT_ENROLLED: 'NOT_ENROLLED',
  ENROLLING: 'ENROLLING',
  ENROLLED: 'ENROLLED',
  ENROLLMENT_FAILED: 'ENROLLMENT_FAILED',
});

const GateResult = Object.freeze({
  MATCH: 'MATCH',
  FALLBACK: 'FALLBACK',
});

function initialEnrollmentState() {
  return { enrollment: EnrollmentState.NOT_ENROLLED };
}

function reduceEnrollment(state, event) {
  switch (event.type) {
    case 'ENROLLMENT_STARTED':
      return { ...state, enrollment: EnrollmentState.ENROLLING };
    case 'ENROLLMENT_SUCCEEDED':
      return { ...state, enrollment: EnrollmentState.ENROLLED };
    case 'ENROLLMENT_FAILED':
      return { ...state, enrollment: EnrollmentState.ENROLLMENT_FAILED };
    case 'ENROLLMENT_RESET':
      return initialEnrollmentState();
    default:
      return state;
  }
}

/**
 * The no-lockout invariant, mirrored at the app layer: this function has
 * exactly two possible return values, matching
 * nvband_brainprint_gate_result_t. There is no third "denied" outcome —
 * a mismatch (or brainprintMatched === false for any reason, including
 * "not enrolled") always routes to FALLBACK, meaning the app must present
 * the conventional passcode/device-pairing unlock path. It is a bug for
 * any caller of this function to treat FALLBACK as "access denied" rather
 * than "use the other unlock path."
 */
function evaluateBrainprintGate(brainprintMatched) {
  return brainprintMatched ? GateResult.MATCH : GateResult.FALLBACK;
}

module.exports = {
  EnrollmentState,
  GateResult,
  initialEnrollmentState,
  reduceEnrollment,
  evaluateBrainprintGate,
};
