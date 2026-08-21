"""
clipping.py — bound a local model update before it may leave the device.

Addendum 2 §D: "The delta is clipped to a bounded L2 norm (defends against
a single session producing an outsized, potentially model-poisoning
update)." A second, optional noise step is a differential-privacy
*mechanism*, not a finalized privacy guarantee -- see TODO(OI-8).

A "delta" here is the dict shape SoftmaxClassifier.to_dict() produces for
the parts that can change: {"weights": [[...]], "bias": [...]}. Pure
stdlib only.
"""
import math
import random
from typing import Dict

DEFAULT_MAX_DELTA_L2_NORM = 1.0

# TODO(OI-8): this noise scale is a mechanism placeholder, not a reviewed
# differential-privacy epsilon budget. Do not treat this as a finalized
# privacy guarantee anywhere in documentation or product claims.
DEFAULT_NOISE_SCALE = 0.0  # off by default until OI-8 is resolved


def _flatten(delta: Dict) -> list:
    flat = []
    for row in delta["weights"]:
        flat.extend(row)
    flat.extend(delta["bias"])
    return flat


def _l2_norm(values: list) -> float:
    return math.sqrt(sum(v * v for v in values))


def delta_l2_norm(delta: Dict) -> float:
    return _l2_norm(_flatten(delta))


def clip_delta(delta: Dict, max_l2_norm: float = DEFAULT_MAX_DELTA_L2_NORM) -> Dict:
    """Scales the whole delta down uniformly if its L2 norm exceeds
    max_l2_norm; otherwise returns it unchanged (as a copy). Never scales
    UP -- a small delta is left alone."""
    if max_l2_norm <= 0:
        raise ValueError("max_l2_norm must be positive")

    norm = delta_l2_norm(delta)
    if norm <= max_l2_norm or norm == 0.0:
        scale = 1.0
    else:
        scale = max_l2_norm / norm

    return {
        "weights": [[v * scale for v in row] for row in delta["weights"]],
        "bias": [v * scale for v in delta["bias"]],
    }


def add_calibrated_noise(delta: Dict, noise_scale: float = DEFAULT_NOISE_SCALE,
                          seed: int = 0) -> Dict:
    """Adds i.i.d. Gaussian noise of the given scale to every element.
    noise_scale=0 is a no-op (returns an unchanged copy) -- the mechanism
    exists and is tested, but is not enabled by default pending OI-8."""
    if noise_scale < 0:
        raise ValueError("noise_scale must be non-negative")
    rng = random.Random(seed)

    def noisy(v):
        return v + (rng.gauss(0, noise_scale) if noise_scale > 0 else 0.0)

    return {
        "weights": [[noisy(v) for v in row] for row in delta["weights"]],
        "bias": [noisy(v) for v in delta["bias"]],
    }
