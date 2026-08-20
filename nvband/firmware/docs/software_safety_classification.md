# IEC 62304-style software safety classification (rationale, not a certification)

CLAUDE.md §1 / §10: "treat this codebase as Class C, the highest software
safety class, because a software failure could contribute to death or
serious injury" and "propose Class C given failure could contribute to
serious injury; flag for real regulatory sign-off."

**This is a rationale for a proposed classification, not a certified
classification.** IEC 62304 classification is formally assigned through
a documented risk-management process by a qualified reviewer against a
real hazard analysis (see `../../docs/risk-management/`, currently a
scaffold with placeholder likelihood/severity). This document exists so
that process has a documented starting position to accept, adjust, or
reject — not to assert the classification is final.

## Proposed classification: Class C, system-wide

Rationale: the system delivers electrical current to a person via
transcranial electrodes. A software defect in the stimulation command
path, the interlock read logic, the charge-balance validator, or the
watchdog contingency logic could plausibly contribute to a hazardous
situation whose harm is not limited to "non-serious injury" (IEC 62304's
Class B/C boundary) — see hazard log entries HAZ-01 through HAZ-04 in
`../../docs/risk-management/hazard_log.csv`. Class C is the conservative
starting position; a qualified reviewer may downgrade specific SOFTWARE
ITEMS (not the whole system) where a documented hazard analysis supports
it — for example, `cloud/analytics/` (opt-in, de-identified, no
stimulation-path connection) is a plausible Class A candidate on its own
hazard analysis, but this document does not perform that analysis; it
only proposes the conservative default and flags the possibility.

## Module-level rationale (not independently classified — proposal only)

| Module | Proposed class | Why |
|---|---|---|
| `firmware/core0_safety_signal/safety/` | C | Directly implements/observes the interlock, charge-balance, and watchdog logic — see CLAUDE.md §0.1. |
| `firmware/core0_safety_signal/drivers/` | C | AFE self-test, mux sequencing, and charger/PMIC sequencing directly gate what firmware may attempt to command. |
| `firmware/core1_inference_radio/inference/stim_command_clamp.c` | C | The explicit hard boundary between model output and any firmware stimulation command. |
| `firmware/core1_inference_radio/ble/attestation.c` | C | Governs whether STOP-adjacent vs. session-start commands are accepted — a defect here is a security/safety hazard (HAZ-03, HAZ-06). |
| `firmware/core1_inference_radio/session/`, `firmware/bootloader/` | B | Data-integrity and update-safety concerns; a failure degrades evidence/updatability rather than directly enabling a stimulation hazard, but interacts with Class C paths (OTA -> new stim logic). |
| `models/` (training/eval/export/registry) | Not classified — offline tooling, not itself part of the deployed device software. The exported *artifact* it produces is subject to the on-device inference module's classification once that integration exists (see STATUS.md). |
| `app/`, `cloud/` | B (app: STOP/status paths), A (cloud: analytics, portal review) — proposal only, not analyzed against a real hazard log. |

## What this document does not do

It does not certify anything. It does not substitute for a qualified
reviewer's sign-off. It does not claim IEC 62304 process compliance
(documented software development plan, verified unit/integration/system
test records beyond what's linked in `traceability_matrix.csv`, SOUP
analysis, etc.) — those artifacts either don't exist yet or are
explicitly out of scope for this pass (see `../../STATUS.md`).
