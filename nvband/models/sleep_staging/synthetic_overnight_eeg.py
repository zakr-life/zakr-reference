"""
synthetic_overnight_eeg.py — Synthetic overnight forehead-EEG + IMU epoch
generator, one epoch per 30s (the conventional PSG epoch length), labeled
with a sleep stage (Wake / N1 / N2 / N3 / REM).

Addendum 2 §E: "sleep monitoring is sensing, scoring, and reporting only
... trained and evaluated with the same synthetic-data, subject-level-
split, honestly-reported-accuracy discipline as the existing state
classifier (CLAUDE.md §4)." This module is that discipline's data
generator, mirroring `models/training/synthetic_eeg.py`:

- Every record carries `data_provenance: "synthetic"` — NONE of the
  numbers here are measured from a person. Band-power ranges reuse the
  same 5-band convention (delta/theta/alpha/beta/gamma) as
  `models/training/synthetic_eeg.py`, loosely informed by public EEG
  literature conventions purely to give the pipeline something
  *structured* to learn — this is explicitly NOT a clinical claim about
  what any real sleep stage looks like in real EEG.
- Dataset structure: subjects -> nights (sessions) -> epochs. Splits
  (see split.py) are enforced at the SUBJECT level, never the epoch
  level, exactly like the state classifier's pipeline.

Stage-appropriate synthetic signal design (the part specific to sleep,
not shared with the state classifier):
- N3 (deep/slow-wave sleep) gets the largest delta-band bias and the
  lowest IMU movement — loosely, deep sleep is the quietest stage in both
  senses.
- Wake gets elevated beta/gamma and the highest baseline IMU movement.
- REM gets a theta/beta-ish signature and, deliberately, its movement is
  *bursty* rather than uniformly low: a fraction of REM epochs get a
  short movement blip ("REM-adjacent transition"), reflecting that REM
  and arousal-prone transitions are where movement is least predictable
  night to night — this is a synthetic modeling choice to give the
  motion-rejection pipeline something realistic to reject, not a
  physiological claim.

A simple Markov chain (WAKE -> N1 -> N2 -> N3 -> ... -> REM -> ...) drives
stage sequencing within a night so that adjacent epochs are correlated,
like a real hypnogram, rather than i.i.d. random labels — this also makes
the generated dataset meaningful input for `state/sleepReport.js`'s
time-in-stage and stage-timeline computation on the app side.

Pure Python standard library only (no numpy), same as the rest of this
pipeline, so it runs anywhere.
"""
import csv
import random
from dataclasses import dataclass, asdict
from typing import List

STAGES = ["WAKE", "N1", "N2", "N3", "REM"]
BANDS = ["delta", "theta", "alpha", "beta", "gamma"]
NUM_EEG_CHANNELS = 4  # matches OI-1 placeholder figure used elsewhere in this repo

# Stage-transition probabilities for a single 30s epoch. Deliberately
# simple (no full sleep-cycle-length modeling) — just enough
# autocorrelation that a night looks like a plausible hypnogram instead
# of shuffled noise. Rows sum to 1.0.
STAGE_TRANSITIONS = {
    "WAKE": {"WAKE": 0.60, "N1": 0.35, "N2": 0.03, "N3": 0.00, "REM": 0.02},
    "N1":   {"WAKE": 0.10, "N1": 0.40, "N2": 0.45, "N3": 0.00, "REM": 0.05},
    "N2":   {"WAKE": 0.02, "N1": 0.05, "N2": 0.60, "N3": 0.23, "REM": 0.10},
    "N3":   {"WAKE": 0.01, "N1": 0.02, "N2": 0.27, "N3": 0.65, "REM": 0.05},
    "REM":  {"WAKE": 0.05, "N1": 0.10, "N2": 0.25, "N3": 0.00, "REM": 0.60},
}


@dataclass
class OvernightEpoch:
    subject_id: str
    session_id: str          # one overnight recording ("night")
    epoch_id: int             # sequential 30s-epoch index within the night
    stage: str                 # ground-truth label: WAKE/N1/N2/N3/REM
    data_provenance: str
    # band power per EEG channel (channel count from OI-1 placeholder: 4)
    ch0_delta: float; ch0_theta: float; ch0_alpha: float; ch0_beta: float; ch0_gamma: float
    ch1_delta: float; ch1_theta: float; ch1_alpha: float; ch1_beta: float; ch1_gamma: float
    ch2_delta: float; ch2_theta: float; ch2_alpha: float; ch2_beta: float; ch2_gamma: float
    ch3_delta: float; ch3_theta: float; ch3_alpha: float; ch3_beta: float; ch3_gamma: float
    imu_motion_magnitude: float   # synthetic accel-derived motion index
    motion_contaminated: bool     # ground-truth IMU-derived artifact flag


def _next_stage(current: str, rng: random.Random) -> str:
    row = STAGE_TRANSITIONS[current]
    stages = list(row.keys())
    weights = list(row.values())
    return rng.choices(stages, weights=weights, k=1)[0]


