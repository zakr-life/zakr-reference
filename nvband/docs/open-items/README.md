# docs/open-items/

Mirrors CLAUDE.md §11 verbatim. Code references these IDs via
`TODO(OI-n)` comments — grep the repo for `TODO(OI-` to find every place
a decision is deliberately deferred rather than silently guessed.

## OI-1 — Electrode/channel count unresolved

The parameter set assumes 2 stim + 4 EEG electrodes on two tails; the
mechanical carrier drawing doesn't yet reconcile with that. This
directly changes firmware stimulation channel count and total delivered
charge per session.

**Where this shows up in code:** `firmware/shared/nvband_channel_config.h`
makes channel count and mapping a provisioned parameter, not a
compile-time constant, specifically so this eventual decision doesn't
require a firmware rewrite. `NVBAND_PLACEHOLDER_STIM_CHANNEL_COUNT` /
`NVBAND_PLACEHOLDER_EEG_CHANNEL_COUNT` in `nvband_constants.h` are the
current placeholder figures (2 stim + 4 EEG), used only in bench/
simulation builds — a production boot path without a valid provisioning
record fails closed rather than falling back to these defaults.

## OI-2 — Isolation barrier creepage/clearance are placeholders

Pending a qualified safety reviewer against IEC 60601-1. No direct
software dependency, but the firmware-side provisioning-record gate
(CLAUDE.md §3.2 — not yet implemented in this pass, see `../../STATUS.md`)
exists partly because of this; it must not be weakened in anticipation
of this resolving favorably.

## OI-3 — Band fit margin is tight (+3/+5 mm)

No firmware dependency. Noted for completeness.

## OI-4 — Flex tail bend-cycle count not yet set

No direct firmware dependency, but this is the highest field-failure-
risk item in the device. `firmware/core0_safety_signal/drivers/README.md`
notes that the AFE/impedance-monitoring driver (not register-level
implemented in this pass) should log impedance *trend*, not just
threshold-crossing, to help diagnose a failing tail in the field versus a
lifted electrode.

## OI-5 — Nothing here is a fabrication release or has passed independent check

Applies to the hardware dossier, not code, but reinforces that nothing
in this codebase claims device-level safety certification anywhere in
its documentation strings, comments, or generated reports. See every
model card, bench-test report, and CI-gates document's explicit
non-goal section, and `../../STATUS.md`.

---

The items below extend this list per `../ADDENDUM_2_biometric_federated_sleep.md`
(voiceprint, brainprint, federated learning, bounded-rationale generation,
sleep-state reporting). Same discipline: implement behind a named,
easily-changed configuration point and flag with `TODO(OI-n)` rather than
picking a silent default that looks final.

## OI-6 — Microphone (U21) part/placement not yet mechanically reconciled

The voiceprint feature (Addendum 2 §B) assumes a MEMS PDM microphone
somewhere on the band; the mechanical carrier drawing does not yet specify
where. **Where this shows up in code:** the microphone's identity/config is
a provisioned parameter, matching the OI-1 pattern, not a hardcoded pin/part
assumption.

## OI-7 — EEG/voice biometric anti-spoofing is not solved by this design

Brainprint (Addendum 2 §A) and voiceprint (§B) are documented as additive,
never-sole-authority local authentication factors specifically because
liveness/anti-spoofing for either modality is an open research problem, not
a solved one. No claim of spoof-resistance appears anywhere in either
feature's code or model cards. **Where this shows up in code:**
`firmware/core1_inference_radio/biometric/README.md` and
`firmware/core1_inference_radio/audio/README.md`.

## OI-8 — Federated-learning differential-privacy budget not finalized

The federated-learning pathway (Addendum 2 §D) clips every local update to a
bounded norm before it may leave the device; an additional noise
(differential-privacy) step exists as a mechanism but its epsilon/noise-scale
parameter is a named constant, not a reviewed privacy guarantee. **Where
this shows up in code:** `models/federated/clipping.py`.

## OI-9 — Biometric template retention under jurisdictional law not reviewed

Voiceprint and brainprint templates are biometric data. Statutes in some
jurisdictions (e.g., Illinois BIPA and similar) impose specific notice,
consent, and retention/destruction requirements. This repository implements
the technical *capability* for explicit consent gating, defined retention,
and an on-device-only default — it does not itself constitute a legal
compliance determination. **Where this shows up in code:**
`cloud/biometrics/voiceprintTemplateStore.js`.

## OI-10 — Automated real-time physician alerting is out of scope

Sleep-state reporting (Addendum 2 §E) surfaces flagged nights to a
human-reviewed clinician-portal queue only. Automated real-time alerting
(push/SMS/pager) is a materially different, higher-regulatory-bar product
(effectively an alarm system) and is explicitly not designed or built here.
**Where this shows up in code:** `cloud/clinician-portal/sleepTrend.js`.
