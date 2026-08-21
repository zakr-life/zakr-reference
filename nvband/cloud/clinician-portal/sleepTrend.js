/**
 * sleepTrend.js — Sleep Trend clinician-portal view, Addendum 2 §E.
 *
 * "Physician informing: the clinician portal gains a Sleep Trend view —
 * same RBAC + mandatory audit-log pairing as the existing
 * session/adherence review (CLAUDE.md §6). A clinician-configurable
 * threshold can flag a night's data for human review in the portal. This
 * is a human-reviewed queue, not an automated real-time alert to a
 * physician (no push/SMS/pager) — identical in spirit to the existing
 * fleet-OTA 'halt-on-fault-spike surfaces to a human, never auto-acts'
 * rule."
 *
 * `evaluateSleepTrendFlag` is pure decision logic: given one night's
 * already-computed summary (the same shape
 * `app/mobile/src/state/sleepReport.js` produces on-device/in-app) and a
 * clinician's own configured thresholds, it decides whether that night
 * belongs in the human-review queue. It has no side effects of any kind
 * — no database write, no notification, no network call. A caller is
 * responsible for persisting the returned flagged-item record into
 * whatever the portal's actual review-queue store is (not implemented in
 * this pass, same "policy logic first, framework wiring later" split as
 * the rest of cloud/ — see cloud/README.md).
 *
 * TODO(OI-10): real-time automated physician alerting (push/SMS/pager)
 * is explicitly OUT OF SCOPE here and everywhere in this repository. A
 * flagged night sits in a queue a clinician reviews in the portal on
 * their own schedule, exactly like `cloud/fleet-ota/rollout.js` halts a
 * rollout and surfaces it for a human rather than auto-acting. Nothing
 * in this file sends anything to anyone.
 */
const { authorizeAndAudit } = require('./rbac');

/**
 * Design choice, stated explicitly (Addendum 2 §E asks for "a
 * clinician-configurable threshold," this repo's job is to pick a
 * reasonable default and document why, not leave it silently
 * unspecified): flag a night if EITHER of two independent signals cross
 * a clinician-set line —
 *
 *   1. Unusually low deep sleep (N3 minutes below `minN3MinutesForNormal`)
 *      — N3 is the stage most consistently reduced across a wide range
 *      of things worth a clinician's attention (age, medication,
 *      recovery from illness, etc.), so "materially less deep sleep than
 *      this patient's baseline" is a reasonable single first-pass signal
 *      to make configurable, without this module claiming to know WHY
 *      it's reduced (that is exactly the diagnostic claim Addendum 2 §E
 *      forbids this feature from making).
 *   2. High fragmentation (`stageTransitionCount` above
 *      `maxFragmentationTransitionsForNormal`) — a night with an unusually
 *      high number of stage-to-stage transitions is a general marker of
 *      disrupted sleep continuity, again without this module asserting a
 *      cause.
 *
 * Both thresholds are optional on `clinicianThreshold` — a clinician who
 * only cares about one signal can configure only that one; an
 * unconfigured (undefined) threshold field never contributes a reason to
 * flag.
 */
const DEFAULT_CLINICIAN_THRESHOLD = Object.freeze({
  minN3MinutesForNormal: 40,
  maxFragmentationTransitionsForNormal: 60,
});

/**
 * @param {{
 *   patientId: string,
 *   nightDate: string,
 *   timeInStageMinutes: {WAKE: number, N1: number, N2: number, N3: number, REM: number},
 *   totalRecordedMinutes: number,
 *   sleepEfficiencyPercent: number | null,
 *   stageTransitionCount: number,
 * }} nightSummary
 * @param {{minN3MinutesForNormal?: number, maxFragmentationTransitionsForNormal?: number}} clinicianThreshold
 * @param {number} nowMs
 */
function evaluateSleepTrendFlag(nightSummary, clinicianThreshold = DEFAULT_CLINICIAN_THRESHOLD, nowMs = Date.now()) {
  if (!nightSummary || typeof nightSummary !== 'object') {
    throw new Error('nightSummary is required');
  }

  const reasons = [];

  const minN3 = clinicianThreshold?.minN3MinutesForNormal;
  if (typeof minN3 === 'number') {
    const n3 = nightSummary.timeInStageMinutes?.N3 ?? 0;
    if (n3 < minN3) {
      reasons.push(`deep sleep (N3) was ${n3} min, below the configured ${minN3} min threshold`);
    }
  }

  const maxFragmentation = clinicianThreshold?.maxFragmentationTransitionsForNormal;
  if (typeof maxFragmentation === 'number') {
    const transitions = nightSummary.stageTransitionCount ?? 0;
    if (transitions > maxFragmentation) {
      reasons.push(`${transitions} stage transitions recorded, above the configured ${maxFragmentation} threshold`);
    }
  }

  return {
    patientId: nightSummary.patientId,
    nightDate: nightSummary.nightDate,
    flagged: reasons.length > 0,
    reasons,
    // What this night looked like, for the reviewing clinician's
    // context -- not re-derived, just carried through, so the queue
    // entry is self-contained.
    nightSummarySnapshot: {
      timeInStageMinutes: nightSummary.timeInStageMinutes,
      totalRecordedMinutes: nightSummary.totalRecordedMinutes,
      sleepEfficiencyPercent: nightSummary.sleepEfficiencyPercent,
      stageTransitionCount: nightSummary.stageTransitionCount,
    },
    createdAtMs: nowMs,
    // See TODO(OI-10) above: this record is queue input only. No
    // notification of any kind is ever sent from this function.
  };
}

/**
 * Convenience entry point for a portal request handler: gates access via
 * the existing RBAC + mandatory-audit-log pairing (rbac.js's
 * `authorizeAndAudit`, imported, never reimplemented here) before
 * computing anything, then only evaluates the sleep-trend flag if access
 * was allowed. Sleep-trend summaries are `derived_analytics` (never
 * `raw_session_data` — a clinician viewing this view sees a computed
 * night summary, not raw EEG/IMU samples).
 *
 * @param {{role: string, actorId: string}} actor
 * @param {{patientId: string, clinicianConnections: Set<string>}} patientRecord
 * @param {object} nightSummary
 * @param {object} clinicianThreshold
 * @param {import('../audit/auditLog').AuditLog} auditLog
 */
function reviewSleepTrendForPatient(actor, patientRecord, nightSummary, clinicianThreshold, auditLog, nowMs = Date.now()) {
  const { allowed, reason } = authorizeAndAudit(actor, patientRecord, 'derived_analytics', auditLog, nowMs);

  if (!allowed) {
    return { allowed: false, reason, flag: null };
  }

  const flag = evaluateSleepTrendFlag(nightSummary, clinicianThreshold, nowMs);
  return { allowed: true, reason, flag };
}

module.exports = {
  evaluateSleepTrendFlag,
  reviewSleepTrendForPatient,
  DEFAULT_CLINICIAN_THRESHOLD,
};
