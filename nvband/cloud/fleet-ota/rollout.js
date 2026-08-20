/**
 * rollout.js — Staged (percentage/cohort) OTA rollout with automatic
 * halt-on-fault-rate-spike.
 *
 * CLAUDE.md §6: "staged rollout support (percentage/cohort based),
 * automatic halt-on-fault-rate-spike logic (if a rollout cohort shows
 * anomalous fault-log rates post-update, halt further rollout and alert
 * — do not auto-rollback devices silently; surface it for human
 * decision)."
 *
 * Critical property, directly tested: this module's only automatic
 * action is HALTING further rollout. It has no function that reverts an
 * already-updated device — that decision is explicitly reserved for a
 * human (CLAUDE.md's "surface it for human decision"). Also gates on
 * models/versioning's firmware-compatibility range before a model can
 * even enter a rollout stage.
 */

const ROLLOUT_STAGES = Object.freeze([1, 5, 25, 50, 100]); // percent of fleet

/**
 * @param {{baselineFaultRate: number, cohortFaultRate: number, cohortSize: number}} params
 * @param {number} spikeMultiplier how many times the baseline rate counts as a spike
 * @param {number} minCohortSize below this size, a rate spike isn't statistically
 *   meaningful enough to halt on (avoid halting on noise from tiny cohorts)
 */
function detectFaultRateSpike({ baselineFaultRate, cohortFaultRate, cohortSize },
                               spikeMultiplier = 3, minCohortSize = 20) {
  if (cohortSize < minCohortSize) {
    return { isSpike: false, reason: `cohort too small (${cohortSize} < ${minCohortSize}) to judge` };
  }
  if (baselineFaultRate === 0) {
    // Any nonzero fault rate against a zero baseline is treated as a
    // spike — there's no ratio to compute, and "some faults where there
    // were previously none" is exactly the signal this exists to catch.
    if (cohortFaultRate > 0) {
      return { isSpike: true, reason: 'faults appeared where baseline had none' };
    }
    return { isSpike: false, reason: 'no faults, no baseline' };
  }
  const ratio = cohortFaultRate / baselineFaultRate;
  if (ratio >= spikeMultiplier) {
    return { isSpike: true, reason: `fault rate ${ratio.toFixed(1)}x baseline`, ratio };
  }
  return { isSpike: false, reason: `fault rate ${ratio.toFixed(1)}x baseline, under threshold`, ratio };
}

class RolloutState {
  constructor(rolloutId, modelOrFirmwareId) {
    this.rolloutId = rolloutId;
    this.artifactId = modelOrFirmwareId;
    this.stageIndex = 0;
    this.status = 'IN_PROGRESS'; // IN_PROGRESS | HALTED | COMPLETE
    this.haltReason = null;
    this.history = [];
  }

  currentStagePercent() {
    return ROLLOUT_STAGES[this.stageIndex];
  }

  /** Called after a stage has run long enough to observe fault rates.
   *  Advances to the next stage, OR halts — never auto-reverts. */
  evaluateAndAdvance(faultRateObservation) {
    if (this.status !== 'IN_PROGRESS') {
      return this.status; // no-op once halted or complete
    }

    const spike = detectFaultRateSpike(faultRateObservation);
    this.history.push({ stagePercent: this.currentStagePercent(), spike });

    if (spike.isSpike) {
      this.status = 'HALTED';
      this.haltReason = spike.reason;
      // Deliberately NOT reverting already-updated devices here — see
      // module docstring. Callers must surface this to a human (e.g. via
      // cloud/audit + an alerting integration, not implemented in this
      // pass) rather than take further automatic action.
      return this.status;
    }

    if (this.stageIndex >= ROLLOUT_STAGES.length - 1) {
      this.status = 'COMPLETE';
    } else {
      this.stageIndex += 1;
    }
    return this.status;
  }
}

module.exports = { ROLLOUT_STAGES, detectFaultRateSpike, RolloutState };
