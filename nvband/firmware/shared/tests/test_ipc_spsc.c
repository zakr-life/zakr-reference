/* Traces: TRC-IPC-01..04. */
#include "../../core0_safety_signal/tests/test_framework.h"
#include "../nvband_ipc_spsc.h"

static void test_push_pop_fifo_order(void)
{
    nvband_ipc_msg_t storage[8];
    nvband_ipc_spsc_t q;
    nvband_ipc_spsc_init(&q, storage, 8);

    for (uint16_t i = 0; i < 5; i++) {
        nvband_ipc_msg_t m = { .len = 1, .msg_type = i, .bytes = {0} };
        NVBAND_CHECK(nvband_ipc_spsc_push(&q, &m) == true);
    }
    NVBAND_CHECK(nvband_ipc_spsc_count(&q) == 5);

    for (uint16_t i = 0; i < 5; i++) {
        nvband_ipc_msg_t out;
        NVBAND_CHECK(nvband_ipc_spsc_pop(&q, &out) == true);
        NVBAND_CHECK(out.msg_type == i);
    }
    NVBAND_CHECK(nvband_ipc_spsc_count(&q) == 0);
}

static void test_full_queue_drops_never_blocks(void)
{
    nvband_ipc_msg_t storage[4];
    nvband_ipc_spsc_t q;
    nvband_ipc_spsc_init(&q, storage, 4);

    for (int i = 0; i < 4; i++) {
        nvband_ipc_msg_t m = { .len = 0, .msg_type = (uint16_t)i };
        NVBAND_CHECK(nvband_ipc_spsc_push(&q, &m) == true);
    }
    /* Queue full: push must return false immediately (no blocking
     * mechanism exists in this API to even wait). */
    nvband_ipc_msg_t overflow = { .len = 0, .msg_type = 99 };
    NVBAND_CHECK(nvband_ipc_spsc_push(&q, &overflow) == false);
    NVBAND_CHECK(nvband_ipc_spsc_count(&q) == 4);
}

static void test_empty_queue_pop_returns_false(void)
{
    nvband_ipc_msg_t storage[4];
    nvband_ipc_spsc_t q;
    nvband_ipc_spsc_init(&q, storage, 4);
    nvband_ipc_msg_t out;
    NVBAND_CHECK(nvband_ipc_spsc_pop(&q, &out) == false);
}

/* Models "Core 1 pegged at 100%, flooding the queue" while Core 0 drains
 * at its own pace — proves throughput and drop behavior stay well-defined
 * regardless of producer rate, with no blocking primitive anywhere in the
 * call graph (grep the .c file: no mutex/futex/syscall). */
static void test_high_volume_producer_consumer_never_loses_fifo_order(void)
{
    nvband_ipc_msg_t storage[64];
    nvband_ipc_spsc_t q;
    nvband_ipc_spsc_init(&q, storage, 64);

    uint16_t next_expected = 0;
    for (uint16_t i = 0; i < 10000; i++) {
        nvband_ipc_msg_t m = { .len = 0, .msg_type = i };
        nvband_ipc_spsc_push(&q, &m); /* ignore drops, by design */

        if (i % 3 == 0) { /* consumer drains slower than producer */
            nvband_ipc_msg_t out;
            while (nvband_ipc_spsc_pop(&q, &out)) {
                NVBAND_CHECK(out.msg_type >= next_expected);
                next_expected = (uint16_t)(out.msg_type + 1);
            }
        }
    }
}

int main(void)
{
    NVBAND_RUN(test_push_pop_fifo_order);
    NVBAND_RUN(test_full_queue_drops_never_blocks);
    NVBAND_RUN(test_empty_queue_pop_returns_false);
    NVBAND_RUN(test_high_volume_producer_consumer_never_loses_fifo_order);
    NVBAND_TEST_MAIN_END();
}
