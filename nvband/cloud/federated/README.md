# cloud/federated/

Server-side aggregation policy for the federated-learning pathway, per
`../../docs/ADDENDUM_2_biometric_federated_sleep.md` §D.

Framework-agnostic policy logic only (no HTTP wiring), matching the rest
of `cloud/`. See `models/federated/` for the on-device-algorithm side
(what a device computes before submitting) and `models/versioning/` for
what happens to an aggregated candidate model afterward (unchanged — it
re-enters the existing signed registry + staged fleet-OTA rollout).

## What this guarantees

- **`validateDeltaSubmission`** — rejects any delta whose L2 norm exceeds
  a plausible bound, or that is malformed/wrong-shaped. Defends against a
  single malicious or compromised device attempting to poison the shared
  model with an outsized update.
- **`isCohortReadyToAggregate`** / **`aggregateRound`** — mirrors the
  k-anonymity minimum-group gate already used for analytics
  (`cloud/analytics/deidentify.js`). An aggregation round never runs on
  fewer than `DEFAULT_MIN_COHORT_SIZE` (10) distinct devices' deltas —
  enforced twice: once as a pre-check callers are expected to use, and
  again inside `aggregateRound` itself as a defense-in-depth re-check, so
  a caller bug upstream cannot isolate one device's contribution.

## What this does not do

Does not perform device attestation itself — a real deployment wires
`validateDeltaSubmission` behind the same attested-identity check as
`cloud/ingestion/deviceAuth.js` before a delta is even considered
"pending" toward a cohort. Does not sign or version the resulting
aggregate — that is `models/versioning/registry.py`'s job, unchanged.
