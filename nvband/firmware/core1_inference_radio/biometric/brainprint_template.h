/**
 * brainprint_template.h — EEG feature-template extraction for "brainprint"
 * local authentication.
 *
 * Addendum 2 §A: an enrollment template is a fixed-length vector of
 * relative band-power ratios per channel, extracted from N resting-state
 * epochs over the EXISTING EEG acquisition path (no new hardware). This
 * module has nothing to do with stimulation and must never be given a
 * dependency on core0_safety_signal/safety/ or
 * core1_inference_radio/inference/stim_command_clamp.c.
 *
 * Per-channel ratios (bands sum to 1.0 within a channel) are used rather
 * than raw band power so the template is robust to session-to-session
 * amplitude differences (electrode contact, gain) that are not part of the
 * person-specific signature.
 */
#ifndef NVBAND_BRAINPRINT_TEMPLATE_H
#define NVBAND_BRAINPRINT_TEMPLATE_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define NVBAND_BRAINPRINT_CHANNELS      4u   /* matches OI-1 placeholder EEG channel count */
#define NVBAND_BRAINPRINT_BANDS         5u   /* delta/theta/alpha/beta/gamma, matches models/training convention */
#define NVBAND_BRAINPRINT_TEMPLATE_LEN  (NVBAND_BRAINPRINT_CHANNELS * NVBAND_BRAINPRINT_BANDS)
#define NVBAND_BRAINPRINT_MIN_ENROLL_EPOCHS 3u
#define NVBAND_BRAINPRINT_MAX_ENROLL_EPOCHS 16u

typedef float nvband_brainprint_band_power_t[NVBAND_BRAINPRINT_CHANNELS][NVBAND_BRAINPRINT_BANDS];
typedef float nvband_brainprint_template_t[NVBAND_BRAINPRINT_TEMPLATE_LEN];

/**
 * Extract one epoch's feature vector (per-channel band-power ratios) into
 * `out`. Returns false (and leaves `out` unmodified) on malformed input —
 * e.g. a channel whose total band power is ~0 (flat/disconnected channel),
 * which must never silently normalize to a fabricated ratio.
 */
bool nvband_brainprint_epoch_features(const nvband_brainprint_band_power_t band_power,
                                       float out[NVBAND_BRAINPRINT_TEMPLATE_LEN]);

/**
 * Average `epoch_count` already-extracted per-epoch feature vectors into
 * one enrollment template. Requires
 * NVBAND_BRAINPRINT_MIN_ENROLL_EPOCHS <= epoch_count <= NVBAND_BRAINPRINT_MAX_ENROLL_EPOCHS;
 * returns false otherwise (fails closed — never enrolls from too little
 * data, never silently truncates too much).
 */
bool nvband_brainprint_build_template(const float epoch_features[][NVBAND_BRAINPRINT_TEMPLATE_LEN],
                                       uint32_t epoch_count,
                                       nvband_brainprint_template_t out);

#endif /* NVBAND_BRAINPRINT_TEMPLATE_H */
