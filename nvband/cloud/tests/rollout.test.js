const { test } = require('node:test');
const assert = require('node:assert/strict');
const { ROLLOUT_STAGES, detectFaultRateSpike, RolloutState } = require('../fleet-ota/rollout');

test('small cohort never triggers a halt regardless of rate', () => {
  const r = detectFaultRateSpike({ baselineFaultRate: 0.01, cohortFaultRate: 0.9, cohortSize: 3 });
  assert.equal(r.isSpike, false);
});

test('large cohort with 3x+ baseline rate is a spike', () => {
  const r = detectFaultRateSpike({ baselineFaultRate: 0.01, cohortFaultRate: 0.05, cohortSize: 100 });
  assert.equal(r.isSpike, true);
});

test('faults appearing against a zero baseline is a spike', () => {
  const r = detectFaultRateSpike({ baselineFaultRate: 0, cohortFaultRate: 0.02, cohortSize: 100 });
  assert.equal(r.isSpike, true);
});

test('zero baseline and zero cohort rate is not a spike', () => {
  const r = detectFaultRateSpike({ baselineFaultRate: 0, cohortFaultRate: 0, cohortSize: 100 });
  assert.equal(r.isSpike, false);
});

test('rollout advances through stages when healthy', () => {
  const rollout = new RolloutState('r1', 'fw-1.2.0');
  assert.equal(rollout.currentStagePercent(), ROLLOUT_STAGES[0]);
  for (let i = 0; i < ROLLOUT_STAGES.length - 1; i++) {
    const status = rollout.evaluateAndAdvance({ baselineFaultRate: 0.01, cohortFaultRate: 0.01, cohortSize: 100 });
    assert.equal(status, 'IN_PROGRESS');
  }
  const finalStatus = rollout.evaluateAndAdvance({ baselineFaultRate: 0.01, cohortFaultRate: 0.01, cohortSize: 100 });
  assert.equal(finalStatus, 'COMPLETE');
});

test('rollout halts on a fault-rate spike and never auto-advances or reverts after', () => {
  const rollout = new RolloutState('r2', 'fw-1.3.0');
  const status = rollout.evaluateAndAdvance({ baselineFaultRate: 0.01, cohortFaultRate: 0.5, cohortSize: 100 });
  assert.equal(status, 'HALTED');
  assert.ok(rollout.haltReason);

  // Further evaluation calls are no-ops — halted stays halted, no
  // automatic recovery or reversion of any kind.
  const status2 = rollout.evaluateAndAdvance({ baselineFaultRate: 0.01, cohortFaultRate: 0.01, cohortSize: 100 });
  assert.equal(status2, 'HALTED');
});

test('RolloutState has no revert/rollback-devices method', () => {
  const rollout = new RolloutState('r3', 'fw-1.4.0');
  assert.equal(typeof rollout.revertDevices, 'undefined');
  assert.equal(typeof rollout.rollbackFleet, 'undefined');
});
