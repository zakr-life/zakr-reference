const { test } = require('node:test');
const assert = require('node:assert/strict');
const { SessionState, initialState, reduce, isStopAvailable, isStartAvailable } =
  require('../mobile/src/state/sessionStateMachine');

test('stop is never available while disconnected or idle', () => {
  let s = initialState();
  assert.equal(isStopAvailable(s), false);
  s = reduce(s, { type: 'BLE_CONNECTED' });
  assert.equal(s.ui, SessionState.IDLE);
  assert.equal(isStopAvailable(s), false);
});

test('stop is available immediately once a start is pending, no confirmation step', () => {
  let s = initialState();
  s = reduce(s, { type: 'BLE_CONNECTED' });
  s = reduce(s, { type: 'USER_TAPPED_START' });
  assert.equal(s.ui, SessionState.PENDING_START);
  assert.equal(isStopAvailable(s), true);
});

test('stop is available while active', () => {
  let s = initialState();
  s = reduce(s, { type: 'BLE_CONNECTED' });
  s = reduce(s, { type: 'DEVICE_STATUS_SESSION_ACTIVE', status: {} });
  assert.equal(s.ui, SessionState.ACTIVE);
  assert.equal(isStopAvailable(s), true);
});

test('stop is available during a fault (never blocked by fault state)', () => {
  let s = initialState();
  s = reduce(s, { type: 'BLE_CONNECTED' });
  s = reduce(s, { type: 'DEVICE_STATUS_SESSION_ACTIVE', status: {} });
  s = reduce(s, { type: 'DEVICE_STATUS_FAULT', fault: 'INTERLOCK_TRIPPED' });
  assert.equal(s.ui, SessionState.PAUSED_FAULT);
  assert.equal(isStopAvailable(s), true);
});

test('start only available when idle', () => {
  let s = initialState();
  assert.equal(isStartAvailable(s), false);
  s = reduce(s, { type: 'BLE_CONNECTED' });
  assert.equal(isStartAvailable(s), true);
  s = reduce(s, { type: 'USER_TAPPED_START' });
  assert.equal(isStartAvailable(s), false);
});

test('local optimism is never assumed: start tap does not itself claim ACTIVE', () => {
  let s = initialState();
  s = reduce(s, { type: 'BLE_CONNECTED' });
  s = reduce(s, { type: 'USER_TAPPED_START' });
  assert.notEqual(s.ui, SessionState.ACTIVE);
  assert.equal(s.ui, SessionState.PENDING_START);
});

test('disconnect from any state returns to DISCONNECTED', () => {
  let s = initialState();
  s = reduce(s, { type: 'BLE_CONNECTED' });
  s = reduce(s, { type: 'DEVICE_STATUS_SESSION_ACTIVE', status: {} });
  s = reduce(s, { type: 'BLE_DISCONNECTED' });
  assert.equal(s.ui, SessionState.DISCONNECTED);
  assert.equal(isStopAvailable(s), false);
});

test('unknown event type leaves state unchanged', () => {
  let s = initialState();
  const s2 = reduce(s, { type: 'NOT_A_REAL_EVENT' });
  assert.deepEqual(s, s2);
});
