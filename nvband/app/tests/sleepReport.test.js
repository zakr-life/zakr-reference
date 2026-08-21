const { test } = require('node:test');
const assert = require('node:assert/strict');
const { computeSleepReport, summarizeStageSegments } = require('../mobile/src/state/sleepReport');
const sleepReportCopy = require('../mobile/src/screens/sleepReportCopy');

function makeEpoch(stage, i) {
  return { stage, confidence: 0.9, timestampMs: i * 30000 };
}

test('empty timeline reports no data rather than fabricating a percentage', () => {
  const r = computeSleepReport([]);
  assert.equal(r.hasData, false);
  assert.equal(r.sleepEfficiencyPercent, null);
});

test('all-WAKE timeline is 0% sleep efficiency', () => {
  const timeline = Array.from({ length: 10 }, (_, i) => makeEpoch('WAKE', i));
  const r = computeSleepReport(timeline);
  assert.equal(r.hasData, true);
  assert.equal(r.sleepEfficiencyPercent, 0);
  assert.equal(r.timeInStageMinutes.WAKE, 5); // 10 epochs * 30s = 5 min
});

test('all-asleep timeline (no WAKE epochs) is 100% sleep efficiency', () => {
  const timeline = [
    ...Array.from({ length: 5 }, (_, i) => makeEpoch('N2', i)),
    ...Array.from({ length: 5 }, (_, i) => makeEpoch('N3', i + 5)),
  ];
  const r = computeSleepReport(timeline);
  assert.equal(r.sleepEfficiencyPercent, 100);
});

test('mixed timeline computes time-in-stage and efficiency correctly', () => {
  // 8 epochs: 2 WAKE, 2 N1, 2 N2, 1 N3, 1 REM -- 30s each == 4 min total.
  const stages = ['WAKE', 'WAKE', 'N1', 'N1', 'N2', 'N2', 'N3', 'REM'];
  const timeline = stages.map((s, i) => makeEpoch(s, i));
  const r = computeSleepReport(timeline);

  assert.equal(r.totalRecordedMinutes, 4);
  assert.equal(r.timeInStageMinutes.WAKE, 1);
  assert.equal(r.timeInStageMinutes.N1, 1);
  assert.equal(r.timeInStageMinutes.N2, 1);
  assert.equal(r.timeInStageMinutes.N3, 0.5);
  assert.equal(r.timeInStageMinutes.REM, 0.5);
  // asleep = 6/8 epochs = 75%
  assert.equal(r.sleepEfficiencyPercent, 75);
});

test('unknown stage label throws rather than silently miscounting', () => {
  const timeline = [{ stage: 'BOGUS', confidence: 0.5, timestampMs: 0 }];
  assert.throws(() => computeSleepReport(timeline));
});

test('summarizeStageSegments collapses consecutive same-stage epochs', () => {
  const stages = ['WAKE', 'WAKE', 'N1', 'N2', 'N2', 'N2', 'REM'];
  const timeline = stages.map((s, i) => makeEpoch(s, i));
  const segments = summarizeStageSegments(timeline);

  assert.equal(segments.length, 4);
  assert.deepEqual(segments.map((s) => s.stage), ['WAKE', 'N1', 'N2', 'REM']);
  assert.equal(segments[0].epochCount, 2);
  assert.equal(segments[2].epochCount, 3);
});

test('summarizeStageSegments on empty timeline returns empty list', () => {
  assert.deepEqual(summarizeStageSegments([]), []);
});

// --- Mechanical enforcement of "no diagnostic claims" (Addendum 2 §E) ---
//
// Every string value reachable from sleepReportCopy.js's exports is
// checked against a small denylist of diagnostic-sounding terms. This
// is the automated test that makes the "wellness, not diagnostic"
// requirement real rather than a matter of code-review vigilance.

const BANNED_TERMS = [
  'apnea', 'hypopnea', 'disorder', 'abnorm', 'diagnos', 'insomnia',
  'narcolepsy', 'syndrome', 'patholog', 'disease', 'illness', 'symptom',
];

function collectStrings(value, out = []) {
  if (typeof value === 'string') {
    out.push(value);
  } else if (value && typeof value === 'object') {
    for (const v of Object.values(value)) {
      collectStrings(v, out);
    }
  }
  return out;
}

test('sleepReportCopy.js exports contain no diagnostic-sounding language', () => {
  const strings = collectStrings(sleepReportCopy);
  assert.ok(strings.length > 0, 'expected at least one copy string to scan');

  const violations = [];
  for (const s of strings) {
    const lower = s.toLowerCase();
    for (const term of BANNED_TERMS) {
      if (lower.includes(term)) {
        violations.push(`"${s}" contains banned term "${term}"`);
      }
    }
  }
  assert.deepEqual(violations, []);
});

test('disclaimer text is present and communicates wellness/non-diagnostic framing', () => {
  const d = sleepReportCopy.DISCLAIMER.toLowerCase();
  assert.match(d, /wellness/);
  assert.match(d, /does not identify|not a substitute|not medical advice|does not diagnose/);
});

test('every stage label used by state/sleepReport.js has copy text', () => {
  const { STAGES } = require('../mobile/src/state/sleepReport');
  for (const stage of STAGES) {
    assert.ok(
      Object.prototype.hasOwnProperty.call(sleepReportCopy.STAGE_LABELS, stage),
      `missing STAGE_LABELS entry for ${stage}`
    );
  }
});
