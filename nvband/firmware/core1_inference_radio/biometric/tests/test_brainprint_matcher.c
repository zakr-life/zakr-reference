#include "../brainprint_matcher.h"
#include "../../../core0_safety_signal/tests/test_framework.h"

static void fill(float v[NVBAND_BRAINPRINT_TEMPLATE_LEN], float base)
{
    for (uint32_t i = 0; i < NVBAND_BRAINPRINT_TEMPLATE_LEN; i++) {
        v[i] = base + 0.01f * (float)i;
    }
}

static void test_identical_vectors_match(void)
{
    float a[NVBAND_BRAINPRINT_TEMPLATE_LEN];
    fill(a, 1.0f);
    float sim = nvband_brainprint_cosine_similarity(a, a);
    NVBAND_CHECK(sim > 0.999f);
    NVBAND_CHECK(nvband_brainprint_matches(a, a, NVBAND_BRAINPRINT_DEFAULT_THRESHOLD));
}

static void test_orthogonal_vectors_do_not_match(void)
{
    float a[NVBAND_BRAINPRINT_TEMPLATE_LEN] = {0};
    float b[NVBAND_BRAINPRINT_TEMPLATE_LEN] = {0};
    a[0] = 1.0f;
    b[1] = 1.0f;
    float sim = nvband_brainprint_cosine_similarity(a, b);
    NVBAND_CHECK(sim < 0.01f);
    NVBAND_CHECK(!nvband_brainprint_matches(a, b, NVBAND_BRAINPRINT_DEFAULT_THRESHOLD));
}

static void test_different_subject_below_threshold(void)
{
    float a[NVBAND_BRAINPRINT_TEMPLATE_LEN];
    float b[NVBAND_BRAINPRINT_TEMPLATE_LEN];
    fill(a, 1.0f);
    for (uint32_t i = 0; i < NVBAND_BRAINPRINT_TEMPLATE_LEN; i++) {
        b[i] = (i % 2 == 0) ? 5.0f : 0.01f; /* clearly different band-power shape */
    }
    NVBAND_CHECK(!nvband_brainprint_matches(a, b, NVBAND_BRAINPRINT_DEFAULT_THRESHOLD));
}

static void test_degenerate_and_null_inputs_fail_closed(void)
{
    float zero[NVBAND_BRAINPRINT_TEMPLATE_LEN] = {0};
    float a[NVBAND_BRAINPRINT_TEMPLATE_LEN];
    fill(a, 1.0f);

    NVBAND_CHECK(nvband_brainprint_cosine_similarity(zero, a) == 0.0f);
    NVBAND_CHECK(nvband_brainprint_cosine_similarity(NULL, a) == 0.0f);
    NVBAND_CHECK(!nvband_brainprint_matches(NULL, a, NVBAND_BRAINPRINT_DEFAULT_THRESHOLD));
    NVBAND_CHECK(!nvband_brainprint_matches(a, NULL, NVBAND_BRAINPRINT_DEFAULT_THRESHOLD));
    NVBAND_CHECK(!nvband_brainprint_matches(a, a, 1.5f));   /* out-of-range threshold */
    NVBAND_CHECK(!nvband_brainprint_matches(a, a, -1.5f));
}

int main(void)
{
    NVBAND_RUN(test_identical_vectors_match);
    NVBAND_RUN(test_orthogonal_vectors_do_not_match);
    NVBAND_RUN(test_different_subject_below_threshold);
    NVBAND_RUN(test_degenerate_and_null_inputs_fail_closed);
    NVBAND_TEST_MAIN_END();
}
