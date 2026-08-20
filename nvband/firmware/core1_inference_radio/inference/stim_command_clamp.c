#include "stim_command_clamp.h"
#include "../../shared/nvband_constants.h"
#include <string.h>

void nvband_stim_command_clamp(const nvband_stim_command_t *proposed,
                                nvband_stim_clamp_result_t *out)
{
    if (out == NULL) {
        return;
    }
    memset(out, 0, sizeof(*out));

    if (proposed == NULL) {
        /* No input => no stimulation. Never invent a default nonzero
         * command. */
        return;
    }

    nvband_stim_command_t c = *proposed;

    float mag = c.requested_current_mA < 0 ? -c.requested_current_mA
                                            : c.requested_current_mA;
    if (mag > NVBAND_CURRENT_CEILING_MA) {
        float sign = c.requested_current_mA < 0 ? -1.0f : 1.0f;
        c.requested_current_mA = sign * NVBAND_CURRENT_CEILING_MA;
        out->current_was_clamped = true;
    }

    if (c.requested_duration_us > NVBAND_MAX_SINGLE_BURST_DURATION_US) {
        c.requested_duration_us = NVBAND_MAX_SINGLE_BURST_DURATION_US;
        out->duration_was_clamped = true;
    }

    out->clamped = c;
}
