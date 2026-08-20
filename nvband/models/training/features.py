"""
features.py — Epoch -> feature-vector extraction and motion-artifact
rejection.

CLAUDE.md §4, Task 1: "using synchronized IMU data to reject motion-
contaminated epochs *before* they reach the classifier, not just to
weight them."

`reject_motion_contaminated` is the enforcement point: every training and
evaluation entry point in this pipeline calls it before any epoch's
band-power features are used to fit or score the classifier. Contaminated
epochs are returned separately (never discarded/lost) precisely so
evaluation/evaluate.py can score the trained model's behavior on them as
an explicit robustness slice (§4: "artifact-robustness evaluation using
the IMU-labeled contaminated epochs as an explicit test slice") — the
model never trains on them, but we still need to know what it does when
one gets through.
"""
from dataclasses import asdict
from typing import List, Tuple
from synthetic_eeg import SyntheticEpoch, BANDS

FEATURE_ORDER = [f"ch{ch}_{band}" for ch in range(4) for band in BANDS]
LABELS = ["ENCODING", "RECALL", "NEITHER"]


def reject_motion_contaminated(
        epochs: List[SyntheticEpoch]
) -> Tuple[List[SyntheticEpoch], List[SyntheticEpoch]]:
    clean = [e for e in epochs if not e.motion_contaminated]
    contaminated = [e for e in epochs if e.motion_contaminated]
    return clean, contaminated


def epoch_to_feature_vector(epoch: SyntheticEpoch) -> List[float]:
    d = asdict(epoch)
    return [d[name] for name in FEATURE_ORDER]


def label_to_index(label: str) -> int:
    return LABELS.index(label)
