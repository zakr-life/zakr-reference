/**
 * stim_command_clamp.h — Hard clamp between model output and the firmware
 * command sent toward the DAC.
 *
 * CLAUDE.md §4: "predict phase and schedule a stimulation burst request
 * phase-locked to it, respecting the current/charge ceilings ... as hard
 * constraints the model cannot exceed regardless of its output — clamp
 * at the boundary between model output and firmware command, in
 * firmware, not just in the model." and "No model output may ever be the
 * sole gate for delivering current to a person — reiterate and enforce
 * this at the model -> firmware API boundary with an explicit
 * clamp/limiter component that is unit-tested independently of the
 * model."
 *
 * This module is intentionally dumb: it knows nothing about inference,
 * phase-locking, or adaptation. It takes whatever a model proposed and
 * either passes it through unchanged or clamps it to the hard ceilings in
 * nvband_constants.h. A clamped command is also flagged so the caller can
 * log an adaptation-decision entry noting the clamp occurred (§4
 * explainability requirement). Output still has to separately pass
 * charge_balance validation before transmission — this clamp does not
 * replace that check, it runs before it.
 */
#ifndef NVBAND_STIM_COMMAND_CLAMP_H
#define NVBAND_STIM_COMMAND_CLAMP_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    float    requested_current_mA;    /* may be signed (bipolar)         */
    uint32_t requested_duration_us;
    uint64_t requested_start_time_us; /* scheduled burst start           */
} nvband_stim_command_t;

typedef struct {
    nvband_stim_command_t clamped;
    bool current_was_clamped;
    bool duration_was_clamped;
} nvband_stim_clamp_result_t;

/** Maximum single-burst duration, independent of any model-tunable
 *  parameter — a defense-in-depth ceiling matching the average current
 *  budget so a single burst cannot alone exhaust the daily budget. */
#define NVBAND_MAX_SINGLE_BURST_DURATION_US (5u * 1000u * 1000u) /* 5 s */

/**
 * Clamp a model-proposed command to the hard ceilings. Pure function.
 * Never expands a request, only ever holds it at or below the ceiling.
 * A NULL input is treated as "no stimulation" (a zero-current, zero-
 * duration command), never passed through as-is.
 */
void nvband_stim_command_clamp(const nvband_stim_command_t *proposed,
                                nvband_stim_clamp_result_t *out);

#endif /* NVBAND_STIM_COMMAND_CLAMP_H */
