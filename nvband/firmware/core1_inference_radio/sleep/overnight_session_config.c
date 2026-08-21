/**
 * overnight_session_config.c — see overnight_session_config.h for the
 * file-level "no stimulation-related field, ever" comment; nothing here
 * changes that.
 */
#include "overnight_session_config.h"
#include <string.h>

/* Lower than a waking closed-loop session's EEG sample rate: sleep
 * staging classifies 30s epochs of band power, not phase-locked timing,
 * so it does not need the higher rate the phase-locked-loop path uses. */
#define NVBAND_OVERNIGHT_DEFAULT_EEG_SAMPLE_RATE_HZ 128u

/* Conventional polysomnography epoch length. */
#define NVBAND_OVERNIGHT_DEFAULT_EPOCH_DURATION_S 30u

/* Extended vs. a waking session's cap: an overnight session may run up
 * to 10 hours. */
#define NVBAND_OVERNIGHT_DEFAULT_MAX_DURATION_MIN (10u * 60u)

/* Upper sanity bound on session_max_duration_min: 24 hours. Anything
 * above this is not a plausible single overnight session and is treated
 * as invalid configuration, not clamped/silently accepted. */
#define NVBAND_OVERNIGHT_MAX_PLAUSIBLE_DURATION_MIN (24u * 60u)

void nvband_overnight_session_config_default(nvband_overnight_session_config_t *out)
{
    if (out == NULL) {
        return;
    }
    memset(out, 0, sizeof(*out));
    out->eeg_sample_rate_hz = NVBAND_OVERNIGHT_DEFAULT_EEG_SAMPLE_RATE_HZ;
    out->epoch_duration_s = NVBAND_OVERNIGHT_DEFAULT_EPOCH_DURATION_S;
    out->session_max_duration_min = NVBAND_OVERNIGHT_DEFAULT_MAX_DURATION_MIN;
    out->imu_channel_active = true;
}

bool nvband_overnight_session_config_is_valid(const nvband_overnight_session_config_t *cfg)
{
    if (cfg == NULL) {
        return false;
    }
    if (cfg->eeg_sample_rate_hz == 0u) {
        return false;
    }
    if (cfg->epoch_duration_s == 0u) {
        return false;
    }
    if (cfg->session_max_duration_min == 0u ||
        cfg->session_max_duration_min > NVBAND_OVERNIGHT_MAX_PLAUSIBLE_DURATION_MIN) {
        return false;
    }
    /* Sleep staging without a movement channel defeats the purpose of
     * this feature (Addendum 2 §E: band power + IMU movement features
     * together) -- treat that as an invalid configuration rather than a
     * silently degraded one. */
    if (!cfg->imu_channel_active) {
        return false;
    }
    return true;
}

uint32_t nvband_overnight_session_expected_epoch_count(const nvband_overnight_session_config_t *cfg)
{
    if (!nvband_overnight_session_config_is_valid(cfg)) {
        return 0u;
    }
    uint64_t total_seconds = (uint64_t)cfg->session_max_duration_min * 60u;
    return (uint32_t)(total_seconds / cfg->epoch_duration_s);
}

bool nvband_sleep_epoch_result_is_valid(const nvband_sleep_epoch_result_t *result)
{
    if (result == NULL) {
        return false;
    }
    if (result->stage > NVBAND_SLEEP_STAGE_REM) {
        return false;
    }
    if (result->confidence < 0.0f || result->confidence > 1.0f) {
        return false;
    }
    return true;
}
