"""
synthetic_eeg.py — Synthetic forehead-EEG + IMU epoch generator.

CLAUDE.md §12.3: "Build the model pipeline against synthetic/simulated
EEG data initially (clearly labeled as such) since no real session data
exists yet for a device that hasn't been prototyped — do not fabricate or
imply real clinical data."

Every record this module produces carries `"data_provenance": "synthetic"`
and this module's docstring is the single source of truth for that: NONE
of the numbers here are measured from a person. Band-power ranges are
loosely informed by publicly known EEG literature conventions (delta
0.5-4Hz, theta 4-8Hz, alpha 8-13Hz, beta 13-30Hz, gamma 30-40Hz) purely to
make the synthetic features *structured* enough to be a meaningful test of
the training/evaluation pipeline — this is explicitly NOT a clinical
claim about what encoding/recall states look like in real EEG.

Pure Python standard library only (no numpy) so this runs anywhere.

Dataset structure: subjects -> sessions -> epochs. Splits (see split.py)
are enforced at the SUBJECT level, never the epoch level, per CLAUDE.md
§4 ("explicit train/val/test splits with subject-level (not epoch-level)
separation to avoid leakage").
"""
import csv
import math
import random
from dataclasses import dataclass, asdict
from typing import List

LABELS = ["ENCODING", "RECALL", "NEITHER"]
BANDS = ["delta", "theta", "alpha", "beta", "gamma"]


@dataclass
class SyntheticEpoch:
    subject_id: str
    session_id: str
    epoch_id: int
    label: str
    data_provenance: str
    # band power per EEG channel (channel count from OI-1 placeholder: 4)
    ch0_delta: float; ch0_theta: float; ch0_alpha: float; ch0_beta: float; ch0_gamma: float
    ch1_delta: float; ch1_theta: float; ch1_alpha: float; ch1_beta: float; ch1_gamma: float
    ch2_delta: float; ch2_theta: float; ch2_alpha: float; ch2_beta: float; ch2_gamma: float
    ch3_delta: float; ch3_theta: float; ch3_alpha: float; ch3_beta: float; ch3_gamma: float
    imu_motion_magnitude: float   # synthetic accel-derived motion index
    motion_contaminated: bool     # ground-truth IMU-derived artifact flag


def _band_power_for_label(label: str, band: str, rng: random.Random) -> float:
    """Synthetic band power with a label-dependent bias, plus noise, so
    the classifier has *something* structured to learn without this being
    a claim about real neurophysiology (see module docstring)."""
    base = {"delta": 1.0, "theta": 1.0, "alpha": 1.0, "beta": 1.0, "gamma": 0.5}[band]

    bias = 0.0
    if label == "ENCODING" and band == "theta":
        bias = 0.6           # loosely: encoding literature often cites theta
    elif label == "RECALL" and band == "alpha":
        bias = 0.5           # loosely: recall literature often cites alpha
    elif label == "NEITHER":
        bias = 0.0

    noise = rng.gauss(0, 0.25)
    return max(0.0, base + bias + noise)


def generate_dataset(num_subjects: int = 30,
                      sessions_per_subject: int = 2,
                      epochs_per_session: int = 100,
                      contamination_rate: float = 0.12,
                      seed: int = 42) -> List[SyntheticEpoch]:
    rng = random.Random(seed)
    epochs: List[SyntheticEpoch] = []

    for s in range(num_subjects):
        subject_id = f"SYN-SUBJ-{s:03d}"
        # Each subject gets a small per-subject offset, simulating
        # inter-subject variability (a real, if synthetic, source of
        # difficulty a subject-level split is meant to guard against).
        subject_offset = rng.gauss(0, 0.15)

        for sess in range(sessions_per_subject):
            session_id = f"{subject_id}-SESS-{sess}"
            for e in range(epochs_per_session):
                label = LABELS[rng.randrange(len(LABELS))]
                contaminated = rng.random() < contamination_rate

                band_values = {}
                for ch in range(4):
                    for band in BANDS:
                        v = _band_power_for_label(label, band, rng) + subject_offset
                        if contaminated:
                            # Motion artifact inflates broadband power,
                            # especially low frequencies — a synthetic
                            # stand-in for real movement artifact.
                            v += rng.uniform(1.5, 4.0)
                        band_values[f"ch{ch}_{band}"] = round(max(0.0, v), 4)

                motion_mag = rng.uniform(2.5, 6.0) if contaminated else rng.uniform(0.0, 1.0)

                epochs.append(SyntheticEpoch(
                    subject_id=subject_id,
                    session_id=session_id,
                    epoch_id=e,
                    label=label,
                    data_provenance="synthetic",
                    imu_motion_magnitude=round(motion_mag, 4),
                    motion_contaminated=contaminated,
                    **band_values,
                ))

    return epochs


def write_csv(epochs: List[SyntheticEpoch], path: str) -> None:
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
    write_csv(ds, "models/training/data/synthetic_dataset.csv")
    print(f"wrote {len(ds)} synthetic epochs "
          f"({sum(1 for e in ds if e.motion_contaminated)} motion-contaminated) "
          "to models/training/data/synthetic_dataset.csv")
