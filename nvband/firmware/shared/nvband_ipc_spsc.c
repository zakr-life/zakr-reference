#include "nvband_ipc_spsc.h"
#include <string.h>

void nvband_ipc_spsc_init(nvband_ipc_spsc_t *q, nvband_ipc_msg_t *storage,
                           size_t capacity_pow2)
{
    if (q == NULL) {
        return;
    }
    q->slots = storage;
    q->capacity = capacity_pow2;
    atomic_store_explicit(&q->head, 0, memory_order_relaxed);
    atomic_store_explicit(&q->tail, 0, memory_order_relaxed);
}

bool nvband_ipc_spsc_push(nvband_ipc_spsc_t *q, const nvband_ipc_msg_t *msg)
{
    if (q == NULL || msg == NULL || q->slots == NULL) {
        return false;
    }
    size_t tail = atomic_load_explicit(&q->tail, memory_order_relaxed);
    size_t head = atomic_load_explicit(&q->head, memory_order_acquire);

    if (tail - head >= q->capacity) {
        return false; /* full: drop, never block */
    }

    q->slots[tail & (q->capacity - 1)] = *msg;
    atomic_store_explicit(&q->tail, tail + 1, memory_order_release);
    return true;
}

bool nvband_ipc_spsc_pop(nvband_ipc_spsc_t *q, nvband_ipc_msg_t *out)
{
    if (q == NULL || out == NULL || q->slots == NULL) {
        return false;
    }
    size_t head = atomic_load_explicit(&q->head, memory_order_relaxed);
    size_t tail = atomic_load_explicit(&q->tail, memory_order_acquire);

    if (head == tail) {
        return false; /* empty */
    }

    *out = q->slots[head & (q->capacity - 1)];
    atomic_store_explicit(&q->head, head + 1, memory_order_release);
    return true;
}

size_t nvband_ipc_spsc_count(const nvband_ipc_spsc_t *q)
{
    if (q == NULL) {
        return 0;
    }
    size_t head = atomic_load_explicit(&q->head, memory_order_acquire);
    size_t tail = atomic_load_explicit(&q->tail, memory_order_acquire);
    return tail - head;
}
