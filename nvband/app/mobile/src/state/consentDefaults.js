/**
 * consentDefaults.js — Consent toggle defaults and change logging.
 *
 * CLAUDE.md §5: "explicit, separate consent toggles for (a) clinician
 * data sharing and (b) de-identified research/analytics use — default
 * both to off."
 */

function defaultConsentState() {
  return Object.freeze({
    clinicianDataSharing: false,
    researchAnalytics: false,
  });
}

/**
 * Every consent change is a discrete, logged event (feeds
 * cloud/audit/'s consent-change log — CLAUDE.md §6) — never a silent
 * mutation. Returns the new state and the log entry to persist/sync.
 */
function setConsent(currentState, key, value, nowMs = Date.now()) {
  if (key !== 'clinicianDataSharing' && key !== 'researchAnalytics') {
    throw new Error(`unknown consent key: ${key}`);
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

module.exports = { defaultConsentState, setConsent };
