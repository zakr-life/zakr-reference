const { test } = require('node:test');
const assert = require('node:assert/strict');
const { ManufacturingRecordStore, REQUIRED_RECORD_TYPES } = require('../manufacturingRecords');
const { KeyCeremonyClient } = require('../keyCeremony');

function writeAllPassing(store, unitId, overrides = {}) {
  for (const type of REQUIRED_RECORD_TYPES) {
    store.writeRecord(unitId, type, { pass: true, ...(overrides[type] || {}) });
  }
}

test('unit with no records at all is not built', () => {
  const store = new ManufacturingRecordStore();
  const result = store.isUnitBuilt('unit-001');
  assert.equal(result.built, false);
});

test('unit missing exactly one required record is not built', () => {
  const store = new ManufacturingRecordStore();
  for (const type of REQUIRED_RECORD_TYPES.slice(0, -1)) {
    store.writeRecord('unit-002', type, { pass: true });
  }
  const result = store.isUnitBuilt('unit-002');
  assert.equal(result.built, false);
  assert.match(result.reason, new RegExp(REQUIRED_RECORD_TYPES.at(-1)));
});

test('unit with all four records present but one FAILING is not built', () => {
  const store = new ManufacturingRecordStore();
  writeAllPassing(store, 'unit-003');
  store.writeRecord('unit-003', 'hipot', { pass: false, note: 'shorted at 1200V' });
  const result = store.isUnitBuilt('unit-003');
  assert.equal(result.built, false);
  assert.match(result.reason, /hipot/);
});

test('unit with all four records present and passing IS built', () => {
  const store = new ManufacturingRecordStore();
  writeAllPassing(store, 'unit-004');
  const result = store.isUnitBuilt('unit-004');
  assert.equal(result.built, true);
});

test('unknown record type is rejected at write time', () => {
  const store = new ManufacturingRecordStore();
  assert.throws(() => store.writeRecord('unit-005', 'not_a_real_gate', { pass: true }));
});

test('record without a boolean pass field is rejected', () => {
  const store = new ManufacturingRecordStore();
  assert.throws(() => store.writeRecord('unit-006', 'hipot', { note: 'no pass field' }));
});

test('firmware-facing HiPot reference check matches the combined invariant', () => {
  const store = new ManufacturingRecordStore();
  assert.equal(store.hasValidHipotReference('unit-007'), false);
  store.writeRecord('unit-007', 'hipot', { pass: true });
  assert.equal(store.hasValidHipotReference('unit-007'), true);
});

test('key ceremony runs once per unit and refuses a second run', () => {
  const store = new ManufacturingRecordStore();
  const client = new KeyCeremonyClient(store, (unitId) => ({
    publicKeyPem: '-----BEGIN PUBLIC KEY-----\nMOCK\n-----END PUBLIC KEY-----',
    serial: `SN-${unitId}`,
  }));

  const first = client.runCeremony('unit-008');
  assert.equal(first.success, true);
  assert.throws(() => client.runCeremony('unit-008'));
});

test('key ceremony failure is recorded as a failing record, not silently skipped', () => {
  const store = new ManufacturingRecordStore();
  const client = new KeyCeremonyClient(store, () => null); // simulate SE failure
  const result = client.runCeremony('unit-009');
  assert.equal(result.success, false);
  const built = store.isUnitBuilt('unit-009');
  assert.equal(built.built, false);
});

test('end-to-end: a unit is only built after ceremony + all manufacturing gates pass', () => {
  const store = new ManufacturingRecordStore();
  const client = new KeyCeremonyClient(store, (unitId) => ({
    publicKeyPem: 'MOCKPUB', serial: `SN-${unitId}`,
  }));
  client.runCeremony('unit-010');
  assert.equal(store.isUnitBuilt('unit-010').built, false); // ceremony alone isn't enough

  store.writeRecord('unit-010', 'shield_fence_electrical_test', { pass: true });
  store.writeRecord('unit-010', 'hipot', { pass: true });
  store.writeRecord('unit-010', 'swell_gap_check', { pass: true });

  assert.equal(store.isUnitBuilt('unit-010').built, true);
});
