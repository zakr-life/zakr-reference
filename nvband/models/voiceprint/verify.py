"""
verify.py — 1:1 cosine-similarity voiceprint verification.

Addendum 2 §B: "1:1 verification against one enrolled user's own
template, never 1:N identification/surveillance against a population."

This module's entire API surface enforces that structurally, not just by
policy: every function here takes exactly ONE template vector to compare
against, never a collection/dict/list of enrolled templates to search or
rank over. There is deliberately no `identify_speaker(templates, probe)`
or similar function anywhere in this file, and none should ever be added
-- a 1:N search function is a different, out-of-scope product
(surveillance/identification), not what Addendum 2 §B authorizes.
"""
import math
from typing import List

from synthetic_voice import FEATURE_DIM

# Placeholder pending a dedicated threshold-tuning pass against real
# enrollment data once available -- see export_model.py, which measures
# (does not hand-tune) false-accept/false-reject rates at this threshold
# on held-out synthetic subjects and reports them honestly in
# model_card.md.
DEFAULT_VERIFICATION_THRESHOLD = 0.85


def cosine_similarity(a: List[float], b: List[float]) -> float:
    if len(a) != len(b):
        raise ValueError(f"vector length mismatch: {len(a)} vs {len(b)}")
    dot = sum(x * y for x, y in zip(a, b))
    norm_a = math.sqrt(sum(x * x for x in a))
    norm_b = math.sqrt(sum(y * y for y in b))
    if norm_a == 0.0 or norm_b == 0.0:
        return 0.0
    return dot / (norm_a * norm_b)


def verify_1to1(template: List[float], probe: List[float],
                 threshold: float = DEFAULT_VERIFICATION_THRESHOLD) -> dict:
    """Compares `probe` against exactly ONE claimed identity's
    `template`. This is the entire verification decision this module
    ever makes -- see module docstring for why there is no N-way search
    function alongside it.

    Like brainprint (Addendum 2 §A) and the master build prompt's
    attestation model, a voiceprint match is necessary-but-never-solely-
    sufficient: `match` here augments, never replaces, secure-element
    attestation (CLAUDE.md §7). Nothing in this module gates stimulation
    or session control by itself -- see models/voiceprint/README.md.
    """
    if len(template) != FEATURE_DIM or len(probe) != FEATURE_DIM:
        raise ValueError(f"template/probe must both have length {FEATURE_DIM}")

    similarity = cosine_similarity(template, probe)
    return {
        "similarity": similarity,
        "match": similarity >= threshold,
        "threshold": threshold,
    }
