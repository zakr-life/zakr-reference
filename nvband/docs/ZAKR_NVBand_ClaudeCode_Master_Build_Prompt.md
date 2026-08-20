# ZAKR NV-Band — Master Build Prompt for Claude Code

**Paste this whole document into Claude Code as the initial instruction (or save as `CLAUDE.md` / `PROJECT.md` at repo root).** It is grounded entirely in ZAKR Neurotechnology's own design-intent dossier, BOM, interface control, and tolerance schedule for the NV-Band closed-loop tACS/EEG device. Treat every specific number in this document as authoritative until the referenced open item is resolved.

---

## 0. What this device is, in one paragraph

The NV-Band senses EEG from forehead electrodes, decides on-device whether the wearer is in a memory-encoding or recall state, and — when it is — delivers a low-intensity transcranial alternating current stimulation (tACS) burst phase-locked to the ongoing oscillation, then measures what happened and adapts. That loop is the product. All software, firmware, and models exist to make that loop work, or to guarantee it cannot hurt anyone when something fails.

## 0.1 The one rule that overrides every other instruction in this document

**Firmware may veto stimulation. Firmware may never be the sole authority that permits it.** Stimulation ENABLE is a hardware AND of seven independent conditions (current window, impedance window, electrode temperature window, supply rails good, watchdog satisfied, physical STOP switch closed, firmware permit asserted), implemented in discrete logic (U8) that is not firmware-loadable, backstopped by a watchdog IC (U9) with its own independent oscillator. **No code you generate — firmware, app, cloud, or test harness — may create a path that asserts stimulation enable without all seven hardware conditions independently satisfied, may simulate/spoof/bypass the interlock chain outside an explicitly labeled bench-test build, or may leave a debug/bypass path reachable in a production build.** Any bring-up bypass is a soldered link that manufacturing physically removes and signs off — never a firmware flag. If any task in this prompt appears to require weakening this rule, stop and flag it instead of implementing it.

This is not boilerplate. Build every layer — driver, RTOS task, BLE characteristic, app screen, cloud endpoint — so that it is architecturally incapable of being the single point of failure for stimulation safety.

---

## 1. Program context Claude Code should internalize before writing code

