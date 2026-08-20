# ZAKR NV-Band — Master Build Prompt (authoritative)

This file is the verbatim engineering specification this codebase is built
against. It was supplied by ZAKR Neurotechnology as the master build prompt
for the NV-Band closed-loop tACS/EEG device (design-intent dossier, BOM,
interface control, and tolerance schedule, August 2026 revision A).

**Every module in this repository must trace back to a section of this
document.** Where code deviates, extends, or flags an open item, it says so
in a comment referencing the section (e.g. `// see CLAUDE.md §3.5`) or an
open-item ID (`TODO(OI-n)`).

See `STATUS.md` (repo root, one level up) for what is implemented at what
fidelity in this pass, and `README.md` in this directory for the directory
map.

---

## 0. What this device is, in one paragraph

The NV-Band senses EEG from forehead electrodes, decides on-device whether
the wearer is in a memory-encoding or recall state, and — when it is —
delivers a low-intensity transcranial alternating current stimulation
(tACS) burst phase-locked to the ongoing oscillation, then measures what
happened and adapts. That loop is the product. All software, firmware, and
models exist to make that loop work, or to guarantee it cannot hurt anyone
when something fails.

## 0.1 The one rule that overrides every other instruction in this document

**Firmware may veto stimulation. Firmware may never be the sole authority
that permits it.** Stimulation ENABLE is a hardware AND of seven
independent conditions (current window, impedance window, electrode
temperature window, supply rails good, watchdog satisfied, physical STOP
switch closed, firmware permit asserted), implemented in discrete logic
(U8) that is not firmware-loadable, backstopped by a watchdog IC (U9) with
its own independent oscillator. **No code in this repository — firmware,
app, cloud, or test harness — may create a path that asserts stimulation
enable without all seven hardware conditions independently satisfied, may
simulate/spoof/bypass the interlock chain outside an explicitly labeled
bench-test build, or may leave a debug/bypass path reachable in a
production build.** Any bring-up bypass is a soldered link that
manufacturing physically removes and signs off — never a firmware flag. If
any task appears to require weakening this rule, the task stops and the
conflict is flagged instead of implemented.

Every layer — driver, RTOS task, BLE characteristic, app screen, cloud
endpoint — is built so that it is architecturally incapable of being the
single point of failure for stimulation safety.

---

(Sections 1–13 of the original master build prompt — program context,
repository layout, firmware build order, on-device AI/model pipeline,
companion app, cloud/backend, security architecture, manufacturing
provisioning, testing/traceability, documentation, open items, working
method, and definition of done — are reproduced in full in
`docs/ZAKR_NVBand_ClaudeCode_Master_Build_Prompt.md` and govern every
directory below. Read that file first.)

---

## Addendum 2 — Voiceprint, Brainprint, Federated Learning, Bounded-Rationale Generation, Sleep-State Reporting

`docs/ADDENDUM_2_biometric_federated_sleep.md` is a second authoritative
specification, at the same level as the master build prompt above, covering
five features added after the initial build: EEG-based local authentication
("brainprint"), microphone-based 1:1 voice verification ("voiceprint," BOM
addition U21), a constrained bounded-rationale text generator (target ≤0.5%
ungrounded-claim rate, precisely defined there), a federated-learning
pathway for the on-device classifier (only a clipped model delta ever
leaves the device), and sleep-state monitoring with human-reviewed
physician-facing reporting. **Rule 0 above is unchanged and applies to all
five** — none of them may create a new path to stimulation. Open items
OI-6 through OI-10 extend §11 of the master prompt. Read that addendum
before touching any of: `firmware/*/audio/`, `models/brainprint/`,
`models/voiceprint/`, `models/federated/`, `models/nlg_rationale/`,
`models/sleep_staging/`, or their app/cloud counterparts.
