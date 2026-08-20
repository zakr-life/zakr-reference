/**
 * deidentify.js — Opt-in-gated de-identification for the analytics
 * pipeline.
 *
 * CLAUDE.md §6: "Analytics: aggregate and de-identified only, driven
 * strictly by the opt-in toggle in §5; no analytics pipeline may
 * re-identify individual sessions."
 */

const DIRECT_IDENTIFIER_FIELDS = Object.freeze([
  'deviceId', 'patientId', 'clinicianId', 'accountEmail', 'phoneImei',
]);

/**
 * @param {{researchAnalyticsConsent: boolean}} consent
 * @param {object} sessionRecord
 * @returns {{record: object|null, reason: string}} record is null if
 *   consent is not granted — the function refuses to even attempt
 *   de-identification rather than de-identify-then-discard, so there is
 *   no code path where a non-consenting user's identifiers transiently
 *   exist in the analytics-bound object.
 */
function deidentifyForAnalytics(consent, sessionRecord) {
  if (!consent || consent.researchAnalyticsConsent !== true) {
    return { record: null, reason: 'no research-analytics consent on file' };
  }

  const stripped = { ...sessionRecord };
  for (const field of DIRECT_IDENTIFIER_FIELDS) {
    delete stripped[field];
  }

  // Coarse-grain any timestamp to the day level — a common
  // de-identification technique to reduce re-identification risk from
  // precise timing correlation.
  if (typeof stripped.sessionStartMs === 'number') {
    const DAY_MS = 24 * 60 * 60 * 1000;
    stripped.sessionStartDayMs = Math.floor(stripped.sessionStartMs / DAY_MS) * DAY_MS;
    delete stripped.sessionStartMs;
  }

  stripped.deidentified = true;
  return { record: stripped, reason: 'ok' };
}

/**
 * Aggregation must never expose a group smaller than a k-anonymity
 * threshold (otherwise a "group" of 1 is functionally a re-identified
 * individual). This is the enforcement point the analytics pipeline
 * calls before publishing any aggregate.
 */
function isGroupSafeToPublish(groupSize, kThreshold = 10) {
  return groupSize >= kThreshold;
}

module.exports = { deidentifyForAnalytics, isGroupSafeToPublish, DIRECT_IDENTIFIER_FIELDS };
