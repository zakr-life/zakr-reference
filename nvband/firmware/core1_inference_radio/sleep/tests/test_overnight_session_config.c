/* Traces: Addendum 2 §E (sleep-state monitoring config/result types). */
#include "../../../core0_safety_signal/tests/test_framework.h"
#include "../overnight_session_config.h"
#include <string.h>

static void test_default_config_is_valid(void)
{
    nvband_overnight_session_config_t cfg;
    nvband_overnight_session_config_default(&cfg);
    NVBAND_CHECK(nvband_overnight_session_config_is_valid(&cfg) == true);
    NVBAND_CHECK(cfg.eeg_sample_rate_hz > 0);
    NVBAND_CHECK(cfg.epoch_duration_s == 30u);
    NVBAND_CHECK(cfg.imu_channel_active == true);
}

static void test_null_config_default_does_not_crash(void)
{
    /* Must not crash on NULL -- same defensive convention as every
     * other default()/evaluate() function in this repo. */
    nvband_overnight_session_config_default(NULL);
    NVBAND_CHECK(true);
}

static void test_zero_sample_rate_is_invalid(void)
{
    nvband_overnight_session_config_t cfg;
    nvband_overnight_session_config_default(&cfg);
    cfg.eeg_sample_rate_hz = 0;
    NVBAND_CHECK(nvband_overnight_session_config_is_valid(&cfg) == false);
}

static void test_zero_epoch_duration_is_invalid(void)
{
    nvband_overnight_session_config_t cfg;
    nvband_overnight_session_config_default(&cfg);
    cfg.epoch_duration_s = 0;
    NVBAND_CHECK(nvband_overnight_session_config_is_valid(&cfg) == false);
}

static void test_zero_or_implausible_max_duration_is_invalid(void)
{
    nvband_overnight_session_config_t cfg;
    nvband_overnight_session_config_default(&cfg);
    cfg.session_max_duration_min = 0;
    NVBAND_CHECK(nvband_overnight_session_config_is_valid(&cfg) == false);

    nvband_overnight_session_config_default(&cfg);
    cfg.session_max_duration_min = 24u * 60u + 1u; /* > 24h */
    NVBAND_CHECK(nvband_overnight_session_config_is_valid(&cfg) == false);
}

static void test_imu_off_is_invalid_for_this_feature(void)
{
    nvband_overnight_session_config_t cfg;
    nvband_overnight_session_config_default(&cfg);
    cfg.imu_channel_active = false;
    NVBAND_CHECK(nvband_overnight_session_config_is_valid(&cfg) == false);
}

static void test_null_config_is_not_valid(void)
{
    NVBAND_CHECK(nvband_overnight_session_config_is_valid(NULL) == false);
}

static void test_expected_epoch_count_matches_arithmetic(void)
{
    nvband_overnight_session_config_t cfg;
    nvband_overnight_session_config_default(&cfg);
    cfg.session_max_duration_min = 480u; /* 8 hours */
    cfg.epoch_duration_s = 30u;
    /* 8h * 60 min/h * 60 s/min / 30 s/epoch = 960 epochs */
    NVBAND_CHECK(nvband_overnight_session_expected_epoch_count(&cfg) == 960u);
}

static void test_expected_epoch_count_is_zero_for_invalid_config(void)
{
    nvband_overnight_session_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    NVBAND_CHECK(nvband_overnight_session_expected_epoch_count(&cfg) == 0u);
    NVBAND_CHECK(nvband_overnight_session_expected_epoch_count(NULL) == 0u);
}

static void test_epoch_result_valid_for_every_stage(void)
{
    for (int s = NVBAND_SLEEP_STAGE_WAKE; s <= NVBAND_SLEEP_STAGE_REM; s++) {
        nvband_sleep_epoch_result_t r;
        r.stage = (nvband_sleep_stage_t)s;
        r.confidence = 0.75f;
        NVBAND_CHECK(nvband_sleep_epoch_result_is_valid(&r) == true);
    }
}

static void test_epoch_result_invalid_confidence_out_of_range(void)
{
    nvband_sleep_epoch_result_t r;
    r.stage = NVBAND_SLEEP_STAGE_N2;
    r.confidence = 1.5f;
    NVBAND_CHECK(nvband_sleep_epoch_result_is_valid(&r) == false);

    r.confidence = -0.1f;
    NVBAND_CHECK(nvband_sleep_epoch_result_is_valid(&r) == false);
}

static void test_epoch_result_null_is_invalid(void)
{
    NVBAND_CHECK(nvband_sleep_epoch_result_is_valid(NULL) == false);
}

/* Structural check (task-required): nvband_sleep_epoch_result_t must
 * contain ONLY a stage field and a confidence field -- no current,
 * duration, charge, or enable/permit field of any kind. This is not
 * fully expressible as a compile-time static_assert without C11
 * _Generic member enumeration (which the C standard doesn't provide),
 * so it is enforced two ways: (1) this sizeof() check, which will start
 * failing the moment a field is added or removed, forcing a deliberate
 * update of the expected constant right here in the same review that
 * would add a new field; and (2) the mechanical grep-based scan in
 * check_no_stim_coupling.py, run by `make test` in this directory,
 * which independently verifies the whole sleep/ subdirectory (not just
 * this one struct) never spells a stimulation-related identifier. */
static void test_sleep_epoch_result_struct_has_exactly_stage_and_confidence(void)
{
    size_t expected_size = sizeof(nvband_sleep_stage_t) + sizeof(float);
    NVBAND_CHECK(sizeof(nvband_sleep_epoch_result_t) == expected_size);
}

int main(void)
{
    NVBAND_RUN(test_default_config_is_valid);
    NVBAND_RUN(test_null_config_default_does_not_crash);
    NVBAND_RUN(test_zero_sample_rate_is_invalid);
    NVBAND_RUN(test_zero_epoch_duration_is_invalid);
    NVBAND_RUN(test_zero_or_implausible_max_duration_is_invalid);
    NVBAND_RUN(test_imu_off_is_invalid_for_this_feature);
    NVBAND_RUN(test_null_config_is_not_valid);
    NVBAND_RUN(test_expected_epoch_count_matches_arithmetic);
    NVBAND_RUN(test_expected_epoch_count_is_zero_for_invalid_config);
    NVBAND_RUN(test_epoch_result_valid_for_every_stage);
    NVBAND_RUN(test_epoch_result_invalid_confidence_out_of_range);
    NVBAND_RUN(test_epoch_result_null_is_invalid);
    NVBAND_RUN(test_sleep_epoch_result_struct_has_exactly_stage_and_confidence);
    NVBAND_TEST_MAIN_END();
}
