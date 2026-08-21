/**
 * ConsentSettingsScreen — every consent toggle in the app, all default
 * off, each independently logged.
 *
 * CLAUDE.md §5: "explicit, separate consent toggles for (a) clinician
 * data sharing and (b) de-identified research/analytics use — default
 * both to off." Addendum 2 extends this with voiceprint enrollment/cloud
 * backup (§B) and federated model improvement (§D), each its own
 * standalone, independently-tested state module
 * (state/consentDefaults.js, state/voiceprintConsent.js,
 * state/federatedLearningConsent.js) composed together only here, at the
 * UI layer — a bug in one module's logic can never silently affect
 * another's default or its own logged-change trail.
 */
import React, { useState } from 'react';
import { View, Text, Switch, StyleSheet } from 'react-native';
// eslint-disable-next-line @typescript-eslint/no-var-requires
const { defaultConsentState, setConsent } = require('../state/consentDefaults');
// eslint-disable-next-line @typescript-eslint/no-var-requires
const { defaultVoiceprintConsentState, setVoiceprintConsent } = require('../state/voiceprintConsent');
// eslint-disable-next-line @typescript-eslint/no-var-requires
const { defaultFederatedLearningConsentState, setFederatedLearningConsent } = require('../state/federatedLearningConsent');

type Props = { onConsentChanged: (logEntry: any) => void };

export function ConsentSettingsScreen({ onConsentChanged }: Props) {
  const [consent, setConsentState] = useState(defaultConsentState());
  const [voiceprintConsent, setVoiceprintConsentState] = useState(defaultVoiceprintConsentState());
  const [federatedConsent, setFederatedConsentState] = useState(defaultFederatedLearningConsentState());

  const toggle = (key: 'clinicianDataSharing' | 'researchAnalytics', value: boolean) => {
    const { newState, logEntry } = setConsent(consent, key, value);
    setConsentState(newState);
    onConsentChanged(logEntry);
  };

  const toggleVoiceprint = (key: 'voiceprintEnrollmentConsent' | 'voiceprintCloudBackupConsent', value: boolean) => {
    const { newState, logEntry } = setVoiceprintConsent(voiceprintConsent, key, value);
    setVoiceprintConsentState(newState);
    onConsentChanged(logEntry);
  };

  const toggleFederated = (value: boolean) => {
    const { newState, logEntry } = setFederatedLearningConsent(
      federatedConsent, 'federatedLearningImprovement', value);
    setFederatedConsentState(newState);
    onConsentChanged(logEntry);
  };

  return (
    <View style={styles.container}>
      <Text style={styles.sectionHeading} accessibilityRole="header">Sharing with your clinician</Text>
      <View style={styles.row}>
        <View style={styles.textCol}>
          <Text style={styles.label}>Share data with my clinician</Text>
          <Text style={styles.help}>
            Lets a clinician you've connected with view your session history.
            Off by default.
          </Text>
        </View>
        <Switch
          value={consent.clinicianDataSharing}
          onValueChange={(v) => toggle('clinicianDataSharing', v)}
          accessibilityLabel="Share data with my clinician"
        />
      </View>

      <Text style={styles.sectionHeading} accessibilityRole="header">Research and product improvement</Text>
      <View style={styles.row}>
        <View style={styles.textCol}>
          <Text style={styles.label}>De-identified research use</Text>
          <Text style={styles.help}>
            Contributes de-identified, aggregate data to research. Off by
            default; can never re-identify your individual sessions.
          </Text>
        </View>
        <Switch
          value={consent.researchAnalytics}
          onValueChange={(v) => toggle('researchAnalytics', v)}
          accessibilityLabel="De-identified research use"
        />
      </View>
      <View style={styles.row}>
        <View style={styles.textCol}>
          <Text style={styles.label}>Federated model improvement</Text>
          <Text style={styles.help}>
            After a session, your device can compute a small, bounded
            adjustment to the on-device model and contribute it to a
            shared improvement — never your raw EEG or session data,
            only that small adjustment. Off by default.
          </Text>
        </View>
        <Switch
          value={federatedConsent.federatedLearningImprovement}
          onValueChange={toggleFederated}
          accessibilityLabel="Federated model improvement"
        />
      </View>

      <Text style={styles.sectionHeading} accessibilityRole="header">Voiceprint (optional)</Text>
      <View style={styles.row}>
        <View style={styles.textCol}>
          <Text style={styles.label}>Enable voiceprint verification</Text>
          <Text style={styles.help}>
            Lets you confirm it's you using your voice as an extra check,
            e.g. when re-pairing from a new phone. Off by default. The
            microphone is only ever on during a verification you start.
          </Text>
        </View>
        <Switch
          value={voiceprintConsent.voiceprintEnrollmentConsent}
          onValueChange={(v) => toggleVoiceprint('voiceprintEnrollmentConsent', v)}
          accessibilityLabel="Enable voiceprint verification"
        />
      </View>
      <View style={styles.row}>
        <View style={styles.textCol}>
          <Text style={styles.label}>Back up voiceprint to cloud</Text>
          <Text style={styles.help}>
            Only your voiceprint template (never audio) is backed up, and
            only if you turn this on separately. Off by default.
          </Text>
        </View>
        <Switch
          value={voiceprintConsent.voiceprintCloudBackupConsent}
          onValueChange={(v) => toggleVoiceprint('voiceprintCloudBackupConsent', v)}
          accessibilityLabel="Back up voiceprint to cloud"
          disabled={!voiceprintConsent.voiceprintEnrollmentConsent}
        />
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { padding: 24, gap: 20 },
  sectionHeading: { fontSize: 15, fontWeight: '700', color: '#3F3F46', marginTop: 12 },
  row: { flexDirection: 'row', alignItems: 'center', gap: 16 },
  textCol: { flex: 1 },
  label: { fontSize: 18, fontWeight: '600' },
  help: { fontSize: 14, color: '#6B7280', marginTop: 4 },
});
