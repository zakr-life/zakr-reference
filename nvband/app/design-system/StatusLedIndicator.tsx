/**
 * StatusLedIndicator — app-side mirror of the device's three-LED+buzzer
 * state model (firmware/core0_safety_signal/safety/ui_state.h).
 *
 * CLAUDE.md §5: "mirror, never replace, the on-device legibility." This
 * component never claims to BE the device state — it renders whatever
 * status was last received over BLE, and shows a staleness indicator if
 * that status is old, rather than silently showing possibly-outdated
 * "reassuring" state.
 */
import React from 'react';
import { View, Text, StyleSheet } from 'react-native';

export type LedColor = 'off' | 'green' | 'amber' | 'red' | 'blue';

export interface StatusLedIndicatorProps {
  label: string;
  color: LedColor;
  blinking?: boolean;
  lastUpdatedMs: number;
  nowMs?: number;
  staleAfterMs?: number;
}

const COLOR_HEX: Record<LedColor, string> = {
  off: '#3A3A3A',
  green: '#1DB954',
  amber: '#F5A623',
  red: '#D0021B',
  blue: '#2D7FF9',
};

export function StatusLedIndicator({
  label, color, blinking = false, lastUpdatedMs,
  nowMs = Date.now(), staleAfterMs = 15000,
}: StatusLedIndicatorProps) {
  const isStale = nowMs - lastUpdatedMs > staleAfterMs;

  return (
    <View
      style={styles.row}
      accessible
      accessibilityLabel={
        `${label}: ${color}${blinking ? ', blinking' : ''}` +
        (isStale ? ' (status may be out of date — check device)' : '')
      }
    >
      <View style={[styles.dot, { backgroundColor: COLOR_HEX[color] },
                     blinking && styles.blinking]} />
      <Text style={styles.label}>{label}</Text>
      {isStale && <Text style={styles.staleBadge}>stale</Text>}
    </View>
  );
}

const styles = StyleSheet.create({
  row: { flexDirection: 'row', alignItems: 'center', marginVertical: 8 },
  dot: { width: 20, height: 20, borderRadius: 10, marginRight: 12 },
  blinking: { opacity: 0.6 },
  label: { fontSize: 18 },
  staleBadge: {
    marginLeft: 8, fontSize: 12, color: '#B45309',
    backgroundColor: '#FEF3C7', paddingHorizontal: 6, paddingVertical: 2,
    borderRadius: 6,
  },
});
