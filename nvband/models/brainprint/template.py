"""
template.py — Python reference implementation of the same enrollment-
template extraction algorithm as
firmware/core1_inference_radio/biometric/brainprint_template.c, used here
for pipeline-level evaluation (Addendum 2 §A). Kept algorithmically
consistent with the C module deliberately -- both exist because the C
version is what runs on-device (once wired into a real build) and this
Python version is what the evaluation pipeline uses, mirroring how the
rest of this repo keeps a Python reference alongside firmware C.

Pure stdlib only.
"""
from typing import List, Sequence

BANDS = ["delta", "theta", "alpha", "beta", "gamma"]
CHANNELS = 4
TEMPLATE_LEN = CHANNELS * len(BANDS)


def epoch_to_features(band_power: Sequence[Sequence[float]]) -> List[float]:
    """band_power: CHANNELS x len(BANDS) raw powers. Returns a flat
    TEMPLATE_LEN vector of per-channel ratios (each channel's bands sum to
    1.0). Raises ValueError on a flat/near-zero channel, matching the
    firmware's fail-closed behavior -- never fabricates a ratio."""
    if len(band_power) != CHANNELS:
        raise ValueError(f"expected {CHANNELS} channels, got {len(band_power)}")

    out: List[float] = []
    for ch in band_power:
        if len(ch) != len(BANDS):
            raise ValueError(f"expected {len(BANDS)} bands, got {len(ch)}")
        total = sum(ch)
        if any(v < 0 for v in ch):
            raise ValueError("negative band power")
        if total < 1e-9:
            raise ValueError("flat/disconnected channel: refusing to fabricate a ratio")
        out.extend(v / total for v in ch)
    return out


def build_template(epoch_features: Sequence[Sequence[float]],
                    min_epochs: int = 3, max_epochs: int = 16) -> List[float]:
    """Average N per-epoch feature vectors into one enrollment template."""
    n = len(epoch_features)
    if not (min_epochs <= n <= max_epochs):
        raise ValueError(f"epoch count {n} outside [{min_epochs}, {max_epochs}]")
    for f in epoch_features:
        if len(f) != TEMPLATE_LEN:
            raise ValueError(f"expected feature vector of length {TEMPLATE_LEN}")

    sums = [0.0] * TEMPLATE_LEN
    for f in epoch_features:
        for i, v in enumerate(f):
            sums[i] += v
    return [s / n for s in sums]
