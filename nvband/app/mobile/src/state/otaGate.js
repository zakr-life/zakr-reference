/**
 * otaGate.js — OTA update flow gating.
 *
 * CLAUDE.md §5: "never auto-apply a firmware update during an active or
 * scheduled session window."
 */

/**
 * @param {'DISCONNECTED'|'IDLE'|'PENDING_START'|'ACTIVE'|'PAUSED_FAULT'|'PENDING_STOP'} sessionUiState
 * @param {{startsAtMs: number, endsAtMs: number}[]} scheduledSessionWindows
 * @param {number} nowMs
 */
function canAutoApplyUpdate(sessionUiState, scheduledSessionWindows, nowMs) {
  if (sessionUiState === 'ACTIVE' || sessionUiState === 'PENDING_START' ||
      sessionUiState === 'PENDING_STOP' || sessionUiState === 'PAUSED_FAULT') {
    return { allowed: false, reason: 'a session is active or in transition' };
  }

  const inScheduledWindow = scheduledSessionWindows.some(
    w => nowMs >= w.startsAtMs && nowMs <= w.endsAtMs
  );
  if (inScheduledWindow) {
    return { allowed: false, reason: 'within a scheduled session window' };
  }

  return { allowed: true, reason: null };
}

/** User-initiated updates are never blocked by this gate — only
 *  AUTOMATIC/scheduled application is. A user who explicitly taps
 *  "Update Now" during a session window still sees a clear warning in
 *  the UI, but the decision is theirs; this function is not the source
 *  of that separate confirmation-copy path (see screens/OtaUpdateScreen). */
module.exports = { canAutoApplyUpdate };
