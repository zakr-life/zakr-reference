#include "brainprint_template.h"
#include <string.h>
#include <math.h>

bool nvband_brainprint_epoch_features(const nvband_brainprint_band_power_t band_power,
                                       float out[NVBAND_BRAINPRINT_TEMPLATE_LEN])
{
    if (band_power == NULL || out == NULL) {
        return false;
    }

    for (uint32_t ch = 0; ch < NVBAND_BRAINPRINT_CHANNELS; ch++) {
        float total = 0.0f;
        for (uint32_t b = 0; b < NVBAND_BRAINPRINT_BANDS; b++) {
            float v = band_power[ch][b];
            if (v < 0.0f || !(v == v) /* NaN check */) {
                return false; /* fail closed on malformed/negative power */
            }
            total += v;
        }
        /* A channel with ~0 total power is flat/disconnected -- refuse to
         * fabricate a ratio for it rather than dividing by ~0. */
        if (total < 1e-9f) {
            return false;
        }
        for (uint32_t b = 0; b < NVBAND_BRAINPRINT_BANDS; b++) {
            out[ch * NVBAND_BRAINPRINT_BANDS + b] = band_power[ch][b] / total;
        }
    }
    return true;
}

bool nvband_brainprint_build_template(const float epoch_features[][NVBAND_BRAINPRINT_TEMPLATE_LEN],
                                       uint32_t epoch_count,
                                       nvband_brainprint_template_t out)
{
    if (epoch_features == NULL || out == NULL) {
        return false;
    }
    if (epoch_count < NVBAND_BRAINPRINT_MIN_ENROLL_EPOCHS ||
        epoch_count > NVBAND_BRAINPRINT_MAX_ENROLL_EPOCHS) {
        return false;
    }

    float sum[NVBAND_BRAINPRINT_TEMPLATE_LEN];
    memset(sum, 0, sizeof(sum));

    for (uint32_t e = 0; e < epoch_count; e++) {
        for (uint32_t i = 0; i < NVBAND_BRAINPRINT_TEMPLATE_LEN; i++) {
            float v = epoch_features[e][i];
            if (v < 0.0f || v > 1.0f || !(v == v)) {
                return false; /* each input must already be a valid ratio */
            }
            sum[i] += v;
        }
    }
    for (uint32_t i = 0; i < NVBAND_BRAINPRINT_TEMPLATE_LEN; i++) {
        out[i] = sum[i] / (float)epoch_count;
    }
    return true;
}
