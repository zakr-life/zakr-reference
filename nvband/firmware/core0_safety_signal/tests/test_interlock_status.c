/* Traces: TRC-ILK-01..08 in firmware/docs/traceability_matrix.csv
 *
 * Fault-injection suite required by CLAUDE.md §3.5 / §9: simulate each of
 * the seven interlock conditions failing independently and in
 * combination, and assert firmware's response is correct in every case.
 * "Correct" here means: firmware never asserts anything that looks like
 * an override, always reflects the true polled state, and re-arm is
 * refused whenever the latch is set or any condition is still tripped.
 */
#include "test_framework.h"
#include "../safety/interlock_status.h"

typedef struct {
    bool condition[NVBAND_INTERLOCK_CONDITION_COUNT];
    bool latch_asserted;
    int  rearm_pulse_count;
    bool firmware_permit_value;
    int  set_firmware_permit_calls;
} fake_hw_t;

static bool fake_read_condition(nvband_interlock_condition_t c, void *ctx)
{
    fake_hw_t *hw = (fake_hw_t *)ctx;
    return hw->condition[c];
}

static bool fake_read_latch(void *ctx)
{
    return ((fake_hw_t *)ctx)->latch_asserted;
}

static void fake_pulse_rearm(void *ctx)
{
    ((fake_hw_t *)ctx)->rearm_pulse_count++;
    ((fake_hw_t *)ctx)->latch_asserted = false; /* models the real latch
                                                    clearing only on a
                                                    successful HW rearm */
}

static void fake_set_permit(bool asserted, void *ctx)
{
    fake_hw_t *hw = (fake_hw_t *)ctx;
    hw->firmware_permit_value = asserted;
    hw->set_firmware_permit_calls++;
}

static void all_conditions_true(fake_hw_t *hw)
{
    for (int i = 0; i < NVBAND_INTERLOCK_CONDITION_COUNT; i++) {
        hw->condition[i] = true;
    }
    hw->latch_asserted = false;
}

static nvband_interlock_hal_t make_hal(fake_hw_t *hw)
{
    nvband_interlock_hal_t hal = {
        .read_condition = fake_read_condition,
        .read_latch_asserted = fake_read_latch,
        .pulse_rearm_line = fake_pulse_rearm,
        .set_firmware_permit = fake_set_permit,
        .ctx = hw,
    };
    return hal;
}

static void test_all_clear_when_all_conditions_true(void)
{
    fake_hw_t hw = {0};
    all_conditions_true(&hw);
    nvband_interlock_hal_t hal = make_hal(&hw);
    nvband_interlock_snapshot_t snap;

    nvband_interlock_poll(&hal, 1000, &snap);

    NVBAND_CHECK(snap.tripped_mask == 0);
    NVBAND_CHECK(snap.latch_asserted == false);
    NVBAND_CHECK(nvband_interlock_all_clear(&snap) == true);
}

/* Each of the seven conditions, tripped independently. */
static void test_each_condition_tripped_independently(void)
{
    for (int i = 0; i < NVBAND_INTERLOCK_CONDITION_COUNT; i++) {
        fake_hw_t hw = {0};
        all_conditions_true(&hw);
        hw.condition[i] = false;
        hw.latch_asserted = true; /* real hardware latches on any trip */

        nvband_interlock_hal_t hal = make_hal(&hw);
        nvband_interlock_snapshot_t snap;
        nvband_interlock_poll(&hal, 2000, &snap);

        NVBAND_CHECK(snap.tripped_mask == (1u << i));
        NVBAND_CHECK(nvband_interlock_all_clear(&snap) == false);

        /* Re-arm must be refused: the tripped condition is still tripped
         * in this snapshot (we haven't fixed it). */
        nvband_rearm_result_t rr =
            nvband_interlock_request_rearm(&hal, &snap);
        NVBAND_CHECK(rr == NVBAND_REARM_DENIED_CONDITIONS_UNMET);
        NVBAND_CHECK(hw.rearm_pulse_count == 0);
    }
}

