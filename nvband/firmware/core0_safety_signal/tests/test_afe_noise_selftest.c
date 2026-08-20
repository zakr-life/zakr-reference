/* Traces: TRC-AFE-01..03. */
#include "test_framework.h"
#include "../drivers/afe_noise_selftest.h"

static void test_low_noise_channel_passes(void)
{
    float ch0[100];
    for (int i = 0; i < 100; i++) {
        ch0[i] = (i % 2 == 0) ? 0.1f : -0.1f; /* ~0.1 uVrms, well under 1.0 */
    }
    const float *channels[1] = { ch0 };
    nvband_afe_selftest_result_t r;

    NVBAND_CHECK(nvband_afe_noise_selftest_evaluate(channels, 1, 100, &r) == true);
    NVBAND_CHECK(r.pass == true);
}

static void test_high_noise_channel_fails_and_blocks_session(void)
{
    float ch0[100];
    for (int i = 0; i < 100; i++) {
        ch0[i] = (i % 2 == 0) ? 5.0f : -5.0f; /* ~5 uVrms, over threshold */
    }
    const float *channels[1] = { ch0 };
    nvband_afe_selftest_result_t r;

    NVBAND_CHECK(nvband_afe_noise_selftest_evaluate(channels, 1, 100, &r) == true);
    NVBAND_CHECK(r.pass == false);
    NVBAND_CHECK(r.first_failing_channel == 0);
}

static void test_one_bad_channel_among_several_fails_whole_test(void)
{
    float good[50]; for (int i=0;i<50;i++) good[i] = (i%2)?0.1f:-0.1f;
    float bad[50];  for (int i=0;i<50;i++) bad[i] = (i%2)?9.0f:-9.0f;
    const float *channels[3] = { good, good, bad };
    nvband_afe_selftest_result_t r;

    NVBAND_CHECK(nvband_afe_noise_selftest_evaluate(channels, 3, 50, &r) == true);
    NVBAND_CHECK(r.pass == false);
    NVBAND_CHECK(r.first_failing_channel == 2);
}

static void test_malformed_input_rejected(void)
{
    nvband_afe_selftest_result_t r;
    NVBAND_CHECK(nvband_afe_noise_selftest_evaluate(NULL, 1, 10, &r) == false);
    NVBAND_CHECK(nvband_afe_noise_selftest_evaluate((const float *const[]){0}, 0, 10, &r) == false);
}

int main(void)
{
    NVBAND_RUN(test_low_noise_channel_passes);
    NVBAND_RUN(test_high_noise_channel_fails_and_blocks_session);
    NVBAND_RUN(test_one_bad_channel_among_several_fails_whole_test);
    NVBAND_RUN(test_malformed_input_rejected);
    NVBAND_TEST_MAIN_END();
}
