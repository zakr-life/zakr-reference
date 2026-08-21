"""
federated_round.py — orchestrates one simulated federated-learning round:
several "devices" each compute + clip a local delta, deltas are
federated-averaged, and the aggregate is applied to a global model to
produce a new CANDIDATE model artifact.

Addendum 2 §D. This module produces a candidate float model in the same
JSON shape as models/export/state_classifier_float.json -- it does NOT
re-implement signing or registry logic. The candidate this writes is meant
to then go through the EXISTING models/export/export_model.py and
models/versioning/registry.py pipeline unchanged, exactly like any other
trained model. See README.md.

Pure stdlib only.
"""
import copy
import json
import os
import sys
from typing import Dict, List

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "training"))
from synthetic_eeg import generate_dataset  # noqa: E402
from features import reject_motion_contaminated, epoch_to_feature_vector, label_to_index  # noqa: E402

from clipping import clip_delta, DEFAULT_MAX_DELTA_L2_NORM  # noqa: E402
from local_update import compute_local_delta, apply_delta  # noqa: E402


def federated_average(deltas: List[Dict]) -> Dict:
    """Element-wise mean of a list of already-clipped deltas. Refuses an
    empty list rather than returning a fabricated zero delta silently."""
    if not deltas:
        raise ValueError("cannot average zero deltas")

    num_classes = len(deltas[0]["weights"])
    num_features = len(deltas[0]["weights"][0])
    n = len(deltas)

    avg_weights = [[0.0] * num_features for _ in range(num_classes)]
    avg_bias = [0.0] * num_classes
    for d in deltas:
        for c in range(num_classes):
            for j in range(num_features):
                avg_weights[c][j] += d["weights"][c][j] / n
            avg_bias[c] += d["bias"][c] / n

    return {"weights": avg_weights, "bias": avg_bias}


def _simulate_device_batch(seed: int, num_epochs: int = 24):
    """One simulated device's local, post-session labeled batch. In a real
    device this would be that device's own recent session epochs; here it
    is a small synthetic slice, deliberately small (one "device") compared
    to the full training set."""
    epochs = generate_dataset(num_subjects=1, sessions_per_subject=1,
                               epochs_per_session=num_epochs, seed=seed)
    clean, _contaminated = reject_motion_contaminated(epochs)
    X = [epoch_to_feature_vector(e) for e in clean]
    y = [label_to_index(e.label) for e in clean]
    return X, y


def run_round(global_model: Dict, num_devices: int = 12,
              max_delta_l2_norm: float = DEFAULT_MAX_DELTA_L2_NORM,
              base_seed: int = 500) -> Dict:
    """Simulates `num_devices` devices each computing and clipping one
    local delta, averages them, and returns the new candidate model dict.
    Does not write anything -- callers write the artifact (see __main__)."""
    clipped_deltas = []
    for i in range(num_devices):
        X, y = _simulate_device_batch(seed=base_seed + i)
        if not X:
            continue  # a device whose local batch was entirely motion-contaminated contributes nothing
        raw_delta = compute_local_delta(global_model, X, y)
        clipped_deltas.append(clip_delta(raw_delta, max_delta_l2_norm))

    if not clipped_deltas:
        raise RuntimeError("no device produced a usable local delta this round")

    aggregate = federated_average(clipped_deltas)
    candidate = apply_delta(global_model, aggregate)
    return candidate


if __name__ == "__main__":
    with open("models/export/state_classifier_float.json") as f:
        global_model = json.load(f)

    candidate = run_round(global_model)

    out_path = "models/federated/candidate_state_classifier_float.json"
    with open(out_path, "w") as f:
        json.dump(candidate, f, indent=2)

    # Sanity report: how far the candidate moved from the starting model.
    from clipping import delta_l2_norm  # local import to avoid polluting module namespace
    moved = delta_l2_norm({
        "weights": [[candidate["weights"][c][j] - global_model["weights"][c][j]
                     for j in range(global_model["num_features"])]
                    for c in range(global_model["num_classes"])],
        "bias": [candidate["bias"][c] - global_model["bias"][c]
                 for c in range(global_model["num_classes"])],
    })
    print(f"wrote candidate model to {out_path} (L2 movement from global: {moved:.4f})")
