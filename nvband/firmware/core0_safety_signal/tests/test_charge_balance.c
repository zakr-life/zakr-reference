/* Traces: TRC-CHG-01..05 in firmware/docs/traceability_matrix.csv */
#include "test_framework.h"
#include "../safety/charge_balance.h"
#include "../../shared/nvband_constants.h"

static void test_balanced_biphasic_pulse_passes(void)
{
    /* Symmetric biphasic pulse: +500uA for 1000us, then -500uA for
     * 1000us. Net charge = 0. */
    int32_t samples[] = { 500, 500, -500, -500 };
    nvband_stim_waveform_t wf = { samples, 4, 500 };
    nvband_charge_balance_result_t r;

    NVBAND_CHECK(nvband_charge_balance_validate(&wf, &r) == true);
    NVBAND_CHECK(r.charge_balanced == true);
    NVBAND_CHECK(r.ok_to_transmit == true);
}

static void test_dc_offset_waveform_rejected(void)
{
    /* All-positive waveform: massive net DC charge -> must be rejected,
     * regardless of amplitude being under the current ceiling. */
    int32_t samples[] = { 300, 300, 300, 300 };
    nvband_stim_waveform_t wf = { samples, 4, 500 };
    nvband_charge_balance_result_t r;

    NVBAND_CHECK(nvband_charge_balance_validate(&wf, &r) == true);
    NVBAND_CHECK(r.charge_balanced == false);
    NVBAND_CHECK(r.ok_to_transmit == false);
}

static void test_over_ceiling_current_rejected_even_if_balanced(void)
{
    /* Perfectly balanced, but peak current exceeds the derived ceiling
     * (NVBAND_CURRENT_CEILING_MA ~= 2.01 mA for the Ø16mm pad). A huge
     * peak must be rejected even though net charge is exactly zero. */
    int32_t samples[] = { 50000, -50000 }; /* 50 mA peak */
    nvband_stim_waveform_t wf = { samples, 2, 500 };
    nvband_charge_balance_result_t r;

    NVBAND_CHECK(nvband_charge_balance_validate(&wf, &r) == true);
    NVBAND_CHECK(r.charge_balanced == true);
    NVBAND_CHECK(r.within_current_ceiling == false);
    NVBAND_CHECK(r.ok_to_transmit == false);
}

static void test_over_phase_charge_rejected(void)
{
    /* Balanced and under the current ceiling, but each phase individually
     * carries more than NVBAND_MAX_CHARGE_PER_PHASE_UC (1.0 uC): a long,
     * low-amplitude phase can still smuggle excess charge per phase even
     * while staying under the instantaneous current ceiling. */
    int32_t samples[2000];
    for (int i = 0; i < 1000; i++) samples[i] = 1900;   /* +1.9 mA */
    for (int i = 1000; i < 2000; i++) samples[i] = -1900;
    nvband_stim_waveform_t wf = { samples, 2000, 1000 }; /* 1ms/sample */
    nvband_charge_balance_result_t r;

    NVBAND_CHECK(nvband_charge_balance_validate(&wf, &r) == true);
    NVBAND_CHECK(r.charge_balanced == true);
    NVBAND_CHECK(r.within_current_ceiling == true);
    NVBAND_CHECK(r.within_phase_charge_limit == false);
    NVBAND_CHECK(r.ok_to_transmit == false);
}

static void test_malformed_waveform_never_defaults_safe(void)
{
    nvband_charge_balance_result_t r;
    NVBAND_CHECK(nvband_charge_balance_validate(NULL, &r) == false);
    NVBAND_CHECK(r.ok_to_transmit == false);

    nvband_stim_waveform_t wf = { NULL, 0, 0 };
    NVBAND_CHECK(nvband_charge_balance_validate(&wf, &r) == false);
    NVBAND_CHECK(r.ok_to_transmit == false);

    NVBAND_CHECK(nvband_charge_balance_validate(&wf, NULL) == false);
}

int main(void)
{
    NVBAND_RUN(test_balanced_biphasic_pulse_passes);
    NVBAND_RUN(test_dc_offset_waveform_rejected);
    NVBAND_RUN(test_over_ceiling_current_rejected_even_if_balanced);
    NVBAND_RUN(test_over_phase_charge_rejected);
    NVBAND_RUN(test_malformed_waveform_never_defaults_safe);
    NVBAND_TEST_MAIN_END();
}
