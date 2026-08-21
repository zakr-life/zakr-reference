"""
export_model.py — quantize/size-check the voiceprint verification
artifact against a small named latency/size budget, fail the build if
either is exceeded, and generate an honest model_card.md.

Mirrors models/export/export_model.py's pattern (int8 symmetric per-
tensor quantization, hand-rolled rather than a TFLite dependency this
pipeline doesn't have, budgets as named constants, a real gate that
`sys.exit(1)`s on failure). The "model" here is not a learned weight
matrix -- 1:1 voiceprint verification is cosine similarity against an
enrolled template (verify.py) -- so what gets quantized/size-checked is
the enrollment TEMPLATE itself (the only per-user artifact that would
ever need to live on-device or sync to cloud backup), and what gets
latency-benchmarked is the verification comparison
(quantize -> dequantize -> cosine similarity), i.e. the actual on-device
compute path, not the float32 generation-time path.

False-accept/false-reject rates are MEASURED on held-out synthetic
subjects (never seen during "enrollment" in the evaluation loop) at
verify.py's DEFAULT_VERIFICATION_THRESHOLD, and reported in the model
card exactly as measured -- not hand-tuned to hit a target number. This
is not a build gate (Addendum 2 does not specify a target FAR/FRR the
way it specifies a hallucination-rate target for §C), only latency and
size are gated, matching models/export/export_model.py's own gating
scope.
"""
import os
import sys
import time

sys.path.insert(0, os.path.dirname(__file__))
from synthetic_voice import generate_subjects, generate_utterances, FEATURE_DIM  # noqa: E402
from enrollment import enroll_template, REQUIRED_ENROLLMENT_UTTERANCES  # noqa: E402
from verify import cosine_similarity, DEFAULT_VERIFICATION_THRESHOLD  # noqa: E402

EXPORT_DIR = os.path.dirname(__file__)

# Budgets, named and small, mirroring models/export/export_model.py's
# discipline. A cosine similarity over a 16-d int8 vector is trivial
# compute and the artifact is a handful of bytes -- these are set with
# wide margin, not tuned to just barely pass.
MAX_VERIFY_LATENCY_MS = 2.0
MAX_TEMPLATE_SIZE_KB = 1.0

EVAL_NUM_SUBJECTS = 30
EVAL_UTTERANCES_PER_SUBJECT = 6
EVAL_SEED = 3003


def quantize_symmetric_int8(values):
    """Per-tensor symmetric int8 quantization -- same scheme as
    models/export/export_model.py's quantize_symmetric_int8, duplicated
    (not imported) because models/voiceprint/ is deliberately
    self-contained, the same way each firmware driver module is
    self-contained rather than reaching across unrelated directories."""
    max_abs = max((abs(v) for v in values), default=0.0)
    if max_abs == 0.0:
        return [0] * len(values), 1.0
    scale = max_abs / 127.0
    q = [max(-127, min(127, round(v / scale))) for v in values]
    return q, scale


def quantize_template(template):
    q_values, scale = quantize_symmetric_int8(template)
    return {"quantized_int8": q_values, "scale": scale, "dim": len(template)}


def dequantize_template(quantized):
    return [v * quantized["scale"] for v in quantized["quantized_int8"]]


def estimate_template_size_kb(quantized):
    n_values = len(quantized["quantized_int8"])
    size_bytes = (n_values * 1) + 4 + 16  # int8 values + float32 scale + small header slack
    return size_bytes / 1024.0


def benchmark_verify_latency(quantized_template, num_probes: int = 500) -> float:
    """Average latency, in ms, of the actual on-device comparison path:
    dequantize the stored template, then cosine-similarity against a
    probe vector. Measures the QUANTIZED path, not float32 generation."""
    import random
    rng = random.Random(77)
    dim = quantized_template["dim"]
    probes = [[rng.uniform(-2.5, 2.5) for _ in range(dim)] for _ in range(num_probes)]

    t0 = time.perf_counter()
    for probe in probes:
        template = dequantize_template(quantized_template)
        cosine_similarity(template, probe)
    elapsed_s = time.perf_counter() - t0

    return (elapsed_s / num_probes) * 1000.0


