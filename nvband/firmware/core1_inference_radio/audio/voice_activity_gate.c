#include "voice_activity_gate.h"
#include <math.h>
#include <string.h>

/* int16 full scale is 32768; a spoken phrase held close to a MEMS mic
 * typically sits well above the noise floor. This is a starting
 * placeholder, not a datasheet- or field-validated figure — see the
 * header docstring and Addendum 2 `TODO(OI-6)`. */
#define NVBAND_VAD_DEFAULT_RMS_THRESHOLD   400.0f
#define NVBAND_VAD_DEFAULT_MIN_SAMPLES     800u /* 50 ms at 16 kHz */

nvband_vad_config_t nvband_vad_default_config(void)
{
    nvband_vad_config_t cfg;
    cfg.rms_threshold = NVBAND_VAD_DEFAULT_RMS_THRESHOLD;
    cfg.min_samples = NVBAND_VAD_DEFAULT_MIN_SAMPLES;
    return cfg;
}

void nvband_vad_evaluate(const int16_t *samples, uint32_t sample_count,
                          const nvband_vad_config_t *config,
                          nvband_vad_result_t *out)
{
    if (out == NULL) {
        return;
    }
    memset(out, 0, sizeof(*out));

    if (config == NULL || samples == NULL) {
        out->passed = false;
        return;
    }

    out->sample_count = sample_count;

    if (sample_count < config->min_samples) {
        /* Not enough signal to reach any decision at all: abort rather
         * than compute an RMS over too short a window and guess. */
        out->passed = false;
        return;
    }

    double sum_sq = 0.0;
    for (uint32_t i = 0; i < sample_count; i++) {
        double v = (double)samples[i];
        sum_sq += v * v;
    }
    float rms = (float)sqrt(sum_sq / (double)sample_count);
    out->measured_rms = rms;
    out->passed = rms >= config->rms_threshold;
}
