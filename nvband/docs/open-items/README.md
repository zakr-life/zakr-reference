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
