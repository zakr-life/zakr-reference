/**
 * voice_activity_gate.h — Energy-threshold voice-activity check.
 *
 * A minimal, deliberately simple gate that must pass before a captured
 * buffer (mic_capture.h) is handed to feature extraction. If the buffer
 * does not contain sufficient voice-level energy (e.g. the user didn't
 * speak, the mic was muffled, the capture window ended before they
 * started), this module says so and the caller aborts the
 * enrollment/verification attempt rather than extracting features from
 * near-silence and guessing — mirroring the repo's "verify at the
 * boundary, refuse rather than assume" discipline used elsewhere (e.g.
 * afe_noise_selftest.c refusing to proceed past a bad noise-floor
 * check).
 *
 * This is energy-only (RMS against a threshold) — it is explicitly NOT a
 * speech/non-speech classifier, a liveness check, or an anti-spoofing
 * measure (see Addendum 2 `TODO(OI-7)`; this module makes no such
 * claim).
 */
#ifndef NVBAND_VOICE_ACTIVITY_GATE_H
#define NVBAND_VOICE_ACTIVITY_GATE_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    /* RMS (in raw int16 sample units) below which the buffer is judged
     * to have no sufficient voice-level energy. */
    float rms_threshold;
    /* Minimum number of samples required to even evaluate — a capture
     * that ended too early is a failure to reach a decision, not a
     * "quiet" decision. */
    uint32_t min_samples;
} nvband_vad_config_t;

typedef struct {
    bool     passed;
    float    measured_rms;
    uint32_t sample_count;
} nvband_vad_result_t;

/**
 * Reasonable defaults for a fixed enrollment/verification phrase spoken
 * close to the device. Named here as a starting point pending real-mic
 * bring-up (U21, `TODO(OI-6)`) — not claimed as a clinically or
 * acoustically validated threshold.
 */
nvband_vad_config_t nvband_vad_default_config(void);

/**
 * Evaluates [samples, samples+sample_count) against config. Writes a
 * full result (passed/measured_rms/sample_count) to *out. Aborts to
 * passed=false (rather than guessing) when sample_count < min_samples,
 * when samples is NULL, or when measured RMS < rms_threshold.
 */
void nvband_vad_evaluate(const int16_t *samples, uint32_t sample_count,
                          const nvband_vad_config_t *config,
                          nvband_vad_result_t *out);

#endif /* NVBAND_VOICE_ACTIVITY_GATE_H */
