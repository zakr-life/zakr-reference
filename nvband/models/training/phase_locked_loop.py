"""
phase_locked_loop.py — Task 2 (CLAUDE.md §4): given a detected
oscillation of interest, predict its phase and schedule a stimulation
burst request phase-locked to it.

This module produces a REQUEST only — a (proposed_current_mA,
duration_us, scheduled_start_time_us) tuple. It has no authority to send
anything toward hardware; the boundary CLAUDE.md §4 requires ("clamp at
the boundary between model output and firmware command, in firmware, not
just in the model") is implemented in firmware, not here — see
firmware/core1_inference_radio/inference/stim_command_clamp.c, which
firmware applies to whatever this module (or its on-device quantized
equivalent) proposes, regardless of what this module outputs. This
module's own soft ceiling is a courtesy for training/eval convenience,
not a safety control — the file's docstring on the firmware clamp is the
actual control.

Phase estimation here uses a simple Hilbert-transform-free zero-crossing
+ instantaneous-frequency estimator suitable for a narrowband oscillation
(e.g. a theta or alpha band-passed signal) — a lightweight, on-device-
appropriate approach rather than a full FFT-based method, consistent with
the Cortex-M33 budget constraint in CLAUDE.md §4.
"""
import math
from dataclasses import dataclass
from typing import List, Optional


@dataclass
class PhaseEstimate:
    instantaneous_phase_rad: float   # 0..2*pi
    instantaneous_freq_hz: float
    confidence: float                 # 0..1, based on cycle regularity


def estimate_phase(signal: List[float], sample_rate_hz: float) -> Optional[PhaseEstimate]:
    """signal: a short window of an already band-passed oscillation
    (band-passing itself is a firmware/DSP concern upstream of this
    function, not reimplemented here). Returns None if too few zero
    crossings are present to estimate confidently."""
    if len(signal) < 4 or sample_rate_hz <= 0:
        return None

    zero_crossings = []
    for i in range(1, len(signal)):
        if signal[i - 1] < 0 <= signal[i]:
            # Linear interpolation for sub-sample crossing time.
            frac = -signal[i - 1] / (signal[i] - signal[i - 1] + 1e-12)
            zero_crossings.append((i - 1 + frac) / sample_rate_hz)

    if len(zero_crossings) < 2:
        return None

    periods = [zero_crossings[i + 1] - zero_crossings[i]
               for i in range(len(zero_crossings) - 1)]
    mean_period = sum(periods) / len(periods)
    if mean_period <= 0:
        return None
    freq_hz = 1.0 / mean_period

    # Regularity-based confidence: low variance in period => high confidence.
    if len(periods) > 1:
        mean_p = mean_period
        var = sum((p - mean_p) ** 2 for p in periods) / len(periods)
        cv = math.sqrt(var) / mean_p if mean_p > 0 else 1.0
        confidence = max(0.0, min(1.0, 1.0 - cv))
    else:
        confidence = 0.5

    time_since_last_crossing = (len(signal) - 1) / sample_rate_hz - zero_crossings[-1]
    phase = (2 * math.pi * freq_hz * time_since_last_crossing) % (2 * math.pi)

    return PhaseEstimate(instantaneous_phase_rad=phase,
                          instantaneous_freq_hz=freq_hz,
                          confidence=confidence)


@dataclass
class StimRequest:
    proposed_current_mA: float
    duration_us: int
    scheduled_start_time_us: int
    target_phase_rad: float
    confidence: float


def schedule_phase_locked_burst(phase_estimate: PhaseEstimate,
                                 now_us: int,
                                 target_phase_rad: float = 0.0,
                                 requested_current_mA: float = 1.0,
                                 requested_duration_us: int = 500_000,
                                 min_confidence: float = 0.5) -> Optional[StimRequest]:
    """Schedules a burst request timed to land at target_phase_rad on the
    NEXT cycle. Returns None (no request) if confidence is too low — an
    uncertain phase estimate should produce no request at all, not a
    best-effort guess sent toward firmware."""
    if phase_estimate is None or phase_estimate.confidence < min_confidence:
        return None
    if phase_estimate.instantaneous_freq_hz <= 0:
        return None

    period_us = 1_000_000.0 / phase_estimate.instantaneous_freq_hz
    phase_delta = (target_phase_rad - phase_estimate.instantaneous_phase_rad) % (2 * math.pi)
    delay_us = int((phase_delta / (2 * math.pi)) * period_us)

    return StimRequest(
        proposed_current_mA=requested_current_mA,
        duration_us=requested_duration_us,
        scheduled_start_time_us=now_us + delay_us,
        target_phase_rad=target_phase_rad,
        confidence=phase_estimate.confidence,
    )
