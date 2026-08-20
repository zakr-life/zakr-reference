#include "interlock_status.h"
#include <string.h>

void nvband_interlock_poll(const nvband_interlock_hal_t *hal,
                            uint64_t now_us,
                            nvband_interlock_snapshot_t *out)
{
    if (hal == NULL || out == NULL) {
        return;
    }
    memset(out, 0, sizeof(*out));
    out->last_poll_timestamp_us = now_us;

    if (hal->read_latch_asserted != NULL) {
        out->latch_asserted = hal->read_latch_asserted(hal->ctx);
    }

    if (hal->read_condition != NULL) {
        for (int i = 0; i < NVBAND_INTERLOCK_CONDITION_COUNT; i++) {
            bool satisfied = hal->read_condition(
                (nvband_interlock_condition_t)i, hal->ctx);
            out->condition_state[i] = satisfied;
            if (!satisfied) {
                out->tripped_mask |= (1u << i);
            }
        }
    }
}

bool nvband_interlock_all_clear(const nvband_interlock_snapshot_t *snap)
{
    if (snap == NULL) {
        return false;
    }
    return (!snap->latch_asserted) && (snap->tripped_mask == 0u);
}

nvband_rearm_result_t nvband_interlock_request_rearm(
    const nvband_interlock_hal_t *hal,
    const nvband_interlock_snapshot_t *current_snapshot)
{
    if (hal == NULL || current_snapshot == NULL) {
        return NVBAND_REARM_DENIED_CONDITIONS_UNMET;
    }
    if (!current_snapshot->latch_asserted) {
        return NVBAND_REARM_DENIED_NOT_LATCHED;
    }
    if (current_snapshot->tripped_mask != 0u) {
        /* A condition is still unmet: re-arming now would just re-latch
         * immediately, or worse, momentarily present a permit input while
         * the underlying condition is bad. Refuse — never auto-clear. */
        return NVBAND_REARM_DENIED_CONDITIONS_UNMET;
    }
    if (hal->pulse_rearm_line != NULL) {
        hal->pulse_rearm_line(hal->ctx);
    }
    return NVBAND_REARM_OK;
}
