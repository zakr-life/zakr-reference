const { test } = require('node:test');
const assert = require('node:assert/strict');
const { authorizeAndAudit } = require('../clinician-portal/rbac');
const { AuditLog } = require('../audit/auditLog');

function patient(connections = []) {
  return { patientId: 'p1', clinicianConnections: new Set(connections) };
}

test('connected clinician is granted access, and it is audited', () => {
  const log = new AuditLog();
  const result = authorizeAndAudit(
    { role: 'clinician', actorId: 'doc-1' }, patient(['doc-1']),
    'raw_session_data', log
  );
  assert.equal(result.allowed, true);
  assert.equal(log.records.length, 1);
  assert.equal(log.records[0].entry.allowed, true);
});

test('unconnected clinician is denied even though role has store access', () => {
  const log = new AuditLog();
  const result = authorizeAndAudit(
    { role: 'clinician', actorId: 'doc-stranger' }, patient(['doc-1']),
    'raw_session_data', log
  );
  assert.equal(result.allowed, false);
  assert.match(result.reason, /not connected/);
});

test('caregiver denied raw session data regardless of connection', () => {
  const log = new AuditLog();
  const result = authorizeAndAudit(
    { role: 'caregiver', actorId: 'cg-1' }, patient(['cg-1']),
    'raw_session_data', log
  );
  assert.equal(result.allowed, false);
});

test('every access decision — allowed or denied — produces exactly one audit entry', () => {
  const log = new AuditLog();
  authorizeAndAudit({ role: 'clinician', actorId: 'doc-1' }, patient(['doc-1']), 'raw_session_data', log);
  authorizeAndAudit({ role: 'clinician', actorId: 'doc-2' }, patient(['doc-1']), 'raw_session_data', log);
  assert.equal(log.records.length, 2);
  assert.deepEqual(log.verifyChain(), { valid: true });
});

test('admin can access identity store', () => {
  const log = new AuditLog();
  const result = authorizeAndAudit({ role: 'admin', actorId: 'ops-1' }, patient([]), 'identity_store', log);
  assert.equal(result.allowed, true);
});
