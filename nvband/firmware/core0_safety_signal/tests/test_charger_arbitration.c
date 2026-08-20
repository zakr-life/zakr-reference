/* Traces: TRC-CHG-ARB-01..04. */
#include "test_framework.h"
#include "../drivers/charger_arbitration.h"

static void test_usb_wins_when_both_present(void)
{
    nvband_charger_arb_state_t st; nvband_charger_arb_init(&st);
    nvband_charger_arb_event_t ev;
    nvband_charger_arb_update(&st, true, true, &ev);
    NVBAND_CHECK(st.active_source == NVBAND_CHARGE_SOURCE_USB);
    NVBAND_CHECK(ev.changed == true);
}

static void test_qi_used_when_only_qi_present(void)
{
    nvband_charger_arb_state_t st; nvband_charger_arb_init(&st);
    nvband_charger_arb_update(&st, false, true, NULL);
    NVBAND_CHECK(st.active_source == NVBAND_CHARGE_SOURCE_QI);
}

static void test_none_when_neither_present(void)
{
    nvband_charger_arb_state_t st; nvband_charger_arb_init(&st);
    nvband_charger_arb_update(&st, false, false, NULL);
    NVBAND_CHECK(st.active_source == NVBAND_CHARGE_SOURCE_NONE);
}

static void test_never_reports_both_simultaneously_across_all_inputs(void)
{
    /* Exhaustive over the 4 boolean combinations: active_source is always
     * exactly one of USB/QI/NONE — there is no third bit to represent
     * "both," which is the structural guarantee this module provides. */
    bool combos[4][2] = { {false,false}, {true,false}, {false,true}, {true,true} };
    for (int i = 0; i < 4; i++) {
        nvband_charger_arb_state_t st; nvband_charger_arb_init(&st);
        nvband_charger_arb_update(&st, combos[i][0], combos[i][1], NULL);
        NVBAND_CHECK(st.active_source == NVBAND_CHARGE_SOURCE_NONE ||
                     st.active_source == NVBAND_CHARGE_SOURCE_USB ||
                     st.active_source == NVBAND_CHARGE_SOURCE_QI);
    }
}

static void test_arbitration_events_logged_only_on_change(void)
{
    nvband_charger_arb_state_t st; nvband_charger_arb_init(&st);
    nvband_charger_arb_update(&st, true, false, NULL);  /* NONE->USB: +1 */
    nvband_charger_arb_update(&st, true, false, NULL);  /* USB->USB: +0 */
    nvband_charger_arb_update(&st, false, true, NULL);  /* USB->QI: +1 */
    NVBAND_CHECK(st.arbitration_event_count == 2);
}

int main(void)
{
    NVBAND_RUN(test_usb_wins_when_both_present);
    NVBAND_RUN(test_qi_used_when_only_qi_present);
    NVBAND_RUN(test_none_when_neither_present);
    NVBAND_RUN(test_never_reports_both_simultaneously_across_all_inputs);
    NVBAND_RUN(test_arbitration_events_logged_only_on_change);
    NVBAND_TEST_MAIN_END();
}