- **Company:** ZAKR Neurotechnology. **Product:** NV-Band, revision A design intent, August 2026.
- **Regulatory posture:** This is a device under development. Nothing has been through design verification, biocompatibility testing, electrical safety testing, or regulatory review. No code artifact you produce should claim compliance, certification, or clinical validation — it should be *structured to make compliance and validation achievable*, with the gaps clearly marked (see §10).
- **Governing standards to build toward (not claim conformance to):** IEC 60601-1 (electrical safety), IEC 60601-1-2 (EMC), IEC 62304 (medical device software lifecycle — treat this codebase as Class C, the highest software safety class, because a software failure could contribute to death or serious injury), ISO 14971 (risk management), ISO 10993-5/-10 (biocompatibility — hardware/materials concern, not software, but session limits and duty-cycle enforcement in firmware are risk controls that feed the ISO 14971 file), IEC 62133-2 / UN 38.3 (cell — hardware concern, but firmware charge/thermal management is a risk control), 21 CFR Part 11-style principles for audit trails on anything touching session data provenance.
- **Two electrical domains, one architectural mirror in software:** non-patient side (processor U2, radio, memory, PMIC, battery) and patient-applied side (stim output stage, electrodes), separated by a reinforced isolation barrier (U3, ≥5 kVrms). Software must never allow a data or control path that implies these domains are one trust domain. Model this as two logical subsystems even where a single SoC crosses the boundary in silicon via certified isolators.
- **Compute:** Nordic nRF5340, dual-core Cortex-M33 with TrustZone, BLE 5.x, ≥512 KB SRAM. **Core 0** owns the sample clock, the safety-chain GPIO reads, and the watchdog kick — real-time, deterministic, nothing on it may block on inference or radio. **Core 1** owns inference and the BLE stack. If a model overruns its time budget, it loses its own result; it must never be able to stall or delay Core 0's sampling loop or the watchdog kick.
- **Key part numbers firmware must target directly (representative, pending datasheet/lifecycle verification):**
  - U1 EEG AFE: TI ADS1299 / ADS1299-4, 8/4-ch 24-bit ΣΔ, integrated PGA + bias drive, SPI.
  - U2 SoC: Nordic nRF5340 (dev kit nRF5340-DK for bring-up), dual Cortex-M33, TrustZone, BLE 5.x.
  - U5 Stim DAC: AD5687R/AD5662-class, 16-bit bipolar, SPI, drives the isolated Howland current pump.
  - U9 Watchdog: TI TPS3430 / Maxim MAX6746-class, independent oscillator, windowed, external reset.
  - U10 PMIC: TI TPS65219-class or Nordic nPM1300 (pairs natively with nRF5340).
  - U12 Charger: TI BQ25180/BQ24074-class, dual-input arbitration (USB + Qi) — arbitration logic must live partly in firmware supervision, never let both inputs source simultaneously without arbitration.
  - U13 Fuel gauge: TI BQ27441 (or protection integrated into custom pack in production).
  - U14 Secure element: Microchip ATECC608B or NXP SE05x — hardware key storage, keys generated in-place and never leave the part.
  - U15 Firmware flash: QSPI NOR ≥64 Mbit (Winbond W25Q64 / Macronix MX25 series), A/B slots + rollback metadata.
  - U16 Session store: ≥512 Mbit NAND or high-density NOR (Winbond W25N01GV class), wear-levelled, encrypted at rest.
  - U17 IMU: Bosch BMI270 / TDK ICM-42688-P, 6-axis, must be sampled **synchronously** with EEG.
  - U18 Haptic: TI DRV2605L, closed-loop LRA autoresonance.
  - U19 Stim/sense mux: ADI ADG1608/ADG5208-class, **break-before-make mandatory** — a make-before-break part can momentarily connect stim output into the AFE input.
  - U20 RTC: NXP PCF85063 / Maxim DS3231-class + backup cell — session timestamps are clinical evidence and must survive a flat main battery.
- **Physical/timing constants firmware and models must encode as named constants, not magic numbers** (source: tolerance schedule):
  - Stim current density ceiling: **0.995 mA/cm² at 2.0 mA on a Ø16 mm pad**, limit 1.0 mA/cm². Margin is near zero — any future pad-diameter change requires a matched firmware current-ceiling change; encode the relationship, not just the current number.
  - Charge per phase: **≤1.0 µC at rated maximum, charge-balanced, zero net DC.** Firmware must actively verify charge balance per pulse pair, not just command symmetric waveforms.
  - Average current budget: **74.4 mA at 85% depth of discharge**, derived from 700 mAh / 3.7 V over an 8 h wear target. This is a firmware power budget, not just a hardware spec — session scheduling, radio duty cycle, and inference cadence all draw against it.
  - Electrode contact pressure assumption: 1.39 kPa from 0.28 N/electrode — firmware must treat impedance drift as the observable proxy for this, since it cannot measure pressure directly.
  - HiPot record: 1500 V / 60 s per unit — this is a manufacturing-time record, not a runtime one, but session firmware must refuse to arm stim on any unit lacking a valid provisioning record referencing it (see §8).

---

## 2. Repository layout to generate

