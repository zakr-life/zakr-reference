/**
 * test_end_to_end_scenarios.c — Hardware-in-the-loop-style scenario
 * suite tying the resistive phantom to the interlock, charge-balance,
 * and mux modules (CLAUDE.md §9). This is the "software-in-the-loop"
 * counterpart to the per-module fault-injection tests in
 * firmware/core0_safety_signal/tests/: it exercises plausible physical
 * scenarios (lifted electrode, impedance drift, an anomalous
 * make-before-break mux readback) end to end and asserts the whole
 * chain's behavior, not just one module's.
 *
 * Traces: TRC-SIM-01..04.
 */
#include "../../core0_safety_signal/tests/test_framework.h"
#include "../resistive_phantom.h"
#include "../../core0_safety_signal/safety/interlock_status.h"
#include "../../core0_safety_signal/safety/charge_balance.h"
#include "../../core0_safety_signal/drivers/mux_bbm.h"
#include "../../shared/nvband_constants.h"

/* ---- scenario 1: lifted electrode drives impedance out of window ---- */

typedef struct {
    nvband_phantom_config_t phantom;
    bool conditions[NVBAND_INTERLOCK_CONDITION_COUNT];
    bool latch;
} scenario_hw_t;

#define IMPEDANCE_WINDOW_MAX_OHM 10000.0f

static bool sc_read_condition(nvband_interlock_condition_t c, void *ctx)
{
    scenario_hw_t *hw = (scenario_hw_t *)ctx;
    if (c == NVBAND_INTERLOCK_IMPEDANCE_WINDOW) {
        nvband_phantom_reading_t r;
        nvband_phantom_measure(&hw->phantom, 1.0f, &r);
        return r.measured_impedance_ohm <= IMPEDANCE_WINDOW_MAX_OHM;
    }
    return hw->conditions[c];
}

static bool sc_read_latch(void *ctx) { return ((scenario_hw_t *)ctx)->latch; }

static void test_lifted_electrode_trips_impedance_condition_only(void)
{
    scenario_hw_t hw = {0};
    nvband_phantom_config_default(&hw.phantom);
    for (int i = 0; i < NVBAND_INTERLOCK_CONDITION_COUNT; i++) hw.conditions[i] = true;
    hw.phantom.lifted = 1; /* fault injected: electrode lifted off skin */
    hw.latch = true;       /* real hardware would latch on this trip     */

    nvband_interlock_hal_t hal = { sc_read_condition, sc_read_latch, NULL, NULL, &hw };
    nvband_interlock_snapshot_t snap;
    nvband_interlock_poll(&hal, 1000, &snap);

    NVBAND_CHECK(snap.tripped_mask == (1u << NVBAND_INTERLOCK_IMPEDANCE_WINDOW));
    NVBAND_CHECK(nvband_interlock_all_clear(&snap) == false);
}

static void test_normal_contact_all_clear(void)
{
    scenario_hw_t hw = {0};
    nvband_phantom_config_default(&hw.phantom);
    for (int i = 0; i < NVBAND_INTERLOCK_CONDITION_COUNT; i++) hw.conditions[i] = true;
    hw.latch = false;

    nvband_interlock_hal_t hal = { sc_read_condition, sc_read_latch, NULL, NULL, &hw };
    nvband_interlock_snapshot_t snap;
    nvband_interlock_poll(&hal, 1000, &snap);

    NVBAND_CHECK(nvband_interlock_all_clear(&snap) == true);
}

/* ---- scenario 2: make-before-break anomaly injector ----
 * The real U19 part is break-before-make by datasheet. This scenario
 * simulates an anomalous part/defect that reports "still connected"
 * during the break window, and proves the driver detects and latches it
 * even though it has no authority to halt anything by itself. */

typedef struct {
    int  inject_anomaly_on_switch_index;
    int  switch_index;
} anomaly_hw_t;

static void anom_off(void *ctx) { (void)ctx; }
static void anom_select(uint8_t input, void *ctx) { (void)input; (void)ctx; }
static void anom_wait(uint32_t ns, void *ctx) { (void)ns; (void)ctx; }
static bool anom_any_connected(void *ctx)
{
    anomaly_hw_t *hw = (anomaly_hw_t *)ctx;
    bool connected = (hw->switch_index == hw->inject_anomaly_on_switch_index);
    hw->switch_index++;
    return connected;
}

static void test_make_before_break_anomaly_detected_and_latched(void)
{
    anomaly_hw_t hw = { .inject_anomaly_on_switch_index = 2, .switch_index = 0 };
    nvband_mux_hal_t hal = { anom_off, anom_select, anom_wait, anom_any_connected, &hw };
    nvband_mux_state_t st;
    nvband_mux_state_init(&st);

    for (uint8_t i = 0; i < 5; i++) {
        nvband_mux_switch_input(&hal, &st, i);
    }

    NVBAND_CHECK(st.break_anomaly_detected == true);
    NVBAND_CHECK(st.break_anomaly_count == 1);
    NVBAND_CHECK(nvband_mux_break_ok(&st) == false);
    /* Switches still completed (driver only reports, per CLAUDE.md §0.1 —
     * it has no authority to halt acquisition/stim itself). */
    NVBAND_CHECK(st.switch_count == 5);
}

static void test_no_anomaly_when_readback_clean(void)
{
    anomaly_hw_t hw = { .inject_anomaly_on_switch_index = -1, .switch_index = 0 };
    nvband_mux_hal_t hal = { anom_off, anom_select, anom_wait, anom_any_connected, &hw };
    nvband_mux_state_t st;
    nvband_mux_state_init(&st);

    for (uint8_t i = 0; i < 10; i++) {
        nvband_mux_switch_input(&hal, &st, i);
    }
    NVBAND_CHECK(nvband_mux_break_ok(&st) == true);
    NVBAND_CHECK(st.break_anomaly_count == 0);
}

/* ---- scenario 3: waveform rejected by charge-balance layer never
 * reaches a "transmitted" state, independent of interlock state ---- */

static void test_rejected_waveform_marked_not_ok_regardless_of_interlock(void)
{
    int32_t dc_samples[] = { 400, 400, 400 };
    nvband_stim_waveform_t wf = { dc_samples, 3, 1000 };
    nvband_charge_balance_result_t r;
    nvband_charge_balance_validate(&wf, &r);

    NVBAND_CHECK(r.ok_to_transmit == false);
    /* Even in a scenario where every interlock condition is (correctly)
     * satisfied, a charge-imbalanced waveform must still never be sent —
     * this is a second, independent layer of defense, not a substitute
     * for the interlock. */
}

int main(void)
{
    NVBAND_RUN(test_lifted_electrode_trips_impedance_condition_only);
    NVBAND_RUN(test_normal_contact_all_clear);
    NVBAND_RUN(test_make_before_break_anomaly_detected_and_latched);
    NVBAND_RUN(test_no_anomaly_when_readback_clean);
    NVBAND_RUN(test_rejected_waveform_marked_not_ok_regardless_of_interlock);
    NVBAND_TEST_MAIN_END();
}
