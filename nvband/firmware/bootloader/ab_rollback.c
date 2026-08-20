#include "ab_rollback.h"
#include <string.h>

static nvband_ab_slot_t other_slot(nvband_ab_slot_t s)
{
    return (s == NVBAND_AB_SLOT_A) ? NVBAND_AB_SLOT_B : NVBAND_AB_SLOT_A;
}

void nvband_ab_rollback_init(nvband_ab_rollback_state_t *st,
                              nvband_ab_slot_t initial_confirmed_slot)
{
    if (st == NULL) {
        return;
    }
    memset(st, 0, sizeof(*st));
    st->active_slot = initial_confirmed_slot;
    st->active_state = NVBAND_AB_IMAGE_CONFIRMED;
    st->boot_attempt_count = 0;
}

void nvband_ab_begin_update(nvband_ab_rollback_state_t *st,
                             nvband_ab_slot_t new_slot)
{
    if (st == NULL) {
        return;
    }
    st->active_slot = new_slot;
    st->active_state = NVBAND_AB_IMAGE_PENDING;
    st->boot_attempt_count = 0;
}

bool nvband_ab_record_boot_attempt(nvband_ab_rollback_state_t *st)
{
    if (st == NULL) {
        return false;
    }
    if (st->active_state != NVBAND_AB_IMAGE_PENDING) {
        return true; /* confirmed images don't count attempts */
    }

    st->boot_attempt_count++;
    if (st->boot_attempt_count > NVBAND_AB_MAX_BOOT_ATTEMPTS) {
        st->active_state = NVBAND_AB_IMAGE_FAILED;
        return false;
    }
    return true;
}

void nvband_ab_confirm_boot_ok(nvband_ab_rollback_state_t *st)
{
    if (st == NULL || st->active_state != NVBAND_AB_IMAGE_PENDING) {
        return;
    }
    st->active_state = NVBAND_AB_IMAGE_CONFIRMED;
    st->boot_attempt_count = 0;
}

nvband_ab_slot_t nvband_ab_slot_for_next_boot(
    const nvband_ab_rollback_state_t *st)
{
    if (st == NULL) {
        return NVBAND_AB_SLOT_A;
    }
    if (st->active_state == NVBAND_AB_IMAGE_FAILED) {
        return other_slot(st->active_slot);
    }
    return st->active_slot;
}
