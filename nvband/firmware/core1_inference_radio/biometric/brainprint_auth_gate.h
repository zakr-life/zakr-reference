/**
 * brainprint_auth_gate.h — local-authentication decision for gating
 * decryption of the app's local session cache.
 *
 * Addendum 2 §A: brainprint is an ADDITIVE local factor, never a source of
 * cryptographic key material and never the sole path to a legitimate
 * user's own data. The hard invariant this module exists to guarantee:
 * there is NO way to express a permanent, no-fallback denial through this
 * API. `nvband_brainprint_gate_result_t` has exactly two values on
 * purpose -- a caller cannot construct a "locked out forever" outcome
 * because the type system doesn't have one. A brainprint mismatch always
 * routes to NVBAND_BRAINPRINT_GATE_RESULT_FALLBACK, meaning "use the
 * conventional device PIN/passcode path instead" -- it is the caller's
 * (app's) job to make that fallback path actually reachable in the UI,
 * which is independently tested at the app layer
 * (app/mobile/src/state/brainprintGate.js).
 *
 * This module has no dependency on and must never gain a dependency on
 * core0_safety_signal/safety/ or
 * core1_inference_radio/inference/stim_command_clamp.c -- it has nothing
 * to do with stimulation.
 */
#ifndef NVBAND_BRAINPRINT_AUTH_GATE_H
#define NVBAND_BRAINPRINT_AUTH_GATE_H

#include <stdbool.h>

typedef enum {
    NVBAND_BRAINPRINT_GATE_RESULT_MATCH = 0,   /* brainprint matched: unlock via brainprint */
    NVBAND_BRAINPRINT_GATE_RESULT_FALLBACK,    /* no match (or not attempted): use conventional auth */
} nvband_brainprint_gate_result_t;

/**
 * Pure decision function. `brainprint_matched` is the already-computed
 * result of nvband_brainprint_matches(); this function adds no fuzziness
 * of its own -- if brainprint_matched is true, result is MATCH, otherwise
 * FALLBACK, unconditionally. There is no input that can make this
 * function do anything else, by construction.
 */
nvband_brainprint_gate_result_t nvband_brainprint_gate_evaluate(bool brainprint_matched);

#endif /* NVBAND_BRAINPRINT_AUTH_GATE_H */
