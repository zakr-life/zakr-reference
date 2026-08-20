const { test } = require('node:test');
const assert = require('node:assert/strict');
const { defaultConsentState, setConsent } = require('../mobile/src/state/consentDefaults');

test('both consent toggles default to off', () => {
  const s = defaultConsentState();
  assert.equal(s.clinicianDataSharing, false);
  assert.equal(s.researchAnalytics, false);
});

test('setConsent produces a logged, discrete change', () => {
  const s0 = defaultConsentState();
  const { newState, logEntry } = setConsent(s0, 'clinicianDataSharing', true, 12345);
  assert.equal(newState.clinicianDataSharing, true);
  assert.equal(newState.researchAnalytics, false); // other toggle untouched
  assert.equal(logEntry.type, 'CONSENT_CHANGED');
  assert.equal(logEntry.previousValue, false);
  assert.equal(logEntry.newValue, true);
  assert.equal(logEntry.timestampMs, 12345);
});

test('unknown consent key throws rather than silently no-op', () => {
  const s0 = defaultConsentState();
  assert.throws(() => setConsent(s0, 'somethingElse', true));
});

test('consent toggles are independent of each other', () => {
  const s0 = defaultConsentState();
  const { newState: s1 } = setConsent(s0, 'researchAnalytics', true);
  assert.equal(s1.clinicianDataSharing, false);
  assert.equal(s1.researchAnalytics, true);
});
