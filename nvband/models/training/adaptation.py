"""
adaptation.py — Task 3 (CLAUDE.md §4): closed-loop adaptation of future
timing/intensity from post-stimulation EEG response, within
firmware-enforced bounds, with explainability logging.

"log every adaptation decision with enough context to reconstruct it
later (explainability for clinician review, not just accuracy)."

Every adaptation decision produced here is an AdaptationDecision record
that stands on its own: a clinician (or this repo's own test suite)
should be able to look at ONE record and understand why the next
session's parameters differ from this one's, without needing to replay
history. The decision's numeric OUTPUT (proposed_current_delta_mA,
proposed_timing_delta_us) is still subject to the same firmware hard
clamp as Task 2's output (stim_command_clamp.c) — this module never
assumes its own bounds-checking is the last word.
"""
import time
from dataclasses import dataclass, asdict, field
from typing import List, Optional

# Soft, training/eval-side bounds — NOT the safety-authoritative bound
# (that's NVBAND_CURRENT_CEILING_MA / NVBAND_MAX_SINGLE_BURST_DURATION_US
# in firmware, applied downstream regardless of this module's output).
MAX_CURRENT_DELTA_PER_ADAPTATION_MA = 0.1
MAX_TIMING_DELTA_PER_ADAPTATION_US = 2000


@dataclass
class PostStimResponse:
    session_id: str
    burst_id: int
    pre_stim_band_power: float   # band power of interest, before burst
    post_stim_band_power: float  # same band, after burst
    timestamp_us: int


@dataclass
class AdaptationDecision:
    session_id: str
    burst_id: int
    timestamp_us: int
    response_delta: float                 # post - pre band power
    proposed_current_delta_mA: float
    proposed_timing_delta_us: int
    rationale: str                        # human-readable explanation
    clamped_current: bool
    clamped_timing: bool


def _clamp(value: float, limit: float) -> (float, bool):
    if value > limit:
        return limit, True
    if value < -limit:
        return -limit, True
    return value, False


def decide_adaptation(response: PostStimResponse,
                       learning_rate: float = 0.05) -> AdaptationDecision:
    """Simple proportional controller: if the post-stim response moved in
    the desired direction (band power increased, as a stand-in target —
    see module docstring re: no clinical claim), nudge current up
    slightly; if it moved the wrong way or not at all, nudge down. This
    is intentionally simple and fully explainable — a clinician reading
    `rationale` should be able to independently verify the arithmetic
    that produced `proposed_current_delta_mA`.
    """
    delta = response.post_stim_band_power - response.pre_stim_band_power

    raw_current_delta = learning_rate * delta
    current_delta, clamped_current = _clamp(
        raw_current_delta, MAX_CURRENT_DELTA_PER_ADAPTATION_MA)

    # Timing nudge: if response was flat or negative, try shifting timing
    # by a fixed probe step rather than only adjusting intensity (a
    # simple heuristic, not a claim of clinical optimality).
    raw_timing_delta = 0.0 if delta > 0 else 500.0
    timing_delta_f, clamped_timing = _clamp(
        raw_timing_delta, MAX_TIMING_DELTA_PER_ADAPTATION_US)
    timing_delta = int(timing_delta_f)

    rationale = (
        f"post-stim band power {response.post_stim_band_power:.4f} vs "
        f"pre-stim {response.pre_stim_band_power:.4f} "
        f"(delta={delta:+.4f}); proportional controller "
        f"(learning_rate={learning_rate}) proposes current_delta="
        f"{raw_current_delta:+.4f} mA"
        + (f" [CLAMPED to {current_delta:+.4f} mA at soft training-side "
           f"bound {MAX_CURRENT_DELTA_PER_ADAPTATION_MA} mA — firmware's "
           f"independent hard clamp still applies downstream regardless]"
           if clamped_current else "")
        + f", timing_delta={timing_delta:+d} us"
        + (" [CLAMPED at soft training-side bound]" if clamped_timing else "")
    )

    return AdaptationDecision(
        session_id=response.session_id,
        burst_id=response.burst_id,
        timestamp_us=response.timestamp_us,
        response_delta=delta,
        proposed_current_delta_mA=current_delta,
        proposed_timing_delta_us=timing_delta,
        rationale=rationale,
        clamped_current=clamped_current,
        clamped_timing=clamped_timing,
    )


class AdaptationLog:
    """In-memory explainability log. On-device, the equivalent structure
    is written into the session record (session_store.h) alongside the
    session so a clinician reviewing session history later can see every
    adaptation decision, not just the final parameters."""
    def __init__(self):
        self.decisions: List[AdaptationDecision] = []

    def record(self, decision: AdaptationDecision) -> None:
        self.decisions.append(decision)

    def to_list(self) -> List[dict]:
        return [asdict(d) for d in self.decisions]
