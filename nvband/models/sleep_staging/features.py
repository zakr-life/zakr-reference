"""
features.py — Epoch -> feature-vector extraction and motion-artifact
rejection for sleep staging.

Mirrors `models/training/features.py`'s discipline exactly:
`reject_motion_contaminated` is the enforcement point, called before any
epoch's features are used to fit or score the classifier. Contaminated
epochs are returned separately (never discarded/lost) so evaluate.py can
score the trained model's behavior on them as an explicit robustness
slice, same as the state classifier's pipeline.

One deliberate difference from the state classifier's features.py: this
module's feature vector *includes* the IMU movement magnitude as a
feature (band power per channel + IMU movement features, per Addendum 2
§E), because movement level is itself informative for telling Wake/REM
apart from N2/N3 — unlike Task 1's state classifier, where IMU is used
only to reject contaminated epochs, never as a model input. Movement
*artifact* rejection (large motion events, e.g. an arm bump or electrode
adjustment) and movement *as a feature* (the normal, much smaller
stage-associated variation in restfulness) are different things — see
synthetic_overnight_eeg.py's contamination range (2.5-6.0) vs. its
stage-baseline movement ranges (0.0-3.0), which are kept disjoint by
construction so the two purposes don't collide in the synthetic data.
"""
from dataclasses import asdict
from typing import List, Tuple
from synthetic_overnight_eeg import OvernightEpoch, BANDS, STAGES, NUM_EEG_CHANNELS

BAND_FEATURE_ORDER = [f"ch{ch}_{band}" for ch in range(NUM_EEG_CHANNELS) for band in BANDS]
FEATURE_ORDER = BAND_FEATURE_ORDER + ["imu_motion_magnitude"]


def reject_motion_contaminated(
        epochs: List[OvernightEpoch]
) -> Tuple[List[OvernightEpoch], List[OvernightEpoch]]:
    clean = [e for e in epochs if not e.motion_contaminated]
    contaminated = [e for e in epochs if e.motion_contaminated]
    return clean, contaminated


def epoch_to_feature_vector(epoch: OvernightEpoch) -> List[float]:
    d = asdict(epoch)
    return [d[name] for name in FEATURE_ORDER]


def label_to_index(stage: str) -> int:
    return STAGES.index(stage)
