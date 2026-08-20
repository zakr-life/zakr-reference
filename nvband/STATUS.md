# Implementation status (read this before trusting any claim in this repo)

This document exists because CLAUDE.md §10/§13 requires every component's
documentation to "make no unearned safety or regulatory claims" and
because "a component is done when ... it references the correct
open-item TODOs where applicable." This is the single place that says,
plainly, what's real and what isn't, across the whole tree, as of this
build pass.

## What's real: implemented, and independently verified by passing tests in this repo

- **Firmware safety core** (`firmware/core0_safety_signal/safety/`,
  `drivers/`, `sampling/`, `firmware/shared/`, `firmware/bootloader/`,
  `firmware/core1_inference_radio/inference/`, `ble/attestation.c`,
  `session/`): charge-balance validation, the interlock status
  reader/re-arm state machine, watchdog-kick sampling contingency, the
  three-LED/buzzer exhaustive-distinctness UI model, break-before-make
  mux sequencing with anomaly detection, AFE noise-floor self-test logic,
  dual-input charger arbitration, PMIC rail sequencing, the lock-free
  Core0<->Core1 IPC queue, the model->firmware hard stim clamp, epoch
  time-budget drop-on-overrun, mutual-attestation command gating
  (including the "STOP always works" threat-model case), write-then-
  commit session storage, and A/B rollback decision logic.
  **17 host-test binaries, ~10,300 assertions, all passing** —
  `firmware/run_host_tests.sh`. Compiled with `gcc -Wall -Wextra -Werror`,
  no Zephyr SDK needed for these modules because they're deliberately
  hardware-independent (HAL-injected, tested against fakes).
- **Model pipeline** (`models/`): synthetic EEG+IMU generator, subject-
  level train/val/test split with an independently re-verified no-
  leakage guarantee, motion-contaminated-epoch rejection before the
  classifier ever sees them, a trained softmax classifier, held-out
  evaluation with an honestly-reported artifact-robustness gap (94%
  clean accuracy vs. 58% on contaminated epochs — see
  `models/export/model_card.md`), int8 symmetric quantization with a
  real latency/size budget gate, and a signed model registry with
  firmware-compatibility-range enforcement. **23 unit tests + a full
  successful pipeline run** — `models/run_pipeline_and_tests.sh`.
- **App logic layer** (`app/mobile/src/state/`, `src/ble/`): session
  state machine (STOP always available, no local optimism, no
  confirmation gating), consent defaults (both off), an independent
  mirror of firmware's attestation command policy, caregiver adherence
  computation, OTA auto-apply gating. **25 tests** —
  `node --test app/tests/*.test.js`.
- **Cloud logic layer** (`cloud/`): Ed25519 device authentication,
  hash-chained tamper-evident audit log, retention policy with no
  indefinite default, clinician RBAC with mandatory paired audit
  logging, staged OTA rollout with halt-only (never auto-revert)
  fault-spike detection, opt-in-gated de-identification with a
  k-anonymity publish gate. **34 tests** — `node --test cloud/tests/*.test.js`.
- **Provisioning tooling** (`tools/provisioning/`): the "a unit missing
  any required manufacturing record is not a built unit" system
  invariant, key-ceremony one-identity-per-unit enforcement. **10 tests**.

## What's structural scaffolding, not a working implementation

Flagged explicitly in each affected directory's own README, listed here
for one-stop visibility:

- **Register-level hardware drivers**: `firmware/core0_safety_signal/drivers/README.md`
  lists exactly which ADS1299/AD5687R/BMI270/PCF85063 Zephyr drivers are
  not included, and why (they need a datasheet-verified register map;
  guessing register addresses for a device that injects current into a
  person is the kind of guess this repo does not make).
- **Zephyr `main.c` RTOS glue** for both core images — thread creation,
  device-tree bindings, board overlay — gated on the final nRF Connect
  SDK core-split configuration. `CMakeLists.txt`/`prj.conf` are in place
  and reference every real module above.
- **BLE GATT characteristic table** (`gatt_services.h`) — trivial once
  UUIDs are assigned, not yet assigned in this pass.
- **TinyML on-device runtime integration** — the training/export
  pipeline is real and produces a genuine int8 artifact; wiring a
  TFLite-Micro (or comparable) interpreter into `core1_inference_radio/inference/`
  to actually execute it on Cortex-M33 is not done in this pass.
- **React Native screens** (`app/mobile/src/screens/`) are real,
  structurally correct components wired to the *tested* state modules —
  but this environment has no RN toolchain/emulator, so they have not
  been run. Navigation wiring (`App.tsx`) and native `ios/`/`android/`
  project files are not included (the latter are tool-generated, not
  hand-authored source).
- **Cloud HTTP framework wiring** — `cloud/README.md` explains why the
  policy logic (the part that actually matters) is framework-agnostic on
  purpose; Express/Fastify route wiring is not included.
- **MCUboot itself** (image swap, Ed25519/ECDSA signature chain to
  U14's root key) is a west-managed module, not vendored source;
  `firmware/bootloader/ab_rollback.c` is the application-level
  confirm/rollback decision hook it calls into.
- **CI YAML** — `firmware/docs/ci_gates.md` specifies exactly what a
  pipeline must check (and why); it isn't wired to a specific CI
  provider since that depends on where ZAKR hosts this repo.

## Open items (CLAUDE.md §11) still live in this codebase

- **OI-1** (electrode/channel count): `firmware/shared/nvband_channel_config.h`
  is data-driven, not a compile-time constant, exactly because this is
  unresolved. Grep `TODO(OI-1)`.
- **OI-2 through OI-5**: no direct firmware dependency per the master
  prompt; nothing in this codebase claims device-level safety
  certification anywhere (see every model card, bench-test report, and
  CI-gates doc's explicit non-goal section).

## What this status document is not

It is not a certification, a design-verification record, or a claim that
any of the above has passed IEC 60601-1, ISO 14971 review, or clinical
validation. It is an honest inventory of what exists and what was
verified to run, in this repository, in this pass.
