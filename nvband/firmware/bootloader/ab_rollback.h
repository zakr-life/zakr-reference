/**
 * ab_rollback.h — A/B firmware slot selection and automatic rollback.
 *
 * CLAUDE.md §3.8: "A/B firmware slots on U15 with rollback: verify new
 * image signature and run a self-test boot before marking it primary;
 * automatic rollback on repeated boot failure."
 *
 * This is the decision logic MCUboot's application-level "image OK"
 * confirmation hook drives: a newly flashed image boots into a PENDING
 * state, must call nvband_ab_confirm_boot_ok() after passing its own
 * self-tests within NVBAND_AB_MAX_CONFIRM_ATTEMPTS boots, or the next
 * boot falls back to the previous known-good slot. This module tracks
 * only the counting/decision; MCUboot itself owns the actual image swap
 * and signature verification (Ed25519/ECDSA per its own config, not
 * reimplemented here).
 */
#ifndef NVBAND_AB_ROLLBACK_H
#define NVBAND_AB_ROLLBACK_H

#include <stdbool.h>
#include <stdint.h>

#define NVBAND_AB_MAX_BOOT_ATTEMPTS 3u

typedef enum {
    NVBAND_AB_SLOT_A = 0,
    NVBAND_AB_SLOT_B,
} nvband_ab_slot_t;

typedef enum {
    NVBAND_AB_IMAGE_CONFIRMED = 0,  /* known-good, boots normally        */
    NVBAND_AB_IMAGE_PENDING,        /* newly flashed, awaiting self-test */
    NVBAND_AB_IMAGE_FAILED,         /* exceeded boot attempts, must not
                                        be selected again until re-flashed */
} nvband_ab_image_state_t;

typedef struct {
    nvband_ab_slot_t         active_slot;
    nvband_ab_image_state_t  active_state;
    uint8_t                  boot_attempt_count;
} nvband_ab_rollback_state_t;

void nvband_ab_rollback_init(nvband_ab_rollback_state_t *st,
                              nvband_ab_slot_t initial_confirmed_slot);

/** Called when a newly signed-and-verified image is written to the
 *  inactive slot and made pending for the next boot. */
void nvband_ab_begin_update(nvband_ab_rollback_state_t *st,
                             nvband_ab_slot_t new_slot);

/** Called at the start of every boot of a PENDING image. Returns false
 *  (and flips state to FAILED, selecting rollback to the other slot) if
 *  this exceeds NVBAND_AB_MAX_BOOT_ATTEMPTS. */
bool nvband_ab_record_boot_attempt(nvband_ab_rollback_state_t *st);

/** Called by the running image once its own self-tests pass. Only valid
 *  while PENDING; moves to CONFIRMED. */
void nvband_ab_confirm_boot_ok(nvband_ab_rollback_state_t *st);

/** Which slot should the NEXT boot use — the active slot if
 *  CONFIRMED/PENDING-with-attempts-remaining, or the other slot if
 *  FAILED (rollback). */
nvband_ab_slot_t nvband_ab_slot_for_next_boot(
    const nvband_ab_rollback_state_t *st);

#endif /* NVBAND_AB_ROLLBACK_H */
