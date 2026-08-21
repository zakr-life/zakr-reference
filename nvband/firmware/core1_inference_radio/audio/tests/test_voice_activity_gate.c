#include "../../../core0_safety_signal/tests/test_framework.h"
#include "../voice_activity_gate.h"
#include <math.h>

#define TEST_SAMPLE_COUNT 1600u /* 100 ms at 16 kHz, well above default min_samples */

static void fill_loud_tone(int16_t *out, uint32_t n, float amplitude)
{
    for (uint32_t i = 0; i < n; i++) {
        double phase = 2.0 * 3.14159265358979 * 200.0 * ((double)i / 16000.0);
        out[i] = (int16_t)(amplitude * sin(phase));
    }
}

static void fill_silence(int16_t *out, uint32_t n)
{
    for (uint32_t i = 0; i < n; i++) {
        out[i] = (int16_t)((i % 3) - 1); /* tiny dither, effectively silent */
    }
}

static void test_default_config_has_sane_values(void)
{
    nvband_vad_config_t cfg = nvband_vad_default_config();
    NVBAND_CHECK(cfg.rms_threshold > 0.0f);
    NVBAND_CHECK(cfg.min_samples > 0);
}

static void test_loud_tone_passes(void)
{
    int16_t samples[TEST_SAMPLE_COUNT];
    fill_loud_tone(samples, TEST_SAMPLE_COUNT, 5000.0f);
    nvband_vad_config_t cfg = nvband_vad_default_config();
    nvband_vad_result_t result;

    nvband_vad_evaluate(samples, TEST_SAMPLE_COUNT, &cfg, &result);

    NVBAND_CHECK(result.passed == true);
    NVBAND_CHECK(result.measured_rms >= cfg.rms_threshold);
    NVBAND_CHECK(result.sample_count == TEST_SAMPLE_COUNT);
}

static void test_silence_fails_rather_than_guessing(void)
{
    int16_t samples[TEST_SAMPLE_COUNT];
    fill_silence(samples, TEST_SAMPLE_COUNT);
    nvband_vad_config_t cfg = nvband_vad_default_config();
    nvband_vad_result_t result;

    nvband_vad_evaluate(samples, TEST_SAMPLE_COUNT, &cfg, &result);

    NVBAND_CHECK(result.passed == false);
    NVBAND_CHECK(result.measured_rms < cfg.rms_threshold);
}

static void test_too_few_samples_aborts_even_if_loud(void)
{
    int16_t samples[16];
    fill_loud_tone(samples, 16, 30000.0f); /* loud, but far too short a window */
    nvband_vad_config_t cfg = nvband_vad_default_config();
    nvband_vad_result_t result;

    nvband_vad_evaluate(samples, 16, &cfg, &result);

    NVBAND_CHECK(result.passed == false);
}

static void test_null_samples_aborts_safely(void)
{
    nvband_vad_config_t cfg = nvband_vad_default_config();
    nvband_vad_result_t result;
    nvband_vad_evaluate(NULL, TEST_SAMPLE_COUNT, &cfg, &result);
    NVBAND_CHECK(result.passed == false);
}

static void test_null_config_aborts_safely(void)
{
    int16_t samples[TEST_SAMPLE_COUNT];
    fill_loud_tone(samples, TEST_SAMPLE_COUNT, 5000.0f);
    nvband_vad_result_t result;
    nvband_vad_evaluate(samples, TEST_SAMPLE_COUNT, NULL, &result);
    NVBAND_CHECK(result.passed == false);
}

static void test_threshold_boundary_is_inclusive(void)
{
    /* A DC buffer whose |value| exactly equals the threshold has RMS
     * exactly at threshold -- must pass (>=), not be treated as below. */
    nvband_vad_config_t cfg;
    cfg.rms_threshold = 100.0f;
    cfg.min_samples = 4;
    int16_t samples[4] = {100, -100, 100, -100};
    nvband_vad_result_t result;

    nvband_vad_evaluate(samples, 4, &cfg, &result);

    NVBAND_CHECK(result.passed == true);
    NVBAND_CHECK(fabsf(result.measured_rms - 100.0f) < 0.001f);
}

int main(void)
{
    NVBAND_RUN(test_default_config_has_sane_values);
    NVBAND_RUN(test_loud_tone_passes);
    NVBAND_RUN(test_silence_fails_rather_than_guessing);
    NVBAND_RUN(test_too_few_samples_aborts_even_if_loud);
    NVBAND_RUN(test_null_samples_aborts_safely);
    NVBAND_RUN(test_null_config_aborts_safely);
    NVBAND_RUN(test_threshold_boundary_is_inclusive);
    NVBAND_TEST_MAIN_END();
}
