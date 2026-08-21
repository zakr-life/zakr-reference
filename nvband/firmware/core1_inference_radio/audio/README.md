# core1_inference_radio/audio/

Implements Addendum 2 §B ("Voiceprint"). BOM addition `U21` — MEMS digital
microphone, PDM output, wired directly into the nRF5340's PDM peripheral
(Core 1). Placement/part TBD pending mechanical review of the band —
`TODO(OI-6)`, same "flag, don't silently pick a final answer" discipline
as OI-1's channel count.

## The one design rule for this directory

**Gated, session-scoped, no ambient/always-listening capture.** The
mic's power/clock domain can only ever be enabled by an explicit,
session-scoped `nvband_mic_gate_request_capture()` call representing a
user-initiated voice-enrollment or voice-verification action. There is
no wake-word, no background sampling, no code path anywhere in this
directory that powers the mic outside that one function. Once powered,
the mic is guaranteed to be powered off again after a bounded maximum
window (`NVBAND_MIC_MAX_CAPTURE_WINDOW_US`, currently 8 s) even if
nothing ever calls to stop it explicitly — a watchdog
(`nvband_mic_gate_tick()`), not reliance on a caller remembering. See
`mic_power_gate.h`'s docstring for the exact invariant and
`tests/test_mic_power_gate.c` for the tests that prove it.

**Raw audio never reaches the session store or the radio path.** Nothing
in this directory has a function signature that accepts a storage handle,
a BLE/radio handle, or anything resembling one.
`mic_capture.c`'s buffer is caller-owned, in-memory only, and this module
never persists or transmits it. Per Addendum 2 §B, the *only* thing that
may ever leave the device is a fixed-length embedding/template vector
produced by feature extraction (not part of this pass — see "Not
included" below), and only if the user has separately opted into cloud
voiceprint backup — see `app/mobile/src/state/voiceprintConsent.js` and
`cloud/biometrics/voiceprintTemplateStore.js`.

## Layout

- `mic_power_gate.{h,c}` — the mic power/clock domain state machine
  (IDLE / REQUEST_PENDING / CAPTURING / DISCARDING). HAL-injected
  (function pointers for actually driving the power/clock domain), fully
  host-testable against a fake, mirroring `drivers/charger_arbitration.c`.
  This is the single most important correctness property in this
  directory — see its header docstring.
- `mic_capture.{h,c}` — PDM sample buffering during one capture window: a
  simplified but structurally realistic fixed-capacity ring of int16
  samples. Does not model real PDM decimation filter math (that is DSP
  the eventual U21 register-level driver owns, same "not fabricated
  until the datasheet is in hand" discipline as
  `firmware/core0_safety_signal/drivers/README.md`'s treatment of the
  AFE/DAC/IMU/RTC drivers). Caller-owned storage; no function here writes
  it anywhere else.
- `voice_activity_gate.{h,c}` — a simple energy-threshold (RMS) check
  that must pass before a captured buffer proceeds to feature extraction.
  If the buffer lacks sufficient voice-level energy, this aborts the
  attempt rather than guessing. Energy-only — explicitly not a
  speech/non-speech classifier, and not a liveness/anti-spoofing measure
  (`TODO(OI-7)`).
- `tests/` — host-testable suites for all of the above, `gcc -std=c11
  -Wall -Wextra -Werror`, no Zephyr SDK, matching
  `firmware/core0_safety_signal/tests/`'s pattern exactly.

## What's not included in this pass

- **PDM peripheral driver / decimation filter** for the actual U21 part —
  not written for the same reason the AFE/DAC/IMU/RTC drivers in
  `firmware/core0_safety_signal/drivers/README.md` aren't: it needs a
  datasheet-verified register map for a part that isn't mechanically
  finalized yet (`TODO(OI-6)`).
- **Feature extraction** (turning a capture buffer into the fixed-length
  vector `models/voiceprint/` trains/evaluates against) — the model-side
  feature representation is defined and tested in `models/voiceprint/`;
  wiring an on-device extractor that consumes `mic_capture`'s buffer and
  produces that same representation is a firmware/model integration task
  not yet performed, in the same honest category as the existing "TinyML
  runtime integration is not done" item in `STATUS.md`.
- **BLE/app-facing enrollment or verification protocol** (the
  characteristic(s) an app would use to trigger
  `nvband_mic_gate_request_capture()` and receive a result) — not
  included; `ble/attestation.c`'s existing GATT command-gating pattern is
  the template once characteristic UUIDs are assigned
  (`firmware/core1_inference_radio/README.md` already flags
  `gatt_services.h` as pending for the same reason).

## Explicitly out of scope for this subsystem

Nothing in this directory writes to, calls, or otherwise influences
`inference/stim_command_clamp.c`, the interlock chain, or any DAC/
stimulation path. This subsystem is sensing/identity only — Addendum 2
Rule 0 ("none of these five features may create a new path to
stimulation") applies structurally here, not just by policy: there is no
function in `mic_power_gate.h`, `mic_capture.h`, or
`voice_activity_gate.h` whose output type is anything a stim-command path
could consume.
