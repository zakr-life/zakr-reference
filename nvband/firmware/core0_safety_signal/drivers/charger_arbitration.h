/**
 * charger_arbitration.h — Dual-input (USB + Qi) charge source arbitration.
 *
 * CLAUDE.md §3.1: "Implement dual-input charge arbitration (USB vs. Qi)
 * in supervisory firmware: never allow both sources to actively source
 * into the charger simultaneously; log every arbitration event."
 *
 * U12 (BQ25180/BQ24074-class) provides input presence detection in
 * hardware; this module is the supervisory POLICY that decides which one
 * input, if any, is allowed to actively source, and emits a log entry on
 * every transition. It never asserts anything itself — it returns a
 * decision the caller applies via the charger IC's enable pins.
 */
#ifndef NVBAND_CHARGER_ARBITRATION_H
#define NVBAND_CHARGER_ARBITRATION_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    NVBAND_CHARGE_SOURCE_NONE = 0,
    NVBAND_CHARGE_SOURCE_USB,
    NVBAND_CHARGE_SOURCE_QI,
} nvband_charge_source_t;

typedef struct {
    nvband_charge_source_t active_source;
    uint32_t                arbitration_event_count;
} nvband_charger_arb_state_t;

typedef struct {
    nvband_charge_source_t previous_source;
    nvband_charge_source_t new_source;
    bool                    changed;
} nvband_charger_arb_event_t;

void nvband_charger_arb_init(nvband_charger_arb_state_t *state);

/**
 * usb_present / qi_present are the hardware presence-detect readbacks.
 * Policy: USB takes priority if both are present simultaneously (wired
 * connection implies deliberate, supervised charging; arbitrarily but
 * deterministically documented here rather than left to hardware race).
 * Exactly one of {USB, Qi, NONE} is ever the active source — the
 * invariant this module exists to guarantee is that it NEVER returns a
 * state implying both are simultaneously active.
 */
void nvband_charger_arb_update(nvband_charger_arb_state_t *state,
                                bool usb_present, bool qi_present,
                                nvband_charger_arb_event_t *event_out);

#endif /* NVBAND_CHARGER_ARBITRATION_H */
