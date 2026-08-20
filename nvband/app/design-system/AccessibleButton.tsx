/**
 * AccessibleButton — shared button used by both the companion app and
 * (per CLAUDE.md §2) the clinician portal's component library.
 *
 * CLAUDE.md §5: "design for users who may have tremor, low vision, or be
 * a tired caregiver at 10pm ... large touch targets, high contrast,
 * screen-reader support, minimal steps for STOP-adjacent and
 * fault-acknowledgment flows."
 */
import React from 'react';
import { Pressable, Text, StyleSheet, ViewStyle } from 'react-native';

export type ButtonVariant = 'primary' | 'danger' | 'secondary';

export interface AccessibleButtonProps {
  label: string;
  onPress: () => void;
  variant?: ButtonVariant;
  disabled?: boolean;
  /** Set for STOP-adjacent actions: CLAUDE.md §5 requires these be
   *  immediately available, never behind extra confirmation steps. */
  isStopAdjacent?: boolean;
  accessibilityHint?: string;
  style?: ViewStyle;
}

const MIN_TOUCH_TARGET = 56; // px — comfortably above WCAG's 44px minimum,
                              // chosen for tremor/low-dexterity use per §5

export function AccessibleButton({
  label, onPress, variant = 'primary', disabled = false,
  isStopAdjacent = false, accessibilityHint, style,
}: AccessibleButtonProps) {
  return (
    <Pressable
      onPress={onPress}
      disabled={disabled}
      accessible
      accessibilityRole="button"
      accessibilityLabel={label}
      accessibilityHint={accessibilityHint}
      accessibilityState={{ disabled }}
      style={[
        styles.base,
        variantStyles[variant],
        isStopAdjacent && styles.stopAdjacent,
        disabled && styles.disabled,
        style,
      ]}
    >
      <Text style={[styles.label, variant === 'primary' || variant === 'danger'
        ? styles.labelOnDark : styles.labelOnLight]}>
        {label}
      </Text>
    </Pressable>
  );
}

const styles = StyleSheet.create({
  base: {
    minHeight: MIN_TOUCH_TARGET,
    minWidth: MIN_TOUCH_TARGET,
    paddingHorizontal: 24,
    justifyContent: 'center',
    alignItems: 'center',
    borderRadius: 12,
  },
  stopAdjacent: {
    // Visually distinct + always full-width so it's never a small target
    // a tremor-affected user could miss.
    width: '100%',
    borderWidth: 3,
    borderColor: '#000000',
  },
  disabled: { opacity: 0.4 },
  label: { fontSize: 20, fontWeight: '700' },
  labelOnDark: { color: '#FFFFFF' },
  labelOnLight: { color: '#111111' },
});

const variantStyles: Record<ButtonVariant, ViewStyle> = {
  primary: { backgroundColor: '#0B5FFF' },
  danger: { backgroundColor: '#C0122F' },   // high-contrast red for STOP
  secondary: { backgroundColor: '#E5E7EB' },
};
