const { test } = require('node:test');
const assert = require('node:assert/strict');
const { deidentifyForAnalytics, isGroupSafeToPublish, DIRECT_IDENTIFIER_FIELDS } =
  require('../analytics/deidentify');

test('without consent, no de-identified record is produced at all', () => {
  const result = deidentifyForAnalytics({ researchAnalyticsConsent: false }, { deviceId: 'd1' });
  assert.equal(result.record, null);
});

test('missing consent object is treated as no consent, never a default-allow', () => {
  const result = deidentifyForAnalytics(null, { deviceId: 'd1' });
  assert.equal(result.record, null);
});

test('with consent, all direct identifier fields are stripped', () => {
  const session = {
    deviceId: 'd1', patientId: 'p1', clinicianId: 'c1',
    accountEmail: 'x@example.com', phoneImei: '123', sessionStartMs: 1_700_000_000_000,
    durationMs: 60000,
  };
  const { record } = deidentifyForAnalytics({ researchAnalyticsConsent: true }, session);
  for (const field of DIRECT_IDENTIFIER_FIELDS) {
    assert.equal(field in record, false, `${field} should have been stripped`);
  }
  assert.equal(record.durationMs, 60000); // non-identifying field preserved
  assert.equal(record.deidentified, true);
});

test('timestamps are coarsened to day-level, not left precise', () => {
  const session = { sessionStartMs: 1_700_000_123_456 };
  const { record } = deidentifyForAnalytics({ researchAnalyticsConsent: true }, session);
  assert.equal('sessionStartMs' in record, false);
  assert.equal(record.sessionStartDayMs % (24 * 60 * 60 * 1000), 0);
});

test('k-anonymity gate refuses to publish small groups', () => {
  assert.equal(isGroupSafeToPublish(3), false);
  assert.equal(isGroupSafeToPublish(10), true);
  assert.equal(isGroupSafeToPublish(1), false);
});
