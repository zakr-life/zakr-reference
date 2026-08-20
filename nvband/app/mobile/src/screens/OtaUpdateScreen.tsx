/**
 * OtaUpdateScreen — CLAUDE.md §5: "user-initiated or scheduled update
 * flow that surfaces what changed (firmware vs. model, safety-relevant
 * or not) in plain language; never auto-apply a firmware update during
 * an active or scheduled session window."
 */
import React from 'react';
import { View, Text, StyleSheet } from 'react-native';
import { AccessibleButton } from '../../../design-system/AccessibleButton';
// eslint-disable-next-line @typescript-eslint/no-var-requires
const { canAutoApplyUpdate } = require('../state/otaGate');

interface UpdateInfo {
  kind: 'firmware' | 'model';
  version: string;
  safetyRelevant: boolean;
  plainLanguageSummary: string;
}

export function OtaUpdateScreen({
  update, sessionUiState, scheduledSessionWindows, onApply, onDismiss,
}: {
  update: UpdateInfo;
  sessionUiState: string;
  scheduledSessionWindows: { startsAtMs: number; endsAtMs: number }[];
  onApply: () => void;
  onDismiss: () => void;
}) {
  const gate = canAutoApplyUpdate(sessionUiState, scheduledSessionWindows, Date.now());

  return (
    <View style={styles.container}>
      <Text style={styles.heading} accessibilityRole="header">
        {update.kind === 'firmware' ? 'Firmware update available' : 'Model update available'}
      </Text>
      {update.safetyRelevant && (
        <View style={styles.safetyBadge}>
          <Text style={styles.safetyBadgeText}>Includes a safety-relevant change</Text>
        </View>
      )}
      <Text style={styles.body}>{update.plainLanguageSummary}</Text>
      <Text style={styles.version}>Version {update.version}</Text>

      {!gate.allowed && (
        <Text style={styles.warning}>
          This update can't be applied automatically right now
          ({gate.reason}). You can still apply it manually, but it will
          interrupt any in-progress or upcoming session.
        </Text>
      )}

      <AccessibleButton label="Update now" onPress={onApply} />
      <AccessibleButton label="Later" variant="secondary" onPress={onDismiss} />
    </View>
  );
}

const styles = StyleSheet.create({
  container: { padding: 24, gap: 16 },
  heading: { fontSize: 22, fontWeight: '700' },
  body: { fontSize: 17, lineHeight: 24 },
  version: { fontSize: 14, color: '#6B7280' },
  safetyBadge: {
    backgroundColor: '#FEF3C7', paddingHorizontal: 10, paddingVertical: 6,
    borderRadius: 8, alignSelf: 'flex-start',
  },
  safetyBadgeText: { color: '#92400E', fontWeight: '700' },
  warning: { color: '#92400E', fontSize: 15 },
});
