"""
train_state_classifier.py — End-to-end training entry point for Task 1
(state classification: ENCODING / RECALL / NEITHER), CLAUDE.md §4.

Pipeline: generate synthetic dataset -> subject-level split -> reject
motion-contaminated epochs (before the classifier ever sees them) ->
train -> save weights + training metadata.

Run: python3 train_state_classifier.py
"""
import json
import os
import time

from synthetic_eeg import generate_dataset, write_csv
from split import split_subjects, apply_split, assert_no_subject_leakage
from features import reject_motion_contaminated, epoch_to_feature_vector, label_to_index, LABELS
from softmax_classifier import SoftmaxClassifier

DATA_DIR = os.path.join(os.path.dirname(__file__), "data")
OUT_DIR = os.path.join(os.path.dirname(__file__), "..", "export")


def main():
    os.makedirs(DATA_DIR, exist_ok=True)

    print("== generating synthetic dataset (data_provenance=synthetic) ==")
    epochs = generate_dataset(num_subjects=24, sessions_per_subject=2,
                               epochs_per_session=60)
    write_csv(epochs, os.path.join(DATA_DIR, "synthetic_dataset.csv"))
    print(f"{len(epochs)} epochs generated")

    subject_ids = [e.subject_id for e in epochs]
    split = split_subjects(subject_ids)
    assert_no_subject_leakage(split)
    print(f"subject split: train={len(split['train'])} "
          f"val={len(split['val'])} test={len(split['test'])}")

    buckets = apply_split(epochs, split)

    train_clean, train_contam = reject_motion_contaminated(buckets["train"])
    val_clean, val_contam = reject_motion_contaminated(buckets["val"])
    test_clean, test_contam = reject_motion_contaminated(buckets["test"])
    print(f"train: {len(train_clean)} clean / {len(train_contam)} rejected (contaminated)")
    print(f"val:   {len(val_clean)} clean / {len(val_contam)} rejected (contaminated)")
    print(f"test:  {len(test_clean)} clean / {len(test_contam)} rejected (contaminated)")

    X_train = [epoch_to_feature_vector(e) for e in train_clean]
    y_train = [label_to_index(e.label) for e in train_clean]

    model = SoftmaxClassifier(num_features=len(X_train[0]), num_classes=len(LABELS))

    t0 = time.time()
    loss_history = model.train(X_train, y_train, epochs=60)
    train_seconds = time.time() - t0
    print(f"trained in {train_seconds:.1f}s, final training loss={loss_history[-1]:.4f}")

    os.makedirs(OUT_DIR, exist_ok=True)
    model_path = os.path.join(OUT_DIR, "state_classifier_float.json")
    model.save(model_path)

    metadata = {
        "model_name": "nvband_state_classifier",
        "task": "state classification (ENCODING/RECALL/NEITHER)",
        "data_provenance": "synthetic",
        "num_train_epochs_used": len(train_clean),
        "num_train_epochs_rejected_contaminated": len(train_contam),
        "subject_split": split,
        "labels": LABELS,
        "final_training_loss": loss_history[-1],
        "train_seconds": train_seconds,
    }
    with open(os.path.join(OUT_DIR, "state_classifier_train_metadata.json"), "w") as f:
        json.dump(metadata, f, indent=2)

    # Persist the val/test buckets (by subject id + epoch id, not raw
    # features) so evaluation.py can reload exactly the same split.
    with open(os.path.join(DATA_DIR, "split.json"), "w") as f:
        json.dump(split, f, indent=2)

    print(f"saved model to {model_path}")


if __name__ == "__main__":
    main()
