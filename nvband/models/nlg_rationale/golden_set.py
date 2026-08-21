"""
golden_set.py — synthetic structured records for the hallucination-rate
CI gate (Addendum 2 §C: "a golden set of synthetic session/adaptation
records is run through generation + verification; the measured
ungrounded-claim rate on that set is reported by the test suite
honestly").

Every record here is SYNTHETIC (same discipline as
`models/training/synthetic_eeg.py` — no real patient data exists for
this device), generated deterministically from a fixed seed so the
measured rate `hallucination_gate.py` reports is reproducible across
runs, not a fresh random draw each time.

`build_golden_set()` returns a list of
`{"record_type": str, "data": dict, "note": str}` entries spanning both
record types in `record_schema.py`, deliberately including:
- both `AdaptationDecisionRecord` and `SessionSummaryRecord` types,
- both adaptation directions (response improved / worsened / flat),
- values that land exactly on the soft-clamp boundary, both clamped and
  unclamped,
- the `session_summary` optional field (`adherence_pct`) both present
  and absent,
- boundary confidence values (0.0 and 1.0),
- a zero-duration / zero-event session,
- a handful of INVALID records (missing a required field) — these are
  in the golden set on purpose, to exercise and demonstrate the
  "refuse to generate" path (see `hallucination_gate.py`'s report,
  which counts these separately as refusals, not as ungrounded
  sentences: a correct refusal is the safe behavior, not a failure).
"""
import random
from typing import Any, Dict, List

DEFAULT_GOLDEN_SET_SIZE = 200
DEFAULT_SEED = 20260218  # arbitrary, fixed — reproducibility matters more than the value


def _handcrafted_edge_cases() -> List[Dict[str, Any]]:
    entries: List[Dict[str, Any]] = [
        {
            "record_type": "adaptation_decision",
            "note": "flat response (delta exactly 0), current unclamped, timing probed",
            "data": {
                "session_id": "golden-edge-01", "burst_id": 1, "timestamp_us": 1000,
                "response_delta": 0.0, "proposed_current_delta_mA": 0.0,
                "proposed_timing_delta_us": 500, "clamped_current": False,
                "clamped_timing": False,
            },
        },
        {
            "record_type": "adaptation_decision",
            "note": "extreme positive response, current clamped at soft bound",
            "data": {
                "session_id": "golden-edge-02", "burst_id": 2, "timestamp_us": 2000,
                "response_delta": 1000.0, "proposed_current_delta_mA": 0.1,
                "proposed_timing_delta_us": 0, "clamped_current": True,
                "clamped_timing": False,
            },
        },
        {
            "record_type": "adaptation_decision",
            "note": "extreme negative response, timing clamped at soft bound",
            "data": {
                "session_id": "golden-edge-03", "burst_id": 3, "timestamp_us": 3000,
                "response_delta": -1000.0, "proposed_current_delta_mA": -0.1,
                "proposed_timing_delta_us": 2000, "clamped_current": True,
                "clamped_timing": True,
            },
        },
        {
            "record_type": "session_summary",
            "note": "adherence_pct present, boundary confidence 1.0",
            "data": {
                "session_id": "golden-edge-04", "start_timestamp_us": 4000,
                "duration_s": 1800.0, "num_stimulation_events": 40,
                "num_adaptations": 40, "avg_confidence": 1.0,
                "adherence_pct": 100.0,
            },
        },
        {
            "record_type": "session_summary",
            "note": "adherence_pct absent (tracking off), boundary confidence 0.0",
            "data": {
                "session_id": "golden-edge-05", "start_timestamp_us": 5000,
                "duration_s": 0.0, "num_stimulation_events": 0,
                "num_adaptations": 0, "avg_confidence": 0.0,
            },
        },
        {
            "record_type": "adaptation_decision",
            "note": "INVALID: missing required field 'response_delta' — must be refused",
            "data": {
                "session_id": "golden-edge-06", "burst_id": 6, "timestamp_us": 6000,
                "proposed_current_delta_mA": 0.01, "proposed_timing_delta_us": 0,
                "clamped_current": False, "clamped_timing": False,
            },
        },
        {
            "record_type": "session_summary",
            "note": "INVALID: missing required field 'avg_confidence' — must be refused",
            "data": {
                "session_id": "golden-edge-07", "start_timestamp_us": 7000,
                "duration_s": 600.0, "num_stimulation_events": 5,
                "num_adaptations": 5,
            },
        },
    ]
    return entries


def _random_adaptation_record(rng: random.Random, idx: int) -> Dict[str, Any]:
    delta = rng.uniform(-5.0, 5.0)
    raw_current = 0.05 * delta
    current, clamped_current = (
        (0.1, True) if raw_current > 0.1 else
        (-0.1, True) if raw_current < -0.1 else
        (raw_current, False)
    )
    raw_timing = 0.0 if delta > 0 else 500.0
    timing, clamped_timing = (
        (2000, True) if raw_timing > 2000 else (int(raw_timing), False)
    )
    return {
        "session_id": f"golden-rand-adapt-{idx}",
        "burst_id": idx,
        "timestamp_us": idx * 1000,
        "response_delta": delta,
        "proposed_current_delta_mA": current,
        "proposed_timing_delta_us": timing,
        "clamped_current": clamped_current,
        "clamped_timing": clamped_timing,
    }


def _random_session_summary_record(rng: random.Random, idx: int) -> Dict[str, Any]:
    include_adherence = rng.random() > 0.5
    record = {
        "session_id": f"golden-rand-session-{idx}",
        "start_timestamp_us": idx * 60_000_000,
        "duration_s": rng.uniform(0.0, 3600.0),
        "num_stimulation_events": rng.randint(0, 100),
        "num_adaptations": rng.randint(0, 100),
        "avg_confidence": rng.uniform(0.0, 1.0),
    }
    if include_adherence:
        record["adherence_pct"] = rng.uniform(0.0, 100.0)
    return record


def build_golden_set(n: int = DEFAULT_GOLDEN_SET_SIZE,
                      seed: int = DEFAULT_SEED) -> List[Dict[str, Any]]:
    """Returns a deterministic list of golden-set entries. `n` is the
    TOTAL number of entries, including the handcrafted edge cases below
    (so `n` must be >= the number of handcrafted cases)."""
    edge_cases = _handcrafted_edge_cases()
    if n < len(edge_cases):
        raise ValueError(f"n={n} is smaller than the {len(edge_cases)} "
                          f"handcrafted edge cases this golden set requires")

    rng = random.Random(seed)
    entries: List[Dict[str, Any]] = list(edge_cases)

    remaining = n - len(edge_cases)
    for i in range(remaining):
        idx = i + 1
        if i % 2 == 0:
            entries.append({
                "record_type": "adaptation_decision",
                "note": "synthetic random adaptation record",
                "data": _random_adaptation_record(rng, idx),
            })
        else:
            entries.append({
                "record_type": "session_summary",
                "note": "synthetic random session-summary record",
                "data": _random_session_summary_record(rng, idx),
            })

    return entries
