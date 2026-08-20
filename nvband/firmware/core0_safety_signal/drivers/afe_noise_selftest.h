/**
 * afe_noise_selftest.h — EEG AFE (U1, ADS1299/ADS1299-4) startup
 * noise-floor self-test.
 *
 * CLAUDE.md §3.3: "Build a startup self-test that measures noise floor
 * with the mux forced to a shorted/known-impedance state and refuses to
 * proceed to a live session if it exceeds threshold." Target: input-
 * referred noise <1 µV RMS over 0.5-40 Hz (NVBAND_EEG_INPUT_NOISE_UVRMS_MAX).
 *
 * This module is the PASS/FAIL decision logic only, operating on an
 * already-acquired sample buffer from the shorted-input state. The
 * register-level ADS1299 SPI configuration (gain, sample rate, bias
 * drive, shorted-input test mode) is a Zephyr sensor driver
 * (afe_ads1299_zephyr.c, not included in this pass — needs the
 * datasheet-verified register map; see README in this directory) that
 * calls into this module with the resulting buffer.
 */
#ifndef NVBAND_AFE_NOISE_SELFTEST_H
#define NVBAND_AFE_NOISE_SELFTEST_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    float    rms_noise_uV[8];       /* per channel, up to 8ch part option */
    uint8_t  channel_count;
    bool     pass;                  /* all active channels under threshold */
    uint8_t  first_failing_channel; /* valid only if !pass                 */
} nvband_afe_selftest_result_t;

/**
 * samples_uV: [channel_count][sample_count] shorted-input samples in
 * microvolts, already gain-corrected by the caller. Pure function: computes
 * RMS per channel and compares to NVBAND_EEG_INPUT_NOISE_UVRMS_MAX.
 */
bool nvband_afe_noise_selftest_evaluate(const float *const *samples_uV,
                                         uint8_t channel_count,
                                         size_t sample_count,
                                         nvband_afe_selftest_result_t *out);

#endif /* NVBAND_AFE_NOISE_SELFTEST_H */
