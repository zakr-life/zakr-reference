const { test } = require('node:test');
const assert = require('node:assert/strict');
const { canAutoApplyUpdate } = require('../mobile/src/state/otaGate');

test('blocked during an active session', () => {
  const r = canAutoApplyUpdate('ACTIVE', [], Date.now());
  assert.equal(r.allowed, false);
});

test('blocked during pending start/stop transitions', () => {
  assert.equal(canAutoApplyUpdate('PENDING_START', [], Date.now()).allowed, false);
  assert.equal(canAutoApplyUpdate('PENDING_STOP', [], Date.now()).allowed, false);
});

test('blocked within a scheduled session window even if currently idle', () => {
  const now = 5000;
  const windows = [{ startsAtMs: 4000, endsAtMs: 6000 }];
  const r = canAutoApplyUpdate('IDLE', windows, now);
  assert.equal(r.allowed, false);
  assert.match(r.reason, /scheduled session window/);
});

test('allowed when idle and outside any scheduled window', () => {
  const now = 10000;
  const windows = [{ startsAtMs: 4000, endsAtMs: 6000 }];
  const r = canAutoApplyUpdate('IDLE', windows, now);
  assert.equal(r.allowed, true);
});

test('blocked during a fault pause', () => {
  const r = canAutoApplyUpdate('PAUSED_FAULT', [], Date.now());
  assert.equal(r.allowed, false);
});
