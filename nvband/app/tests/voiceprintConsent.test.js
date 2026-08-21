const { test } = require('node:test');
const assert = require('node:assert/strict');
const { defaultVoiceprintConsentState, setVoiceprintConsent } =
  require('../mobile/src/state/voiceprintConsent');

test('both voiceprint consent toggles default to off', () => {
  const s = defaultVoiceprintConsentState();
  assert.equal(s.voiceprintEnrollmentConsent, false);
  assert.equal(s.voiceprintCloudBackupConsent, false);
});

test('setVoiceprintConsent produces a logged, discrete change', () => {
  const s0 = defaultVoiceprintConsentState();
  const { newState, logEntry } = setVoiceprintConsent(s0, 'voiceprintEnrollmentConsent', true, 12345);
  assert.equal(newState.voiceprintEnrollmentConsent, true);
  assert.equal(newState.voiceprintCloudBackupConsent, false); // other toggle untouched
  assert.equal(logEntry.type, 'CONSENT_CHANGED');
  assert.equal(logEntry.key, 'voiceprintEnrollmentConsent');
  assert.equal(logEntry.previousValue, false);
  assert.equal(logEntry.newValue, true);
  assert.equal(logEntry.timestampMs, 12345);
});

test('unknown consent key throws rather than silently no-op', () => {
  const s0 = defaultVoiceprintConsentState();
  assert.throws(() => setVoiceprintConsent(s0, 'somethingElse', true));
});

test('enrollment and cloud-backup consent toggles are independent of each other', () => {
  const s0 = defaultVoiceprintConsentState();
  const { newState: s1 } = setVoiceprintConsent(s0, 'voiceprintCloudBackupConsent', true);
  assert.equal(s1.voiceprintEnrollmentConsent, false);
  assert.equal(s1.voiceprintCloudBackupConsent, true);
});

test('turning cloud backup on does not implicitly turn enrollment on', () => {
  const s0 = defaultVoiceprintConsentState();
  const { newState } = setVoiceprintConsent(s0, 'voiceprintCloudBackupConsent', true);
  assert.equal(newState.voiceprintEnrollmentConsent, false);
});

test('consent state object returned is a plain object usable for the next call', () => {
  const s0 = defaultVoiceprintConsentState();
  const { newState: s1 } = setVoiceprintConsent(s0, 'voiceprintEnrollmentConsent', true);
  const { newState: s2 } = setVoiceprintConsent(s1, 'voiceprintCloudBackupConsent', true);
  assert.equal(s2.voiceprintEnrollmentConsent, true);
  assert.equal(s2.voiceprintCloudBackupConsent, true);
});
