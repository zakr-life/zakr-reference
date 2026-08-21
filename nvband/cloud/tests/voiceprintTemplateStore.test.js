const { test } = require('node:test');
const assert = require('node:assert/strict');
const {
  storeVoiceprintTemplate, looksLikeTemplate,
  EXPECTED_TEMPLATE_LENGTH, MAX_PLAUSIBLE_TEMPLATE_LENGTH,
} = require('../biometrics/voiceprintTemplateStore');

function makeTemplate(length = EXPECTED_TEMPLATE_LENGTH) {
  return Array.from({ length }, (_, i) => i * 0.1);
}

test('without cloud-backup consent, nothing is stored at all', () => {
  const result = storeVoiceprintTemplate(
    { voiceprintCloudBackupConsent: false },
    makeTemplate()
  );
  assert.equal(result.stored, null);
});

test('missing consent object is treated as no consent, never a default-allow', () => {
  const result = storeVoiceprintTemplate(null, makeTemplate());
  assert.equal(result.stored, null);
});

test('with consent and a correctly-shaped template, storage succeeds', () => {
  const template = makeTemplate();
  const { stored, reason } = storeVoiceprintTemplate(
    { voiceprintCloudBackupConsent: true },
    template
  );
  assert.equal(reason, 'ok');
  assert.notEqual(stored, null);
  assert.deepEqual(stored.template, template);
  assert.equal(typeof stored.storedAt, 'number');
});

test('stored template is a copy, not a reference to the caller-owned array', () => {
  const template = makeTemplate();
  const { stored } = storeVoiceprintTemplate({ voiceprintCloudBackupConsent: true }, template);
  template[0] = 999;
  assert.notEqual(stored.template[0], 999);
});

test('a raw-audio-sized payload is rejected even with consent', () => {
  // e.g. 0.1s of 16kHz PCM -- nowhere near template-shaped.
  const rawAudioLike = new Array(1600).fill(0).map(() => Math.floor(Math.random() * 2000 - 1000));
  const { stored, reason } = storeVoiceprintTemplate(
    { voiceprintCloudBackupConsent: true },
    rawAudioLike
  );
  assert.equal(stored, null);
  assert.match(reason, /not template-shaped/);
});

test('a typed array (the real shape raw PDM/PCM audio would arrive as) is rejected', () => {
  const raw = new Int16Array(2000);
  const { stored } = storeVoiceprintTemplate({ voiceprintCloudBackupConsent: true }, raw);
  assert.equal(stored, null);
});

test('a non-numeric payload is rejected', () => {
  const { stored } = storeVoiceprintTemplate(
    { voiceprintCloudBackupConsent: true },
    ['a', 'b', 'c']
  );
  assert.equal(stored, null);
});

test('a payload containing NaN/Infinity is rejected', () => {
  const bad = makeTemplate();
  bad[3] = NaN;
  const { stored } = storeVoiceprintTemplate({ voiceprintCloudBackupConsent: true }, bad);
  assert.equal(stored, null);
});

test('a wrong-length numeric vector (not matching the model pipeline shape) is rejected', () => {
  const shortVector = makeTemplate(8);
  const { stored, reason } = storeVoiceprintTemplate(
    { voiceprintCloudBackupConsent: true },
    shortVector
  );
  assert.equal(stored, null);
  assert.match(reason, /does not match expected length/);
});

test('an object payload (not an array) is rejected', () => {
  const { stored } = storeVoiceprintTemplate(
    { voiceprintCloudBackupConsent: true },
    { 0: 1, 1: 2, length: 2 }
  );
  assert.equal(stored, null);
});

test('looksLikeTemplate: empty array is not template-shaped', () => {
  assert.equal(looksLikeTemplate([]), false);
});

test('looksLikeTemplate: exactly at the plausible-length ceiling is accepted structurally', () => {
  assert.equal(looksLikeTemplate(makeTemplate(MAX_PLAUSIBLE_TEMPLATE_LENGTH)), true);
});

test('looksLikeTemplate: one past the plausible-length ceiling is rejected', () => {
  assert.equal(looksLikeTemplate(makeTemplate(MAX_PLAUSIBLE_TEMPLATE_LENGTH + 1)), false);
});
