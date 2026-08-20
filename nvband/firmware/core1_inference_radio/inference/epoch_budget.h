/**
 * epoch_budget.h — Per-epoch inference time-budget enforcement.
 *
 * CLAUDE.md §3.6 / §4: "If an inference task overruns its allotted time
 * budget, the correct behavior is: drop that epoch's result, log it,
 * continue — never retry-block, never signal Core 0."
 *
 * This module wraps a single epoch's inference timing decision. It does
 * not run the model itself (that's models/export's runtime, invoked by
 * the caller) — it only decides, given start/end timestamps and the
 * configured budget, whether the result is usable. On overrun it never
 * raises anything toward Core 0 (no IPC message, no interrupt) — Core 0
 * is entirely unaware inference happened at all, which is exactly the
 * isolation property this module exists to preserve.
 */
#ifndef NVBAND_EPOCH_BUDGET_H
#define NVBAND_EPOCH_BUDGET_H

#include <stdbool.h>
#include <stdint.h>

#define NVBAND_INFERENCE_EPOCH_BUDGET_US (100u * 1000u) /* 100 ms/epoch */

typedef struct {
    uint64_t epoch_start_us;
    uint64_t epoch_end_us;
    bool     usable;             /* false => drop this epoch's result   */
    uint64_t overrun_us;         /* 0 if usable                          */
} nvband_epoch_result_t;

typedef struct {
    uint64_t total_epochs;
    uint64_t dropped_epochs;
} nvband_epoch_budget_stats_t;

void nvband_epoch_budget_stats_init(nvband_epoch_budget_stats_t *stats);

/**
 * Evaluate one epoch's timing. Updates stats. Never blocks, never has any
 * side effect visible outside this struct/stats (in particular: no call
 * into anything Core-0-facing).
 */
void nvband_epoch_budget_evaluate(nvband_epoch_budget_stats_t *stats,
                                   uint64_t epoch_start_us,
                                   uint64_t epoch_end_us,
                                   nvband_epoch_result_t *out);

#endif /* NVBAND_EPOCH_BUDGET_H */
