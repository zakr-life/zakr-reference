/**
 * SessionControlScreen — CLAUDE.md §5 session flow.
 *
 * STOP is rendered unconditionally whenever isStopAvailable() says so —
 * no confirmation modal, no "hold to confirm," no nested navigation. It
 * is always the single most prominent element on screen while available.
 */
import React, { useEffect, useState } from 'react';
import { View, StyleSheet, ScrollView } from 'react-native';
import { AccessibleButton } from '../../../design-system/AccessibleButton';
import { StatusLedIndicator } from '../../../design-system/StatusLedIndicator';
// eslint-disable-next-line @typescript-eslint/no-var-requires
const { SessionState, initialState, reduce, isStopAvailable, isStartAvailable } =
  require('../state/sessionStateMachine');

export function SessionControlScreen({ bleClient }: { bleClient: any }) {
  const [state, setState] = useState(initialState());
  const [lastUpdatedMs, setLastUpdatedMs] = useState(Date.now());

  useEffect(() => {
    const unsubscribe = bleClient.onDeviceStatus((status: any) => {
      setLastUpdatedMs(Date.now());
      if (status.fault) {
        setState((s: any) => reduce(s, { type: 'DEVICE_STATUS_FAULT', fault: status.fault }));
      } else if (status.sessionActive) {
        setState((s: any) => reduce(s, { type: 'DEVICE_STATUS_SESSION_ACTIVE', status }));
      } else {
        setState((s: any) => reduce(s, { type: 'DEVICE_STATUS_SESSION_IDLE', status }));
      }
    });
    return unsubscribe;
  }, [bleClient]);

  const onStop = () => {
    // Dispatched immediately, synchronously with the tap — see
    // sessionStateMachine.js header comment on why there is no local
    // optimistic "already stopped" transition, and no confirmation step.
    setState((s: any) => reduce(s, { type: 'USER_TAPPED_STOP' }));
    bleClient.sendSessionStop();
  };

  const onStart = () => {
    setState((s: any) => reduce(s, { type: 'USER_TAPPED_START' }));
    bleClient.sendSessionStart();
  };

  return (
    <ScrollView contentContainerStyle={styles.container}>
      <View style={styles.statusBlock}>
        <StatusLedIndicator label="Charge" color="green" lastUpdatedMs={lastUpdatedMs} />
        <StatusLedIndicator
          label="Session"
          color={state.ui === SessionState.ACTIVE ? 'green' : 'off'}
          blinking={state.ui === SessionState.PENDING_START}
          lastUpdatedMs={lastUpdatedMs}
        />
        <StatusLedIndicator
          label="Fault"
          color={state.ui === SessionState.PAUSED_FAULT ? 'red' : 'off'}
          lastUpdatedMs={lastUpdatedMs}
        />
      </View>

      {isStopAvailable(state) && (
        <AccessibleButton
          label="STOP SESSION"
          variant="danger"
          isStopAdjacent
          onPress={onStop}
          accessibilityHint="Immediately stops stimulation and ends the session."
        />
      )}

      {isStartAvailable(state) && (
        <AccessibleButton label="Start Session" variant="primary" onPress={onStart} />
      )}
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { padding: 20, gap: 16 },
  statusBlock: { marginBottom: 24 },
});