def evaluate_far_frr(threshold: float = DEFAULT_VERIFICATION_THRESHOLD) -> dict:
    """Held-out synthetic-subject evaluation of the QUANTIZED artifact
    (enroll -> quantize -> dequantize -> compare), mirroring
    models/export/export_model.py's "benchmark the shipped path, not the
    training-time path" discipline.

    For every subject: enroll from their first REQUIRED_ENROLLMENT_UTTERANCES
    utterances; every remaining utterance from that SAME subject is a
    genuine trial; one utterance from every OTHER subject is an impostor
    trial against this subject's template. Subject-disjoint by
    construction (a subject's own utterances are the only genuine trials
    for their own template) -- no leakage of one subject's data into
    another's enrollment, mirroring the state classifier's subject-level
    split discipline (models/training/split.py)."""
    subjects = generate_subjects(num_subjects=EVAL_NUM_SUBJECTS, seed=EVAL_SEED)
    utterances = generate_utterances(subjects, utterances_per_subject=EVAL_UTTERANCES_PER_SUBJECT,
                                      seed=EVAL_SEED + 1)

    by_subject = {}
    for u in utterances:
        by_subject.setdefault(u.subject_id, []).append(u.features)

    genuine_total = 0
    genuine_accepted = 0
    impostor_total = 0
    impostor_accepted = 0

    for subject_id, feats in by_subject.items():
        enrollment_feats = feats[:REQUIRED_ENROLLMENT_UTTERANCES]
        probe_feats = feats[REQUIRED_ENROLLMENT_UTTERANCES:]

        template = enroll_template(enrollment_feats)
        quantized = quantize_template(template)
        dequantized = dequantize_template(quantized)

        for probe in probe_feats:
            sim = cosine_similarity(dequantized, probe)
            genuine_total += 1
            if sim >= threshold:
                genuine_accepted += 1

        for other_subject_id, other_feats in by_subject.items():
            if other_subject_id == subject_id:
                continue
            impostor_probe = other_feats[0]
            sim = cosine_similarity(dequantized, impostor_probe)
            impostor_total += 1
            if sim >= threshold:
                impostor_accepted += 1

    far = impostor_accepted / impostor_total if impostor_total else float("nan")
    frr = 1.0 - (genuine_accepted / genuine_total) if genuine_total else float("nan")

    return {
        "threshold": threshold,
        "genuine_trials": genuine_total,
        "genuine_accepted": genuine_accepted,
        "impostor_trials": impostor_total,
        "impostor_accepted": impostor_accepted,
        "false_reject_rate": frr,
        "false_accept_rate": far,
        "num_subjects": EVAL_NUM_SUBJECTS,
    }


