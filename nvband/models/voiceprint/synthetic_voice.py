"""
synthetic_voice.py — Synthetic per-subject "utterance" feature vectors.

Addendum 2 §B ("Voiceprint"): "Enrollment: 3 spoken utterances of a fixed
enrollment phrase -> per-utterance feature vectors averaged into one
template." No real voice data exists for this device yet (it has not been
prototyped, mirroring the same honesty note as CLAUDE.md §12.3 for EEG) —
this module generates directly in FEATURE SPACE, never synthesized
waveforms/audio, exactly because the on-device feature extractor that
would turn raw PDM samples into this representation is not written in
this pass (see `firmware/core1_inference_radio/audio/README.md`, "Not
included in this pass"). Every record carries
`"data_provenance": "synthetic"`.

Feature representation: a fixed-length vector of subject-specific
formant-like band-energy features (loosely: "how much energy this
subject's voice tends to carry in each of a small number of frequency
bands," the same spirit as synthetic_eeg.py's per-channel band powers,
NOT a claim about real acoustic-phonetic measurement). Each subject gets
a fixed profile vector; each utterance is that profile plus independent
per-utterance noise, simulating natural utterance-to-utterance variation
in the same speaker.

Pure Python standard library only (no numpy), matching
models/training/synthetic_eeg.py.
"""
import csv
import random
from dataclasses import dataclass, asdict
from typing import List

FEATURE_DIM = 16
FEATURE_ORDER = [f"band_energy_{i}" for i in range(FEATURE_DIM)]

# Profile magnitude range: wide enough that different subjects' profiles
# are well-separated in cosine-similarity space (see
# models/voiceprint/export_model.py's measured FAR/FRR for the honest,
# empirical consequence of this choice -- not hand-tuned to a target).
PROFILE_RANGE = (-2.0, 2.0)
# Per-utterance noise standard deviation, small relative to profile
# magnitude -- models natural utterance-to-utterance variation from the
# SAME speaker (fatigue, mic distance, pronunciation) without swamping
# the subject-specific signal.
UTTERANCE_NOISE_SIGMA = 0.35


@dataclass
class SyntheticUtterance:
    subject_id: str
    utterance_id: int
    data_provenance: str
    features: List[float]


def _subject_profile(subject_id: str, rng: random.Random) -> List[float]:
    lo, hi = PROFILE_RANGE
    return [rng.uniform(lo, hi) for _ in range(FEATURE_DIM)]


def _utterance_from_profile(profile: List[float], rng: random.Random) -> List[float]:
    return [round(v + rng.gauss(0.0, UTTERANCE_NOISE_SIGMA), 6) for v in profile]


def generate_subjects(num_subjects: int = 30, seed: int = 42) -> dict:
    """Returns {subject_id: profile_vector}. Exposed separately from
    generate_utterances() so evaluation code can hold out entire subjects
    (never individual utterances) the same way models/training/split.py
    enforces subject-level splits for the EEG classifier."""
    rng = random.Random(seed)
    subjects = {}
    for s in range(num_subjects):
        subject_id = f"SYN-VOICE-{s:03d}"
        subjects[subject_id] = _subject_profile(subject_id, rng)
    return subjects


def generate_utterances(subjects: dict, utterances_per_subject: int = 6,
                         seed: int = 43) -> List[SyntheticUtterance]:
    """utterances_per_subject must be >= 3 so at least the first 3 can be
    used for enrollment (see enrollment.py) with the remainder available
    as genuine-trial probes for evaluation."""
    if utterances_per_subject < 3:
        raise ValueError("need at least 3 utterances per subject (enrollment requires exactly 3)")

    rng = random.Random(seed)
    utterances: List[SyntheticUtterance] = []
    for subject_id, profile in subjects.items():
        for u in range(utterances_per_subject):
            utterances.append(SyntheticUtterance(
                subject_id=subject_id,
                utterance_id=u,
                data_provenance="synthetic",
                features=_utterance_from_profile(profile, rng),
            ))
    return utterances


def write_csv(utterances: List[SyntheticUtterance], path: str) -> None:
    if not utterances:
        raise ValueError("refusing to write an empty dataset")
    with open(path, "w", newline="") as f:
        fieldnames = ["subject_id", "utterance_id", "data_provenance"] + FEATURE_ORDER
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        for u in utterances:
            row = {"subject_id": u.subject_id, "utterance_id": u.utterance_id,
                   "data_provenance": u.data_provenance}
            row.update({FEATURE_ORDER[i]: u.features[i] for i in range(FEATURE_DIM)})
            writer.writerow(row)


if __name__ == "__main__":
    import os
    subjects = generate_subjects()
    utterances = generate_utterances(subjects)
    out_path = os.path.join(os.path.dirname(__file__), "data", "synthetic_voice_dataset.csv")
    os.makedirs(os.path.dirname(out_path), exist_ok=True)
    write_csv(utterances, out_path)
    print(f"wrote {len(utterances)} synthetic utterances "
          f"({len(subjects)} subjects) to {out_path}")
