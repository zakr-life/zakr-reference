# firmware/core1_inference_radio/biometric/

EEG-based local authentication ("brainprint"), per
`../../../docs/ADDENDUM_2_biometric_federated_sleep.md` §A.

## What this is

An **additive, local** authentication factor that gates decryption of the
app's local session cache — it augments, never replaces, the secure-element
attestation in `firmware/secure/` (CLAUDE.md §7).

## Why brainprint is not used as cryptographic key material

A biometric signal has far lower usable entropy than a proper key, cannot
be rotated or revoked if compromised (you cannot re-enroll a different
brain), and drifts with electrode placement, fatigue, and physiological
state. Deriving key material directly from it is a known anti-pattern.
Instead: a brainprint match is a *gate* that must additionally pass before
the app will use the already-existing secure-element-derived key — the
same relationship a phone's Face ID has to its keychain.

## The no-lockout invariant

`brainprint_auth_gate.h`'s `nvband_brainprint_gate_result_t` has exactly
two values (`MATCH`, `FALLBACK`) — there is no third value meaning
"permanently denied." A mismatch always routes to the conventional
fallback path; it is structurally impossible for this module to express a
no-fallback denial. See `tests/test_brainprint_auth_gate.c`.

## Files

- `brainprint_template.h/.c` — per-epoch feature extraction (per-channel
  band-power ratios) and enrollment-template averaging.
- `brainprint_matcher.h/.c` — 1:1 cosine-similarity match against one
  caller-supplied enrolled template. Never searches/ranks across multiple
  templates.
- `brainprint_auth_gate.h/.c` — the no-lockout decision function.

## Open items

- `TODO(OI-7)`: EEG-biometric anti-spoofing/liveness is not solved by this
  design. No claim of spoof-resistance is made anywhere in this module or
  its model card (`models/brainprint/model_card.md`, once generated). This
  is documented as an additive convenience/local-security factor pending a
  dedicated security review — the same discipline OI-2 applies to the
  isolation barrier pending a qualified reviewer.

## What this is not

This module has no dependency on, and must never gain a dependency on,
`firmware/core0_safety_signal/safety/` or
`firmware/core1_inference_radio/inference/stim_command_clamp.c`. It has
nothing to do with stimulation.
