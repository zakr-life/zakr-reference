"""
evaluate_verification.py — held-out 1:1 verification evaluation for
brainprint (Addendum 2 §A).

Measures false-reject rate (FRR: genuine subject wrongly rejected) and
false-accept rate (FAR: impostor wrongly accepted) at the default
threshold, on synthetic data, reporting whatever is actually measured --
not hand-tuned to hit a target number (per Addendum 2 §C's honesty
discipline, applied here too). Subject-level discipline: a subject's own
enrollment/verification split never mixes with another subject's data
except deliberately, for the impostor trials.

Pure stdlib only.
"""
import json
import math
import random
from collections import defaultdict
from typing import Dict, List

from synthetic_eeg_biometric import generate_dataset, BANDS, CHANNELS
from template import epoch_to_features, build_template, TEMPLATE_LEN

DEFAULT_THRESHOLD = 0.90  # matches firmware NVBAND_BRAINPRINT_DEFAULT_THRESHOLD


def cosine_similarity(a: List[float], b: List[float]) -> float:
    dot = sum(x * y for x, y in zip(a, b))
    mag_a = math.sqrt(sum(x * x for x in a))
    mag_b = math.sqrt(sum(y * y for y in b))
    if mag_a < 1e-12 or mag_b < 1e-12:
        return 0.0
    sim = dot / (mag_a * mag_b)
    return max(-1.0, min(1.0, sim))


def _epoch_band_power(row: Dict) -> List[List[float]]:
    return [[row[f"ch{ch}_{band}"] for band in BANDS] for ch in range(CHANNELS)]


def assert_no_subject_data_reused_across_enrollment_and_impostor_role(
        enrolled_subject: str, probe_subject: str) -> None:
    """Documents the one leakage rule that matters here: an impostor trial
    must never use the SAME subject's own epochs as both the claimed
    identity's enrollment and the probe."""
    if enrolled_subject == probe_subject:
        raise AssertionError("impostor trial must use a different subject's epochs")


def run_evaluation(threshold: float = DEFAULT_THRESHOLD, seed: int = 99) -> Dict:
    epochs = generate_dataset()
    by_subject: Dict[str, List[Dict]] = defaultdict(list)
    for e in epochs:
        by_subject[e.subject_id].append(e.__dict__)

    subjects = sorted(by_subject.keys())
    rng = random.Random(seed)

    templates: Dict[str, List[float]] = {}
    verification_epochs: Dict[str, List[List[float]]] = {}

    for subj in subjects:
        rows = sorted(by_subject[subj], key=lambda r: (r["session_id"], r["epoch_id"]))
        sessions = sorted(set(r["session_id"] for r in rows))
        enroll_session, verify_sessions = sessions[0], sessions[1:]

        enroll_rows = [r for r in rows if r["session_id"] == enroll_session]
        enroll_features = [epoch_to_features(_epoch_band_power(r)) for r in enroll_rows]
        templates[subj] = build_template(enroll_features)

        verify_rows = [r for r in rows if r["session_id"] in verify_sessions]
        verification_epochs[subj] = [epoch_to_features(_epoch_band_power(r)) for r in verify_rows]

    genuine_total = 0
    false_rejects = 0
    for subj in subjects:
        for feat in verification_epochs[subj]:
            genuine_total += 1
            if cosine_similarity(feat, templates[subj]) < threshold:
                false_rejects += 1

    impostor_total = 0
    false_accepts = 0
    for subj in subjects:
        others = [s for s in subjects if s != subj]
        # one impostor probe per genuine verification epoch, claimed
        # identity = subj, probe = a different subject's epoch
        for _ in verification_epochs[subj]:
            impostor_subj = rng.choice(others)
            assert_no_subject_data_reused_across_enrollment_and_impostor_role(subj, impostor_subj)
            probe_feat = rng.choice(verification_epochs[impostor_subj])
            impostor_total += 1
            if cosine_similarity(probe_feat, templates[subj]) >= threshold:
                false_accepts += 1

    result = {
        "data_provenance": "synthetic",
        "threshold": threshold,
        "num_subjects": len(subjects),
        "genuine_trials": genuine_total,
        "false_rejects": false_rejects,
        "false_reject_rate": round(false_rejects / genuine_total, 4) if genuine_total else None,
        "impostor_trials": impostor_total,
        "false_accepts": false_accepts,
        "false_accept_rate": round(false_accepts / impostor_total, 4) if impostor_total else None,
        "note": ("Measured on synthetic data only. Not a claim of real-world "
                 "biometric performance -- see TODO(OI-7)."),
    }
    return result


if __name__ == "__main__":
    result = run_evaluation()
    with open("models/brainprint/evaluation_report.json", "w") as f:
        json.dump(result, f, indent=2)
    print(json.dumps(result, indent=2))
