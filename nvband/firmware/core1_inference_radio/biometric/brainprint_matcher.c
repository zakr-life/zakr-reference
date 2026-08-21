#include "brainprint_matcher.h"
#include <math.h>
#include <stddef.h>

float nvband_brainprint_cosine_similarity(const float a[NVBAND_BRAINPRINT_TEMPLATE_LEN],
                                           const float b[NVBAND_BRAINPRINT_TEMPLATE_LEN])
{
    if (a == NULL || b == NULL) {
        return 0.0f;
    }

    double dot = 0.0, mag_a = 0.0, mag_b = 0.0;
    for (uint32_t i = 0; i < NVBAND_BRAINPRINT_TEMPLATE_LEN; i++) {
        dot   += (double)a[i] * (double)b[i];
        mag_a += (double)a[i] * (double)a[i];
        mag_b += (double)b[i] * (double)b[i];
    }
    if (mag_a < 1e-12 || mag_b < 1e-12) {
        return 0.0f; /* degenerate vector carries no signal -- fail closed */
    }
    double sim = dot / (sqrt(mag_a) * sqrt(mag_b));
    if (sim > 1.0) sim = 1.0;
    if (sim < -1.0) sim = -1.0;
    return (float)sim;
}

bool nvband_brainprint_matches(const float live[NVBAND_BRAINPRINT_TEMPLATE_LEN],
                                const float enrolled[NVBAND_BRAINPRINT_TEMPLATE_LEN],
                                float threshold)
{
    if (live == NULL || enrolled == NULL) {
        return false;
    }
    if (threshold < -1.0f || threshold > 1.0f) {
        return false;
    }
    return nvband_brainprint_cosine_similarity(live, enrolled) >= threshold;
}
