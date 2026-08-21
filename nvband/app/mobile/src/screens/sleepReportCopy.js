/**
 * sleepReportCopy.js — Plain-language text strings for SleepReportScreen.
 *
 * Addendum 2 §E: "the app's sleep report ... is descriptive. It carries a
 * persistent, non-dismissable-per-session disclaimer and contains no
 * diagnostic language (e.g., no apnea/disorder naming) — enforced by an
 * automated string-content test over the UI copy, mirroring the
 * existing 'no clinical jargon in caregiver mode' discipline."
 *
 * Kept as its own module, separate from SleepReportScreen.tsx's JSX,
 * specifically so app/tests/sleepReport.test.js can import it and scan
 * every string here for diagnostic-sounding language mechanically. If
 * you are adding copy to the sleep report screen, add it HERE, not as an
 * inline string in the .tsx file — otherwise it is invisible to that
 * test.
 *
 * No word in this file may describe, name, suggest, or rule out any
 * medical condition. This is a wellness/descriptive summary of recorded
 * patterns only.
 */

const TITLE = 'Sleep summary';

const DISCLAIMER =
  'This sleep summary is a wellness overview based on movement and ' +
  'brainwave patterns your NV-Band recorded overnight. It is descriptive ' +
  'only: it does not identify, assess, or rule out any medical condition, ' +
  'and it is not a substitute for professional care. If you have ' +
  'concerns about your sleep, please talk with a doctor.';

const NO_DATA_MESSAGE = 'No sleep data recorded for this night yet.';

const EFFICIENCY_LABEL = 'Time spent asleep';

// Friendly labels for the classifier's five output stages. These are
// standard, descriptive sleep-science terms (not evaluative and not
// naming any condition) — the denylist test cares about words like
// "apnea"/"disorder"/"abnormal"/"diagnosis", not about using the words
// Wake/N1/N2/N3/REM themselves.
const STAGE_LABELS = {
  WAKE: 'Awake',
  N1: 'Light sleep (N1)',
  N2: 'Light sleep (N2)',
  N3: 'Deep sleep (N3)',
  REM: 'REM sleep',
};

const STAGE_TIMELINE_HEADING = 'Overnight pattern';
const TIME_IN_STAGE_HEADING = 'Time in each stage';

// Plain-language summaries, keyed by rough sleep-efficiency band, mirror
// the phrasing produced by state/sleepReport.js's own `summary` field so
// the screen and the underlying computation never say materially
// different things.
const EFFICIENCY_SUMMARY = {
  high: 'You spent most of the recorded time asleep.',
  medium: 'You spent a good portion of the recorded time asleep, with some wakeful time mixed in.',
  low: 'There was a lot of wakeful time mixed into the recorded window.',
};

module.exports = {
  TITLE,
  DISCLAIMER,
  NO_DATA_MESSAGE,
  EFFICIENCY_LABEL,
  STAGE_LABELS,
  STAGE_TIMELINE_HEADING,
  TIME_IN_STAGE_HEADING,
  EFFICIENCY_SUMMARY,
};
