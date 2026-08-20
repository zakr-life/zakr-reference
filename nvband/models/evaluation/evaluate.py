"""
evaluate.py — Held-out evaluation, CLAUDE.md §4 / models/evaluation/.

Requirements enforced here, not just asserted in prose:
1. Re-verify subject-level split has no leakage (defense in depth: even
   though split.py enforces this at split time, evaluation independently
   re-checks before reporting any number, exactly like firmware's
   charge-balance double-check philosophy).
2. Report accuracy/confusion-matrix on the held-out CLEAN test set.
3. Report a SEPARATE artifact-robustness metric: how the model (trained
   only on clean epochs) performs on the motion-contaminated slice it
   never saw in training. This is expected to be worse — the point is to
   measure and record the gap, not to hide it (§10: no unearned claims).
"""
import json
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "training"))

from synthetic_eeg import generate_dataset  # noqa: E402
from split import apply_split, assert_no_subject_leakage  # noqa: E402
from features import (reject_motion_contaminated, epoch_to_feature_vector,  # noqa: E402
                       label_to_index, LABELS)
from softmax_classifier import SoftmaxClassifier  # noqa: E402

DATA_DIR = os.path.join(os.path.dirname(__file__), "..", "training", "data")
EXPORT_DIR = os.path.join(os.path.dirname(__file__), "..", "export")


def confusion_matrix(y_true, y_pred, num_classes):
    cm = [[0] * num_classes for _ in range(num_classes)]
    for t, p in zip(y_true, y_pred):
        cm[t][p] += 1
    return cm


def accuracy(y_true, y_pred):
    if not y_true:
        return None
    correct = sum(1 for t, p in zip(y_true, y_pred) if t == p)
    return correct / len(y_true)


def score(model, epochs):
    X = [epoch_to_feature_vector(e) for e in epochs]
    y = [label_to_index(e.label) for e in epochs]
    y_pred = [model.predict(x) for x in X]
    return y, y_pred


def main():
    with open(os.path.join(DATA_DIR, "split.json")) as f:
        split = json.load(f)

    # Requirement 1: re-verify no leakage, independent of split.py having
    # already enforced it at split time.
    assert_no_subject_leakage(split)
    print("subject-level leakage check: PASS (no subject in >1 bucket)")

    # Regenerate the same synthetic dataset deterministically (same seed
    # as training) so evaluation doesn't depend on a separately-persisted
    # raw dataset file staying in sync with the split.
    epochs = generate_dataset(num_subjects=24, sessions_per_subject=2,
                               epochs_per_session=60)
    buckets = apply_split(epochs, split)

    test_clean, test_contam = reject_motion_contaminated(buckets["test"])

    model = SoftmaxClassifier.load(
        os.path.join(EXPORT_DIR, "state_classifier_float.json"))

    y_true, y_pred = score(model, test_clean)
    acc = accuracy(y_true, y_pred)
    cm = confusion_matrix(y_true, y_pred, len(LABELS))

    print(f"\n== held-out CLEAN test set (n={len(test_clean)}) ==")
    print(f"accuracy: {acc:.4f}")
    print("confusion matrix (rows=true, cols=pred):")
    print("        " + " ".join(f"{l:>9s}" for l in LABELS))
    for i, row in enumerate(cm):
        print(f"{LABELS[i]:>8s} " + " ".join(f"{v:9d}" for v in row))

    # Requirement 3: artifact-robustness slice.
    robustness_result = {"n_contaminated": len(test_contam)}
    if test_contam:
        y_true_c, y_pred_c = score(model, test_contam)
        acc_c = accuracy(y_true_c, y_pred_c)
        robustness_result["accuracy_on_contaminated"] = acc_c
        print(f"\n== artifact-robustness slice (n={len(test_contam)}, "
              f"NEVER seen in training) ==")
        print(f"accuracy on motion-contaminated epochs: {acc_c:.4f} "
              f"(clean test accuracy: {acc:.4f}; gap={acc - acc_c:+.4f})")
        print("This gap is expected and is the reason firmware rejects "
              "motion-contaminated epochs before inference (§4 Task 1), "
              "not something this pipeline should try to hide or "
              "average away.")

    report = {
        "clean_test_accuracy": acc,
        "clean_test_n": len(test_clean),
        "confusion_matrix": cm,
        "labels": LABELS,
        "artifact_robustness": robustness_result,
        "subject_leakage_check": "PASS",
        "data_provenance": "synthetic",
    }
    report_path = os.path.join(os.path.dirname(__file__), "evaluation_report.json")
    with open(report_path, "w") as f:
        json.dump(report, f, indent=2)
    print(f"\nwrote {report_path}")


if __name__ == "__main__":
    main()
