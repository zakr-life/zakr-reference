const { test } = require('node:test');
const assert = require('node:assert/strict');
const {
  defaultFederatedLearningConsentState, setFederatedLearningConsent,
  federatedLearningAllowed,
} = require('../mobile/src/state/federatedLearningConsent');

test('federated learning consent defaults off', () => {
  const s = defaultFederatedLearningConsentState();
  assert.equal(s.federatedLearningImprovement, false);
  assert.equal(federatedLearningAllowed(s), false);
});

test('setFederatedLearningConsent logs a discrete change event', () => {
  const s = defaultFederatedLearningConsentState();
  const { newState, logEntry } = setFederatedLearningConsent(s, 'federatedLearningImprovement', true);
  assert.equal(newState.federatedLearningImprovement, true);
  assert.equal(logEntry.type, 'CONSENT_CHANGED');
  assert.equal(logEntry.previousValue, false);
  assert.equal(logEntry.newValue, true);
});

test('federatedLearningAllowed reflects the current toggle', () => {
  let s = defaultFederatedLearningConsentState();
  assert.equal(federatedLearningAllowed(s), false);
  ({ newState: s } = setFederatedLearningConsent(s, 'federatedLearningImprovement', true));
  assert.equal(federatedLearningAllowed(s), true);
});

test('unknown consent key throws', () => {
  const s = defaultFederatedLearningConsentState();
  assert.throws(() => setFederatedLearningConsent(s, 'notARealKey', true));
});
