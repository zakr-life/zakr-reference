# CI gates for a fleet-ota-eligible firmware/model artifact

CLAUDE.md §9: "No build reaches the fleet-ota-eligible artifact stage
without passing the full interlock fault-injection suite, the
charge-balance unit tests, the watchdog-independence stress test, and the
debug-lockout check."

This document is the concrete gate list a CI pipeline (not yet wired to a
specific CI provider in this pass — no `.github/workflows` assumed, since
that depends on where ZAKR hosts this repo) must enforce before an
artifact is eligible for upload to `cloud/fleet-ota/`.

## Gate 1 — Firmware host test suite (implemented, runnable today)

```
firmware/run_host_tests.sh
```

Runs, and requires 100% pass on:

| Suite | Covers | Traceability prefix |
|---|---|---|
| `shared/tests` | Core0<->Core1 lock-free IPC | TRC-IPC |
| `core0_safety_signal/tests` | charge balance, interlock fault injection, watchdog independence, UI state exhaustiveness, channel config, mux break-before-make, AFE noise self-test, charger arbitration, PMIC rail sequencing, synchronous sampling ring buffer | TRC-CHG, TRC-ILK, TRC-WDG, TRC-UI, TRC-CFG, TRC-MUX, TRC-AFE, TRC-CHG-ARB, TRC-PMIC, TRC-SAMP |
| `core1_inference_radio/tests` | model->firmware stim clamp, epoch time-budget drop-on-overrun, mutual attestation / threat-model cases, session store write-then-commit | TRC-CLAMP, TRC-EPOCH, TRC-ATT, TRC-SESS |
| `bootloader/tests` | A/B rollback decision logic | TRC-BOOT |
| `sim/tests` | end-to-end software-in-the-loop scenarios (lifted electrode, make-before-break anomaly injection, rejected-waveform-independent-of-interlock) | TRC-SIM |

Full IDs and code/test links: `firmware/docs/traceability_matrix.csv`.

## Gate 2 — Debug-lockout check (build-config assertion, not a host test)

A release/fleet-ota build must be built from a `*_release.conf` overlay
(see `../bootloader/README.md`) with, at minimum:
- `CONFIG_DEBUG=n`
- SWD/JTAG access locked (APPROTECT or nRF5340-equivalent enabled)
- Secure boot verification enforced (image cannot boot unsigned)

CI must inspect the build's resolved Kconfig (`zephyr/.config` after
`west build`) and **fail the pipeline** if any of the above do not hold,
whenever the target channel is `fleet-ota`. This check has not been
wired into an actual CI YAML in this pass — see the open item at the top
of this file — but the assertion it must perform is fully specified here
so implementing it is a config-diff, not a design decision.

## Gate 3 — Model/firmware compatibility (see `models/versioning/`)

`cloud/fleet-ota/` independently refuses to distribute a model whose
declared firmware-compatibility range doesn't include the target
firmware version (enforced server-side, not by this CI gate) — see
`models/versioning/README.md`.

## Non-goal

Passing Gates 1-3 is a software-behavior gate. It is explicitly **not**
IEC 60601-1 electrical safety testing, biocompatibility testing (ISO
10993), or clinical validation, and must never be represented as such in
release notes, model cards, or marketing copy (CLAUDE.md §9, §10).
