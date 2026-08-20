/**
 * nvband_constants.h — Named physical/timing constants for the NV-Band.
 *
 * Source: ZAKR NV-Band Master Build Prompt §1 (tolerance schedule / BOM).
 * Every number here is authoritative until the referenced open item (OI-n)
 * is resolved. Do not hardcode any of these values anywhere else in the
 * codebase — include this header (or, on the app/cloud side, the generated
 * mirror in shared/constants.json) instead.
 *
 * Values are derived, not just stated, where the dossier gives a
 * derivation, so that a future parameter change (e.g. pad diameter)
 * propagates instead of silently drifting from its source.
 */
#ifndef NVBAND_CONSTANTS_H
#define NVBAND_CONSTANTS_H

#include <stdint.h>

/* ---------------------------------------------------------------------
 * §1 — Electrode / channel configuration (OI-1, data-driven, NOT a
 * compile-time constant for production — see nvband_channel_config.h).
 * These are the CURRENT PLACEHOLDER figures pending mechanical
 * reconciliation. TODO(OI-1): electrode/channel count unresolved; do not
 * treat this default as final.
 * ------------------------------------------------------------------- */
#define NVBAND_PLACEHOLDER_STIM_CHANNEL_COUNT   2u
#define NVBAND_PLACEHOLDER_EEG_CHANNEL_COUNT    4u

/* ---------------------------------------------------------------------
 * §1 — Stimulation current density ceiling.
 * 0.995 mA/cm^2 at 2.0 mA on a Ø16 mm pad; regulatory limit 1.0 mA/cm^2.
 * Margin is near zero. Encode the RELATIONSHIP (current / area), not a
 * bare current figure, so a future pad-diameter change forces a matching
 * firmware recompute rather than silently invalidating the ceiling.
 * ------------------------------------------------------------------- */
#define NVBAND_STIM_PAD_DIAMETER_MM             16.0f
#define NVBAND_STIM_PAD_AREA_CM2 \
    (3.14159265358979f * (NVBAND_STIM_PAD_DIAMETER_MM / 20.0f) * \
                          (NVBAND_STIM_PAD_DIAMETER_MM / 20.0f))
#define NVBAND_MAX_CURRENT_DENSITY_MA_PER_CM2   1.0f   /* regulatory limit */
#define NVBAND_RATED_MAX_CURRENT_MA             2.0f   /* dossier rated max */
/* Derived ceiling: firmware MUST compute this, not assume 2.0 mA, so a
 * pad-area change is caught at build/provisioning time, not silently
 * accepted. See nvband_current_ceiling_mA() in stim waveform validator. */
#define NVBAND_CURRENT_CEILING_MA \
    (NVBAND_MAX_CURRENT_DENSITY_MA_PER_CM2 * NVBAND_STIM_PAD_AREA_CM2)

/* ---------------------------------------------------------------------
 * §1 — Charge balance.
 * <=1.0 uC per phase at rated maximum, charge-balanced, zero net DC.
 * ------------------------------------------------------------------- */
#define NVBAND_MAX_CHARGE_PER_PHASE_UC          1.0f
/* Firmware charge-balance verifier tolerance: the integral of a commanded
 * waveform over one pulse pair must be within this of zero net charge.
 * Non-zero only to absorb DAC/ADC quantization, never as a "close enough"
 * safety margin. */
#define NVBAND_CHARGE_BALANCE_TOLERANCE_UC      0.01f

/* ---------------------------------------------------------------------
 * §1 — Average current / power budget.
 * 700 mAh / 3.7 V over an 8 h wear target at 85% depth of discharge
 * => 700mAh * 0.85 / 8h = 74.375 mA average current budget.
 * ------------------------------------------------------------------- */
#define NVBAND_BATTERY_CAPACITY_MAH             700.0f
#define NVBAND_BATTERY_NOMINAL_VOLTAGE_V        3.7f
#define NVBAND_DEPTH_OF_DISCHARGE_FRACTION      0.85f
#define NVBAND_WEAR_TARGET_HOURS                8.0f
#define NVBAND_AVERAGE_CURRENT_BUDGET_MA \
    (NVBAND_BATTERY_CAPACITY_MAH * NVBAND_DEPTH_OF_DISCHARGE_FRACTION / \
     NVBAND_WEAR_TARGET_HOURS)   /* == 74.375 mA, dossier rounds to 74.4 */

