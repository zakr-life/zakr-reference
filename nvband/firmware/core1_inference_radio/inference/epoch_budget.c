#include "epoch_budget.h"
#include <string.h>

void nvband_epoch_budget_stats_init(nvband_epoch_budget_stats_t *stats)
{
    if (stats == NULL) {
        return;
    }
    memset(stats, 0, sizeof(*stats));
}

void nvband_epoch_budget_evaluate(nvband_epoch_budget_stats_t *stats,
                                   uint64_t epoch_start_us,
                                   uint64_t epoch_end_us,
                                   nvband_epoch_result_t *out)
{
    if (out == NULL) {
        return;
    }
    memset(out, 0, sizeof(*out));
    out->epoch_start_us = epoch_start_us;
    out->epoch_end_us = epoch_end_us;

    uint64_t elapsed = (epoch_end_us >= epoch_start_us)
                            ? (epoch_end_us - epoch_start_us)
                            : 0;

    if (elapsed <= NVBAND_INFERENCE_EPOCH_BUDGET_US) {
        out->usable = true;
        out->overrun_us = 0;
    } else {
        out->usable = false;
        out->overrun_us = elapsed - NVBAND_INFERENCE_EPOCH_BUDGET_US;
    }

    if (stats != NULL) {
        stats->total_epochs++;
        if (!out->usable) {
            stats->dropped_epochs++;
        }
    }
}
