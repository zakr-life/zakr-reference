/**
 * brainprint_matcher.h — 1:1 similarity match of a live EEG template
 * against one enrolled template.
 *
 * Addendum 2 §A. This is a LOCAL comparison only (one live vector against
 * one caller-supplied enrolled vector) — there is deliberately no function
 * here that searches or ranks across multiple enrolled templates.
 */
#ifndef NVBAND_BRAINPRINT_MATCHER_H
#define NVBAND_BRAINPRINT_MATCHER_H

#include <stdbool.h>
#include "brainprint_template.h"

/** Default match threshold on cosine similarity, [-1, 1]. Provisioned
 *  constant, not a claim of a validated false-accept/false-reject
 *  operating point -- see TODO(OI-7). */
#define NVBAND_BRAINPRINT_DEFAULT_THRESHOLD 0.90f

/**
 * Cosine similarity of two template vectors. Returns 0.0f (never 1.0f --
 * fails closed toward "no match") if either vector is NULL or has ~zero
 * magnitude, since a degenerate vector carries no biometric signal.
 */
float nvband_brainprint_cosine_similarity(const float a[NVBAND_BRAINPRINT_TEMPLATE_LEN],
                                           const float b[NVBAND_BRAINPRINT_TEMPLATE_LEN]);

/**
 * True if `live` matches `enrolled` at or above `threshold`. False on any
 * malformed input (NULL live/enrolled, threshold outside [-1, 1]) -- a
 * matcher error is always "no match", never "match".
 */
bool nvband_brainprint_matches(const float live[NVBAND_BRAINPRINT_TEMPLATE_LEN],
                                const float enrolled[NVBAND_BRAINPRINT_TEMPLATE_LEN],
                                float threshold);

#endif /* NVBAND_BRAINPRINT_MATCHER_H */
