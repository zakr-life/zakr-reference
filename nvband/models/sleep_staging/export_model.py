"""
export_model.py — int8 quantization + latency/memory benchmark + model
card generation for the sleep stage classifier, mirroring
`models/export/export_model.py`'s pattern (CLAUDE.md §4 discipline,
applied to Addendum 2 §E's classifier).

"Quantize (int8 or comparable) and benchmark inference latency and
memory footprint explicitly; fail the build if either exceeds the budget
you define." Budgets below are named constants, same convention as
`models/export/export_model.py`.

This produces a genuine int8-quantized artifact (linear per-tensor
quantization, symmetric) — not a TensorFlow/TFLite dependency; the
module structure (weights as a flat list of floats) is simple enough
that hand-rolled symmetric quantization is both correct and auditable.
"""
import json
import os
import sys
import time

sys.path.insert(0, os.path.dirname(__file__))
from softmax_classifier import SoftmaxClassifier  # noqa: E402
from features import FEATURE_ORDER  # noqa: E402
from synthetic_overnight_eeg import STAGES  # noqa: E402

SLEEP_STAGING_DIR = os.path.dirname(__file__)

# Budgets — mirror models/export/export_model.py's philosophy of setting
# this gate stricter than firmware's hard drop threshold (100 ms/epoch,
# see firmware/core1_inference_radio/inference/epoch_budget.h) so this
# gate catches a regression before it ever reaches firmware's defense-
# in-depth drop logic. Sleep staging runs far less often than the state
# classifier (once per 30s epoch overnight, not in the closed adaptation
# loop), but the same margin discipline applies.
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
        "labels": STAGES,
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


def compute_model_size_kb(quantized: dict) -> float:
    n_weights = len(quantized["quantized_weights_int8"])
    n_bias = len(quantized["quantized_bias_int8"])
    n_float_params = len(quantized["feature_mean"]) + len(quantized["feature_std"])
    size_bytes = (n_weights * 1) + (n_bias * 1) + (n_float_params * 4) + 64  # + header slack
    return size_bytes / 1024.0


