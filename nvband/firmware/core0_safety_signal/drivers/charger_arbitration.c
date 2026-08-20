#include "charger_arbitration.h"
#include <string.h>

void nvband_charger_arb_init(nvband_charger_arb_state_t *state)
{
    if (state == NULL) {
        return;
    }
    memset(state, 0, sizeof(*state));
    state->active_source = NVBAND_CHARGE_SOURCE_NONE;
}

void nvband_charger_arb_update(nvband_charger_arb_state_t *state,
                                bool usb_present, bool qi_present,
                                nvband_charger_arb_event_t *event_out)
{
    if (state == NULL) {
        return;
    }

    nvband_charge_source_t decided;
    if (usb_present) {
        decided = NVBAND_CHARGE_SOURCE_USB; /* USB always wins if present */
    } else if (qi_present) {
        decided = NVBAND_CHARGE_SOURCE_QI;
    } else {
        decided = NVBAND_CHARGE_SOURCE_NONE;
    }

    bool changed = (decided != state->active_source);
    if (event_out != NULL) {
        event_out->previous_source = state->active_source;
        event_out->new_source = decided;
        event_out->changed = changed;
    }

    if (changed) {
        state->arbitration_event_count++;
    }
    state->active_source = decided;
}
