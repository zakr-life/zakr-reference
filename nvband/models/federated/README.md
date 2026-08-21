# models/federated/

Federated-learning pathway for the on-device state classifier, per
`../../docs/ADDENDUM_2_biometric_federated_sleep.md` §D: **only a clipped
model delta ever leaves a device** — never raw EEG, never a de-identified
session record.

This is separate from, and does not change, the existing centralized
de-identified analytics pathway (`cloud/analytics/deidentify.js`, opt-in,
k-anonymity-gated). Federated learning is gated by its own separate
consent toggle ("federated model improvement") and produces model
*deltas*, not session summaries.

## Pipeline

```
local_update.py       clipping.py               federated_round.py
(one device's         (bound the delta's L2      (simulate N devices,
 gradient step         norm before it may          average their clipped
 -> a delta)           leave the device;           deltas, apply to the
                        optional DP noise,          global model -> a
                        TODO(OI-8))                 candidate artifact)
```

Run from the `nvband/` directory:

```bash
python3 -m unittest discover -s models/federated/tests -v
python3 models/federated/federated_round.py
```

## What this produces

`federated_round.py` writes
`models/federated/candidate_state_classifier_float.json` — a new candidate
model in the **same JSON shape** as
`models/export/state_classifier_float.json`. This module does not sign or
register it. The candidate is meant to flow through the **existing,
unchanged** `models/export/export_model.py` (quantize + budget-gate) and
`models/versioning/registry.py` (sign + firmware-compat range) pipeline
like any other trained model — federated learning produces a *candidate*,
it does not bypass any existing distribution safety gate.

## Measured result (this run, this synthetic simulation)

12 simulated devices, each computing one local gradient step on a small
synthetic per-device batch, each delta clipped to L2 norm ≤ 1.0, averaged:
the resulting candidate model moved **0.0071 L2** from the starting global
model — a small, bounded nudge, not a large or unbounded jump, which is
the property the clipping + averaging design exists to guarantee.

## Why only a clipped delta, never raw or de-identified data

A local delta for this classifier is a handful of floating-point numbers
(3 classes × 20 features + 3 biases = 63 values) — categorically different
from a session record or an EEG signal. Clipping bounds what even a single
compromised device could contribute (defends against model poisoning);
see `cloud/federated/aggregator.js` for the cloud-side minimum-cohort gate
that additionally prevents any one device's contribution from being
isolated in an aggregation round.

## Honesty note on where this runs

This module implements and tests the local-update **algorithm** at the
pipeline level — exactly how the base classifier's own training already
works (`models/training/`) before any on-device runtime integration.
Wiring actual on-device gradient computation into Core 1 firmware (so a
real device computes its own delta after a real session) is a firmware
bring-up task **not done in this pass**, in the same honest category as
the existing "TinyML runtime integration is not done" item in
`../../STATUS.md`. Nothing here claims to run on real device hardware.

## Open items

- `TODO(OI-8)`: the differential-privacy noise mechanism
  (`clipping.add_calibrated_noise`) exists and is tested, but its noise
  scale is a placeholder (default: off) pending a formal epsilon-budget
  privacy review — not a finalized privacy guarantee.
