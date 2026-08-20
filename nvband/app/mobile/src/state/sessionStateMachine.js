/**
 * sessionStateMachine.js — App-side mirror of device session state.
 *
 * CLAUDE.md §5: "start/stop session (stop must always be immediately
 * available and unambiguous in the UI, and must never be gated behind
 * multi-step confirmation the way a destructive action might be — a stop
 * request is never 'destructive')" and "live session status mirroring
 * the device LEDs" and "the app never becomes the only way to understand
 * device state — mirror, never replace, the on-device legibility."
 *
 * This is a pure state machine: it decides what the UI should render and
 * whether a STOP action is available, given the last known device status
 * mirrored over BLE. It never assumes it IS the device state — every
 * transition here is driven by an incoming status message, never by
 * local optimism (e.g. tapping "Start" does not locally transition to
 * ACTIVE; it stays PENDING until the device confirms).
 */

/** @typedef {'DISCONNECTED'|'IDLE'|'PENDING_START'|'ACTIVE'|'PAUSED_FAULT'|'PENDING_STOP'} SessionUiState */

const SessionState = Object.freeze({
  DISCONNECTED: 'DISCONNECTED',
  IDLE: 'IDLE',
  PENDING_START: 'PENDING_START',
  ACTIVE: 'ACTIVE',
  PAUSED_FAULT: 'PAUSED_FAULT',
  PENDING_STOP: 'PENDING_STOP',
});

function initialState() {
  return { ui: SessionState.DISCONNECTED, lastDeviceStatus: null, lastFault: null };
}

/**
 * STOP is always available except when there is nothing to stop
 * (disconnected or already idle) — never gated by attestation, by a
 * confirmation dialog, or by any other multi-step flow. This mirrors
 * firmware's attestation.c: NVBAND_BLE_CMD_SESSION_STOP is allowed in
 * every attestation state.
 */
function isStopAvailable(state) {
  return state.ui === SessionState.ACTIVE ||
         state.ui === SessionState.PENDING_START ||
         state.ui === SessionState.PAUSED_FAULT;
}

function isStartAvailable(state) {
  return state.ui === SessionState.IDLE;
}

/** Reducer driven entirely by events; never by wall-clock timers making
 *  an optimistic local transition. */
function reduce(state, event) {
  switch (event.type) {
    case 'BLE_CONNECTED':
      return { ...state, ui: SessionState.IDLE };
    case 'BLE_DISCONNECTED':
      return { ...state, ui: SessionState.DISCONNECTED };
    case 'USER_TAPPED_START':
      if (!isStartAvailable(state)) return state;
      return { ...state, ui: SessionState.PENDING_START };
    case 'USER_TAPPED_STOP':
      if (!isStopAvailable(state)) return state;
      // No confirmation step, no dialog — send immediately. The reducer
      // itself does not need to "confirm"; the actual BLE stop command
      // is dispatched by the caller synchronously on this same event.
      return { ...state, ui: SessionState.PENDING_STOP };
    case 'DEVICE_STATUS_SESSION_ACTIVE':
      return { ...state, ui: SessionState.ACTIVE, lastDeviceStatus: event.status };
    case 'DEVICE_STATUS_SESSION_IDLE':
      return { ...state, ui: SessionState.IDLE, lastDeviceStatus: event.status };
    case 'DEVICE_STATUS_FAULT':
      return { ...state, ui: SessionState.PAUSED_FAULT, lastFault: event.fault };
    default:
      return state;
  }
}

module.exports = { SessionState, initialState, reduce, isStopAvailable, isStartAvailable };
