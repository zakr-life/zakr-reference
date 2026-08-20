/**
 * adherence.js — Caregiver-mode adherence computation.
 *
 * CLAUDE.md §5: "at-a-glance adherence view (worn/not worn, from
 * IMU-derived data — see dossier note that this is explicitly designed
 * to answer 'was it worn,' not just 'was it charged')."
 *
 * Input is a timeline of synced device status samples (already uploaded
 * via cloud/ingestion and fetched by the app, or read live over BLE);
 * this module is pure computation over that timeline, no I/O.
 */

/**
 * @param {{timestampMs: number, imuMotionDetected: boolean, isCharging: boolean}[]} samples
 * @param {number} windowStartMs
 * @param {number} windowEndMs
 */
function computeAdherence(samples, windowStartMs, windowEndMs) {
  const inWindow = samples.filter(
    s => s.timestampMs >= windowStartMs && s.timestampMs <= windowEndMs
  );

  if (inWindow.length === 0) {
    return { wornFraction: null, chargedFraction: null, sampleCount: 0,
             summary: 'no data in window' };
  }

  const wornCount = inWindow.filter(s => s.imuMotionDetected).length;
  const chargedCount = inWindow.filter(s => s.isCharging).length;

  const wornFraction = wornCount / inWindow.length;
  const chargedFraction = chargedCount / inWindow.length;

  // The caregiver-relevant distinction the dossier calls out explicitly:
  // a device that is charged 100% of the time but never shows worn
  // signal is NOT adherent, even though naive "was it plugged in and
  // present" monitoring would look fine.
  let summary;
  if (wornFraction < 0.1 && chargedFraction > 0.5) {
    summary = 'device is being charged but shows little sign of being worn';
  } else if (wornFraction >= 0.5) {
    summary = 'good adherence: worn for the majority of the window';
  } else {
    summary = 'low adherence: worn signal detected in a minority of the window';
  }

  return { wornFraction, chargedFraction, sampleCount: inWindow.length, summary };
}

module.exports = { computeAdherence };
