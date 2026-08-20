/**
 * CaregiverModeScreen — CLAUDE.md §5: "at-a-glance adherence view
 * (worn/not worn ...), fault history in plain language, no clinical
 * jargon required to understand 'something needs attention.'"
 */
import React from 'react';
import { View, Text, StyleSheet } from 'react-native';
// eslint-disable-next-line @typescript-eslint/no-var-requires
const { computeAdherence } = require('../state/adherence');

const PLAIN_LANGUAGE_FAULT: Record<string, string> = {
  INTERLOCK_TRIPPED: 'The band paused itself as a safety precaution and needs to be checked.',
  ELECTRODE_CONTACT: 'The band may not be sitting snugly — check the fit.',
  WATCHDOG_RESET: 'The band restarted unexpectedly. If this keeps happening, contact support.',
  LOW_BATTERY: 'The band is low on charge.',
  STORAGE_FULL: 'The band needs to sync with the app soon to free up space.',
  PROVISIONING_INVALID: 'This band needs attention from support before it can run a session.',
};

export function CaregiverModeScreen({
  statusSamples, windowStartMs, windowEndMs, faultHistory,
}: {
  statusSamples: any[]; windowStartMs: number; windowEndMs: number;
  faultHistory: { faultCode: string; timestampMs: number }[];
}) {
  const adherence = computeAdherence(statusSamples, windowStartMs, windowEndMs);

  return (
    <View style={styles.container}>
      <Text style={styles.heading} accessibilityRole="header">This week</Text>
      <Text style={styles.bigStat}>
        {adherence.wornFraction === null
          ? 'No data yet'
          : `Worn ${Math.round(adherence.wornFraction * 100)}% of the time`}
      </Text>
      <Text style={styles.body}>{adherence.summary}</Text>

      <Text style={styles.heading} accessibilityRole="header">Needs attention</Text>
      {faultHistory.length === 0 && (
        <Text style={styles.body}>Nothing needs attention right now.</Text>
      )}
      {faultHistory.map((f, i) => (
        <View key={i} style={styles.faultRow}>
          <Text style={styles.body}>
            {PLAIN_LANGUAGE_FAULT[f.faultCode] ?? 'The band flagged something to check.'}
          </Text>
          <Text style={styles.timestamp}>{new Date(f.timestampMs).toLocaleString()}</Text>
        </View>
      ))}
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, padding: 24, gap: 12 },
  heading: { fontSize: 22, fontWeight: '700', marginTop: 16 },
  bigStat: { fontSize: 28, fontWeight: '800' },
  body: { fontSize: 17, lineHeight: 24 },
  faultRow: { paddingVertical: 8, borderBottomWidth: 1, borderBottomColor: '#E5E7EB' },
  timestamp: { fontSize: 13, color: '#6B7280', marginTop: 2 },
});
