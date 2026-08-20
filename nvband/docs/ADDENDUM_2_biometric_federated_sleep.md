# ZAKR NV-Band — Addendum 2: Voiceprint, Brainprint, Federated Learning, Bounded-Rationale Generation, and Sleep-State Reporting

**Status: authoritative addendum to `CLAUDE.md` / the master build prompt.** This
document is to the five features below what the master build prompt is to the
rest of the repository: every new module traces back to a section here, and
every open engineering question is a named, tracked open item (`OI-6`
onward — `OI-1`..`OI-5` remain as defined in the master prompt).

This addendum was written in response to a request to add: (1) "brainprint"
detection tied to encryption/decryption, (2) "voiceprint" detection via a new
microphone, (3) a bounded hallucination rate (target ≤0.5%) for AI-generated
text, (4) federated learning so "only mathematical data leaves the device,"
and (5) physician informing based on sleep-state monitoring. All five are
real, scoped, and implemented against the plan below — none are stubbed out
as unavailable. Where a design choice was required that the request didn't
specify, this document makes the choice explicitly and states why, rather
than leaving it silently ambiguous.

**Rule 0, unchanged and non-negotiable:** every feature below is additive to
sensing, identity, reporting, or model-training. **None of them may create a
new path to stimulation, and none of them may weaken CLAUDE.md §0.1.** The
seven-condition hardware interlock, the firmware stim-command hard clamp, and
the "no model output is ever the sole gate for current delivery" rule are
unchanged by everything in this document. Where a feature's natural design
would have created such a path (e.g., using an EEG "brainprint" as
cryptographic key material, or letting a sleep-stage model influence
stimulation), this document explicitly designs it out and says so.

---

## A. "Brainprint" — EEG-based local authentication factor for decryption gating

**What it is:** an opt-in, additional local-authentication factor — not a
replacement for the secure-element root of trust (CLAUDE.md §7), and **not a
source of cryptographic key material.**

**Why not derive keys directly from EEG ("biometric key derivation"):** a
biometric signal has far lower usable entropy than a proper key, cannot be
rotated or revoked if compromised (you cannot re-enroll a different brain),
and drifts with electrode placement, fatigue, and physiological state. Using
it as literal key material is a known anti-pattern. Instead: **brainprint
match is a gate that must additionally pass before the app will use the
already-existing secure-element-derived key to decrypt the local session
cache** — architecturally identical to how a phone's Face ID gates the
existing keychain rather than becoming the encryption key itself.

**Enrollment:** during a dedicated enrollment session (explicit user action,
not passive), the device captures N (default 5) resting-state EEG epochs
over the existing acquisition path (no new hardware). A feature template
(relative band-power ratios per channel, normalized) is extracted and stored
locally, encrypted with the existing secure-element-derived key.

**Verification:** at unlock time, one live epoch is compared to the enrolled
template by cosine similarity against a configurable threshold.

**Hard requirement — no lockout:** brainprint match is *never* the only path
to a legitimate user's own data. A conventional fallback (device
PIN/passcode via the paired, attested app) is always available and is not
gated by brainprint. This is enforced as a testable invariant, not a UX
suggestion.

**Open items:**
- `TODO(OI-7)`: anti-spoofing / liveness detection for EEG biometrics is an
  active research area with no settled best practice; this module does not
  claim spoof-resistance and is documented as an additive convenience/local
  security factor pending a dedicated security review, exactly as OI-2
  (isolation barrier) is pending a qualified reviewer.

---

## B. "Voiceprint" — new microphone hardware, opt-in 1:1 speaker verification

**BOM addition:** `U21 — MEMS digital microphone`, PDM output (e.g.,
Infineon IM69D130-class), wired directly into the nRF5340's PDM peripheral
(Core 1) — no separate audio codec needed. Placement: TBD pending mechanical
review of the band — `TODO(OI-6)`, following the same "flag, don't silently
pick a final answer" discipline as OI-1's channel count.

**The one rule for this subsystem, mirroring §0.1's rigor:** *the microphone's
power/clock domain is gated by an explicit firmware state machine and can
only be enabled during a session-scoped, user-initiated voice-enrollment or
voice-verification request. There is no firmware, app, or cloud code path
that can power the microphone outside that explicit request window — no
"always listening," no wake-word, no ambient capture.* Raw audio is
processed in-memory for feature extraction and is discarded immediately
after; it is never written to the session store and never transmitted,
unless the user has separately and explicitly opted into cloud voiceprint
backup (a distinct, off-by-default consent toggle) — and even then, only the
fixed-length embedding vector may sync, never raw audio.

