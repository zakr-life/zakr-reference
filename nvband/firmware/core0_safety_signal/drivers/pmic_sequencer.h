/**
 * pmic_sequencer.h — Rail bring-up sequencing and readback confirmation.
 *
 * CLAUDE.md §3.1: "Sequence rail bring-up and confirm every rail settles
 * (via ADC/comparator readback, not assumption) before releasing the AFE
 * from reset." and "Expose rail-good status as one of the read-only
 * inputs the interlock chain can observe — firmware reads this, it does
 * not gate stim with it in software; the hardware AND already does
 * that."
 *
 * This is a small state machine over an ordered list of rails, each
 * confirmed by hardware readback (not a timer/assumption) before
 * proceeding to the next. It has exactly one output binary decision:
 * whether it is safe to release the AFE from reset. It has NO output
 * that touches stimulation enable — rail-good is one of the seven
 * hardware interlock AND inputs (NVBAND_INTERLOCK_SUPPLY_RAILS_GOOD),
 * read there independently; this module does not feed it directly.
 */
#ifndef NVBAND_PMIC_SEQUENCER_H
#define NVBAND_PMIC_SEQUENCER_H

#include <stdbool.h>
#include <stdint.h>

#define NVBAND_MAX_RAILS 6u

typedef struct {
    const char *name;                 /* e.g. "1V8_DIG", "3V3_ANA" */
    bool (*readback_settled)(void *ctx); /* ADC/comparator check, not a timer */
} nvband_rail_t;

typedef struct {
    nvband_rail_t rails[NVBAND_MAX_RAILS];
    uint8_t        rail_count;
    void          *ctx;
} nvband_rail_sequence_t;

typedef struct {
    bool     all_settled;
    uint8_t  first_failed_rail_index; /* valid only if !all_settled */
    uint8_t  rails_confirmed;
} nvband_rail_sequence_result_t;

/**
 * Walk the rails in order; stop at the first that does not read back
 * settled. Never assumes success — a rail with a NULL readback function
 * is treated as failed, not skipped.
 */
void nvband_rail_sequence_run(const nvband_rail_sequence_t *seq,
                               nvband_rail_sequence_result_t *out);

/** AFE reset is only released when every configured rail confirmed
 *  settled. */
bool nvband_rail_sequence_afe_reset_release_ok(
    const nvband_rail_sequence_result_t *result);

#endif /* NVBAND_PMIC_SEQUENCER_H */
