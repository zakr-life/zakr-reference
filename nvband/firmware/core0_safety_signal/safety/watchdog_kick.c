#include "watchdog_kick.h"
#include "../../shared/nvband_constants.h"
#include <string.h>

void nvband_watchdog_state_init(nvband_watchdog_state_t *state)
{
    if (state == NULL) {
        return;
    }
    memset(state, 0, sizeof(*state));
}

bool nvband_watchdog_tick(const nvband_watchdog_hal_t *hal,
                           nvband_watchdog_state_t *state,
                           uint64_t current_sample_count,
                           uint64_t now_us)
{
    if (hal == NULL || state == NULL) {
        return false;
    }

    bool sampling_advanced =
        current_sample_count > state->last_kicked_sample_count;

    uint64_t elapsed_us = (state->last_kick_time_us == 0)
                               ? 0
                               : (now_us - state->last_kick_time_us);
    bool within_deadline =
        (state->last_kick_time_us == 0) ||
        (elapsed_us <= (uint64_t)NVBAND_WATCHDOG_MAX_KICK_INTERVAL_MS * 1000u);

    if (!sampling_advanced) {
        /* Sampling loop has not produced a new sample: whether Core 0 is
         * merely slow, wedged, or Core 1 is somehow (incorrectly)
         * blocking it is irrelevant — we refuse to kick. This is the
         * entire safety property of this module. */
        state->missed_kick_count++;
        return false;
    }

    if (hal->pulse_kick_line != NULL) {
        hal->pulse_kick_line(hal->ctx);
    }
    state->last_kicked_sample_count = current_sample_count;
    state->last_kick_time_us = now_us;
    (void)within_deadline; /* diagnostic hook point; U9's own oscillator is
                               the real deadline enforcer, not this flag */
    return true;
}