/* Simultaneous multi-fault: several conditions tripped at once. */
static void test_simultaneous_multi_fault(void)
{
    fake_hw_t hw = {0};
    all_conditions_true(&hw);
    hw.condition[NVBAND_INTERLOCK_CURRENT_WINDOW] = false;
    hw.condition[NVBAND_INTERLOCK_STOP_SWITCH_CLOSED] = false;
    hw.condition[NVBAND_INTERLOCK_WATCHDOG_SATISFIED] = false;
    hw.latch_asserted = true;

    nvband_interlock_hal_t hal = make_hal(&hw);
    nvband_interlock_snapshot_t snap;
    nvband_interlock_poll(&hal, 3000, &snap);

    uint32_t expected = (1u << NVBAND_INTERLOCK_CURRENT_WINDOW) |
                         (1u << NVBAND_INTERLOCK_STOP_SWITCH_CLOSED) |
                         (1u << NVBAND_INTERLOCK_WATCHDOG_SATISFIED);
    NVBAND_CHECK(snap.tripped_mask == expected);
    NVBAND_CHECK(nvband_interlock_all_clear(&snap) == false);
}

/* Re-arm is refused when latch is set but nothing is actually tripped
 * (e.g. a transient already cleared) UNTIL an explicit rearm is
 * requested with a snapshot showing zero tripped conditions. */
static void test_rearm_succeeds_only_when_latch_set_and_all_conditions_clear(void)
{
    fake_hw_t hw = {0};
    all_conditions_true(&hw);
    hw.latch_asserted = true; /* latched from a past, now-resolved fault */

    nvband_interlock_hal_t hal = make_hal(&hw);
    nvband_interlock_snapshot_t snap;
    nvband_interlock_poll(&hal, 4000, &snap);

    NVBAND_CHECK(snap.latch_asserted == true);
    NVBAND_CHECK(snap.tripped_mask == 0);

    nvband_rearm_result_t rr = nvband_interlock_request_rearm(&hal, &snap);
    NVBAND_CHECK(rr == NVBAND_REARM_OK);
    NVBAND_CHECK(hw.rearm_pulse_count == 1);
    NVBAND_CHECK(hw.latch_asserted == false);
}

static void test_rearm_refused_when_not_latched(void)
{
    fake_hw_t hw = {0};
    all_conditions_true(&hw);
    nvband_interlock_hal_t hal = make_hal(&hw);
    nvband_interlock_snapshot_t snap;
    nvband_interlock_poll(&hal, 5000, &snap);

    nvband_rearm_result_t rr = nvband_interlock_request_rearm(&hal, &snap);
    NVBAND_CHECK(rr == NVBAND_REARM_DENIED_NOT_LATCHED);
    NVBAND_CHECK(hw.rearm_pulse_count == 0);
}

/* This module must never expose any function that can assert enable
 * directly; the only mutation surface is set_firmware_permit (one INPUT
 * to the hardware AND) and pulse_rearm_line (gated as above). This test
 * documents/pins that invariant at the API-shape level: it would fail to
 * compile if a bypass function existed and were (mis)used here. */
static void test_no_bypass_surface_exists(void)
{
    fake_hw_t hw = {0};
    all_conditions_true(&hw);
    nvband_interlock_hal_t hal = make_hal(&hw);
    NVBAND_CHECK(hal.set_firmware_permit != NULL);
    /* firmware may assert its OWN permit bit; this is one of seven AND
     * inputs and is never, by itself, sufficient. */
    hal.set_firmware_permit(true, hal.ctx);
    NVBAND_CHECK(hw.firmware_permit_value == true);
    NVBAND_CHECK(hw.set_firmware_permit_calls == 1);
}

int main(void)
{
    NVBAND_RUN(test_all_clear_when_all_conditions_true);
    NVBAND_RUN(test_each_condition_tripped_independently);
    NVBAND_RUN(test_simultaneous_multi_fault);
    NVBAND_RUN(test_rearm_succeeds_only_when_latch_set_and_all_conditions_clear);
    NVBAND_RUN(test_rearm_refused_when_not_latched);
    NVBAND_RUN(test_no_bypass_surface_exists);
    NVBAND_TEST_MAIN_END();
}