/* ---------------------------------------------------------------------
 * §1 — Electrode contact pressure assumption (not directly measurable;
 * impedance drift is the firmware-observable proxy — see safety/
 * impedance_trend.c).
 * ------------------------------------------------------------------- */
#define NVBAND_ELECTRODE_FORCE_N                0.28f
#define NVBAND_ELECTRODE_PRESSURE_KPA_ASSUMED   1.39f

/* ---------------------------------------------------------------------
 * §1 — Manufacturing HiPot record (runtime firmware gate only; the test
 * itself is a manufacturing-time event — see §3.2, tools/provisioning).
 * ------------------------------------------------------------------- */
#define NVBAND_HIPOT_TEST_VOLTAGE_V             1500.0f
#define NVBAND_HIPOT_TEST_DURATION_S            60u

/* ---------------------------------------------------------------------
 * §3.3 — EEG acquisition noise floor target.
 * ------------------------------------------------------------------- */
#define NVBAND_EEG_INPUT_NOISE_UVRMS_MAX        1.0f
#define NVBAND_EEG_BANDWIDTH_LOW_HZ             0.5f
#define NVBAND_EEG_BANDWIDTH_HIGH_HZ            40.0f

/* ---------------------------------------------------------------------
 * §3.3 — Mux (U19) break-before-make timing margin. The real part
 * (ADG1608/ADG5208-class) is break-before-make; this is the firmware-
 * enforced minimum break interval, independent of the datasheet number,
 * so a part substitution cannot silently erode it without a code review.
 * ------------------------------------------------------------------- */
#define NVBAND_MUX_BREAK_BEFORE_MAKE_MIN_NS     500u

/* ---------------------------------------------------------------------
 * §3.6 — Watchdog cadence. Core 0 kicks U9 on a cadence tied to real
 * sample acquisition (see safety/watchdog_kick.c) — this is the maximum
 * allowed interval between kicks before U9's own independent oscillator
 * times out and forces a hardware reset (interlock defeat-safe).
 * ------------------------------------------------------------------- */
#define NVBAND_WATCHDOG_MAX_KICK_INTERVAL_MS    200u
#define NVBAND_EEG_SAMPLE_RATE_HZ               250u

/* ---------------------------------------------------------------------
 * §9 — Interlock chain: seven independent hardware conditions read back
 * by firmware (read-only, except NVBAND_INTERLOCK_FIRMWARE_PERMIT, the
 * one bit firmware legitimately owns). Order matches the dossier's
 * Sheet reference; do not renumber without updating the traceability
 * matrix in firmware/docs/traceability_matrix.csv.
 * ------------------------------------------------------------------- */
typedef enum {
    NVBAND_INTERLOCK_CURRENT_WINDOW = 0,
    NVBAND_INTERLOCK_IMPEDANCE_WINDOW,
    NVBAND_INTERLOCK_ELECTRODE_TEMP_WINDOW,
    NVBAND_INTERLOCK_SUPPLY_RAILS_GOOD,
    NVBAND_INTERLOCK_WATCHDOG_SATISFIED,
    NVBAND_INTERLOCK_STOP_SWITCH_CLOSED,
    NVBAND_INTERLOCK_FIRMWARE_PERMIT,
    NVBAND_INTERLOCK_CONDITION_COUNT
} nvband_interlock_condition_t;

#define NVBAND_INTERLOCK_ALL_SATISFIED_MASK \
    ((1u << NVBAND_INTERLOCK_CONDITION_COUNT) - 1u)

/* Electrode temperature window (skin-contact sensor on the electrode
 * substrate — NEVER conflate with board/MCU temperature; see §3.7). */
#define NVBAND_ELECTRODE_TEMP_MIN_C             28.0f
#define NVBAND_ELECTRODE_TEMP_MAX_C             40.0f

#endif /* NVBAND_CONSTANTS_H */
