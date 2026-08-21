/**
 * SleepReportScreen — Addendum 2 §E: a wellness-framed, descriptive
 * overnight sleep summary (stage timeline, time-in-stage, movement-based
 * sleep efficiency). Structurally consistent with CaregiverModeScreen.tsx
 * (plain language, no clinical jargon required to read it).
 *
 * The disclaimer below is rendered unconditionally, every time this
 * screen mounts — there is no dismiss control, no "don't show again"
 * toggle, and no persisted "seen it" flag gating it. That is
 * deliberate: Addendum 2 §E requires a "persistent, non-dismissable-
 * per-session disclaimer," so this component simply never gives the
 * disclaimer a way to be hidden.
 */
import React from 'react';
import { View, Text, StyleSheet } from 'react-native';
// eslint-disable-next-line @typescript-eslint/no-var-requires
const { computeSleepReport, summarizeStageSegments } = require('../state/sleepReport');
// eslint-disable-next-line @typescript-eslint/no-var-requires
const copy = require('./sleepReportCopy');

function efficiencyBand(sleepEfficiencyPercent: number | null): 'high' | 'medium' | 'low' {
  if (sleepEfficiencyPercent === null) return 'low';
  if (sleepEfficiencyPercent >= 85) return 'high';
  if (sleepEfficiencyPercent >= 65) return 'medium';
  return 'low';
}

export function SleepReportScreen({
  epochTimeline,
  epochDurationS = 30,
}: {
  epochTimeline: { stage: string; confidence: number; timestampMs: number }[];
  epochDurationS?: number;
}) {
  const report = computeSleepReport(epochTimeline, epochDurationS);
  const segments = summarizeStageSegments(epochTimeline, epochDurationS);

  return (
    <View style={styles.container}>
      <Text style={styles.heading} accessibilityRole="header">{copy.TITLE}</Text>

      {/* Persistent, non-dismissable-per-session disclaimer (Addendum 2 §E). */}
      <View style={styles.disclaimerBox} accessibilityRole="alert">
        <Text style={styles.disclaimerText}>{copy.DISCLAIMER}</Text>
      </View>

      {!report.hasData && (
        <Text style={styles.body}>{copy.NO_DATA_MESSAGE}</Text>
      )}

      {report.hasData && (
        <>
          <Text style={styles.subheading}>{copy.EFFICIENCY_LABEL}</Text>
          <Text style={styles.bigStat}>{report.sleepEfficiencyPercent}%</Text>
          <Text style={styles.body}>
            {copy.EFFICIENCY_SUMMARY[efficiencyBand(report.sleepEfficiencyPercent)]}
          </Text>

          <Text style={styles.subheading}>{copy.STAGE_TIMELINE_HEADING}</Text>
          <View style={styles.timelineRow}>
            {segments.map((seg, i) => (
              <View
                key={i}
                style={[styles.timelineSegment, { flexGrow: seg.epochCount }]}
                accessibilityLabel={`${copy.STAGE_LABELS[seg.stage] ?? seg.stage}`}
              />
            ))}
          </View>

          <Text style={styles.subheading}>{copy.TIME_IN_STAGE_HEADING}</Text>
          {Object.keys(report.timeInStageMinutes).map((stage) => (
            <View key={stage} style={styles.stageRow}>
              <Text style={styles.body}>{copy.STAGE_LABELS[stage] ?? stage}</Text>
              <Text style={styles.body}>{report.timeInStageMinutes[stage]} min</Text>
            </View>
          ))}
        </>
      )}
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, padding: 24, gap: 12 },
  heading: { fontSize: 22, fontWeight: '700', marginTop: 16 },
  subheading: { fontSize: 17, fontWeight: '700', marginTop: 16 },
  bigStat: { fontSize: 28, fontWeight: '800' },
  body: { fontSize: 17, lineHeight: 24 },
  disclaimerBox: {
    padding: 12,
    borderRadius: 8,
    borderWidth: 1,
    borderColor: '#9CA3AF',
    backgroundColor: '#F3F4F6',
  },
  disclaimerText: { fontSize: 14, lineHeight: 20, color: '#374151' },
  timelineRow: { flexDirection: 'row', height: 24, borderRadius: 4, overflow: 'hidden' },
  timelineSegment: { height: 24, backgroundColor: '#9CA3AF', borderRightWidth: 1, borderRightColor: '#FFFFFF' },
  stageRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    paddingVertical: 6,
    borderBottomWidth: 1,
    borderBottomColor: '#E5E7EB',
  },
});
