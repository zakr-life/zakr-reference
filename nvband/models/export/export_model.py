"""
export_model.py — int8 quantization + latency/memory benchmark +
model card generation, CLAUDE.md §4.

"Quantize (int8 or comparable) and benchmark inference latency and
memory footprint explicitly; fail the build if either exceeds the budget
you define."

Budgets are defined here as named constants (mirroring how firmware
names its physical constants in nvband_constants.h — see
NVBAND_INFERENCE_EPOCH_BUDGET_US in
firmware/core1_inference_radio/inference/epoch_budget.h, which this
export step's latency budget is deliberately set below, with margin, so a
model that passes this gate cannot itself be the reason an epoch overruns
firmware's drop threshold).

This produces a genuine int8-quantized artifact (linear per-tensor
quantization, symmetric, matching what a TFLite-Micro int8 export would
produce in spirit) — not a TensorFlow/TFLite dependency, since this
pipeline has none installed and the module structure (weights as a flat
list of floats) is simple enough that hand-rolled symmetric quantization
is both correct and auditable.
"""
import json
import math
import os
import sys
import time

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "training"))
from softmax_classifier import SoftmaxClassifier  # noqa: E402
from features import FEATURE_ORDER, LABELS  # noqa: E402

EXPORT_DIR = os.path.dirname(__file__)

# Budgets — deliberately stricter than firmware's hard drop threshold
# (100 ms/epoch, see epoch_budget.h) so this gate catches a regression
# before it ever reaches firmware's defense-in-depth drop logic.
MAX_INFERENCE_LATENCY_MS = 20.0
MAX_MODEL_SIZE_KB = 64.0


def quantize_symmetric_int8(values):
    """Per-tensor symmetric int8 quantization. Returns (int8_values, scale)
    such that dequantized ~= int8_values * scale."""
    max_abs = max((abs(v) for v in values), default=0.0)
    if max_abs == 0.0:
        return [0] * len(values), 1.0
    scale = max_abs / 127.0
    q = [max(-127, min(127, round(v / scale))) for v in values]
    return q, scale


def quantize_model(model: SoftmaxClassifier) -> dict:
    flat_weights = [w for row in model.weights for w in row]
    q_weights, w_scale = quantize_symmetric_int8(flat_weights)
    q_bias, b_scale = quantize_symmetric_int8(model.bias)

    return {
        "num_features": model.num_features,
        "num_classes": model.num_classes,
        "weight_scale": w_scale,
        "quantized_weights_int8": q_weights,   # flat, row-major [class][feature]
        "bias_scale": b_scale,
        "quantized_bias_int8": q_bias,
        "feature_mean": model.feature_mean,    # normalization stays float32;
        "feature_std": model.feature_std,      # applied on-device before quantized matmul
        "feature_order": FEATURE_ORDER,
        "labels": LABELS,
        "quantization": "int8_symmetric_per_tensor",
    }


def dequantized_predict(quantized: dict, x):
    mean = quantized["feature_mean"]
    std = quantized["feature_std"]
    xn = [(x[j] - mean[j]) / std[j] for j in range(quantized["num_features"])]

    w_scale = quantized["weight_scale"]
    b_scale = quantized["bias_scale"]
    qw = quantized["quantized_weights_int8"]
    qb = quantized["quantized_bias_int8"]
    nf = quantized["num_features"]

    logits = []
    for c in range(quantized["num_classes"]):
        s = qb[c] * b_scale
        base = c * nf
        for j in range(nf):
            s += (qw[base + j] * w_scale) * xn[j]
        logits.append(s)
    return max(range(len(logits)), key=lambda i: logits[i])


def benchmark_latency(quantized: dict, num_samples: int = 200) -> float:
    """Returns average inference latency in milliseconds over synthetic
    inputs. This measures the QUANTIZED int8-simulated path (dequantize-
    on-the-fly, matching what the on-device runtime does), not the
    float32 training path."""
    import random
    rng = random.Random(123)
    nf = quantized["num_features"]
    samples = [[rng.uniform(0, 3) for _ in range(nf)] for _ in range(num_samples)]

    t0 = time.perf_counter()
    for x in samples:
        dequantized_predict(quantized, x)
    elapsed_s = time.perf_counter() - t0

    return (elapsed_s / num_samples) * 1000.0


def estimate_model_size_kb(quantized: dict) -> float:
    n_weights = len(quantized["quantized_weights_int8"])
    n_bias = len(quantized["quantized_bias_int8"])
    n_float_params = len(quantized["feature_mean"]) + len(quantized["feature_std"])
    size_bytes = (n_weights * 1) + (n_bias * 1) + (n_float_params * 4) + 64  # + header slack
    return size_bytes / 1024.0


