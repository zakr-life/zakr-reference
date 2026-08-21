/**
 * overnight_session_config.h — Configuration and output data types for
 * overnight sleep-monitoring sessions.
 *
 * Addendum 2 §E: "sleep monitoring is sensing, scoring, and reporting
 * only. It adds zero new stimulation authority. The sleep-stage
 * classifier's output type has no field that can reach the stim-command
 * hard clamp or the interlock chain — this is a structural fact about
 * the data type, not a policy that could be bypassed by a future
 * change."
 *
 * ---------------------------------------------------------------------
 * THIS MODULE HAS NO DEPENDENCY ON, AND MUST NEVER BE MODIFIED TO
 * INCLUDE, ANY STIMULATION-RELATED FIELD, FUNCTION, OR HEADER.
 *
 * In particular, this file and overnight_session_config.c must never
 * include or reference:
 *   - firmware/core1_inference_radio/inference/stim_command_clamp.h
 *   - firmware/core0_safety_signal/safety/interlock_status.h
 *   - firmware/core0_safety_signal/safety/charge_balance.h
 * nor gain any field naming a current, duration-of-burst, charge, or
 * enable/permit concept. `nvband_sleep_epoch_result_t` below carries a
 * stage and a confidence value and nothing else — that is a promise, not
 * just a description; see tests/test_overnight_session_config.c and
 * tests/check_no_stim_coupling.py for the mechanical checks that this
 * promise holds.
 * ---------------------------------------------------------------------
 *
 * This module is intentionally HAL-independent (no register access, no
 * RTOS calls) so it host-tests the same way epoch_budget.h and
 * stim_command_clamp.h do — see ../tests/ one level up for that
 * convention; this directory has its own tests/ for the same reason.
 * Reuses the existing EEG (U1) + IMU (U17) synchronous acquisition path
 * already used for waking sessions — no new hardware, no new driver.
 */
#ifndef NVBAND_OVERNIGHT_SESSION_CONFIG_H
#define NVBAND_OVERNIGHT_SESSION_CONFIG_H

#include <stdbool.h>
#include <stdint.h>

/** Five-stage sleep classification, Addendum 2 §E. Ordinal values are
 *  stable (used for confusion-matrix indexing on the model-training
 *  side) but carry no severity/ranking meaning — do not compare stages
 *  with < or > as if they formed a scale. */
typedef enum {
    NVBAND_SLEEP_STAGE_WAKE = 0,
    NVBAND_SLEEP_STAGE_N1   = 1,
    NVBAND_SLEEP_STAGE_N2   = 2,
    NVBAND_SLEEP_STAGE_N3   = 3,
    NVBAND_SLEEP_STAGE_REM  = 4,
} nvband_sleep_stage_t;

#define NVBAND_SLEEP_STAGE_COUNT 5u

/** Configuration for an overnight sensing session: a lower EEG sample
 *  rate than a waking closed-loop session needs (sleep staging works on
 *  30 s epochs of band power, not phase-locked timing) and an extended
 *  duration cap, since an overnight session runs for hours rather than
 *  the minutes a waking session runs for. No stimulation-related field
 *  belongs here, ever — see the file-level comment above. */
typedef struct {
    uint32_t eeg_sample_rate_hz;        /* lower than a waking session's rate */
    uint32_t epoch_duration_s;          /* conventional PSG epoch length: 30s */
    uint32_t session_max_duration_min;  /* extended vs. a waking session's cap */
    bool     imu_channel_active;        /* movement channel stays on for the
                                            whole overnight session */
} nvband_overnight_session_config_t;

/** Per-epoch sleep-stage classifier output. Deliberately minimal: a
 *  stage and a confidence value, and NOTHING ELSE. No current, no
 *  duration-of-burst, no charge, no enable/permit flag of any kind — see
 *  the file-level comment above for why that is a structural promise,
 *  not an oversight to "improve" later. */
typedef struct {
    nvband_sleep_stage_t stage;
    float                confidence;  /* [0.0, 1.0]; model confidence in `stage` */
} nvband_sleep_epoch_result_t;

/** Populate `out` with the current default overnight session
 *  configuration. Pure function, no I/O, no HAL access. */
void nvband_overnight_session_config_default(nvband_overnight_session_config_t *out);

/** True iff `cfg` is internally consistent (nonzero rates/durations,
 *  duration within a sane bound). Does not validate against any specific
 *  unit's provisioning record — this module has no concept of
 *  provisioning, unlike nvband_channel_config.h. */
bool nvband_overnight_session_config_is_valid(const nvband_overnight_session_config_t *cfg);

/** Number of epochs a full-duration overnight session is expected to
 *  produce, given `cfg`. Pure arithmetic (session_max_duration_min * 60
 *  / epoch_duration_s); returns 0 if `cfg` is not valid or is NULL. */
uint32_t nvband_overnight_session_expected_epoch_count(const nvband_overnight_session_config_t *cfg);

/** True iff `result` is internally consistent: `stage` is one of the
 *  five defined values and `confidence` is within [0.0, 1.0]. A NULL
 *  input is not valid. */
bool nvband_sleep_epoch_result_is_valid(const nvband_sleep_epoch_result_t *result);

#endif /* NVBAND_OVERNIGHT_SESSION_CONFIG_H */