**What it is used for:** an optional secondary identity-confirmation factor
during pairing/re-pairing or a high-risk app action (e.g., a remote-unlock
request from a new phone) — **1:1 verification against one enrolled user's
own template, never 1:N identification/surveillance against a population.**
Like brainprint, it is necessary-but-never-solely-sufficient: it augments,
never replaces, the secure-element attestation in CLAUDE.md §7.

**Enrollment:** 3 spoken utterances of a fixed enrollment phrase → per-utterance
feature vectors averaged into one template.

**Open items:**
- `TODO(OI-6)`: microphone part/placement pending mechanical BOM reconciliation.
- `TODO(OI-9)`: voiceprint (and brainprint) templates are biometric data;
  jurisdictional biometric-privacy statutes (e.g., Illinois BIPA and
  similar) impose specific notice, consent, and retention/destruction
  obligations that this repository implements a technical *capability* for
  (explicit consent gate, defined retention, on-device-only default) but
  does not itself constitute legal compliance review — flagged for counsel
  review before this ships, exactly as OI-2/OI-5 flag hardware items
  pending a qualified reviewer.

---

## C. Bounded-rationale generation — target ≤0.5% ungrounded-claim rate

**Precise definition used throughout this codebase:** the existing
closed-loop adaptation logging (CLAUDE.md §4 Task 3) and a new
clinician-facing session-summary feature both produce natural-language text
for human readers. A **hallucination**, for this codebase, is defined as: *a
generated sentence that asserts a specific factual claim (a number, a
direction, a named event) that is not directly traceable to a field in the
structured record the text was generated from.*

**Why the architecture targets ≤0.5% by construction, not by hoping a large
model behaves:** this is **not** a free-generation large language model.
It is a constrained, slot-filling generator: every sentence is built from a
small fixed grammar whose slots are bound to named fields of the structured
adaptation/session record. Structurally, the generator cannot assert a
number it wasn't given. A second, independent **factuality verifier**
re-parses every generated sentence and cross-checks every asserted value
against the source record before the sentence is allowed to render —
defense in depth, the same philosophy as the stim-command hard clamp:
verify at the boundary, do not just trust the generator.

**Test harness:** a golden set of synthetic session/adaptation records is
run through generation + verification; the measured ungrounded-claim rate on
that set is reported by the test suite honestly, not asserted as a fixed
number in documentation. The CI gate fails the build if the measured rate on
the golden set exceeds the 0.5% target.

**Explicit non-claim:** this design achieves a low, verifiable ungrounded-claim
rate *because* generation is constrained and independently checked — it is
not evidence that hallucination in an unconstrained/general-purpose language
model has been "solved" or reduced to 0.5%, and no claim to that effect
should ever be made from this codebase.

---

## D. Federated learning — only a clipped model delta leaves the device

**What already existed (Addendum 1, unchanged):** de-identified, opt-in,
k-anonymity-gated *session-summary* analytics (`cloud/analytics/deidentify.js`).
That pathway is separate from, and unaffected by, this one.

**What is new:** a federated-learning pathway for improving the on-device
state classifier, gated by its own separate, off-by-default consent toggle
("federated model improvement"). After a session, the device computes a
**local update** (a gradient/delta) for the classifier from that session's
locally-labeled data. Before anything leaves the device:

1. The delta is **clipped** to a bounded L2 norm (defends against a single
   session producing an outsized, potentially model-poisoning update).
2. Calibrated noise may be added (a differential-privacy step) —
   `TODO(OI-8)`: the formal DP noise/epsilon budget is a statistics-and-policy
   decision pending a dedicated privacy review, not silently finalized here.
3. Only that clipped (and optionally noised) delta vector — a handful of
   floating-point numbers matching the classifier's tiny parameter count,
   never raw EEG, never a de-identified session record — is what may sync,
   and only over the existing attested device-identity channel.

**Cloud side — secure aggregation, minimum-cohort gated:** the aggregation
service accepts deltas only from attested devices, rejects any delta outside
plausible clipping bounds, and **never updates the shared global model from
fewer than a minimum cohort of devices in one aggregation round** — mirroring
the k-anonymity gate already used for analytics, so no aggregation round can
ever isolate one device's contribution. The resulting updated global model
re-enters the **existing** signed model registry and staged fleet-OTA
rollout (CLAUDE.md §6/§9) unchanged — this pathway produces a candidate
model update, it does not bypass any existing distribution safety gate.