```
nvband/
├── firmware/
│   ├── core0_safety_signal/        # Zephyr RTOS app, Core 0 image
│   │   ├── drivers/                # AFE (ADS1299), DAC (AD5687R), mux (ADG1608), IMU, RTC, watchdog kick, PMIC/charger
│   │   ├── safety/                 # interlock STATUS reader (read-only), fault latch handling, charge-balance verifier
│   │   ├── sampling/                # synchronous EEG+IMU acquisition, ring buffers, timestamping
│   │   └── tests/
│   ├── core1_inference_radio/      # Core 1 image
│   │   ├── ble/                     # GATT services, pairing, OTA transport, attestation handshake
│   │   ├── inference/               # TinyML runtime, quantized model loader, epoch scoring
│   │   ├── session/                 # session state machine, encrypted session store writer
│   │   └── tests/
│   ├── secure/                      # secure element (ATECC608B/SE05x) driver, key ceremony client, secure boot config
│   ├── bootloader/                  # MCUboot-based A/B updater, rollback, image signing verification
│   ├── shared/                      # HAL abstractions, constants (see §1), CRC/encoding, IPC between cores
│   ├── sim/                         # hardware-in-the-loop and software-in-the-loop simulators (resistive phantom model, fault injectors)
│   └── docs/                        # traceability matrix, IEC 62304 artifact stubs (see §10)
├── models/
│   ├── training/                    # offline training pipeline (Python), data loaders, labeling schema
│   ├── evaluation/                  # held-out validation, artifact-robustness testing, clinical-metric reporting stubs
│   ├── export/                      # quantization + export to TinyML runtime format, model card generator
│   └── versioning/                  # model registry, signing, compatibility matrix vs firmware version
├── app/
│   ├── mobile/                      # cross-platform companion app (React Native or native iOS/Android — see §5)
│   ├── design-system/               # accessible component library shared with clinician portal
│   └── tests/
├── cloud/
│   ├── ingestion/                   # session upload API, attestation-gated device auth
│   ├── storage/                     # encrypted-at-rest session data, retention policy engine
│   ├── clinician-portal/            # web app for clinicians/caregivers
│   ├── fleet-ota/                   # signed firmware/model distribution service
│   ├── analytics/                   # aggregate, de-identified analytics (opt-in only)
│   └── audit/                       # tamper-evident audit log service
├── tools/
│   ├── provisioning/                # manufacturing-line key ceremony + HiPot record ingestion tooling
│   └── bench-test/                  # phantom-driven bench validation scripts (explicitly non-production)
└── docs/
    ├── risk-management/             # ISO 14971 hazard log scaffold, linked to code via traceability IDs
    ├── design-history-file/         # DHF folder scaffold
    └── open-items/                  # mirrors the five open items below; code must reference these IDs
```

---

## 3. Firmware — build this first, and in this order

Follow the dossier's bring-up order conceptually even in software: get each layer trustworthy before the next depends on it.

### 3.1 Power and rail supervision (`core0_safety_signal/drivers/pmic`, `charger`)
- Sequence rail bring-up and confirm every rail settles (via ADC/comparator readback, not assumption) before releasing the AFE from reset.
- Implement dual-input charge arbitration (USB vs. Qi) in supervisory firmware: never allow both sources to actively source into the charger simultaneously; log every arbitration event.
- Continuously monitor cell voltage/temperature via the fuel gauge (U13) and fold back charge current on NTC excursions.
- Expose rail-good status as one of the read-only inputs the interlock chain can observe — firmware reads this, it does not gate stim with it in software; the hardware AND already does that.

