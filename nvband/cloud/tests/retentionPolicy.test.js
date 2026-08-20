const { test } = require('node:test');
const assert = require('node:assert/strict');
const { RetentionPolicy, canAccessStore } = require('../storage/retentionPolicy');

test('construction requires finite positive retention for every store', () => {
  assert.throws(() => new RetentionPolicy({ rawSessionDataDays: Infinity, derivedAnalyticsDays: 30, identityStoreDays: 3650 }));
  assert.throws(() => new RetentionPolicy({ rawSessionDataDays: 90, derivedAnalyticsDays: -1, identityStoreDays: 3650 }));
  assert.throws(() => new RetentionPolicy({ rawSessionDataDays: 90, derivedAnalyticsDays: 30 })); // missing field
});

test('shouldPurge respects per-store limits independently', () => {
  const policy = new RetentionPolicy({ rawSessionDataDays: 90, derivedAnalyticsDays: 365, identityStoreDays: 3650 });
  assert.equal(policy.shouldPurge('raw_session_data', 89), false);
  assert.equal(policy.shouldPurge('raw_session_data', 91), true);
  assert.equal(policy.shouldPurge('derived_analytics', 91), false); // different, longer limit
});

test('unknown store name throws rather than silently allowing/denying', () => {
  const policy = new RetentionPolicy({ rawSessionDataDays: 90, derivedAnalyticsDays: 365, identityStoreDays: 3650 });
  assert.throws(() => policy.shouldPurge('made_up_store', 1));
});

test('store access scoping: clinician never gets identity store directly', () => {
  assert.equal(canAccessStore('clinician', 'raw_session_data'), true);
  assert.equal(canAccessStore('clinician', 'identity_store'), false);
});

test('researcher only ever gets derived/de-identified analytics', () => {
  assert.equal(canAccessStore('researcher', 'derived_analytics'), true);
  assert.equal(canAccessStore('researcher', 'raw_session_data'), false);
  assert.equal(canAccessStore('researcher', 'identity_store'), false);
});

test('unknown role has no access to anything', () => {
  assert.equal(canAccessStore('random_role', 'derived_analytics'), false);
});
