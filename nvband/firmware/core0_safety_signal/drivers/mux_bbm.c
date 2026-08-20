#include "mux_bbm.h"
#include "../../shared/nvband_constants.h"
#include <string.h>

void nvband_mux_state_init(nvband_mux_state_t *state)
{
    if (state == NULL) {
        return;
    }
    memset(state, 0, sizeof(*state));
}

void nvband_mux_switch_input(const nvband_mux_hal_t *hal,
                              nvband_mux_state_t *state,
                              uint8_t new_input)
{
    if (hal == NULL || state == NULL) {
        return;
    }

    /* Every switch — including error recovery — goes through: break (all
     * inputs off) -> mandatory minimum dwell -> make (select new input).
     * There is no code path that selects a new input without first
     * clearing the old one and waiting. */
    if (hal->set_all_inputs_off != NULL) {
        hal->set_all_inputs_off(hal->ctx);
    }
    state->input_selected = false;

    if (hal->wait_ns != NULL) {
        hal->wait_ns(NVBAND_MUX_BREAK_BEFORE_MAKE_MIN_NS, hal->ctx);
    }

    /* Anomaly check: during the break window, readback must show nothing
     * connected. If it doesn't (a make-before-break defect/wrong part),
     * latch the anomaly but still complete the switch — this driver has
     * no authority to halt acquisition or stimulation on its own; it can
     * only report. See CLAUDE.md §0.1 and interlock_status.h. */
    if (hal->any_input_connected != NULL && hal->any_input_connected(hal->ctx)) {
        state->break_anomaly_detected = true;
        state->break_anomaly_count++;
    }

    if (hal->select_input != NULL) {
        hal->select_input(new_input, hal->ctx);
    }
    state->current_input = new_input;
    state->input_selected = true;
    state->switch_count++;
}

bool nvband_mux_break_ok(const nvband_mux_state_t *state)
{
    if (state == NULL) {
        return false;
    }
    return !state->break_anomaly_detected;
}
