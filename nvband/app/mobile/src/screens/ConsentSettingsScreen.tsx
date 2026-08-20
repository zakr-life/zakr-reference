/**
 * ConsentSettingsScreen — CLAUDE.md §5 consent toggles, both default off
 * (see state/consentDefaults.js).
 */
import React, { useState } from 'react';
import { View, Text, Switch, StyleSheet } from 'react-native';
// eslint-disable-next-line @typescript-eslint/no-var-requires
const { defaultConsentState, setConsent } = require('../state/consentDefaults');

export function ConsentSettingsScreen({ onConsentChanged }: { onConsentChanged: (logEntry: any) => void }) {
  const [consent, setConsentState] = useState(defaultConsentState());

  const toggle = (key: 'clinicianDataSharing' | 'researchAnalytics', value: boolean) => {
    const { newState, logEntry } = setConsent(consent, key, value);
    setConsentState(newState);
    onConsentChanged(logEntry); // feeds cloud/audit's consent-change log
  };

  return (
    <View style={styles.container}>
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
    </View>
  );
}

const styles = StyleSheet.create({
  container: { padding: 24, gap: 24 },
  row: { flexDirection: 'row', alignItems: 'center', gap: 16 },
  textCol: { flex: 1 },
  label: { fontSize: 18, fontWeight: '600' },
  help: { fontSize: 14, color: '#6B7280', marginTop: 4 },
});
