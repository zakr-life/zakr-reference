#include "charge_balance.h"
#include "../../shared/nvband_constants.h"
#include <math.h>
#include <string.h>

bool nvband_charge_balance_validate(const nvband_stim_waveform_t *waveform,
                                     nvband_charge_balance_result_t *out)
{
    if (out == NULL) {
        return false;
    }
    memset(out, 0, sizeof(*out));

    if (waveform == NULL || waveform->samples_uA == NULL ||
        waveform->sample_count == 0 || waveform->sample_period_us == 0) {
        /* Malformed input is never "safe by default." */
        return false;
    }

    /* Trapezoidal-ish rectangular integration: each sample held for
     * sample_period_us contributes charge = current * time. Working in
     * microamps * microseconds gives picocoulombs; convert to
     * microcoulombs at the end to match the constants' units. */
    double charge_pC = 0.0;
    int32_t peak_uA = 0;

    for (size_t i = 0; i < waveform->sample_count; i++) {
        int32_t s = waveform->samples_uA[i];
        int32_t abs_s = (s < 0) ? -s : s;
        if (abs_s > peak_uA) {
            peak_uA = abs_s;
        }
        charge_pC += (double)s * (double)waveform->sample_period_us;
    }

    /* uA * us = pC (1e-6 * 1e-6 = 1e-12); 1 uC = 1e6 pC. */
    double charge_uC = charge_pC / 1.0e6;
    double peak_mA = (double)peak_uA / 1000.0;

    out->net_charge_uC   = (float)charge_uC;
    out->peak_current_mA = (float)peak_mA;

    out->charge_balanced =
        fabs(charge_uC) <= (double)NVBAND_CHARGE_BALANCE_TOLERANCE_UC;

    out->within_current_ceiling =
        peak_mA <= (double)NVBAND_CURRENT_CEILING_MA;

    /* Per-phase charge: approximate a "phase" as the positive-going or
     * negative-going lobe. For a validator operating on the whole
     * commanded pair, the conservative and testable proxy is: the larger
     * of the positive-half integral and the absolute negative-half
     * integral must not exceed the per-phase ceiling. */
    double pos_pC = 0.0, neg_pC = 0.0;
    for (size_t i = 0; i < waveform->sample_count; i++) {
        double contrib = (double)waveform->samples_uA[i] *
                          (double)waveform->sample_period_us;
        if (contrib > 0) {
            pos_pC += contrib;
        } else {
            neg_pC += -contrib;
        }
    }
    double max_phase_uC = (pos_pC > neg_pC ? pos_pC : neg_pC) / 1.0e6;
    out->within_phase_charge_limit =
        max_phase_uC <= (double)NVBAND_MAX_CHARGE_PER_PHASE_UC;

    out->ok_to_transmit = out->charge_balanced &&
                           out->within_current_ceiling &&
                           out->within_phase_charge_limit;

    return true;
}