### 3.2 Grounds, isolation posture (software-observable half)
- Firmware cannot verify galvanic isolation (that's a manufacturing HiPot test), but it must refuse to arm any stimulation session on a unit whose provisioning record (see §8) does not carry a valid stored HiPot pass reference. Treat a missing/invalid record as a hard fault, not a warning.

### 3.3 EEG acquisition (`sampling/`, driver for U1 ADS1299)
- Configure the AFE for the noise/bandwidth targets in the dossier: input-referred noise <1 µV RMS over 0.5–40 Hz. Build a startup self-test that measures noise floor with the mux forced to a shorted/known-impedance state and refuses to proceed to a live session if it exceeds threshold.
- Sample all active EEG channels **synchronously with the IMU (U17)** on a shared clock/trigger — this is required for artifact rejection and adherence logging, not optional.
- Implement impedance measurement using small AC injection only — verify in code review/tests that no DC path can be commanded through this measurement path.
- Drive the mux (U19) with explicit break-before-make timing margins in the driver; add a unit test that asserts the break interval is never zero under any code path, including error recovery.

### 3.4 Stimulation output control (`safety/`, driver for U5 DAC + Howland pump)
- Firmware's only authority is to *request* a current setpoint via the isolated SPI path to the DAC. It must never write directly to any GPIO that is one of the seven hardware interlock inputs except the one line it legitimately owns: "firmware permit."
- Enforce charge balance at the waveform-generation layer: every commanded stimulation waveform must be validated (in code, with a test) to integrate to zero net charge before it is transmitted to the DAC, in addition to (not instead of) the hardware DC-blocking capacitor.
- Enforce the current ceiling and per-session/per-day charge budget in firmware as a *defense-in-depth* layer, explicitly documented as non-authoritative relative to the hardware chain.
- Session duration, duty cycle, and cumulative charge must be logged with RTC (U20) timestamps for every session, pass or fail.

### 3.5 Interlock chain interface (`safety/interlock_status.c` or equivalent)
- Firmware reads (does not write, except its own permit bit) the latched fault status from U8. On any fault, firmware must: (a) immediately stop attempting to command new stimulation, (b) surface the fault to UI/LED/buzzer per §3.7, (c) log the fault with timestamp and, where available, which of the seven conditions tripped, (d) require an explicit, deliberate re-arm sequence — never auto-clear a latch.
- Build a fault-injection test harness (`firmware/sim/`) that can simulate each of the seven conditions failing independently and assert firmware's response matches the above in every case, including simultaneous multi-fault scenarios.

### 3.6 Watchdog (U9) and Core partitioning
- Core 0 kicks the external watchdog on a fixed cadence tied to real sample acquisition, not a free-running timer — i.e., the kick must be provably contingent on the sampling loop actually running, not just on the core being alive.
- Core 1 (inference/BLE) must never be able to block, starve, or delay the Core 0 kick. Use IPC design (shared memory + lock-free queues, not blocking mutexes) to guarantee this; write a stress test that pegs Core 1 at 100% and confirms Core 0's watchdog kick timing is unaffected.
- If an inference task overruns its allotted time budget, the correct behavior is: drop that epoch's result, log it, continue — never retry-block, never signal Core 0.

### 3.7 User-facing state (`core0` or shared, per Sheet 7)
- The physical STOP control is a normally-closed hardware switch in series with the enable line — firmware's job regarding STOP is purely observational (log the event); it must never be the mechanism that turns stimulation off. Do not build any UI or firmware path that treats STOP as a debounced interrupt to be "handled" as the actual safety action.
- Implement the three-LED + buzzer state model: charge state, session state, fault state, with fault visually and audibly distinguishable from low-battery — encode this as a state machine with an exhaustive test asserting no two distinct fault/battery conditions render identically.
- Put the temperature sensor reading (from the electrode-substrate-mounted sensor) into both the interlock-observable path and the session log; firmware must not conflate board temperature with skin-contact temperature anywhere in code or naming.

### 3.8 Secure boot, identity, OTA (`secure/`, `bootloader/`)
- Secure boot on, root key resident only in U14, debug port locked in production builds (build system must produce a distinct, clearly labeled "engineering/debug" build type that is never the artifact shipped to fleet-ota).
- Firmware requests signatures from the secure element; the private key never transits the SoC, never appears in firmware binaries, never appears in logs.
- A/B firmware slots on U15 with rollback: verify new image signature and run a self-test boot before marking it primary; automatic rollback on repeated boot failure.
- Model updates (from `models/export/`) are signed and versioned separately from firmware but validated for firmware-compatibility before load (see §6 model registry).

### 3.9 Session store (`session/`, driver for U16)
- Encrypt session data at rest using keys derived via U14; size the store for one full session at full rate plus a week of unsynced history plus wear-levelling headroom, and implement an explicit low-storage warning surfaced to the app well before the hard limit.
- Never allow a partially-written session record to be uploaded or marked complete; use a write-then-commit pattern with a checksum.

### 3.10 Radio (`ble/`)
- Keep BLE transmit bursts out of the EEG acquisition critical section where possible (already an antenna-placement/analog concern in hardware, but firmware must also avoid transmitting during a live stimulation session's sensitive sampling windows where feasible, and log actual TX timing so it can be correlated against EEG noise-floor QA later).
- Pairing requires mutual attestation: phone confirms device identity via secure-element-signed challenge before any session control commands are accepted; device requires an authenticated app before accepting session-start commands (never before accepting session-stop or STOP-adjacent commands).

---

## 4. On-device AI / signal processing models

Build the full pipeline: offline training (`models/training/`) → evaluation (`models/evaluation/`) → quantized export (`models/export/`) → on-device runtime (`firmware/core1_inference_radio/inference/`).

- **Task 1 — State classification.** Classify forehead EEG epochs (informed by literature-typical bands/features; do not hardcode clinical claims) as encoding-state, recall-state, or neither, using synchronized IMU data to reject motion-contaminated epochs *before* they reach the classifier, not just to weight them.
- **Task 2 — Phase-locked loop for stim timing.** Given a detected oscillation of interest, predict phase and schedule a stimulation burst request phase-locked to it, respecting the current/charge ceilings from §1 as hard constraints the model cannot exceed regardless of its output — clamp at the boundary between model output and firmware command, in firmware, not just in the model.
- **Task 3 — Closed-loop adaptation.** Use post-stimulation EEG response to adapt future timing/intensity within firmware-enforced bounds; log every adaptation decision with enough context to reconstruct it later (explainability for clinician review, not just accuracy).
- **On-device runtime constraints:** target Cortex-M33, must fit Core 1's time and memory budget, must never be able to stall Core 0 (§3.6). Quantize (int8 or comparable) and benchmark inference latency and memory footprint explicitly; fail the build if either exceeds the budget you define.
- **Training pipeline requirements:** versioned datasets, explicit train/val/test splits with subject-level (not epoch-level) separation to avoid leakage, artifact-robustness evaluation using the IMU-labeled contaminated epochs as an explicit test slice, and a model card generated per trained model recording data provenance, intended use, and known limitations.
- **Model registry:** every exported model is signed, versioned, and tagged with a firmware-compatibility range; the fleet-ota service (§6) must refuse to distribute a model/firmware combination outside that declared range.
- **No model output may ever be the sole gate for delivering current to a person** — reiterate and enforce this at the model → firmware API boundary with an explicit clamp/limiter component that is unit-tested independently of the model.

---

## 5. Companion mobile app

- **Platform:** build as a single cross-platform codebase (React Native, or Flutter if the team prefers — pick one and be consistent) covering iOS and Android, plus a caregiver-oriented simplified mode.
- **Pairing & provisioning:** BLE scan, mutual attestation (§3.10), first-run onboarding that explains the physical STOP control and session-state LEDs so the app never becomes the only way to understand device state — mirror, never replace, the on-device legibility from §3.7.
- **Session flow:** start/stop session (stop must always be immediately available and unambiguous in the UI, and must never be gated behind multi-step confirmation the way a destructive action might be — a stop request is never "destructive"), live session status mirroring the device LEDs, post-session summary.
- **Data & sync:** encrypted local cache, background sync of session data to cloud (§6) with clear user-visible sync status and offline queuing; explicit, separate consent toggles for (a) clinician data sharing and (b) de-identified research/analytics use — default both to off.
- **Firmware/model OTA:** user-initiated or scheduled update flow that surfaces what changed (firmware vs. model, safety-relevant or not) in plain language; never auto-apply a firmware update during an active or scheduled session window.
- **Caregiver mode:** at-a-glance adherence view (worn/not worn, from IMU-derived data — see dossier note that this is explicitly designed to answer "was it worn," not just "was it charged"), fault history in plain language, no clinical jargon required to understand "something needs attention."
- **Accessibility:** design for users who may have tremor, low vision, or be a tired caregiver at 10pm (direct callout from the mechanical dossier for electrode fitting — carry the same design ethos into the app: large touch targets, high contrast, screen-reader support, minimal steps for STOP-adjacent and fault-acknowledgment flows).

---

## 6. Cloud / backend / clinician portal

- **Ingestion:** device-authenticated (attestation-based, not password-based) session upload API; reject any payload without a valid device identity signature.
- **Storage:** encrypted at rest, explicit data retention policy configuration (not indefinite by default), separation between raw session data, derived analytics, and identity/PII stores so access can be scoped independently.
- **Clinician portal:** web app for reviewing session history, adherence, and adaptation logs (from §4's explainability logging) per patient, with role-based access control and a tamper-evident audit log (§ audit/) of every access and export — build this now, structured for eventual Part 11-style review, without claiming compliance.
- **Fleet OTA:** signed artifact distribution for firmware and models, staged rollout support (percentage/cohort based), automatic halt-on-fault-rate-spike logic (if a rollout cohort shows anomalous fault-log rates post-update, halt further rollout and alert — do not auto-rollback devices silently; surface it for human decision).
- **Analytics:** aggregate and de-identified only, driven strictly by the opt-in toggle in §5; no analytics pipeline may re-identify individual sessions.
- **Audit service:** append-only, tamper-evident log of data access, consent changes, OTA rollouts, and provisioning events referenced in §8.

---

## 7. Security & identity architecture (cross-cutting)

- Root of trust: secure element (U14) per device, one identity per unit, provisioned at manufacture (§8), never re-derivable off-device.
- All inter-component trust boundaries (device↔app, device↔cloud, app↔cloud) use mutual authentication; no shared static secrets in firmware or app binaries.
- Threat-model explicitly: a lost/stolen device, a compromised phone, a malicious BLE peer attempting to command stimulation, and a compromised cloud account attempting to push a malicious OTA — write these as concrete test cases, not just prose.
- "A device with open debug access is not an encrypted device" — carry this principle into CI: production build pipeline must fail if debug-enabled artifacts are tagged for the fleet-ota release channel.

---

## 8. Manufacturing/provisioning tooling (`tools/provisioning/`)

- Key ceremony client that talks to U14 during manufacture, records one identity per unit, and writes the resulting record to the same system that will later store each unit's HiPot pass/fail (§1) and swell-gap check record (mirrors the dossier's three ship-blocking manufacturing gates: shield-fence electrical test, HiPot, swell-gap check).
- Firmware's §3.2 refusal-to-arm-without-valid-record logic and this tool's record-writing must be built and tested together — treat "a unit missing any of the three records is not a built unit" as a system-level invariant with an automated test, not just a process note.

---

## 9. Testing, verification, and traceability

- **Hardware-in-the-loop / software-in-the-loop simulators (`firmware/sim/`):** a resistive head-phantom model for closed-loop testing without a person; fault injectors for each of the seven interlock conditions independently and in combination; a "make-before-break mux" fault injector even though the real part is break-before-make, to prove firmware detects the anomalous readback if it ever occurred.
- **Traceability matrix:** every safety-relevant requirement in this document (interlock behavior, charge balance, current ceiling, watchdog independence, STOP behavior, session-record gating) gets a stable ID, linked to the code module that implements it and the automated test that verifies it. Generate this as a living document in `firmware/docs/`.
- **CI gates:** no build reaches the fleet-ota-eligible artifact stage without passing the full interlock fault-injection suite, the charge-balance unit tests, the watchdog-independence stress test, and the debug-lockout check.
- **Explicit non-goal:** none of this testing constitutes IEC 60601-1 electrical safety testing, biocompatibility testing, or clinical validation. It verifies that the software does what this document says it should; it does not certify the device.

---

## 10. Documentation to produce alongside code (not just code)

- **IEC 62304-style artifacts** (`firmware/docs/`): software safety classification rationale (propose Class C given failure could contribute to serious injury; flag for real regulatory sign-off), software requirements linked to the traceability matrix, unit/integration test evidence.
- **ISO 14971-style hazard log scaffold** (`docs/risk-management/`): seed it with the hazards already named in the dossier — net-DC injury, over-current from lifted electrode, interlock defeat, undetected processor failure, OTA-introduced regression, data confidentiality breach — each with a placeholder for likelihood/severity/mitigation that a qualified risk reviewer must complete.
- **Design History File scaffold** (`docs/design-history-file/`): folder structure only, cross-referenced to where each artifact type will live once produced, not populated with unverified claims.

---

## 11. Open items Claude Code must treat as blocking or must explicitly flag

Do not silently guess past these. Where a task depends on one of these, implement it behind a clearly named, easily-changed configuration point and add a `TODO(OI-n)` referencing the item, rather than picking a silent default that looks final.

- **OI-1 — Electrode/channel count unresolved.** The parameter set assumes 2 stim + 4 EEG electrodes on two tails; the mechanical carrier drawing doesn't yet reconcile with that. **This directly changes firmware stimulation channel count and total delivered charge per session.** Build the stim/mux driver and channel configuration as data-driven (channel count and mapping as a provisioned parameter, not a compile-time constant), so the eventual clinical decision doesn't require a firmware rewrite — but do not invent a channel count; use the 2 stim + 4 EEG figure as the current placeholder and flag it.
- **OI-2 — Isolation barrier creepage/clearance are placeholders**, pending a qualified safety reviewer against IEC 60601-1. Software has no direct dependency, but the §3.2 provisioning-record gate exists partly because of this — do not weaken it in anticipation of this being resolved favorably.
- **OI-3 — Band fit margin is tight (+3/+5 mm).** No firmware dependency; note only.
- **OI-4 — Flex tail bend-cycle count not yet set.** No firmware dependency directly, but this is the highest field-failure-risk item in the device; ensure firmware's electrode-impedance monitoring (§3.3) logs enough detail (trend, not just threshold-crossing) to help diagnose a failing tail in the field versus a lifted electrode.
- **OI-5 — Nothing here is a fabrication release or has passed independent check.** Applies to the hardware dossier, not code, but reinforces that nothing in this codebase should claim device-level safety certification anywhere in its documentation strings, comments, or generated reports.

---

## 12. How Claude Code should actually work through this

1. Scaffold the repository structure in §2 first, with README stubs in every directory explaining its purpose and pointing back to the relevant section of this document.
2. Build firmware bottom-up per the ordering in §3 (power → acquisition → stim control → interlock interface → watchdog/cores → UI state → secure boot → session store → radio), writing the fault-injection and stress tests alongside each layer, not after.
3. Build the model pipeline (§4) against synthetic/simulated EEG data initially (clearly labeled as such) since no real session data exists yet for a device that hasn't been prototyped — do not fabricate or imply real clinical data.
4. Build the app (§5) and cloud (§6) against the firmware's BLE service contracts and cloud API contracts defined as part of §3.10/§6, keeping the trust/attestation model consistent end-to-end.
5. At every layer, if implementing a requirement would require weakening §0.1, stop, do not implement a workaround, and surface the conflict explicitly instead.
6. Keep everything in this repository; do not send device identifiers, keys, session data schemas, or this specification to third-party services, connectors, or external tools as part of implementing it — treat this program as confidential to the working repository.

---

## 13. Definition of done for a given milestone

A component is done when: it does what its section above specifies, it has passing tests for the specific safety/data-integrity behaviors called out (not just happy-path tests), it cannot — by construction — become the sole authority for delivering stimulation current, it references the correct open-item TODOs where applicable, and its documentation makes no unearned safety or regulatory claims.
