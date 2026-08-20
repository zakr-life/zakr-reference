#include "sync_ring_buffer.h"
#include <string.h>

void nvband_sync_ring_init(nvband_sync_ring_buffer_t *rb)
{
    if (rb == NULL) {
        return;
    }
    memset(rb, 0, sizeof(*rb));
}

bool nvband_sync_ring_push(nvband_sync_ring_buffer_t *rb,
                            const nvband_sync_sample_t *sample)
{
    if (rb == NULL || sample == NULL) {
        return false;
    }

    bool overwrote_unread =
        (rb->write_index - rb->read_index) >= NVBAND_SYNC_RING_CAPACITY;

    rb->samples[rb->write_index % NVBAND_SYNC_RING_CAPACITY] = *sample;
    rb->write_index++;

    if (overwrote_unread) {
        /* Reader fell a full buffer behind: advance read_index to match,
         * so it always points at the oldest still-valid sample. */
        rb->read_index = rb->write_index - NVBAND_SYNC_RING_CAPACITY;
    }

    return overwrote_unread;
}

bool nvband_sync_ring_pop(nvband_sync_ring_buffer_t *rb,
                           nvband_sync_sample_t *out)
{
    if (rb == NULL || out == NULL) {
        return false;
    }
    if (rb->read_index >= rb->write_index) {
        return false;
    }
    *out = rb->samples[rb->read_index % NVBAND_SYNC_RING_CAPACITY];
    rb->read_index++;
    return true;
}

uint64_t nvband_sync_ring_total_pushed(const nvband_sync_ring_buffer_t *rb)
{
    return (rb == NULL) ? 0 : rb->write_index;
}
