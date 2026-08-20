const { test } = require('node:test');
const assert = require('node:assert/strict');
const { BleCommand, AttestState, isCommandAllowed } = require('../mobile/src/ble/attestationGate');

test('unauthenticated app cannot start session, OTA, or rearm', () => {
  assert.equal(isCommandAllowed(AttestState.UNAUTHENTICATED, BleCommand.SESSION_START), false);
  assert.equal(isCommandAllowed(AttestState.UNAUTHENTICATED, BleCommand.OTA_BEGIN), false);
  assert.equal(isCommandAllowed(AttestState.UNAUTHENTICATED, BleCommand.REARM_REQUEST), false);
});

test('stop/pause/read-status allowed in every attestation state', () => {
  for (const state of Object.values(AttestState)) {
    assert.equal(isCommandAllowed(state, BleCommand.SESSION_STOP), true, `stop denied in ${state}`);
    assert.equal(isCommandAllowed(state, BleCommand.SESSION_PAUSE), true, `pause denied in ${state}`);
    assert.equal(isCommandAllowed(state, BleCommand.READ_STATUS), true, `status denied in ${state}`);
  }
});

test('authenticated state allows session start', () => {
  assert.equal(isCommandAllowed(AttestState.AUTHENTICATED, BleCommand.SESSION_START), true);
});

test('revoked state behaves like unauthenticated for gated commands', () => {
  assert.equal(isCommandAllowed(AttestState.REVOKED, BleCommand.SESSION_START), false);
  assert.equal(isCommandAllowed(AttestState.REVOKED, BleCommand.SESSION_STOP), true);
});
