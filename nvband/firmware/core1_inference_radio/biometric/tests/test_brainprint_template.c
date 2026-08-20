#include "../brainprint_template.h"
#include "../../../core0_safety_signal/tests/test_framework.h"

static void test_epoch_features_normalizes_per_channel(void)
{
    nvband_brainprint_band_power_t bp = {
        {10.0f, 10.0f, 10.0f, 10.0f, 10.0f}, /* ch0: equal -> 0.2 each */
        {4.0f, 0.0f, 0.0f, 0.0f, 0.0f},       /* ch1: all in band0 -> 1.0,0,0,0,0 */
        {1.0f, 1.0f, 1.0f, 1.0f, 1.0f},
        {2.0f, 2.0f, 2.0f, 2.0f, 2.0f},
    };
    float out[NVBAND_BRAINPRINT_TEMPLATE_LEN];
    bool ok = nvband_brainprint_epoch_features(bp, out);
    NVBAND_CHECK(ok);
    NVBAND_CHECK(out[0] > 0.19f && out[0] < 0.21f);
    NVBAND_CHECK(out[5] > 0.99f); /* ch1 band0 */
    NVBAND_CHECK(out[6] < 0.01f); /* ch1 band1 */

    float sum_ch0 = out[0] + out[1] + out[2] + out[3] + out[4];
    NVBAND_CHECK(sum_ch0 > 0.99f && sum_ch0 < 1.01f);
}

static void test_epoch_features_rejects_flat_channel(void)
{
    nvband_brainprint_band_power_t bp = {
        {0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, /* flat/disconnected channel */
        {1.0f, 1.0f, 1.0f, 1.0f, 1.0f},
        {1.0f, 1.0f, 1.0f, 1.0f, 1.0f},
        {1.0f, 1.0f, 1.0f, 1.0f, 1.0f},
    };
    float out[NVBAND_BRAINPRINT_TEMPLATE_LEN];
    bool ok = nvband_brainprint_epoch_features(bp, out);
    NVBAND_CHECK(!ok); /* must refuse, never fabricate a ratio for a dead channel */
}

static void test_epoch_features_rejects_negative_and_null(void)
{
    nvband_brainprint_band_power_t bp = {
        {-1.0f, 1.0f, 1.0f, 1.0f, 1.0f},
        {1.0f, 1.0f, 1.0f, 1.0f, 1.0f},
        {1.0f, 1.0f, 1.0f, 1.0f, 1.0f},
        {1.0f, 1.0f, 1.0f, 1.0f, 1.0f},
    };
    float out[NVBAND_BRAINPRINT_TEMPLATE_LEN];
    NVBAND_CHECK(!nvband_brainprint_epoch_features(bp, out));
    NVBAND_CHECK(!nvband_brainprint_epoch_features(NULL, out));
    NVBAND_CHECK(!nvband_brainprint_epoch_features(bp, NULL));
}

static void test_build_template_averages_and_enforces_epoch_bounds(void)
{
    float epochs[NVBAND_BRAINPRINT_MIN_ENROLL_EPOCHS][NVBAND_BRAINPRINT_TEMPLATE_LEN];
    for (uint32_t e = 0; e < NVBAND_BRAINPRINT_MIN_ENROLL_EPOCHS; e++) {
        for (uint32_t i = 0; i < NVBAND_BRAINPRINT_TEMPLATE_LEN; i++) {
            epochs[e][i] = 0.05f * (float)(i + 1); /* same across epochs -> average == itself */
        }
    }
    nvband_brainprint_template_t out;
    bool ok = nvband_brainprint_build_template(epochs, NVBAND_BRAINPRINT_MIN_ENROLL_EPOCHS, out);
    NVBAND_CHECK(ok);
    NVBAND_CHECK(out[0] > 0.049f && out[0] < 0.051f);

    /* too few epochs refused */
    NVBAND_CHECK(!nvband_brainprint_build_template(epochs, NVBAND_BRAINPRINT_MIN_ENROLL_EPOCHS - 1, out));
    /* too many epochs refused */
    NVBAND_CHECK(!nvband_brainprint_build_template(epochs, NVBAND_BRAINPRINT_MAX_ENROLL_EPOCHS + 1, out));
    NVBAND_CHECK(!nvband_brainprint_build_template(NULL, NVBAND_BRAINPRINT_MIN_ENROLL_EPOCHS, out));
    NVBAND_CHECK(!nvband_brainprint_build_template(epochs, NVBAND_BRAINPRINT_MIN_ENROLL_EPOCHS, NULL));
}

int main(void)
{
    NVBAND_RUN(test_epoch_features_normalizes_per_channel);
    NVBAND_RUN(test_epoch_features_rejects_flat_channel);
    NVBAND_RUN(test_epoch_features_rejects_negative_and_null);
    NVBAND_RUN(test_build_template_averages_and_enforces_epoch_bounds);
    NVBAND_TEST_MAIN_END();
}
