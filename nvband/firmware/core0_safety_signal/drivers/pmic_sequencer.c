#include "pmic_sequencer.h"
#include <string.h>

void nvband_rail_sequence_run(const nvband_rail_sequence_t *seq,
                               nvband_rail_sequence_result_t *out)
{
    if (out == NULL) {
        return;
    }
    memset(out, 0, sizeof(*out));

    if (seq == NULL || seq->rail_count == 0 ||
        seq->rail_count > NVBAND_MAX_RAILS) {
        return; /* all_settled stays false */
    }

    for (uint8_t i = 0; i < seq->rail_count; i++) {
        bool settled = (seq->rails[i].readback_settled != NULL) &&
                        seq->rails[i].readback_settled(seq->ctx);
        if (!settled) {
            out->all_settled = false;
            out->first_failed_rail_index = i;
            out->rails_confirmed = i;
            return;
        }
    }

    out->all_settled = true;
    out->rails_confirmed = seq->rail_count;
}

bool nvband_rail_sequence_afe_reset_release_ok(
    const nvband_rail_sequence_result_t *result)
{
    return result != NULL && result->all_settled;
}
