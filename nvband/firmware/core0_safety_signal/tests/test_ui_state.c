/* Traces: TRC-UI-01.
 * Exhaustive test (CLAUDE.md §3.7): no two distinct fault/battery
 * conditions render identically. */
#include "test_framework.h"
#include "../safety/ui_state.h"

static void test_all_faults_pairwise_distinguishable(void)
{
    nvband_ui_pattern_t patterns[NVBAND_FAULT_COUNT];
    for (int f = 0; f < NVBAND_FAULT_COUNT; f++) {
        nvband_ui_state_t st = {
            .charge = NVBAND_CHARGE_STATE_NOT_CHARGING,
            .session = NVBAND_SESSION_STATE_IDLE,
            .fault = (nvband_fault_reason_t)f,
        };
        patterns[f] = nvband_ui_render(st);
    }

    for (int i = 0; i < NVBAND_FAULT_COUNT; i++) {
        for (int j = 0; j < NVBAND_FAULT_COUNT; j++) {
            if (i == j) continue;
            bool distinguishable =
                nvband_ui_pattern_distinguishable(&patterns[i], &patterns[j]);
            NVBAND_CHECK(distinguishable == true);
        }
    }
}

static void test_low_battery_never_looks_like_a_fault_color(void)
{
    nvband_ui_state_t low_batt = {
        NVBAND_CHARGE_STATE_NOT_CHARGING, NVBAND_SESSION_STATE_IDLE,
        NVBAND_FAULT_LOW_BATTERY
    };
    nvband_ui_state_t interlock = {
        NVBAND_CHARGE_STATE_NOT_CHARGING, NVBAND_SESSION_STATE_IDLE,
        NVBAND_FAULT_INTERLOCK_TRIPPED
    };
    nvband_ui_pattern_t pb = nvband_ui_render(low_batt);
    nvband_ui_pattern_t pi = nvband_ui_render(interlock);

    NVBAND_CHECK(pb.fault_led_color != NVBAND_LED_RED);
    NVBAND_CHECK(pi.fault_led_color == NVBAND_LED_RED);
    NVBAND_CHECK(nvband_ui_pattern_distinguishable(&pb, &pi) == true);
}

static void test_render_is_pure_and_deterministic(void)
{
    nvband_ui_state_t st = {
        NVBAND_CHARGE_STATE_CHARGING, NVBAND_SESSION_STATE_ACTIVE,
        NVBAND_FAULT_NONE
    };
    nvband_ui_pattern_t a = nvband_ui_render(st);
    nvband_ui_pattern_t b = nvband_ui_render(st);
    NVBAND_CHECK(nvband_ui_pattern_distinguishable(&a, &b) == false);
}

int main(void)
{
    NVBAND_RUN(test_all_faults_pairwise_distinguishable);
    NVBAND_RUN(test_low_battery_never_looks_like_a_fault_color);
    NVBAND_RUN(test_render_is_pure_and_deterministic);
    NVBAND_TEST_MAIN_END();
}
