/**
 * charge_balance.h — Waveform charge-balance verifier.
 *
 * CLAUDE.md §3.4: "Enforce charge balance at the waveform-generation layer:
 * every commanded stimulation waveform must be validated (in code, with a
 * test) to integrate to zero net charge before it is transmitted to the
 * DAC, in addition to (not instead of) the hardware DC-blocking capacitor."
 *
 * This module is pure, host-testable logic with no hardware dependency:
 * it operates on an already-sampled/commanded waveform buffer and answers
 * "is this safe to transmit," never issues the transmit itself. It is
 * DEFENSE IN DEPTH — see nvband_constants.h and CLAUDE.md §0.1: this
 * check can only ever REFUSE to send a waveform to the DAC. It has no path
 * to assert stimulation enable; that remains the hardware interlock AND
 * of seven conditions (see interlock_status.h).
 */
#ifndef NVBAND_CHARGE_BALANCE_H
#define NVBAND_CHARGE_BALANCE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** One commanded stimulation waveform: signed current samples in
 *  micro-amps, uniformly spaced at sample_period_us. */
typedef struct {
    const int32_t *samples_uA;
    size_t         sample_count;
    uint32_t       sample_period_us;
} nvband_stim_waveform_t;

typedef struct {
    float    net_charge_uC;         /* signed integral over the waveform  */
    float    peak_current_mA;       /* max |sample|, for ceiling check    */
    bool     charge_balanced;       /* |net_charge_uC| <= tolerance       */
    bool     within_current_ceiling;
    bool     within_phase_charge_limit;
    bool     ok_to_transmit;        /* AND of all checks above            */
} nvband_charge_balance_result_t;

/**
 * Validate a commanded waveform against charge-balance, current-ceiling,
 * and per-phase charge limits (nvband_constants.h). Pure function: no
 * side effects, no hardware access. Returns false (and zeroed result) on
 * a malformed waveform (null buffer, zero samples, zero period) — treated
 * as "not ok to transmit," never as "assume safe."
 */
bool nvband_charge_balance_validate(const nvband_stim_waveform_t *waveform,
                                     nvband_charge_balance_result_t *out);

#endif /* NVBAND_CHARGE_BALANCE_H */
