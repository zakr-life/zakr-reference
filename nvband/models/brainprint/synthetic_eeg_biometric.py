"""
synthetic_eeg_biometric.py — synthetic per-subject resting-state EEG epoch
generator for the brainprint enrollment/verification pipeline.

Addendum 2 §A. Mirrors models/training/synthetic_eeg.py's discipline:
every record carries data_provenance="synthetic", pure stdlib only, no
claim about real neurophysiology. Unlike the state classifier's data (whose
band-power bias comes from a shared *task label*), each subject here gets
its own persistent per-channel band-power "signature" plus per-session
noise/drift, so subjects are genuinely separable in the synthetic feature
space -- this is what makes false-accept/false-reject measurement on this
data meaningful, though it is still synthetic and not a claim about real
EEG-biometric performance (see TODO(OI-7) in the model card).
"""
import csv
import random
from dataclasses import dataclass, asdict
from typing import List

BANDS = ["delta", "theta", "alpha", "beta", "gamma"]
CHANNELS = 4


@dataclass
class BiometricEpoch:
    subject_id: str
    session_id: str
    epoch_id: int
    data_provenance: str
    ch0_delta: float; ch0_theta: float; ch0_alpha: float; ch0_beta: float; ch0_gamma: float
    ch1_delta: float; ch1_theta: float; ch1_alpha: float; ch1_beta: float; ch1_gamma: float
    ch2_delta: float; ch2_theta: float; ch2_alpha: float; ch2_beta: float; ch2_gamma: float
    ch3_delta: float; ch3_theta: float; ch3_alpha: float; ch3_beta: float; ch3_gamma: float


def _subject_signature(rng: random.Random):
    """A persistent per-channel, per-band relative-power bias unique to
    one synthetic subject -- the "signature" the verifier is meant to
    recognize."""
    return [[rng.uniform(0.2, 3.0) for _ in BANDS] for _ in range(CHANNELS)]


def generate_dataset(num_subjects: int = 20,
                      sessions_per_subject: int = 3,
                      epochs_per_session: int = 8,
                      session_drift: float = 0.15,
                      epoch_noise: float = 0.10,
                      seed: int = 11) -> List[BiometricEpoch]:
    rng = random.Random(seed)
    epochs: List[BiometricEpoch] = []

    for s in range(num_subjects):
        subject_id = f"SYN-BP-{s:03d}"
        signature = _subject_signature(rng)

        for sess in range(sessions_per_subject):
            session_id = f"{subject_id}-SESS-{sess}"
            # Session-level drift: same subject, slightly different day.
            drift = [[rng.gauss(0, session_drift) for _ in BANDS] for _ in range(CHANNELS)]

            for e in range(epochs_per_session):
                band_values = {}
                for ch in range(CHANNELS):
                    for bi, band in enumerate(BANDS):
                        v = signature[ch][bi] + drift[ch][bi] + rng.gauss(0, epoch_noise)
                        band_values[f"ch{ch}_{band}"] = round(max(0.0, v), 4)

                epochs.append(BiometricEpoch(
                    subject_id=subject_id,
                    session_id=session_id,
                    epoch_id=e,
                    data_provenance="synthetic",
                    **band_values,
                ))

    return epochs


def write_csv(epochs: List[BiometricEpoch], path: str) -> None:
    if not epochs:
        raise ValueError("refusing to write an empty dataset")
    fieldnames = list(asdict(epochs[0]).keys())
    with open(path, "w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        for e in epochs:
            writer.writerow(asdict(e))


if __name__ == "__main__":
    ds = generate_dataset()
    write_csv(ds, "models/brainprint/data_synthetic_biometric.csv")
    print(f"wrote {len(ds)} synthetic biometric epochs to "
          "models/brainprint/data_synthetic_biometric.csv")
