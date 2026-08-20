/**
 * rbac.js — Role-based access control for the clinician portal, with
 * mandatory audit logging of every access decision.
 *
 * CLAUDE.md §6: "web app for reviewing session history, adherence, and
 * adaptation logs ... per patient, with role-based access control and a
 * tamper-evident audit log of every access and export."
 *
 * `authorizeAndAudit` is the single entry point: it makes the access
 * decision AND appends the audit record in the same call, so there is no
 * code path that grants access without a corresponding audit entry (the
 * two cannot drift apart because they are not two separate calls a
 * caller could reorder or skip).
 */
const { canAccessStore } = require('../storage/retentionPolicy');

/**
 * @param {{role: string, actorId: string}} actor
 * @param {{patientId: string, clinicianConnections: Set<string>}} patientRecord
 *   clinicianConnections holds actorIds explicitly connected to this
 *   patient (mirrors the app's clinician-data-sharing consent toggle,
 *   which must be on for this set to be non-empty for that patient).
 * @param {string} store one of 'raw_session_data' | 'derived_analytics' | 'identity_store'
 * @param {import('../audit/auditLog').AuditLog} auditLog
 */
function authorizeAndAudit(actor, patientRecord, store, auditLog, nowMs = Date.now()) {
  let allowed = false;
  let reason;

  if (!canAccessStore(actor.role, store)) {
    reason = `role '${actor.role}' has no access to store '${store}'`;
  } else if (actor.role === 'clinician' && !patientRecord.clinicianConnections.has(actor.actorId)) {
    reason = 'clinician is not connected to this patient (no consent on record)';
  } else {
    allowed = true;
    reason = 'ok';
  }

  auditLog.append({
    type: 'DATA_ACCESS',
    actorId: actor.actorId,
    role: actor.role,
    patientId: patientRecord.patientId,
    store,
    allowed,
    reason,
    timestampMs: nowMs,
  });

  return { allowed, reason };
}

module.exports = { authorizeAndAudit };
