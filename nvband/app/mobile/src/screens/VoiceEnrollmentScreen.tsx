/**
 * VoiceEnrollmentScreen — Addendum 2 §B ("Voiceprint") enrollment flow.
 *
 * Structurally consistent with PairingScreen.tsx: large touch targets
 * (AccessibleButton), accessible labels/roles, no jargon. Talks to the
 * device's mic-capture request/stop calls via micClient, mirroring how
 * PairingScreen talks to attestation via bleClient — the actual
 * PDM/feature-extraction work happens on-device
 * (firmware/core1_inference_radio/audio/, models/voiceprint/), never in
 * this screen.
 *
 * HARD UI REQUIREMENT (Addendum 2 §B, "no ambient/always-listening"):
 * whenever the mic is actually capturing, this screen shows an explicit,
 * unambiguous "microphone is recording" indicator — never optional,
 * never a state the user could miss. See <RecordingIndicator> below.
 *
 * This screen only ever requests capture while the enrollment flow is
 * itself the active screen — see
 * app/mobile/src/state/voiceprintGate.js's requestCaptureAllowed(),
 * called here with CaptureRequestAppState.VOICE_ENROLLMENT_ACTIVE for
 * exactly as long as (and no longer than) a RECORDING step is on screen.
 */
import React, { useEffect, useRef, useState } from 'react';
import { View, Text, StyleSheet, Animated, Easing } from 'react-native';
import { AccessibleButton } from '../../../design-system/AccessibleButton';
// eslint-disable-next-line @typescript-eslint/no-var-requires
const { EnrollmentState, CaptureRequestAppState, ENROLLMENT_EVENTS,
  requestCaptureAllowed, nextEnrollmentState } = require('../state/voiceprintGate');

const REQUIRED_UTTERANCES = 3;

type FlowStep = 'INTRO' | 'CONSENT_REQUIRED' | 'RECORDING' | 'PROCESSING' | 'DONE' | 'FAILED';

function RecordingIndicator() {
  const pulse = useRef(new Animated.Value(0)).current;

  useEffect(() => {
    const loop = Animated.loop(
      Animated.sequence([
        Animated.timing(pulse, { toValue: 1, duration: 600, easing: Easing.ease, useNativeDriver: true }),
        Animated.timing(pulse, { toValue: 0, duration: 600, easing: Easing.ease, useNativeDriver: true }),
      ])
    );
    loop.start();
    return () => loop.stop();
  }, [pulse]);

  const opacity = pulse.interpolate({ inputRange: [0, 1], outputRange: [0.4, 1] });

  return (
    <View
      style={styles.recordingBanner}
      accessible
      accessibilityRole="alert"
      accessibilityLiveRegion="assertive"
      accessibilityLabel="Microphone is recording"
    >
      <Animated.View style={[styles.recordingDot, { opacity }]} />
      <Text style={styles.recordingText}>Microphone is recording</Text>
    </View>
  );
}

