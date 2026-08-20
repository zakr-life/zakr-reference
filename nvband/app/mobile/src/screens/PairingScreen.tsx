/**
 * PairingScreen — BLE scan + mutual attestation onboarding.
 *
 * CLAUDE.md §5: "first-run onboarding that explains the physical STOP
 * control and session-state LEDs so the app never becomes the only way
 * to understand device state." Pairing itself talks to
 * firmware/core1_inference_radio/ble/attestation.c's challenge/response
 * flow via bleClient; the secure-element-backed signature verification
 * happens on-device (firmware/secure/), never in this app.
 */
import React, { useState } from 'react';
import { View, Text, StyleSheet } from 'react-native';
import { AccessibleButton } from '../../../design-system/AccessibleButton';

type PairingStep = 'SCAN' | 'CONNECTING' | 'ATTESTING' | 'ONBOARDING_STOP' | 'ONBOARDING_LEDS' | 'DONE';

export function PairingScreen({ bleClient, onComplete }: { bleClient: any; onComplete: () => void }) {
  const [step, setStep] = useState<PairingStep>('SCAN');
  const [error, setError] = useState<string | null>(null);

  const startScan = async () => {
    setStep('CONNECTING');
    try {
      const device = await bleClient.scanAndConnect();
      setStep('ATTESTING');
      await bleClient.performMutualAttestation(device);
      setStep('ONBOARDING_STOP');
    } catch (e: any) {
      setError(e?.message ?? 'pairing failed');
      setStep('SCAN');
    }
  };

  if (step === 'ONBOARDING_STOP') {
    return (
      <View style={styles.container}>
        <Text style={styles.heading} accessibilityRole="header">
          Your NV-Band has a physical STOP switch
        </Text>
        <Text style={styles.body}>
          The switch on the band itself always works, even if this phone
          is out of range, off, or the app has a problem. This app mirrors
          the band's status — it never replaces the physical switch as
          the way to stop a session.
        </Text>
        <AccessibleButton label="Next: session lights"
          onPress={() => setStep('ONBOARDING_LEDS')} />
      </View>
    );
  }

  if (step === 'ONBOARDING_LEDS') {
    return (
      <View style={styles.container}>
        <Text style={styles.heading} accessibilityRole="header">
          Three lights on the band tell you what's happening
        </Text>
        <Text style={styles.body}>
          Charge, Session, and Fault lights each mean something different
          — a fault always looks and sounds different from just being low
          on battery, so you can tell them apart without this app.
        </Text>
        <AccessibleButton label="Done" onPress={() => { setStep('DONE'); onComplete(); }} />
      </View>
    );
  }

  return (
    <View style={styles.container}>
      <Text style={styles.heading} accessibilityRole="header">Pair your NV-Band</Text>
      {error && <Text style={styles.error}>{error}</Text>}
      <AccessibleButton
        label={step === 'CONNECTING' || step === 'ATTESTING' ? 'Connecting…' : 'Scan for device'}
        onPress={startScan}
        disabled={step === 'CONNECTING' || step === 'ATTESTING'}
      />
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, padding: 24, justifyContent: 'center', gap: 16 },
  heading: { fontSize: 24, fontWeight: '700' },
  body: { fontSize: 17, lineHeight: 24 },
  error: { color: '#C0122F', fontSize: 15 },
});
