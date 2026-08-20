/* Traces: TRC-PMIC-01..03. */
#include "test_framework.h"
#include "../drivers/pmic_sequencer.h"

static bool always_true(void *ctx) { (void)ctx; return true; }
static bool always_false(void *ctx) { (void)ctx; return false; }

static void test_all_rails_settle_allows_afe_release(void)
{
    nvband_rail_sequence_t seq = {0};
    seq.rail_count = 3;
    seq.rails[0] = (nvband_rail_t){ "1V8", always_true };
    seq.rails[1] = (nvband_rail_t){ "3V3", always_true };
    seq.rails[2] = (nvband_rail_t){ "STIM_HV", always_true };

    nvband_rail_sequence_result_t r;
    nvband_rail_sequence_run(&seq, &r);
    NVBAND_CHECK(r.all_settled == true);
    NVBAND_CHECK(r.rails_confirmed == 3);
    NVBAND_CHECK(nvband_rail_sequence_afe_reset_release_ok(&r) == true);
}

static void test_first_failing_rail_stops_sequence(void)
{
    nvband_rail_sequence_t seq = {0};
    seq.rail_count = 3;
    seq.rails[0] = (nvband_rail_t){ "1V8", always_true };
    seq.rails[1] = (nvband_rail_t){ "3V3", always_false };
    seq.rails[2] = (nvband_rail_t){ "STIM_HV", always_true };

    nvband_rail_sequence_result_t r;
    nvband_rail_sequence_run(&seq, &r);
    NVBAND_CHECK(r.all_settled == false);
    NVBAND_CHECK(r.first_failed_rail_index == 1);
    NVBAND_CHECK(nvband_rail_sequence_afe_reset_release_ok(&r) == false);
}

static void test_null_readback_never_assumed_settled(void)
{
    nvband_rail_sequence_t seq = {0};
    seq.rail_count = 1;
    seq.rails[0] = (nvband_rail_t){ "UNCONFIGURED", NULL };

    nvband_rail_sequence_result_t r;
    nvband_rail_sequence_run(&seq, &r);
    NVBAND_CHECK(r.all_settled == false);
}

int main(void)
{
    NVBAND_RUN(test_all_rails_settle_allows_afe_release);
    NVBAND_RUN(test_first_failing_rail_stops_sequence);
    NVBAND_RUN(test_null_readback_never_assumed_settled);
    NVBAND_TEST_MAIN_END();
}