def _generate_night_stage_sequence(num_epochs: int, rng: random.Random) -> List[str]:
    """One epoch-per-30s stage sequence for a single night, starting at
    WAKE (lights-off usually begins in a brief wake period), driven by
    STAGE_TRANSITIONS. This is what gives the dataset a plausible
    hypnogram shape instead of i.i.d.-random labels."""
    seq = ["WAKE"]
    for _ in range(num_epochs - 1):
        seq.append(_next_stage(seq[-1], rng))
    return seq


def _band_power_for_stage(stage: str, band: str, rng: random.Random) -> float:
    """Synthetic band power with a stage-dependent bias, plus noise —
    structured enough for the classifier pipeline to have something to
    learn, not a claim about real polysomnography (see module docstring).
    """
    base = {"delta": 1.0, "theta": 1.0, "alpha": 1.0, "beta": 1.0, "gamma": 0.5}[band]

    bias = 0.0
    if stage == "WAKE":
        if band == "beta":
            bias = 0.6
        elif band == "gamma":
            bias = 0.3
    elif stage == "N1":
        if band == "theta":
            bias = 0.2
        elif band == "alpha":
            bias = -0.3   # loosely: alpha attenuates as N1 begins
    elif stage == "N2":
        if band == "delta":
            bias = 0.4
        elif band == "theta":
            bias = 0.3
    elif stage == "N3":
        if band == "delta":
            bias = 1.0    # slow-wave sleep: the dominant synthetic signature
        elif band == "theta":
            bias = 0.2
    elif stage == "REM":
        if band == "theta":
            bias = 0.4
        elif band == "beta":
            bias = 0.2

    noise = rng.gauss(0, 0.25)
    return max(0.0, base + bias + noise)


def _movement_magnitude_for_stage(stage: str, rng: random.Random) -> float:
    """Baseline (non-contaminated) IMU movement magnitude for a stage.
    Wake is highest and most variable; N3 is lowest and steadiest; REM is
    low on average but occasionally bursty ("REM-adjacent transition"),
    which is the specific pattern this module is asked to represent."""
    if stage == "WAKE":
        return rng.uniform(1.0, 3.0)
    if stage == "N1":
        return rng.uniform(0.3, 1.0)
    if stage == "N2":
        return rng.uniform(0.05, 0.4)
    if stage == "N3":
        return rng.uniform(0.0, 0.2)
    if stage == "REM":
        if rng.random() < 0.15:  # REM-adjacent transition burst
            return rng.uniform(1.0, 2.5)
        return rng.uniform(0.1, 0.5)
    raise ValueError(f"unknown stage: {stage}")


def generate_dataset(num_subjects: int = 22,
                      nights_per_subject: int = 2,
                      epochs_per_night: int = 180,
                      contamination_rate: float = 0.08,
                      seed: int = 42) -> List[OvernightEpoch]:
    rng = random.Random(seed)
    epochs: List[OvernightEpoch] = []

    for s in range(num_subjects):
        subject_id = f"SYN-SLEEP-SUBJ-{s:03d}"
        # Per-subject offset simulating inter-subject variability, same
        # convention as models/training/synthetic_eeg.py.
        subject_offset = rng.gauss(0, 0.15)

        for night in range(nights_per_subject):
            session_id = f"{subject_id}-NIGHT-{night}"
            stage_seq = _generate_night_stage_sequence(epochs_per_night, rng)

            for e, stage in enumerate(stage_seq):
                contaminated = rng.random() < contamination_rate

                band_values = {}
                for ch in range(NUM_EEG_CHANNELS):
                    for band in BANDS:
                        v = _band_power_for_stage(stage, band, rng) + subject_offset
                        if contaminated:
                            # Motion artifact inflates broadband power,
                            # especially low frequencies — same synthetic
                            # convention as models/training/synthetic_eeg.py.
                            v += rng.uniform(1.5, 4.0)
                        band_values[f"ch{ch}_{band}"] = round(max(0.0, v), 4)

                if contaminated:
                    motion_mag = rng.uniform(2.5, 6.0)
                else:
                    motion_mag = _movement_magnitude_for_stage(stage, rng)

                epochs.append(OvernightEpoch(
                    subject_id=subject_id,
                    session_id=session_id,
                    epoch_id=e,
                    stage=stage,
                    data_provenance="synthetic",
                    imu_motion_magnitude=round(motion_mag, 4),
                    motion_contaminated=contaminated,
                    **band_values,
                ))

    return epochs


def write_csv(epochs: List[OvernightEpoch], path: str) -> None:
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
    write_csv(ds, "models/sleep_staging/data/synthetic_overnight_dataset.csv")
    print(f"wrote {len(ds)} synthetic overnight epochs "
          f"({sum(1 for e in ds if e.motion_contaminated)} motion-contaminated) "
          "to models/sleep_staging/data/synthetic_overnight_dataset.csv")