export function VoiceEnrollmentScreen({
  micClient,
  voiceprintEnrollmentConsent,
  onEnrollmentComplete,
}: {
  micClient: any;
  voiceprintEnrollmentConsent: boolean;
  onEnrollmentComplete: (result: { enrolled: boolean }) => void;
}) {
  const [step, setStep] = useState<FlowStep>(
    voiceprintEnrollmentConsent ? 'INTRO' : 'CONSENT_REQUIRED'
  );
  const [enrollState, setEnrollState] = useState(EnrollmentState.NOT_ENROLLED);
  const [utteranceIndex, setUtteranceIndex] = useState(0);
  const [error, setError] = useState<string | null>(null);

  const appState = step === 'RECORDING'
    ? CaptureRequestAppState.VOICE_ENROLLMENT_ACTIVE
    : CaptureRequestAppState.IDLE_FOREGROUND;

  const beginEnrollment = () => {
    setEnrollState((s: any) => nextEnrollmentState(s, ENROLLMENT_EVENTS.START));
    setUtteranceIndex(0);
    setStep('RECORDING');
  };

  const captureOneUtterance = async () => {
    if (!requestCaptureAllowed(appState)) {
      // Structurally unreachable given the flow above, but this screen
      // never issues a capture request without checking the same gate
      // firmware itself enforces — see file header.
      return;
    }
    try {
      await micClient.requestCapture(); // firmware: nvband_mic_gate_request_capture()
      const nextIndex = utteranceIndex + 1;
      setUtteranceIndex(nextIndex);
      setEnrollState((s: any) => nextEnrollmentState(s, ENROLLMENT_EVENTS.UTTERANCE_CAPTURED));

      if (nextIndex >= REQUIRED_UTTERANCES) {
        setStep('PROCESSING');
        const result = await micClient.finishEnrollment(); // averages 3 utterances into a template
        setEnrollState((s: any) => nextEnrollmentState(s, ENROLLMENT_EVENTS.SUCCEEDED));
        setStep('DONE');
        onEnrollmentComplete({ enrolled: true, ...result });
      }
    } catch (e: any) {
      setError(e?.message ?? 'voice capture failed');
      setEnrollState((s: any) => nextEnrollmentState(s, ENROLLMENT_EVENTS.FAILED));
      setStep('FAILED');
    }
  };

  const retry = () => {
    setError(null);
    setEnrollState((s: any) => nextEnrollmentState(s, ENROLLMENT_EVENTS.RETRY));
    setUtteranceIndex(0);
    setStep('RECORDING');
  };

  if (step === 'CONSENT_REQUIRED') {
    return (
      <View style={styles.container}>
        <Text style={styles.heading} accessibilityRole="header">
          Voice verification isn't turned on
        </Text>
        <Text style={styles.body}>
          To set up voice verification you'll first need to turn on the
          "Voice enrollment" option in your privacy settings. It's off by
          default and completely optional — you can use the band fully
          without it.
        </Text>
      </View>
    );
  }

  if (step === 'INTRO') {
    return (
      <View style={styles.container}>
        <Text style={styles.heading} accessibilityRole="header">
          Set up voice verification
        </Text>
        <Text style={styles.body}>
          This is optional. It adds a quick voice check as an extra way
          to confirm it's really you — for example when pairing a new
          phone. It only ever compares your voice to your own saved
          sample (never to anyone else's), and it's never used to listen
          in or record you at any other time.
        </Text>
        <Text style={styles.body}>
          You'll say a short phrase {REQUIRED_UTTERANCES} times. The
          microphone is only ever on while you're actively recording a
          phrase on this screen.
        </Text>
        <AccessibleButton label="Start voice setup" onPress={beginEnrollment} />
      </View>
    );
  }

  if (step === 'RECORDING') {
    return (
      <View style={styles.container}>
        <Text style={styles.heading} accessibilityRole="header">
          Say the phrase ({utteranceIndex + 1} of {REQUIRED_UTTERANCES})
        </Text>
        <RecordingIndicator />
        <Text style={styles.body}>
          Hold the band a few inches away and speak clearly. Recording
          stops automatically as soon as your phrase is captured.
        </Text>
        <AccessibleButton
          label={`Record phrase ${utteranceIndex + 1}`}
          onPress={captureOneUtterance}
          accessibilityHint="Starts a short voice recording for enrollment."
        />
      </View>
    );
  }

  if (step === 'PROCESSING') {
    return (
      <View style={styles.container}>
        <Text style={styles.heading} accessibilityRole="header">Setting up your voice sample&hellip;</Text>
        <Text style={styles.body}>The microphone is now off.</Text>
      </View>
    );
  }

  if (step === 'DONE') {
    return (
      <View style={styles.container}>
        <Text style={styles.heading} accessibilityRole="header">Voice verification is ready</Text>
        <Text style={styles.body}>
          You can turn this off at any time in your privacy settings.
        </Text>
      </View>
    );
  }

  // step === 'FAILED'
  return (
    <View style={styles.container}>
      <Text style={styles.heading} accessibilityRole="header">That didn't work</Text>
      {error && <Text style={styles.error}>{error}</Text>}
      <Text style={styles.body}>The microphone is off. You can try again, or skip this for now.</Text>
      <AccessibleButton label="Try again" onPress={retry} />
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, padding: 24, justifyContent: 'center', gap: 16 },
  heading: { fontSize: 24, fontWeight: '700' },
  body: { fontSize: 17, lineHeight: 24 },
  error: { color: '#C0122F', fontSize: 15 },
  recordingBanner: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 12,
    backgroundColor: '#FDECEC',
    borderWidth: 2,
    borderColor: '#C0122F',
    borderRadius: 12,
    padding: 16,
  },
  recordingDot: {
    width: 20,
    height: 20,
    borderRadius: 10,
    backgroundColor: '#C0122F',
  },
  recordingText: { fontSize: 18, fontWeight: '700', color: '#C0122F' },
});
