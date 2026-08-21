"""
enrollment.py — Average N=3 synthetic utterance feature vectors into one
enrollment template.

Addendum 2 §B: "Enrollment: 3 spoken utterances of a fixed enrollment
phrase -> per-utterance feature vectors averaged into one template."
Exactly 3, per the addendum's own wording (not "at least 3") -- a caller
with more available utterances chooses which 3 to enroll with, this
function does not silently average more or fewer than were asked for.
"""
from typing import List

from synthetic_voice import FEATURE_DIM

REQUIRED_ENROLLMENT_UTTERANCES = 3


def enroll_template(utterance_feature_vectors: List[List[float]]) -> List[float]:
    """utterance_feature_vectors: exactly REQUIRED_ENROLLMENT_UTTERANCES
    vectors, each of length FEATURE_DIM. Returns the elementwise mean as
    the enrollment template. Raises ValueError on any shape mismatch
    rather than silently proceeding with the wrong number of utterances
    or a malformed vector."""
    if len(utterance_feature_vectors) != REQUIRED_ENROLLMENT_UTTERANCES:
        raise ValueError(
            f"enrollment requires exactly {REQUIRED_ENROLLMENT_UTTERANCES} utterances, "
            f"got {len(utterance_feature_vectors)}"
        )
    for v in utterance_feature_vectors:
        if len(v) != FEATURE_DIM:
            raise ValueError(f"utterance feature vector must have length {FEATURE_DIM}, got {len(v)}")

    template = []
    for i in range(FEATURE_DIM):
        total = sum(v[i] for v in utterance_feature_vectors)
        template.append(total / REQUIRED_ENROLLMENT_UTTERANCES)
    return template
