#include "nvband_channel_config.h"
#include <string.h>

void nvband_channel_config_bench_default(nvband_channel_config_t *out)
{
    if (out == NULL) {
        return;
    }
    memset(out, 0, sizeof(*out));
    out->stim_channel_count = NVBAND_PLACEHOLDER_STIM_CHANNEL_COUNT;
    out->eeg_channel_count  = NVBAND_PLACEHOLDER_EEG_CHANNEL_COUNT;
    for (uint8_t i = 0; i < out->stim_channel_count; i++) {
        out->stim_mux_input[i] = i;
    }
    for (uint8_t i = 0; i < out->eeg_channel_count; i++) {
        out->eeg_mux_input[i] = i;
    }
    out->loaded_from_provisioning = false;
}

bool nvband_channel_config_is_valid(const nvband_channel_config_t *cfg)
{
    if (cfg == NULL) {
        return false;
    }
    if (cfg->stim_channel_count == 0 ||
        cfg->stim_channel_count > NVBAND_MAX_STIM_CHANNELS) {
        return false;
    }
    if (cfg->eeg_channel_count == 0 ||
        cfg->eeg_channel_count > NVBAND_MAX_EEG_CHANNELS) {
        return false;
    }
    return true;
}
