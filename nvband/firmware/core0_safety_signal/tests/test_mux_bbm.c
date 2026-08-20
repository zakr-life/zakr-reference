/* Traces: TRC-MUX-01..03.
 * Asserts the break interval is never zero under any code path,
 * including simulated error recovery (rapid repeated switches). */
#include "test_framework.h"
#include "../drivers/mux_bbm.h"
#include "../../shared/nvband_constants.h"

typedef struct {
    int      off_calls;
    int      select_calls;
    uint32_t last_wait_ns;
    int      wait_calls;
    bool     off_called_since_last_select;
    bool     violation_selected_without_break; /* set true if a select ever
                                                    happened without a
                                                    preceding off+wait */
} fake_mux_hw_t;

static void fake_off(void *ctx)
{
    fake_mux_hw_t *hw = (fake_mux_hw_t *)ctx;
    hw->off_calls++;
    hw->off_called_since_last_select = true;
}

static void fake_wait(uint32_t ns, void *ctx)
{
    fake_mux_hw_t *hw = (fake_mux_hw_t *)ctx;
    hw->wait_calls++;
    hw->last_wait_ns = ns;
    if (ns == 0) {
        hw->violation_selected_without_break = true;
    }
}

static void fake_select(uint8_t input, void *ctx)
{
    (void)input;
    fake_mux_hw_t *hw = (fake_mux_hw_t *)ctx;
    if (!hw->off_called_since_last_select || hw->wait_calls == 0) {
        hw->violation_selected_without_break = true;
    }
    hw->select_calls++;
    hw->off_called_since_last_select = false;
}

static nvband_mux_hal_t make_hal(fake_mux_hw_t *hw)
{
    nvband_mux_hal_t hal = { fake_off, fake_select, fake_wait, NULL, hw };
    return hal;
}

static void test_normal_switch_uses_minimum_break(void)
{
    fake_mux_hw_t hw = {0};
    nvband_mux_hal_t hal = make_hal(&hw);
    nvband_mux_state_t st;
    nvband_mux_state_init(&st);

    nvband_mux_switch_input(&hal, &st, 3);

    NVBAND_CHECK(hw.off_calls == 1);
    NVBAND_CHECK(hw.wait_calls == 1);
    NVBAND_CHECK(hw.last_wait_ns >= NVBAND_MUX_BREAK_BEFORE_MAKE_MIN_NS);
    NVBAND_CHECK(hw.select_calls == 1);
    NVBAND_CHECK(st.current_input == 3);
    NVBAND_CHECK(hw.violation_selected_without_break == false);
}

/* Simulated rapid error-recovery: many switches back-to-back. The break
 * interval must never collapse to zero on any of them. */
static void test_rapid_error_recovery_never_skips_break(void)
{
    fake_mux_hw_t hw = {0};
    nvband_mux_hal_t hal = make_hal(&hw);
    nvband_mux_state_t st;
    nvband_mux_state_init(&st);

    for (uint8_t i = 0; i < 50; i++) {
        nvband_mux_switch_input(&hal, &st, (uint8_t)(i % 8));
        NVBAND_CHECK(hw.last_wait_ns >= NVBAND_MUX_BREAK_BEFORE_MAKE_MIN_NS);
    }
    NVBAND_CHECK(hw.wait_calls == 50);
    NVBAND_CHECK(hw.violation_selected_without_break == false);
}

static void test_switching_to_same_input_still_breaks(void)
{
    fake_mux_hw_t hw = {0};
    nvband_mux_hal_t hal = make_hal(&hw);
    nvband_mux_state_t st;
    nvband_mux_state_init(&st);

    nvband_mux_switch_input(&hal, &st, 2);
    nvband_mux_switch_input(&hal, &st, 2); /* "same" channel re-selected */

    NVBAND_CHECK(hw.off_calls == 2);
    NVBAND_CHECK(hw.wait_calls == 2);
    NVBAND_CHECK(hw.violation_selected_without_break == false);
}

int main(void)
{
    NVBAND_RUN(test_normal_switch_uses_minimum_break);
    NVBAND_RUN(test_rapid_error_recovery_never_skips_break);
    NVBAND_RUN(test_switching_to_same_input_still_breaks);
    NVBAND_TEST_MAIN_END();
}