**Honesty note on execution location:** the local-update *algorithm* is
implemented and tested at the pipeline level (mirroring exactly how the base
classifier's training algorithm already lives in `models/training/` as
tested Python before any on-device runtime integration). Wiring actual
on-device gradient computation into Core 1 firmware is a firmware bring-up
task not yet performed, in the same honest category as the existing
"TinyML runtime integration is not done" item in `STATUS.md` — this
addendum does not claim otherwise.

---

## E. Sleep-state monitoring and physician informing

**Hardware:** none needed — reuses the existing EEG (U1) + IMU (U17)
synchronous acquisition path in an overnight session mode.

**The one rule for this subsystem:** sleep monitoring is **sensing, scoring,
and reporting only.** It adds zero new stimulation authority. The sleep-stage
classifier's output type has no field that can reach the stim-command hard
clamp or the interlock chain — this is a structural fact about the data type,
not a policy that could be bypassed by a future change. CLAUDE.md §0.1 is
completely unaffected.

**Classifier:** a small, quantized classifier (Wake / N1 / N2 / N3 / REM)
from synchronized EEG band-power + IMU movement features — trained and
evaluated with the same synthetic-data, subject-level-split, honestly-reported-
accuracy discipline as the existing state classifier (CLAUDE.md §4).

**Reporting — deliberately wellness-framed, not diagnostic:** the app's
sleep report (stage timeline, time-in-stage, movement-based sleep
efficiency) is descriptive. It carries a persistent, non-dismissable-per-session
disclaimer and contains no diagnostic language (e.g., no apnea/disorder
naming) — enforced by an automated string-content test over the UI copy,
mirroring the existing "no clinical jargon in caregiver mode" discipline.
Diagnosing a sleep disorder is a materially higher regulatory bar than this
device has been built toward, and no code or copy in this repository claims
it.

**Physician informing:** the clinician portal gains a **Sleep Trend** view —
same RBAC + mandatory audit-log pairing as the existing session/adherence
review (CLAUDE.md §6). A clinician-configurable threshold can flag a night's
data for human review in the portal. This is a **human-reviewed queue, not
an automated real-time alert** to a physician (no push/SMS/pager) —
identical in spirit to the existing fleet-OTA "halt-on-fault-spike surfaces
to a human, never auto-acts" rule. `TODO(OI-10)`: real-time automated
clinical alerting is explicitly out of scope; it is a materially different,
higher-bar product (an alarm system) and is not designed or built here.

---

## Open items index (extends CLAUDE.md §11)

| ID | Item | Handling |
|---|---|---|
| OI-6 | Microphone part/placement not yet mechanically reconciled | Channel/part reference is a provisioned constant, not hardcoded |
| OI-7 | EEG/voice biometric anti-spoofing not solved by this design | Documented as additive-only, never sole gate; pending security review |
| OI-8 | Federated-learning DP noise/epsilon budget not finalized | Clipping is enforced now; noise parameter is a named, overridable constant pending privacy review |
| OI-9 | Biometric template retention under jurisdictional law (e.g., BIPA-style statutes) not legally reviewed | Consent gate, defined retention, and on-device-only default implemented; legal review pending |
| OI-10 | Automated real-time physician alerting | Explicitly out of scope; human-reviewed portal queue only |

## New hazards (extends the ISO 14971 scaffold)

| ID | Hazard | Primary mitigation |
|---|---|---|
| HAZ-09 | Unauthorized/ambient audio capture | Firmware mic-power state machine gated to explicit request window only; tested that no code path enables mic power outside it |
| HAZ-10 | Brainprint/voiceprint false-reject locks a legitimate user out | Conventional fallback authentication always available, tested as a hard invariant |
| HAZ-11 | Brainprint/voiceprint false-accept / spoofing | Additive-only factor, never sole gate (OI-7) |
| HAZ-12 | Model poisoning via malicious federated local update | Delta clipping, norm-bound rejection, attested submission only, minimum-cohort aggregation |
| HAZ-13 | Sleep report misread as a diagnostic claim | Mandatory disclaimer, no diagnostic language, automated copy test |
