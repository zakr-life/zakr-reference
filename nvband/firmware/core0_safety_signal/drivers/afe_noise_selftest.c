#include "afe_noise_selftest.h"
#include "../../shared/nvband_constants.h"
#include <math.h>
#include <string.h>

bool nvband_afe_noise_selftest_evaluate(const float *const *samples_uV,
                                         uint8_t channel_count,
                                         size_t sample_count,
                                         nvband_afe_selftest_result_t *out)
{
    if (out == NULL) {
        return false;
    }
    memset(out, 0, sizeof(*out));

    if (samples_uV == NULL || channel_count == 0 ||
        channel_count > 8 || sample_count == 0) {
        return false; /* malformed input: never pass by default */
    }

    out->channel_count = channel_count;
    out->pass = true;

    for (uint8_t ch = 0; ch < channel_count; ch++) {
        const float *buf = samples_uV[ch];
        if (buf == NULL) {
            out->pass = false;
            out->first_failing_channel = ch;
            return true; /* well-formed result: fail-closed on bad channel */
        }

        double sum_sq = 0.0;
        double mean = 0.0;
        for (size_t i = 0; i < sample_count; i++) {
            mean += buf[i];
        }
        mean /= (double)sample_count;

        for (size_t i = 0; i < sample_count; i++) {
            double d = buf[i] - mean;
            sum_sq += d * d;
        }
        double rms = sqrt(sum_sq / (double)sample_count);
        out->rms_noise_uV[ch] = (float)rms;

        if (rms >= (double)NVBAND_EEG_INPUT_NOISE_UVRMS_MAX) {
            if (out->pass) {
                out->first_failing_channel = ch;
            }
            out->pass = false;
        }
    }

    return true;
}
