/**
 * sleepReport.js — Sleep-report computation, Addendum 2 §E.
 *
 * "Reporting — deliberately wellness-framed, not diagnostic: the app's
 * sleep report (stage timeline, time-in-stage, movement-based sleep
 * efficiency) is descriptive."
 *
 * Input is a timeline of per-epoch sleep-stage classifier results
 * (already synced from the device, or read from a completed overnight
 * session store, mirroring how adherence.js consumes an already-fetched
 * status-sample timeline). This module is pure computation over that
 * timeline, no I/O — same discipline as adherence.js.
 *
 * "Movement-based sleep efficiency": efficiency here is time-asleep
 * (any epoch not classified WAKE) divided by time-in-bed (the full
 * recorded timeline). The WAKE/asleep distinction upstream is itself
 * grounded in IMU movement features (see
 * models/sleep_staging/features.py's feature vector, which includes IMU
 * movement magnitude precisely because it is one of the strongest
 * signals for telling Wake apart from the other stages) — this module
 * does not re-read raw IMU data itself, it consumes the already-scored
 * per-epoch result.
 *
 * No diagnostic language or claim belongs anywhere in this module or its
 * output — see screens/sleepReportCopy.js for the copy layer and
 * app/tests/sleepReport.test.js for the automated check that enforces
 * that.
 */

const STAGES = ['WAKE', 'N1', 'N2', 'N3', 'REM'];
const DEFAULT_EPOCH_DURATION_S = 30;

/**
 * @param {{stage: string, confidence: number, timestampMs: number}[]} epochTimeline
 *   Epochs are expected in chronological order but this function does
 *   not assume they are contiguous or gap-free.
 * @param {number} epochDurationS seconds represented by one epoch
 *   (matches firmware's nvband_overnight_session_config_t.epoch_duration_s)
 */
function computeSleepReport(epochTimeline, epochDurationS = DEFAULT_EPOCH_DURATION_S) {
  if (!epochTimeline || epochTimeline.length === 0) {
    return {
      hasData: false,
      totalRecordedMinutes: 0,
      timeInStageMinutes: zeroedStageMap(),
      sleepEfficiencyPercent: null,
      stageCounts: zeroedStageMap(),
      summary: 'no sleep data recorded for this window',
    };
  }

  const stageCounts = zeroedStageMap();
  for (const epoch of epochTimeline) {
    if (!Object.prototype.hasOwnProperty.call(stageCounts, epoch.stage)) {
      throw new Error(`unknown sleep stage: ${epoch.stage}`);
    }
    stageCounts[epoch.stage] += 1;
  }

  const totalEpochs = epochTimeline.length;
  const minutesPerEpoch = epochDurationS / 60;

  const timeInStageMinutes = {};
  for (const stage of STAGES) {
    timeInStageMinutes[stage] = round1(stageCounts[stage] * minutesPerEpoch);
  }

  const totalRecordedMinutes = round1(totalEpochs * minutesPerEpoch);
  const asleepEpochs = totalEpochs - stageCounts.WAKE;
  const sleepEfficiencyPercent = round1((asleepEpochs / totalEpochs) * 100);

  let summary;
  if (sleepEfficiencyPercent >= 85) {
    summary = 'spent most of the recorded time asleep';
  } else if (sleepEfficiencyPercent >= 65) {
    summary = 'spent a good portion of the recorded time asleep, with some wakeful time mixed in';
  } else {
    summary = 'there was a lot of wakeful time mixed into the recorded window';
  }

  return {
    hasData: true,
    totalRecordedMinutes,
    timeInStageMinutes,
    sleepEfficiencyPercent,
    stageCounts,
    summary,
  };
}

/**
 * Collapses a chronological epoch timeline into contiguous same-stage
 * segments, for rendering a stage-timeline bar in the UI (e.g.
 * SleepReportScreen.tsx). Pure computation, no I/O.
 *
 * @param {{stage: string, timestampMs: number}[]} epochTimeline
 * @param {number} epochDurationS
 */
function summarizeStageSegments(epochTimeline, epochDurationS = DEFAULT_EPOCH_DURATION_S) {
  if (!epochTimeline || epochTimeline.length === 0) {
    return [];
  }

  const segments = [];
  let current = null;

  for (const epoch of epochTimeline) {
    if (current && current.stage === epoch.stage) {
      current.endMs = epoch.timestampMs + epochDurationS * 1000;
      current.epochCount += 1;
    } else {
      if (current) {
        segments.push(current);
      }
      current = {
        stage: epoch.stage,
        startMs: epoch.timestampMs,
        endMs: epoch.timestampMs + epochDurationS * 1000,
        epochCount: 1,
      };
    }
  }
  if (current) {
    segments.push(current);
  }
  return segments;
}

function zeroedStageMap() {
  const m = {};
  for (const stage of STAGES) {
    m[stage] = 0;
  }
  return m;
}

function round1(n) {
  return Math.round(n * 10) / 10;
}

module.exports = { computeSleepReport, summarizeStageSegments, STAGES };
