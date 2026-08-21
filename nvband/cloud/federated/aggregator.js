/**
 * aggregator.js — server-side policy for accepting and aggregating
 * federated-learning local model deltas.
 *
 * Addendum 2 §D: "The aggregation service accepts deltas only from
 * attested devices, rejects any delta outside plausible clipping bounds,
 * and never updates the shared global model from fewer than a minimum
 * cohort of devices in one aggregation round." Mirrors the k-anonymity
 * minimum-group gate already used for analytics
 * (cloud/analytics/deidentify.js's isGroupSafeToPublish) and the
 * attested-submission pattern from cloud/ingestion/deviceAuth.js.
 *
 * Framework-agnostic policy logic only, matching the rest of cloud/ — no
 * HTTP wiring here. A delta is the same shape produced by
 * models/federated/clipping.py: { weights: number[][], bias: number[] }.
 */

const DEFAULT_MAX_NORM_BOUND = 2.0; // generous upper bound vs. the pipeline's own 1.0 clip, to tolerate legitimate variance while still rejecting wildly out-of-range submissions
const DEFAULT_MIN_COHORT_SIZE = 10;  // mirrors the k>=10 anonymity threshold used for analytics

function _isFiniteNumber(v) {
  return typeof v === 'number' && Number.isFinite(v);
}

function _l2Norm(delta) {
  let sumSq = 0;
  for (const row of delta.weights) {
    for (const v of row) sumSq += v * v;
  }
  for (const v of delta.bias) sumSq += v * v;
  return Math.sqrt(sumSq);
}

function _isWellShaped(delta) {
  if (!delta || !Array.isArray(delta.weights) || !Array.isArray(delta.bias)) {
    return false;
  }
  if (delta.weights.length === 0) return false;
  const numFeatures = delta.weights[0].length;
  for (const row of delta.weights) {
    if (!Array.isArray(row) || row.length !== numFeatures) return false;
    if (!row.every(_isFiniteNumber)) return false;
  }
  if (delta.bias.length !== delta.weights.length) return false;
  if (!delta.bias.every(_isFiniteNumber)) return false;
  return true;
}

/**
 * @param {object} delta
 * @param {number} maxNormBound
 * @returns {{accepted: boolean, reason: string, norm?: number}}
 */
function validateDeltaSubmission(delta, maxNormBound = DEFAULT_MAX_NORM_BOUND) {
  if (!_isWellShaped(delta)) {
    return { accepted: false, reason: 'malformed delta: wrong shape or non-finite values' };
  }
  const norm = _l2Norm(delta);
  if (norm > maxNormBound) {
    return { accepted: false, reason: `delta L2 norm ${norm.toFixed(3)} exceeds plausible bound ${maxNormBound} — possible malicious/compromised submission`, norm };
  }
  return { accepted: true, reason: 'ok', norm };
}

/**
 * @param {number} pendingDeltaCount distinct, already-validated, attested devices with a pending delta this round
 * @param {number} minCohortSize
 */
function isCohortReadyToAggregate(pendingDeltaCount, minCohortSize = DEFAULT_MIN_COHORT_SIZE) {
  return pendingDeltaCount >= minCohortSize;
}

/**
 * Aggregates a list of already-validated deltas by simple averaging.
 * Defense-in-depth: refuses (returns null) if called with fewer than the
 * minimum cohort, independently of whether the caller already checked
 * isCohortReadyToAggregate — a caller bug upstream must not be able to
 * isolate one device's contribution into an aggregation round.
 *
 * @param {object[]} validatedDeltas
 * @param {number} minCohortSize
 * @returns {{aggregate: object, reason: string}|{aggregate: null, reason: string}}
 */
function aggregateRound(validatedDeltas, minCohortSize = DEFAULT_MIN_COHORT_SIZE) {
  if (!Array.isArray(validatedDeltas) || validatedDeltas.length < minCohortSize) {
    return { aggregate: null, reason: `cohort too small: ${validatedDeltas ? validatedDeltas.length : 0} < ${minCohortSize}` };
  }
  if (!validatedDeltas.every(_isWellShaped)) {
    return { aggregate: null, reason: 'one or more deltas is malformed — caller must validate before aggregating' };
  }

  const n = validatedDeltas.length;
  const numClasses = validatedDeltas[0].weights.length;
  const numFeatures = validatedDeltas[0].weights[0].length;

  const avgWeights = Array.from({ length: numClasses }, () => new Array(numFeatures).fill(0));
  const avgBias = new Array(numClasses).fill(0);

  for (const d of validatedDeltas) {
    for (let c = 0; c < numClasses; c++) {
      for (let j = 0; j < numFeatures; j++) {
        avgWeights[c][j] += d.weights[c][j] / n;
      }
      avgBias[c] += d.bias[c] / n;
    }
  }

  return {
    aggregate: { weights: avgWeights, bias: avgBias },
    reason: `aggregated ${n} attested device deltas`,
  };
}

module.exports = {
  DEFAULT_MAX_NORM_BOUND,
  DEFAULT_MIN_COHORT_SIZE,
  validateDeltaSubmission,
  isCohortReadyToAggregate,
  aggregateRound,
};
