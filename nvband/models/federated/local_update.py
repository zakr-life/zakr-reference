"""
local_update.py — compute one local model update ("delta") for the
on-device state classifier from one device's local session data.

Addendum 2 §D: "After a session, the device computes a local update
(a gradient/delta) for the classifier from that session's locally-labeled
data." Dimensionally and algorithmically consistent with
models/training/softmax_classifier.py -- same weights/bias shape, same
feature order (see models/training/features.py).

Honesty note (Addendum 2 §D, and STATUS.md's existing discipline): this
file implements and tests the ALGORITHM at the pipeline level, exactly as
the base classifier's training already does before any on-device runtime
integration. Wiring actual on-device gradient computation into Core 1
firmware is a firmware bring-up task not done in this pass.

Pure stdlib only.
"""
import copy
import os
import sys
from typing import Dict, List, Tuple

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "training"))
from softmax_classifier import SoftmaxClassifier  # noqa: E402


def compute_local_delta(global_model: Dict, X: List[List[float]], y: List[int],
                         learning_rate: float = 0.05, epochs: int = 1,
                         seed: int = 0) -> Dict:
    """Starting from `global_model` (a SoftmaxClassifier.to_dict()-shaped
    dict), run a small number of local gradient-descent epochs on (X, y)
    and return the DELTA (new - old) for weights and bias -- never the
    full model, so the caller can clip/noise/transmit only the delta.

    Uses the SAME normalization (feature_mean/feature_std) as the global
    model, unchanged -- a local update must not silently redefine feature
    normalization, only nudge weights/bias.
    """
    if len(X) == 0:
        raise ValueError("refusing to compute a delta from an empty local batch")
    if len(X) != len(y):
        raise ValueError("X and y length mismatch")

    before = SoftmaxClassifier.from_dict(copy.deepcopy(global_model))
    after = SoftmaxClassifier.from_dict(copy.deepcopy(global_model))
    # Keep the global model's normalization fixed; only nudge weights/bias.
    after.feature_mean = list(before.feature_mean)
    after.feature_std = list(before.feature_std)

    Xn = [after._normalize(x) for x in X]  # noqa: SLF001 (internal reuse within the package)
    n = len(Xn)
    for _ in range(epochs):
        grad_w = [[0.0] * after.num_features for _ in range(after.num_classes)]
        grad_b = [0.0] * after.num_classes
        for x, target in zip(Xn, y):
            probs = after._softmax(after._logits(x))  # noqa: SLF001
            for c in range(after.num_classes):
                err = probs[c] - (1.0 if c == target else 0.0)
                grad_b[c] += err
                for j in range(after.num_features):
                    grad_w[c][j] += err * x[j]
        for c in range(after.num_classes):
            for j in range(after.num_features):
                after.weights[c][j] -= learning_rate * (grad_w[c][j] / n)
            after.bias[c] -= learning_rate * (grad_b[c] / n)

    delta = {
        "weights": [[after.weights[c][j] - before.weights[c][j]
                     for j in range(before.num_features)]
                    for c in range(before.num_classes)],
        "bias": [after.bias[c] - before.bias[c] for c in range(before.num_classes)],
    }
    return delta


def apply_delta(global_model: Dict, delta: Dict) -> Dict:
    """Applies a (typically already-aggregated) delta to a global model,
    returning a NEW model dict -- never mutates the input."""
    updated = copy.deepcopy(global_model)
    for c in range(updated["num_classes"]):
        for j in range(updated["num_features"]):
            updated["weights"][c][j] += delta["weights"][c][j]
        updated["bias"][c] += delta["bias"][c]
    return updated