def generate_model_card(quantized: dict, latency_ms: float, size_kb: float,
                         eval_report: dict) -> str:
    return f"""# Model Card — nvband_voiceprint_verification

## Intended use
An OPTIONAL, opt-in, additive 1:1 identity-confirmation factor during
pairing/re-pairing or a high-risk app action (e.g. a remote-unlock
request from a new phone) — Addendum 2 §B. **1:1 verification against
one enrolled user's own template only; never 1:N identification or
surveillance against a population** — `verify.py`'s API has no function
that searches/ranks across multiple templates, by construction. Like
brainprint (Addendum 2 §A), this is necessary-but-never-solely-
sufficient: it augments, never replaces, secure-element attestation
(CLAUDE.md §7), and has no code path into stimulation, the interlock
chain, or session control (CLAUDE.md §0.1, Addendum 2 Rule 0).

## Data provenance
**Synthetic.** Generated by `synthetic_voice.py` directly in feature
space (subject-specific band-energy-like profile vectors + per-utterance
noise) — NOT synthesized waveforms/audio, and not a claim about real
acoustic-phonetic measurement. No real human voice data exists yet for
this device (it has not been prototyped). Every record carries
`"data_provenance": "synthetic"`.

## Enrollment / verification protocol
- Enrollment: exactly {REQUIRED_ENROLLMENT_UTTERANCES} synthetic
  utterances averaged (elementwise mean) into one template
  (`enrollment.py`), matching Addendum 2 §B exactly ("3 spoken utterances
  ... averaged into one template").
- Verification: cosine similarity between the (quantized/dequantized)
  template and a probe utterance's feature vector, compared against
  threshold {eval_report['threshold']} (`verify.py`'s
  `DEFAULT_VERIFICATION_THRESHOLD`) — a placeholder pending a dedicated
  threshold-tuning pass against real enrollment data once available, not
  a clinically or acoustically validated figure.

## Evaluation (held out, subject-disjoint)
Measured on the QUANTIZED artifact (the same path that would ship), over
{eval_report['num_subjects']} synthetic subjects never used to tune the
threshold:
- False reject rate (genuine speaker wrongly rejected):
  {eval_report['false_reject_rate']:.4f}
  ({eval_report['genuine_accepted']}/{eval_report['genuine_trials']}
  genuine trials accepted)
- False accept rate (impostor wrongly accepted):
  {eval_report['false_accept_rate']:.4f}
  ({eval_report['impostor_accepted']}/{eval_report['impostor_trials']}
  impostor trials accepted)

These are reported exactly as measured on this synthetic population, not
hand-tuned to hit a target — see the "Known limitations" section below
for why they should not be read as a real-world performance claim.

## On-device footprint (int8 quantized, symmetric per-tensor, per enrolled template)
- Estimated template size: {size_kb:.4f} KB (budget: {MAX_TEMPLATE_SIZE_KB} KB)
- Measured average verification latency: {latency_ms:.4f} ms/comparison
  (budget: {MAX_VERIFY_LATENCY_MS} ms, measured on the export
  workstation's Python quantized-path simulation, NOT on target
  Cortex-M33 hardware — an on-target benchmark is a firmware bring-up
  task, not yet run, same honest caveat as
  `models/export/model_card.md`)
- Quantization: {quantized['dim']}-dimensional int8 symmetric per-tensor

## Known limitations
- Trained/evaluated entirely on synthetic data generated directly in
  feature space; makes NO claim of real-world voice-verification
  performance. The measured FAR/FRR above describe separability of this
  synthetic feature-generation model, not human voices.
- No anti-spoofing / liveness detection claim (`TODO(OI-7)`, same as
  brainprint) — a recording or synthesized voice matching the enrolled
  template's feature statistics is not defended against by this module.
- Feature representation ("formant-like band-energy") is a structural
  stand-in, not a real acoustic feature-extraction pipeline — see
  `firmware/core1_inference_radio/audio/README.md`, "Not included in
  this pass," for what's genuinely missing before this could run on
  real audio.
- Template retention/consent handling is a technical capability, not a
  legal compliance determination — jurisdictional biometric-privacy
  review pending (`TODO(OI-9)`).
- No independent regulatory or clinical validation of this or any model
  in this repository (CLAUDE.md §9, §10, §12.3, OI-5).

## Firmware / cloud compatibility
The template shape this artifact produces
({quantized['dim']}-dimensional) must match
`cloud/biometrics/voiceprintTemplateStore.js`'s
`EXPECTED_TEMPLATE_LENGTH` exactly — that module structurally rejects
any payload of a different shape (including anything raw-audio-sized) as
not template-shaped.
"""


def main():
    subjects = generate_subjects(num_subjects=3, seed=9)
    utterances = generate_utterances(subjects, utterances_per_subject=3, seed=10)
    example_subject_id = next(iter(subjects))
    example_utterances = [u.features for u in utterances if u.subject_id == example_subject_id]
    example_template = enroll_template(example_utterances)

    quantized = quantize_template(example_template)
    size_kb = estimate_template_size_kb(quantized)
    latency_ms = benchmark_verify_latency(quantized)

    print(f"quantized template size: {size_kb:.4f} KB (budget {MAX_TEMPLATE_SIZE_KB} KB)")
    print(f"avg verification latency: {latency_ms:.4f} ms (budget {MAX_VERIFY_LATENCY_MS} ms)")

    failures = []
    if size_kb > MAX_TEMPLATE_SIZE_KB:
        failures.append(f"template size {size_kb:.4f} KB exceeds budget {MAX_TEMPLATE_SIZE_KB} KB")
    if latency_ms > MAX_VERIFY_LATENCY_MS:
        failures.append(f"latency {latency_ms:.4f} ms exceeds budget {MAX_VERIFY_LATENCY_MS} ms")

    eval_report = evaluate_far_frr()
    print(f"measured FAR: {eval_report['false_accept_rate']:.4f}  "
          f"FRR: {eval_report['false_reject_rate']:.4f} "
          f"(n_subjects={eval_report['num_subjects']})")

    card = generate_model_card(quantized, latency_ms, size_kb, eval_report)
    card_path = os.path.join(EXPORT_DIR, "model_card.md")
    with open(card_path, "w") as f:
        f.write(card)
    print(f"wrote {card_path}")

    if failures:
        print("\nEXPORT GATE FAILED:")
        for f_ in failures:
            print(f"  - {f_}")
        sys.exit(1)

    print("\nexport gate PASSED (latency and size within budget)")


if __name__ == "__main__":
    main()
