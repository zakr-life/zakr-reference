/**
 * BrainprintEnrollmentScreen — optional EEG-based local authentication
 * enrollment.
 *
 * Addendum 2 §A. Structurally consistent with PairingScreen.tsx (large
 * touch targets, plain language). Critically: this screen must never
 * claim brainprint is more secure than the existing passcode/pairing
 * baseline, and must state a fallback always exists.
 */
import React, { useState } from 'react';
import { View, Text, StyleSheet } from 'react-native';
import { AccessibleButton } from '../../../design-system/AccessibleButton';
import { EnrollmentState } from '../state/brainprintGate';

type Props = {
  bleClient: any;
  enrollmentState: { enrollment: string };
  onEnrollmentEvent: (event: { type: string }) => void;
  onDone: () => void;
};

export function BrainprintEnrollmentScreen({ bleClient, enrollmentState, onEnrollmentEvent, onDone }: Props) {
  const [error, setError] = useState<string | null>(null);
  const enrolling = enrollmentState.enrollment === EnrollmentState.ENROLLING;
  const enrolled = enrollmentState.enrollment === EnrollmentState.ENROLLED;

  const startEnrollment = async () => {
    setError(null);
    onEnrollmentEvent({ type: 'ENROLLMENT_STARTED' });
    try {
      await bleClient.captureBrainprintEnrollmentEpochs();
      onEnrollmentEvent({ type: 'ENROLLMENT_SUCCEEDED' });
    } catch (e: any) {
      setError(e?.message ?? 'enrollment failed');
      onEnrollmentEvent({ type: 'ENROLLMENT_FAILED' });
    }
  };

  return (
    <View style={styles.container}>
      <Text style={styles.heading} accessibilityRole="header">
        Optional: unlock with your brainprint
      </Text>
      <Text style={styles.body}>
        Your brainprint is a pattern in your resting EEG. If you turn this
        on, the app can use it as a quick extra way to unlock your local
        session data on this device.
      </Text>
      <Text style={styles.body}>
        This is always optional, and it never replaces your device
        passcode — if your brainprint isn't recognized for any reason
        (a bad connection, a different mood, anything), you can always
        unlock the normal way instead. Turning this on does not make your
        data more secure than it already is; it's a convenience feature.
      </Text>
      {error && <Text style={styles.error}>{error}</Text>}
      {!enrolled && (
        <AccessibleButton
          label={enrolling ? 'Recording resting EEG…' : 'Set up brainprint unlock'}
          onPress={startEnrollment}
          disabled={enrolling}
        />
      )}
      {enrolled && <Text style={styles.body}>Brainprint unlock is set up.</Text>}
      <AccessibleButton label="Skip / not now" onPress={onDone} />
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, padding: 24, justifyContent: 'center', gap: 16 },
  heading: { fontSize: 24, fontWeight: '700' },
  body: { fontSize: 17, lineHeight: 24 },
  error: { color: '#C0122F', fontSize: 15 },
});
