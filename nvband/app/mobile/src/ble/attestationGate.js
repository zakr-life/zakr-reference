/**
 * attestationGate.js — App-side command gating, mirroring
 * firmware/core1_inference_radio/ble/attestation.c.
 *
 * This is deliberately a SEPARATE, independent implementation of the
 * same policy (not a shared library imported by both sides) — the app
 * can be compromised or buggy, so firmware's own gate
 * (attestation_command_allowed in the C module) is the actual
 * authoritative check. This module exists so the app's UI doesn't even
 * attempt to send commands it knows will be refused (better UX), and so
 * this repo has a second, independent test of the SAME policy — a
 * regression in one without the other is exactly the kind of drift this
 * duplication is meant to catch during review.
 */

const BleCommand = Object.freeze({
  SESSION_START: 'SESSION_START',
  SESSION_STOP: 'SESSION_STOP',
  SESSION_PAUSE: 'SESSION_PAUSE',
  OTA_BEGIN: 'OTA_BEGIN',
  READ_STATUS: 'READ_STATUS',
  REARM_REQUEST: 'REARM_REQUEST',
});

const AttestState = Object.freeze({
  UNAUTHENTICATED: 'UNAUTHENTICATED',
  CHALLENGE_SENT: 'CHALLENGE_SENT',
  AUTHENTICATED: 'AUTHENTICATED',
  REVOKED: 'REVOKED',
});

/**
 * Mirrors nvband_attest_command_allowed(): STOP/PAUSE/READ_STATUS are
 * always allowed; everything else requires AUTHENTICATED.
 */
function isCommandAllowed(attestState, command) {
  if (command === BleCommand.SESSION_STOP ||
      command === BleCommand.SESSION_PAUSE ||
      command === BleCommand.READ_STATUS) {
    return true;
  }
  return attestState === AttestState.AUTHENTICATED;
}

module.exports = { BleCommand, AttestState, isCommandAllowed };
