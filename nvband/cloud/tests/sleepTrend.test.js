const { test } = require('node:test');
const assert = require('node:assert/strict');
const {
  evaluateSleepTrendFlag,
  reviewSleepTrendForPatient,
  DEFAULT_CLINICIAN_THRESHOLD,
} = require('../clinician-portal/sleepTrend');
const { AuditLog } = require('../audit/auditLog');

function nightSummary(overrides = {}) {
  return {
    patientId: 'p1',
    nightDate: '2026-08-19',
    timeInStageMinutes: { WAKE: 30, N1: 20, N2: 180, N3: 90, REM: 100 },
    totalRecordedMinutes: 420,
    sleepEfficiencyPercent: 92.9,
    stageTransitionCount: 25,
    ...overrides,
  };
}

function patient(connections = []) {
  return { patientId: 'p1', clinicianConnections: new Set(connections) };
}

test('a normal night is not flagged under default thresholds', () => {
  const result = evaluateSleepTrendFlag(nightSummary());
  assert.equal(result.flagged, false);
  assert.deepEqual(result.reasons, []);
});

test('low N3 minutes below the clinician threshold is flagged with a reason', () => {
  const summary = nightSummary({ timeInStageMinutes: { WAKE: 30, N1: 20, N2: 300, N3: 10, REM: 60 } });
  const result = evaluateSleepTrendFlag(summary, { minN3MinutesForNormal: 40 });
  assert.equal(result.flagged, true);
  assert.equal(result.reasons.length, 1);
  assert.match(result.reasons[0], /deep sleep \(N3\)/);
});

test('high fragmentation above the clinician threshold is flagged with a reason', () => {
  const summary = nightSummary({ stageTransitionCount: 200 });
  const result = evaluateSleepTrendFlag(summary, { maxFragmentationTransitionsForNormal: 60 });
  assert.equal(result.flagged, true);
  assert.match(result.reasons[0], /stage transitions/);
});

test('both signals crossing threshold produces two reasons, not just one', () => {
  const summary = nightSummary({
    timeInStageMinutes: { WAKE: 30, N1: 20, N2: 300, N3: 5, REM: 60 },
    stageTransitionCount: 500,
  });
  const result = evaluateSleepTrendFlag(summary, {
    minN3MinutesForNormal: 40,
    maxFragmentationTransitionsForNormal: 60,
  });
  assert.equal(result.flagged, true);
  assert.equal(result.reasons.length, 2);
});

test('a threshold field left unconfigured (undefined) never contributes a reason', () => {
  const summary = nightSummary({ timeInStageMinutes: { WAKE: 30, N1: 20, N2: 300, N3: 0, REM: 60 } });
  const result = evaluateSleepTrendFlag(summary, { maxFragmentationTransitionsForNormal: 1000 });
  // N3 is effectively zero but no minN3MinutesForNormal was configured,
  // so it must not be a reason to flag.
  assert.equal(result.flagged, false);
});

test('flag record carries patientId/nightDate and a snapshot, but is not itself a notification', () => {
  const result = evaluateSleepTrendFlag(nightSummary(), { minN3MinutesForNormal: 999 }, 1234567);
  assert.equal(result.patientId, 'p1');
  assert.equal(result.nightDate, '2026-08-19');
  assert.equal(result.createdAtMs, 1234567);
  assert.ok(result.nightSummarySnapshot);
  // Explicit negative check on the module's public surface: no
  // send/notify/alert/push function exists on this module at all.
  const sleepTrend = require('../clinician-portal/sleepTrend');
  for (const key of Object.keys(sleepTrend)) {
    assert.doesNotMatch(key.toLowerCase(), /notify|alert|push|sms|pager/);
  }
});

test('DEFAULT_CLINICIAN_THRESHOLD is exported and usable as-is', () => {
  const result = evaluateSleepTrendFlag(nightSummary(), DEFAULT_CLINICIAN_THRESHOLD);
  assert.equal(typeof result.flagged, 'boolean');
});

test('reviewSleepTrendForPatient denies and audits, without computing a flag, for an unconnected clinician', () => {
  const log = new AuditLog();
  const result = reviewSleepTrendForPatient(
    { role: 'clinician', actorId: 'doc-stranger' },
    patient(['doc-1']),
    nightSummary(),
    { minN3MinutesForNormal: 999 },
    log
  );
  assert.equal(result.allowed, false);
  assert.equal(result.flag, null);
  assert.equal(log.records.length, 1);
  assert.equal(log.records[0].entry.allowed, false);
  assert.equal(log.records[0].entry.store, 'derived_analytics');
});

test('reviewSleepTrendForPatient allows, audits, and computes a flag for a connected clinician', () => {
  const log = new AuditLog();
  const result = reviewSleepTrendForPatient(
    { role: 'clinician', actorId: 'doc-1' },
    patient(['doc-1']),
    nightSummary({ timeInStageMinutes: { WAKE: 30, N1: 20, N2: 300, N3: 5, REM: 60 } }),
    { minN3MinutesForNormal: 40 },
    log
  );
  assert.equal(result.allowed, true);
  assert.equal(result.flag.flagged, true);
  assert.equal(log.records.length, 1);
  assert.equal(log.records[0].entry.allowed, true);
  assert.deepEqual(log.verifyChain(), { valid: true });
});

test('reviewSleepTrendForPatient allows a caregiver (derived_analytics access class, same as adherence)', () => {
  const log = new AuditLog();
  const result = reviewSleepTrendForPatient(
    { role: 'caregiver', actorId: 'cg-1' },
    patient([]),
    nightSummary(),
    {},
    log
  );
  // caregiver role DOES have derived_analytics access per
  // cloud/storage/retentionPolicy.js's STORE_ACCESS_BY_ROLE, and (unlike
  // clinician) is never gated by clinicianConnections -- this test
  // documents that a caregiver *is* allowed to see the wellness-framed
  // sleep trend view (not raw data), same access class as adherence.
  assert.equal(result.allowed, true);
});

test('every access decision produces exactly one audit entry, allowed or not', () => {
  const log = new AuditLog();
  reviewSleepTrendForPatient({ role: 'clinician', actorId: 'doc-1' }, patient(['doc-1']), nightSummary(), {}, log);
  reviewSleepTrendForPatient({ role: 'clinician', actorId: 'doc-2' }, patient(['doc-1']), nightSummary(), {}, log);
  assert.equal(log.records.length, 2);
});
