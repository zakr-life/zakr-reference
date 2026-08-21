# ZAKR NV-Band

Firmware, on-device AI, companion app, and cloud backend for the ZAKR
NV-Band closed-loop tACS/EEG device. Built from the master specification
in `CLAUDE.md` (full text also at
`docs/ZAKR_NVBand_ClaudeCode_Master_Build_Prompt.md`) — **read that
first**; every module in this tree traces back to one of its sections.

**Read `STATUS.md` next** — it says plainly what's real/tested vs. a
structural placeholder in this pass, per the master prompt's own rule
against unearned claims (§10).

**Then read `docs/ADDENDUM_2_biometric_federated_sleep.md`** — a second,
equally authoritative specification covering five features added after
the initial build: EEG-based local authentication ("brainprint"),
microphone-based 1:1 voice verification ("voiceprint," BOM addition
U21), constrained bounded-rationale text generation (measured ≤0.5%
ungrounded-claim rate), federated learning (only a clipped model delta
ever leaves a device), and sleep-state monitoring with human-reviewed
clinician reporting. Open items OI-6 through OI-10 extend §11.

## The one rule

CLAUDE.md §0.1: firmware may veto stimulation; nothing in this
repository — firmware, app, cloud, or test harness — may be the sole
authority that permits it. Every layer here is built so it is
architecturally incapable of being that single point of failure.

## Quick start — run everything that's runnable today

```bash
# Firmware safety-core: 17 host-test binaries, ~10,300 assertions, plain gcc
./firmware/run_host_tests.sh

# Model pipeline: 23 tests + full synthetic-data -> train -> eval -> export -> register run
./models/run_pipeline_and_tests.sh

# Companion app logic: 25 tests, plain Node
node --test app/tests/*.test.js

# Cloud services logic: 34 tests, plain Node
node --test cloud/tests/*.test.js

# Provisioning tooling: 10 tests, plain Node
node --test tools/provisioning/tests/*.js
```

No external SDK, npm install, or pip install is required for any of the
above — pure gcc/Python-stdlib/Node-stdlib, deliberately, so the safety-
relevant logic in this repo is verifiable without a full embedded/mobile
toolchain.

## Layout

| Directory | CLAUDE.md section | What it is |
|---|---|---|
| `firmware/` | §3 | Core0/Core1 firmware, safety core, bootloader, secure element client |
| `models/` | §4 | Synthetic-data training/eval/export/registry pipeline |
| `app/` | §5 | React Native companion app (logic layer tested; screens not yet run) |
| `cloud/` | §6 | Ingestion, storage, clinician portal, fleet-ota, analytics, audit |
| `tools/` | §8 | Manufacturing provisioning + non-production bench-test tooling |
| `docs/` | §10 | Risk management, design history file, and open-items scaffolds |

## Open items

CLAUDE.md §11 (OI-1 through OI-5) are treated as blocking or explicitly
flagged throughout the codebase — grep for `TODO(OI-` to find every place
a decision is deliberately deferred rather than silently guessed. See
`docs/open-items/`.
