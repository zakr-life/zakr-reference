/**
 * voiceprintConsent.js — Voiceprint-specific consent toggle defaults and
 * change logging.
 *
 * Addendum 2 §B: "[raw audio is] never transmitted, unless the user has
 * separately and explicitly opted into cloud voiceprint backup (a
 * distinct, off-by-default consent toggle)." This module is a SEPARATE,
 * standalone consent module (deliberately not merged into
 * state/consentDefaults.js in this pass — see the task boundary this was
 * written under) exposing two distinct, independently toggleable,
 * default-off consents:
 *   - voiceprintEnrollmentConsent: local, on-device enrollment/
 *     verification at all (mic capture happens only if this is true).
 *   - voiceprintCloudBackupConsent: the separate, optional decision to
 *     let the enrolled TEMPLATE (never raw audio) sync to cloud backup —
 *     see cloud/biometrics/voiceprintTemplateStore.js, which refuses
 *     storage without this exact flag.
 *
 * Matches state/consentDefaults.js's pattern exactly (both default off,
 * a discrete logged change event per toggle) so it can be merged into
 * the main consent screen later.
 */

function defaultVoiceprintConsentState() {
  return Object.freeze({
    voiceprintEnrollmentConsent: false,
    voiceprintCloudBackupConsent: false,
  });
}

const VALID_KEYS = Object.freeze(['voiceprintEnrollmentConsent', 'voiceprintCloudBackupConsent']);

/**
 * Every consent change is a discrete, logged event (feeds cloud/audit/'s
 * consent-change log — CLAUDE.md §6) — never a silent mutation. Returns
 * the new state and the log entry to persist/sync, mirroring
 * state/consentDefaults.js's setConsent() exactly.
 */
function setVoiceprintConsent(currentState, key, value, nowMs = Date.now()) {
  if (!VALID_KEYS.includes(key)) {
    throw new Error(`unknown voiceprint consent key: ${key}`);
  }
  const newState = { ...currentState, [key]: value };
  const logEntry = {
    type: 'CONSENT_CHANGED',
    key,
    previousValue: currentState[key],
    newValue: value,
    timestampMs: nowMs,
  };
  return { newState, logEntry };
}

module.exports = { defaultVoiceprintConsentState, setVoiceprintConsent, VALID_KEYS };
