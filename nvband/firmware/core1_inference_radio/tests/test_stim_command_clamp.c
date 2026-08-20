/* Traces: TRC-CLAMP-01..04. */
#include "../../core0_safety_signal/tests/test_framework.h"
#include "../inference/stim_command_clamp.h"
#include "../../shared/nvband_constants.h"

static void test_within_bounds_passes_through_unchanged(void)
{
    nvband_stim_command_t cmd = { 1.0f, 100000u, 5000 };
    nvband_stim_clamp_result_t r;
    nvband_stim_command_clamp(&cmd, &r);

    NVBAND_CHECK(r.current_was_clamped == false);
    NVBAND_CHECK(r.duration_was_clamped == false);
    NVBAND_CHECK(r.clamped.requested_current_mA == 1.0f);
}

static void test_model_requesting_absurd_current_gets_clamped(void)
{
    /* A malicious or buggy model output requesting 10000 mA must be
     * clamped to the hard ceiling regardless of what it asked for. */
    nvband_stim_command_t cmd = { 10000.0f, 1000u, 0 };
    nvband_stim_clamp_result_t r;
    nvband_stim_command_clamp(&cmd, &r);

    NVBAND_CHECK(r.current_was_clamped == true);
    NVBAND_CHECK(r.clamped.requested_current_mA <= NVBAND_CURRENT_CEILING_MA);
}

static void test_negative_current_clamped_symmetrically(void)
{
    nvband_stim_command_t cmd = { -10000.0f, 1000u, 0 };
    nvband_stim_clamp_result_t r;
    nvband_stim_command_clamp(&cmd, &r);

    NVBAND_CHECK(r.current_was_clamped == true);
    NVBAND_CHECK(r.clamped.requested_current_mA >= -NVBAND_CURRENT_CEILING_MA);
    NVBAND_CHECK(r.clamped.requested_current_mA < 0);
}

static void test_absurd_duration_clamped(void)
{
    nvband_stim_command_t cmd = { 1.0f, 3600u * 1000000u, 0 }; /* 1 hour */
    nvband_stim_clamp_result_t r;
    nvband_stim_command_clamp(&cmd, &r);

    NVBAND_CHECK(r.duration_was_clamped == true);
    NVBAND_CHECK(r.clamped.requested_duration_us
                 <= NVBAND_MAX_SINGLE_BURST_DURATION_US);
}

static void test_null_input_yields_zero_command(void)
{
    nvband_stim_clamp_result_t r;
    nvband_stim_command_clamp(NULL, &r);
    NVBAND_CHECK(r.clamped.requested_current_mA == 0.0f);
    NVBAND_CHECK(r.clamped.requested_duration_us == 0);
}

int main(void)
{
    NVBAND_RUN(test_within_bounds_passes_through_unchanged);
    NVBAND_RUN(test_model_requesting_absurd_current_gets_clamped);
    NVBAND_RUN(test_negative_current_clamped_symmetrically);
    NVBAND_RUN(test_absurd_duration_clamped);
    NVBAND_RUN(test_null_input_yields_zero_command);
    NVBAND_TEST_MAIN_END();
}
