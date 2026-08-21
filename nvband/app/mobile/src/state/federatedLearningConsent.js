/**
 * federatedLearningConsent.js — Federated-learning consent toggle
 * defaults and change logging.
 *
 * Addendum 2 §D: "gated by its own separate, off-by-default consent
 * toggle ('federated model improvement')." Deliberately a separate,
 * standalone module from state/consentDefaults.js and
 * state/voiceprintConsent.js — the app renders all three together
 * (see screens/ConsentSettingsScreen.tsx) but each concern is
 * independently testable and independently off by default, so a
 * regression in one can never silently flip another.
 *
 * When federatedLearningImprovement is true, the app is permitted to
 * compute and submit a clipped local model delta (see
 * models/federated/local_update.py, cloud/federated/aggregator.js) after
 * a session. When false, no such computation or submission may occur —
 * this flag is the single gate a caller must check before invoking any
 * federated-learning code path.
 */

function defaultFederatedLearningConsentState() {
  return Object.freeze({
    federatedLearningImprovement: false,
  });
}

const VALID_KEYS = Object.freeze(['federatedLearningImprovement']);

function setFederatedLearningConsent(currentState, key, value, nowMs = Date.now()) {
  if (!VALID_KEYS.includes(key)) {
    throw new Error(`unknown federated-learning consent key: ${key}`);
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

/** The single gate a caller must check before computing/submitting any
 *  federated local update. */
function federatedLearningAllowed(consentState) {
  return consentState.federatedLearningImprovement === true;
}

module.exports = {
  defaultFederatedLearningConsentState,
  setFederatedLearningConsent,
  federatedLearningAllowed,
  VALID_KEYS,
};
