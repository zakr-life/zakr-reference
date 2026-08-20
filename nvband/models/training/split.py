"""
split.py — Subject-level (never epoch-level) train/val/test split.

CLAUDE.md §4: "explicit train/val/test splits with subject-level (not
epoch-level) separation to avoid leakage."

The critical property this module guarantees, and that
evaluation/evaluate.py independently re-verifies before scoring anything
(defense in depth, same philosophy as the firmware charge-balance
double-check): no subject_id appears in more than one of
{train, val, test}.
"""
import random
from typing import Dict, List, Sequence
from synthetic_eeg import SyntheticEpoch


def split_subjects(subject_ids: Sequence[str],
                    train_frac: float = 0.7,
                    val_frac: float = 0.15,
                    seed: int = 7) -> Dict[str, List[str]]:
    if not (0 < train_frac < 1 and 0 <= val_frac < 1 and train_frac + val_frac < 1):
        raise ValueError("invalid split fractions")

    subjects = sorted(set(subject_ids))
    rng = random.Random(seed)
    rng.shuffle(subjects)

    n = len(subjects)
    n_train = int(n * train_frac)
    n_val = int(n * val_frac)

    return {
        "train": subjects[:n_train],
        "val": subjects[n_train:n_train + n_val],
        "test": subjects[n_train + n_val:],
    }


def apply_split(epochs: Sequence[SyntheticEpoch],
                 split: Dict[str, List[str]]) -> Dict[str, List[SyntheticEpoch]]:
    subject_to_bucket = {}
    for bucket, subs in split.items():
        for s in subs:
            subject_to_bucket[s] = bucket

    result: Dict[str, List[SyntheticEpoch]] = {"train": [], "val": [], "test": []}
    for e in epochs:
        bucket = subject_to_bucket.get(e.subject_id)
        if bucket is None:
            raise ValueError(f"epoch references unknown subject {e.subject_id}")
        result[bucket].append(e)
    return result


def assert_no_subject_leakage(split: Dict[str, List[str]]) -> None:
    """Raises AssertionError if any subject appears in more than one
    bucket. Called by evaluation/evaluate.py before scoring, and by
    tests/test_split.py."""
    seen = {}
    for bucket, subs in split.items():
        for s in subs:
            if s in seen:
                raise AssertionError(
                    f"subject-level leakage: {s} appears in both "
                    f"'{seen[s]}' and '{bucket}'"
                )
            seen[s] = bucket