def generate_model_card(quantized: dict, latency_ms: float, size_kb: float,
                         train_metadata: dict, eval_report: dict) -> str:
    return f"""# Model Card — {train_metadata['model_name']}

## Intended use
On-device classification of a synchronized overnight EEG+IMU epoch into
Wake / N1 / N2 / N3 / REM for the NV-Band's sleep-monitoring feature
(Addendum 2 §E). **This model has zero stimulation authority.** Its
output type (a stage label and a confidence score) has no field that can
reach `firmware/core1_inference_radio/inference/stim_command_clamp.c` or
any interlock/charge-balance code — that is a structural fact about the
data type this model produces, not a policy that could be bypassed by a
future change. CLAUDE.md §0.1 is completely unaffected by this model.
This is a **wellness/descriptive** classifier: it reports what stage the
data pattern-matches, for a plain-language app summary and a human-
reviewed clinician queue. It is NOT a diagnostic sleep-disorder
classifier and makes no claim to detect apnea or any other condition.

## Data provenance
**Synthetic.** Generated by `models/sleep_staging/synthetic_overnight_eeg.py`.
No real human overnight EEG/IMU data exists yet for this device (it has
not been prototyped) — see CLAUDE.md §12.3. Motion-contamination labels
are synthetic IMU-derived ground truth, not measured artifact.

## Training
- Subjects: {len(train_metadata['subject_split']['train'])} (train) /
  {len(train_metadata['subject_split']['val'])} (val) /
  {len(train_metadata['subject_split']['test'])} (test), split at the
  subject level (never epoch level) — see `models/sleep_staging/split.py`.
- Training epochs used: {train_metadata['num_train_epochs_used']}
  (motion-contaminated epochs rejected before reaching the classifier:
  {train_metadata['num_train_epochs_rejected_contaminated']} excluded).
- Final training loss: {train_metadata['final_training_loss']:.4f}

## Evaluation (held out, subject-disjoint from training)
- Clean test accuracy: {eval_report['clean_test_accuracy']:.4f}
  (n={eval_report['clean_test_n']}) — this is the number this pipeline
  actually measured; data generation was not hand-tuned to hit any
  target figure.
- Artifact-robustness slice (motion-contaminated, never trained on):
  accuracy {eval_report['artifact_robustness'].get('accuracy_on_contaminated', 'n/a')}
  (n={eval_report['artifact_robustness']['n_contaminated']}). Reported,
  not hidden, same as the state classifier's model card.
- Subject-level leakage check: {eval_report['subject_leakage_check']}

## On-device footprint (int8 quantized, symmetric per-tensor)
- Model size: {size_kb:.2f} KB (budget: {MAX_MODEL_SIZE_KB} KB)
- Average inference latency: {latency_ms:.3f} ms/epoch
  (budget: {MAX_INFERENCE_LATENCY_MS} ms, measured on the export
  workstation's Python quantized-path simulation, NOT on target
  Cortex-M33 hardware — an on-target benchmark is a firmware bring-up
  task, not yet run, same open item as the state classifier's)

## Known limitations
- **Trained entirely on synthetic data; no clinical validation of any
  kind.** Makes no claim of real-world diagnostic performance.
- **Wellness/descriptive use only.** This is explicitly NOT a diagnostic
  sleep-disorder classifier — it does not detect, screen for, or rule out
  apnea, insomnia, or any other sleep condition, and no code or copy
  anywhere in this repository claims otherwise (Addendum 2 §E).
- Linear (softmax) classifier — a deliberate pipeline-completeness
  choice, matching the state classifier's, not a claim this architecture
  is clinically sufficient.
- Channel count (4 EEG) matches OI-1's current placeholder figure, not a
  resolved mechanical/electrical decision — see CLAUDE.md §11.
- Sensing/scoring/reporting only: this classifier's output has zero
  coupling to stimulation control (Addendum 2 §E's "one rule"; see
  `tests/test_no_stimulation_coupling.py` for the mechanical proof).
- No independent regulatory or clinical validation of this or any model
  in this repository (CLAUDE.md §9, §10, §12.3, OI-5).

## Physician informing
Flagging a night for human review (`cloud/clinician-portal/sleepTrend.js`)
is a human-reviewed portal queue, never an automated real-time alert —
`TODO(OI-10)`. This model has no path to push/SMS/pager anything.
"""


def main():
    model = SoftmaxClassifier.load(
        os.path.join(SLEEP_STAGING_DIR, "sleep_classifier_float.json"))
    quantized = quantize_model(model)

    latency_ms = benchmark_latency(quantized)
    size_kb = compute_model_size_kb(quantized)

    print(f"quantized model size: {size_kb:.2f} KB (budget {MAX_MODEL_SIZE_KB} KB)")
    print(f"avg inference latency: {latency_ms:.3f} ms (budget {MAX_INFERENCE_LATENCY_MS} ms)")

    failures = []
    if size_kb > MAX_MODEL_SIZE_KB:
        failures.append(f"model size {size_kb:.2f} KB exceeds budget {MAX_MODEL_SIZE_KB} KB")
    if latency_ms > MAX_INFERENCE_LATENCY_MS:
        failures.append(f"latency {latency_ms:.3f} ms exceeds budget {MAX_INFERENCE_LATENCY_MS} ms")

    quantized_path = os.path.join(SLEEP_STAGING_DIR, "sleep_classifier_int8.json")
    with open(quantized_path, "w") as f:
        json.dump(quantized, f, indent=2)
    print(f"wrote {quantized_path}")

    train_meta_path = os.path.join(SLEEP_STAGING_DIR, "sleep_classifier_train_metadata.json")
    with open(train_meta_path) as f:
        train_metadata = json.load(f)

    eval_report_path = os.path.join(SLEEP_STAGING_DIR, "evaluation", "evaluation_report.json")
    with open(eval_report_path) as f:
        eval_report = json.load(f)

    card = generate_model_card(quantized, latency_ms, size_kb, train_metadata, eval_report)
    card_path = os.path.join(SLEEP_STAGING_DIR, "model_card.md")
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
