/* Traces: TRC-EPOCH-01..03. */
#include "../../core0_safety_signal/tests/test_framework.h"
#include "../inference/epoch_budget.h"

static void test_epoch_within_budget_is_usable(void)
{
    nvband_epoch_budget_stats_t stats;
    nvband_epoch_budget_stats_init(&stats);
    nvband_epoch_result_t r;

    nvband_epoch_budget_evaluate(&stats, 0, 50000, &r); /* 50ms, budget 100ms */
    NVBAND_CHECK(r.usable == true);
    NVBAND_CHECK(r.overrun_us == 0);
    NVBAND_CHECK(stats.total_epochs == 1);
    NVBAND_CHECK(stats.dropped_epochs == 0);
}

static void test_epoch_overrun_is_dropped_not_blocked(void)
{
    nvband_epoch_budget_stats_t stats;
    nvband_epoch_budget_stats_init(&stats);
    nvband_epoch_result_t r;

    nvband_epoch_budget_evaluate(&stats, 0, 250000, &r); /* 250ms, over 100ms */
    NVBAND_CHECK(r.usable == false);
    NVBAND_CHECK(r.overrun_us == 150000);
    NVBAND_CHECK(stats.dropped_epochs == 1);
}

static void test_repeated_overruns_keep_counting_never_halt(void)
{
    nvband_epoch_budget_stats_t stats;
    nvband_epoch_budget_stats_init(&stats);

    for (int i = 0; i < 1000; i++) {
        nvband_epoch_result_t r;
        uint64_t start = (uint64_t)i * 200000;
        /* alternate: half overrun, half fine, none of it changes the
         * function's ability to keep evaluating the next epoch */
        uint64_t end = start + ((i % 2 == 0) ? 250000 : 50000);
        nvband_epoch_budget_evaluate(&stats, start, end, &r);
    }
    NVBAND_CHECK(stats.total_epochs == 1000);
    NVBAND_CHECK(stats.dropped_epochs == 500);
}

int main(void)
{
    NVBAND_RUN(test_epoch_within_budget_is_usable);
    NVBAND_RUN(test_epoch_overrun_is_dropped_not_blocked);
    NVBAND_RUN(test_repeated_overruns_keep_counting_never_halt);
    NVBAND_TEST_MAIN_END();
}
