/* Traces: TRC-BOOT-01..05. */
#include "../../core0_safety_signal/tests/test_framework.h"
#include "../ab_rollback.h"

static void test_initial_state_confirmed(void)
{
    nvband_ab_rollback_state_t st;
    nvband_ab_rollback_init(&st, NVBAND_AB_SLOT_A);
    NVBAND_CHECK(st.active_state == NVBAND_AB_IMAGE_CONFIRMED);
    NVBAND_CHECK(nvband_ab_slot_for_next_boot(&st) == NVBAND_AB_SLOT_A);
}

static void test_successful_update_confirms_and_stays_on_new_slot(void)
{
    nvband_ab_rollback_state_t st;
    nvband_ab_rollback_init(&st, NVBAND_AB_SLOT_A);
    nvband_ab_begin_update(&st, NVBAND_AB_SLOT_B);
    NVBAND_CHECK(st.active_state == NVBAND_AB_IMAGE_PENDING);

    NVBAND_CHECK(nvband_ab_record_boot_attempt(&st) == true);
    nvband_ab_confirm_boot_ok(&st);

    NVBAND_CHECK(st.active_state == NVBAND_AB_IMAGE_CONFIRMED);
    NVBAND_CHECK(nvband_ab_slot_for_next_boot(&st) == NVBAND_AB_SLOT_B);
}

static void test_repeated_boot_failure_triggers_rollback(void)
{
    nvband_ab_rollback_state_t st;
    nvband_ab_rollback_init(&st, NVBAND_AB_SLOT_A);
    nvband_ab_begin_update(&st, NVBAND_AB_SLOT_B);

    bool ok = true;
    for (unsigned i = 0; i < NVBAND_AB_MAX_BOOT_ATTEMPTS + 1u; i++) {
        ok = nvband_ab_record_boot_attempt(&st);
        /* Never confirmed — simulates a new image that crashes every boot
         * before it can call confirm_boot_ok(). */
    }
    NVBAND_CHECK(ok == false);
    NVBAND_CHECK(st.active_state == NVBAND_AB_IMAGE_FAILED);
    NVBAND_CHECK(nvband_ab_slot_for_next_boot(&st) == NVBAND_AB_SLOT_A);
}

static void test_confirmed_image_boot_attempts_dont_count(void)
{
    nvband_ab_rollback_state_t st;
    nvband_ab_rollback_init(&st, NVBAND_AB_SLOT_A);
    for (int i = 0; i < 100; i++) {
        NVBAND_CHECK(nvband_ab_record_boot_attempt(&st) == true);
    }
    NVBAND_CHECK(st.active_state == NVBAND_AB_IMAGE_CONFIRMED);
    NVBAND_CHECK(st.boot_attempt_count == 0);
}

static void test_confirm_only_valid_while_pending(void)
{
    nvband_ab_rollback_state_t st;
    nvband_ab_rollback_init(&st, NVBAND_AB_SLOT_A);
    /* Not pending: confirm is a no-op. */
    nvband_ab_confirm_boot_ok(&st);
    NVBAND_CHECK(st.active_state == NVBAND_AB_IMAGE_CONFIRMED);
}

int main(void)
{
    NVBAND_RUN(test_initial_state_confirmed);
    NVBAND_RUN(test_successful_update_confirms_and_stays_on_new_slot);
    NVBAND_RUN(test_repeated_boot_failure_triggers_rollback);
    NVBAND_RUN(test_confirmed_image_boot_attempts_dont_count);
    NVBAND_RUN(test_confirm_only_valid_while_pending);
    NVBAND_TEST_MAIN_END();
}