def generate_model_card(quantized: dict, latency_ms: float, size_kb: float,
                         train_metadata: dict, eval_report: dict) -> str:
    return f"""# Model Card — {train_metadata['model_name']}

## Intended use
On-device classification of a synchronized EEG+IMU epoch into
ENCODING / RECALL / NEITHER for the NV-Band closed-loop tACS/EEG device
(CLAUDE.md §4, Task 1). Output feeds the phase-locked-loop stimulation
scheduler; per CLAUDE.md §4 this model's output is NEVER the sole gate
for delivering current — every downstream command passes through
firmware's independent hard clamp
(`firmware/core1_inference_radio/inference/stim_command_clamp.c`) and
charge-balance validator regardless of what this model outputs.

## Data provenance
**Synthetic.** Generated by `models/training/synthetic_eeg.py`. No real
human EEG data exists yet for this device (it has not been prototyped) —
see CLAUDE.md §12.3. Motion-contamination labels are synthetic IMU-derived
ground truth, not measured artifact.

## Training
- Subjects: {len(train_metadata['subject_split']['train'])} (train) /
  {len(train_metadata['subject_split']['val'])} (val) /
  {len(train_metadata['subject_split']['test'])} (test), split at the
  subject level (never epoch level) — see `models/training/split.py`.
- Training epochs used: {train_metadata['num_train_epochs_used']}
  (motion-contaminated epochs rejected before reaching the classifier:
  {train_metadata['num_train_epochs_rejected_contaminated']} excluded).
- Final training loss: {train_metadata['final_training_loss']:.4f}

## Evaluation (held out, subject-disjoint from training)
- Clean test accuracy: {eval_report['clean_test_accuracy']:.4f}
  (n={eval_report['clean_test_n']})
- Artifact-robustness slice (motion-contaminated, never trained on):
  accuracy {eval_report['artifact_robustness'].get('accuracy_on_contaminated', 'n/a')}
  (n={eval_report['artifact_robustness']['n_contaminated']}). This is
  expected to be materially worse than clean accuracy — it is reported,
  not hidden, and is the reason firmware rejects motion-contaminated
  epochs before they reach inference at all (defense in depth: model
  training AND firmware both refuse contaminated input).
- Subject-level leakage check: {eval_report['subject_leakage_check']}

## On-device footprint (int8 quantized, symmetric per-tensor)
- Estimated model size: {size_kb:.2f} KB (budget: {MAX_MODEL_SIZE_KB} KB)
- Measured average inference latency: {latency_ms:.3f} ms/epoch
  (budget: {MAX_INFERENCE_LATENCY_MS} ms, measured on the export
  workstation's Python quantized-path simulation, NOT on target
  Cortex-M33 hardware — an on-target benchmark is a firmware bring-up
  task, not yet run; see `firmware/docs/open-items`)

## Known limitations
- Trained entirely on synthetic data; makes no claim of real-world
  diagnostic or clinical performance.
- Linear (softmax) classifier — a deliberate pipeline-completeness
  choice (see `softmax_classifier.py` docstring), not a claim this
  architecture is clinically sufficient.
- Channel count (4 EEG) matches OI-1's current placeholder figure, not a
  resolved mechanical/electrical decision — see CLAUDE.md §11.
- No independent regulatory or clinical validation of this or any model
  in this repository (CLAUDE.md §9, §10, §12.3, OI-5).

## Firmware compatibility
See `models/versioning/registry.py` and the corresponding registry entry
for the signed firmware-compatibility range. `cloud/fleet-ota/` refuses
to distribute a model/firmware combination outside that declared range.
"""


def main():
    model = SoftmaxClassifier.load(os.path.join(EXPORT_DIR, "state_classifier_float.json"))
    quantized = quantize_model(model)

    latency_ms = benchmark_latency(quantized)
    size_kb = estimate_model_size_kb(quantized)

    print(f"quantized model size: {size_kb:.2f} KB (budget {MAX_MODEL_SIZE_KB} KB)")
    print(f"avg inference latency: {latency_ms:.3f} ms (budget {MAX_INFERENCE_LATENCY_MS} ms)")

    failures = []
    if size_kb > MAX_MODEL_SIZE_KB:
        failures.append(f"model size {size_kb:.2f} KB exceeds budget {MAX_MODEL_SIZE_KB} KB")
    if latency_ms > MAX_INFERENCE_LATENCY_MS:
        failures.append(f"latency {latency_ms:.3f} ms exceeds budget {MAX_INFERENCE_LATENCY_MS} ms")

    quantized_path = os.path.join(EXPORT_DIR, "state_classifier_int8.json")
    with open(quantized_path, "w") as f:
        json.dump(quantized, f, indent=2)
    print(f"wrote {quantized_path}")

    train_meta_path = os.path.join(EXPORT_DIR, "state_classifier_train_metadata.json")
    with open(train_meta_path) as f:
        train_metadata = json.load(f)

    eval_report_path = os.path.join(os.path.dirname(__file__), "..",
                                     "evaluation", "evaluation_report.json")
    with open(eval_report_path) as f:
        eval_report = json.load(f)

    card = generate_model_card(quantized, latency_ms, size_kb, train_metadata, eval_report)
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
