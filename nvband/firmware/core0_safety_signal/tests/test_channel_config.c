/* Traces: TRC-CFG-01 (OI-1). */
#include "test_framework.h"
#include "../../shared/nvband_channel_config.h"

static void test_bench_default_is_valid_and_flagged_non_provisioned(void)
{
    nvband_channel_config_t cfg;
    nvband_channel_config_bench_default(&cfg);
    NVBAND_CHECK(nvband_channel_config_is_valid(&cfg) == true);
    NVBAND_CHECK(cfg.loaded_from_provisioning == false);
    NVBAND_CHECK(cfg.stim_channel_count == NVBAND_PLACEHOLDER_STIM_CHANNEL_COUNT);
    NVBAND_CHECK(cfg.eeg_channel_count == NVBAND_PLACEHOLDER_EEG_CHANNEL_COUNT);
}

static void test_zero_channel_counts_invalid(void)
{
    nvband_channel_config_t cfg;
    nvband_channel_config_bench_default(&cfg);
    cfg.stim_channel_count = 0;
    NVBAND_CHECK(nvband_channel_config_is_valid(&cfg) == false);
}

static void test_over_bound_channel_counts_invalid(void)
{
    nvband_channel_config_t cfg;
    nvband_channel_config_bench_default(&cfg);
    cfg.eeg_channel_count = NVBAND_MAX_EEG_CHANNELS + 1;
    NVBAND_CHECK(nvband_channel_config_is_valid(&cfg) == false);
}

static void test_null_config_invalid(void)
{
    NVBAND_CHECK(nvband_channel_config_is_valid(NULL) == false);
}

int main(void)
{
    NVBAND_RUN(test_bench_default_is_valid_and_flagged_non_provisioned);
    NVBAND_RUN(test_zero_channel_counts_invalid);
    NVBAND_RUN(test_over_bound_channel_counts_invalid);
    NVBAND_RUN(test_null_config_invalid);
    NVBAND_TEST_MAIN_END();
}
