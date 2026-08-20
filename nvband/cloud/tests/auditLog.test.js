const { test } = require('node:test');
const assert = require('node:assert/strict');
const { AuditLog } = require('../audit/auditLog');

test('appended entries chain together and verify clean', () => {
  const log = new AuditLog();
  log.append({ type: 'DATA_ACCESS', actorId: 'clinician-1', timestampMs: 1 });
  log.append({ type: 'CONSENT_CHANGED', actorId: 'user-1', timestampMs: 2 });
  log.append({ type: 'OTA_ROLLOUT', actorId: 'system', timestampMs: 3 });
  assert.deepEqual(log.verifyChain(), { valid: true });
});

test('rejects malformed entries', () => {
  const log = new AuditLog();
  assert.throws(() => log.append(null));
  assert.throws(() => log.append({ actorId: 'x' })); // missing type
});

test('detects a tampered entry', () => {
  const log = new AuditLog();
  log.append({ type: 'DATA_ACCESS', actorId: 'clinician-1', timestampMs: 1 });
  log.append({ type: 'DATA_ACCESS', actorId: 'clinician-2', timestampMs: 2 });
  // Simulate tampering: mutate an entry's content in place without
  // recomputing the hash (exactly what an attacker editing storage
  // directly would do).
  log.records[0].entry.actorId = 'someone-else';
  const result = log.verifyChain();
  assert.equal(result.valid, false);
  assert.equal(result.brokenAtIndex, 0);
});

test('detects a spliced-out (deleted) entry', () => {
  const log = new AuditLog();
  log.append({ type: 'DATA_ACCESS', actorId: 'a', timestampMs: 1 });
  log.append({ type: 'DATA_ACCESS', actorId: 'b', timestampMs: 2 });
  log.append({ type: 'DATA_ACCESS', actorId: 'c', timestampMs: 3 });
  log.records.splice(1, 1); // remove the middle entry
  const result = log.verifyChain();
  assert.equal(result.valid, false);
});

test('has no edit or delete method — append-only by API shape', () => {
  const log = new AuditLog();
  assert.equal(typeof log.editEntry, 'undefined');
  assert.equal(typeof log.deleteEntry, 'undefined');
  assert.equal(typeof log.append, 'function');
});

test('empty log verifies as valid', () => {
  const log = new AuditLog();
  assert.deepEqual(log.verifyChain(), { valid: true });
});
