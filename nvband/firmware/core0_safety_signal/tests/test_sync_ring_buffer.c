/* Traces: TRC-SAMP-01..04. */
#include "test_framework.h"
#include "../sampling/sync_ring_buffer.h"

static void test_push_pop_preserves_eeg_and_imu_together(void)
{
    nvband_sync_ring_buffer_t rb;
    nvband_sync_ring_init(&rb);

    nvband_sync_sample_t s = {0};
    s.timestamp_us = 12345;
    s.eeg_channel_count = 4;
    for (int i = 0; i < 4; i++) s.eeg_uV[i] = (float)i * 1.5f;
    s.accel_g[0] = 0.98f; s.accel_g[1] = 0.01f; s.accel_g[2] = 0.02f;

    NVBAND_CHECK(nvband_sync_ring_push(&rb, &s) == false); /* no overwrite */

    nvband_sync_sample_t out;
    NVBAND_CHECK(nvband_sync_ring_pop(&rb, &out) == true);
    NVBAND_CHECK(out.timestamp_us == 12345);
    NVBAND_CHECK(out.eeg_uV[2] == 3.0f);
    NVBAND_CHECK(out.accel_g[0] == 0.98f);
}

static void test_total_pushed_is_monotonic_and_feeds_watchdog(void)
{
    nvband_sync_ring_buffer_t rb;
    nvband_sync_ring_init(&rb);
    NVBAND_CHECK(nvband_sync_ring_total_pushed(&rb) == 0);

    for (int i = 0; i < 100; i++) {
        nvband_sync_sample_t s = {0};
        s.timestamp_us = (uint64_t)i;
        nvband_sync_ring_push(&rb, &s);
    }
    NVBAND_CHECK(nvband_sync_ring_total_pushed(&rb) == 100);
}

static void test_empty_pop_returns_false(void)
{
    nvband_sync_ring_buffer_t rb;
    nvband_sync_ring_init(&rb);
    nvband_sync_sample_t out;
    NVBAND_CHECK(nvband_sync_ring_pop(&rb, &out) == false);
}

static void test_overflow_advances_reader_never_blocks_writer(void)
{
    nvband_sync_ring_buffer_t rb;
    nvband_sync_ring_init(&rb);

    /* Fill well past capacity without ever popping: push must never
     * fail/block — it always succeeds, silently overwriting oldest. */
    bool saw_overwrite = false;
    for (uint32_t i = 0; i < NVBAND_SYNC_RING_CAPACITY * 3; i++) {
        nvband_sync_sample_t s = {0};
        s.timestamp_us = i;
        if (nvband_sync_ring_push(&rb, &s)) {
            saw_overwrite = true;
        }
    }
    NVBAND_CHECK(saw_overwrite == true);
    NVBAND_CHECK(nvband_sync_ring_total_pushed(&rb) == NVBAND_SYNC_RING_CAPACITY * 3);

    /* Oldest readable sample should be from near the end of the fill,
     * not from the very start (which was overwritten). */
    nvband_sync_sample_t out;
    NVBAND_CHECK(nvband_sync_ring_pop(&rb, &out) == true);
    NVBAND_CHECK(out.timestamp_us >= NVBAND_SYNC_RING_CAPACITY * 2);
}

int main(void)
{
    NVBAND_RUN(test_push_pop_preserves_eeg_and_imu_together);
    NVBAND_RUN(test_total_pushed_is_monotonic_and_feeds_watchdog);
    NVBAND_RUN(test_empty_pop_returns_false);
    NVBAND_RUN(test_overflow_advances_reader_never_blocks_writer);
    NVBAND_TEST_MAIN_END();
}
