#include "ui_state.h"
#include <stddef.h>

/* Fault-LED + buzzer encoding table. Each row is constructed to be
 * pairwise distinct from every other row (by color, blink pattern, or
 * buzzer pattern) — verified exhaustively by
 * tests/test_ui_state.c::test_all_faults_pairwise_distinguishable().
 * NVBAND_FAULT_LOW_BATTERY is deliberately encoded with AMBER (not RED)
 * so "low battery" never shares the fault-red visual language, per
 * CLAUDE.md §3.7's explicit requirement that fault be distinguishable
 * from low-battery. */
static const struct {
    nvband_led_color_t      color;
    nvband_blink_pattern_t  blink;
    nvband_buzzer_pattern_t buzzer;
} k_fault_table[NVBAND_FAULT_COUNT] = {
    [NVBAND_FAULT_NONE]                 = { NVBAND_LED_OFF,   NVBAND_BLINK_SOLID,        NVBAND_BUZZER_SILENT },
    [NVBAND_FAULT_INTERLOCK_TRIPPED]    = { NVBAND_LED_RED,   NVBAND_BLINK_SOLID,        NVBAND_BUZZER_CONTINUOUS },
    [NVBAND_FAULT_ELECTRODE_CONTACT]    = { NVBAND_LED_RED,   NVBAND_BLINK_SLOW,         NVBAND_BUZZER_SINGLE_CHIRP },
    [NVBAND_FAULT_WATCHDOG_RESET]       = { NVBAND_LED_RED,   NVBAND_BLINK_FAST,         NVBAND_BUZZER_TRIPLE_CHIRP },
    [NVBAND_FAULT_LOW_BATTERY]          = { NVBAND_LED_AMBER, NVBAND_BLINK_SLOW,         NVBAND_BUZZER_SINGLE_CHIRP },
    [NVBAND_FAULT_STORAGE_FULL]         = { NVBAND_LED_RED,   NVBAND_BLINK_DOUBLE_PULSE, NVBAND_BUZZER_SILENT },
    [NVBAND_FAULT_PROVISIONING_INVALID] = { NVBAND_LED_RED,   NVBAND_BLINK_DOUBLE_PULSE, NVBAND_BUZZER_SINGLE_CHIRP },
};

static nvband_led_color_t charge_led_color(nvband_charge_state_t s)
{
    switch (s) {
    case NVBAND_CHARGE_STATE_NOT_CHARGING: return NVBAND_LED_OFF;
    case NVBAND_CHARGE_STATE_CHARGING:     return NVBAND_LED_AMBER;
    case NVBAND_CHARGE_STATE_CHARGED:      return NVBAND_LED_GREEN;
    case NVBAND_CHARGE_STATE_CHARGE_FAULT: return NVBAND_LED_RED;
    default:                               return NVBAND_LED_OFF;
    }
}

static nvband_blink_pattern_t charge_led_blink(nvband_charge_state_t s)
{
    return (s == NVBAND_CHARGE_STATE_CHARGING) ? NVBAND_BLINK_SLOW
                                                : NVBAND_BLINK_SOLID;
}

static nvband_led_color_t session_led_color(nvband_session_state_t s)
{
    switch (s) {
    case NVBAND_SESSION_STATE_IDLE:          return NVBAND_LED_OFF;
    case NVBAND_SESSION_STATE_ARMED:         return NVBAND_LED_BLUE;
    case NVBAND_SESSION_STATE_ACTIVE:        return NVBAND_LED_GREEN;
    case NVBAND_SESSION_STATE_PAUSED_FAULT:  return NVBAND_LED_AMBER;
    default:                                 return NVBAND_LED_OFF;
    }
}

static nvband_blink_pattern_t session_led_blink(nvband_session_state_t s)
{
    if (s == NVBAND_SESSION_STATE_ACTIVE)       return NVBAND_BLINK_SLOW;
    if (s == NVBAND_SESSION_STATE_PAUSED_FAULT) return NVBAND_BLINK_FAST;
    return NVBAND_BLINK_SOLID;
}

nvband_ui_pattern_t nvband_ui_render(nvband_ui_state_t state)
{
    nvband_ui_pattern_t p;

    p.charge_led_color  = charge_led_color(state.charge);
    p.charge_led_blink  = charge_led_blink(state.charge);
    p.session_led_color = session_led_color(state.session);
    p.session_led_blink = session_led_blink(state.session);

    nvband_fault_reason_t f = state.fault;
    if (f < 0 || f >= NVBAND_FAULT_COUNT) {
        f = NVBAND_FAULT_INTERLOCK_TRIPPED; /* unknown fault: fail loud, never silent */
    }
    p.fault_led_color = k_fault_table[f].color;
    p.fault_led_blink = k_fault_table[f].blink;
    p.buzzer          = k_fault_table[f].buzzer;

    return p;
}

bool nvband_ui_pattern_distinguishable(const nvband_ui_pattern_t *a,
                                        const nvband_ui_pattern_t *b)
{
    if (a == NULL || b == NULL) {
        return false;
    }
    return (a->fault_led_color != b->fault_led_color) ||
           (a->fault_led_blink != b->fault_led_blink) ||
           (a->buzzer          != b->buzzer);
}
