/**
 * ui_state.h — Three-LED + buzzer state model (CLAUDE.md §3.7).
 *
 * Encodes charge state, session state, and fault state as an explicit
 * state machine, with a render function producing the physical LED/buzzer
 * pattern for a given state tuple. The accompanying test
 * (tests/test_ui_state.c) exhaustively asserts that no two distinct
 * fault/battery conditions render identically — a wearer or caregiver
 * must always be able to visually/audibly distinguish "fault" from
 * "just low on charge."
 *
 * This module is purely observational/informational: it has no authority
 * over stimulation (see interlock_status.h) and does not gate anything.
 */
#ifndef NVBAND_UI_STATE_H
#define NVBAND_UI_STATE_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    NVBAND_CHARGE_STATE_NOT_CHARGING = 0,
    NVBAND_CHARGE_STATE_CHARGING,
    NVBAND_CHARGE_STATE_CHARGED,
    NVBAND_CHARGE_STATE_CHARGE_FAULT,
    NVBAND_CHARGE_STATE_COUNT
} nvband_charge_state_t;

typedef enum {
    NVBAND_SESSION_STATE_IDLE = 0,
    NVBAND_SESSION_STATE_ARMED,
    NVBAND_SESSION_STATE_ACTIVE,
    NVBAND_SESSION_STATE_PAUSED_FAULT,
    NVBAND_SESSION_STATE_COUNT
} nvband_session_state_t;

typedef enum {
    NVBAND_FAULT_NONE = 0,
    NVBAND_FAULT_INTERLOCK_TRIPPED,
    NVBAND_FAULT_ELECTRODE_CONTACT,
    NVBAND_FAULT_WATCHDOG_RESET,
    NVBAND_FAULT_LOW_BATTERY,
    NVBAND_FAULT_STORAGE_FULL,
    NVBAND_FAULT_PROVISIONING_INVALID,
    NVBAND_FAULT_COUNT
} nvband_fault_reason_t;

typedef enum { NVBAND_LED_OFF = 0, NVBAND_LED_GREEN, NVBAND_LED_AMBER,
               NVBAND_LED_RED, NVBAND_LED_BLUE } nvband_led_color_t;

typedef enum { NVBAND_BLINK_SOLID = 0, NVBAND_BLINK_SLOW,
               NVBAND_BLINK_FAST, NVBAND_BLINK_DOUBLE_PULSE } nvband_blink_pattern_t;

typedef enum { NVBAND_BUZZER_SILENT = 0, NVBAND_BUZZER_SINGLE_CHIRP,
               NVBAND_BUZZER_TRIPLE_CHIRP, NVBAND_BUZZER_CONTINUOUS } nvband_buzzer_pattern_t;

typedef struct {
    nvband_led_color_t     charge_led_color;
    nvband_blink_pattern_t charge_led_blink;
    nvband_led_color_t     session_led_color;
    nvband_blink_pattern_t session_led_blink;
    nvband_led_color_t     fault_led_color;
    nvband_blink_pattern_t fault_led_blink;
    nvband_buzzer_pattern_t buzzer;
} nvband_ui_pattern_t;

typedef struct {
    nvband_charge_state_t   charge;
    nvband_session_state_t  session;
    nvband_fault_reason_t   fault;      /* NVBAND_FAULT_NONE if no fault  */
} nvband_ui_state_t;

/** Deterministic, pure render function: same state always renders the
 *  same pattern. No hidden state, no hardware access. */
nvband_ui_pattern_t nvband_ui_render(nvband_ui_state_t state);

/** Compare two patterns for a human-distinguishable difference (LED
 *  color/blink or buzzer pattern differ). Used by the exhaustiveness
 *  test to prove low-battery never looks/sounds identical to a fault. */
bool nvband_ui_pattern_distinguishable(const nvband_ui_pattern_t *a,
                                        const nvband_ui_pattern_t *b);

#endif /* NVBAND_UI_STATE_H */
