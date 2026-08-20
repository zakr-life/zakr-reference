/**
 * sync_ring_buffer.h — Synchronous EEG+IMU sample ring buffer.
 *
 * CLAUDE.md §3.3: "Sample all active EEG channels synchronously with the
 * IMU (U17) on a shared clock/trigger — this is required for artifact
 * rejection and adherence logging, not optional."
 *
 * A synchronous sample bundles EEG channel readings and IMU readings
 * taken from the SAME trigger instant into one record with one
 * timestamp. There is deliberately no API to push an EEG-only or IMU-
 * only record — the type system enforces "always both, always together."
 * This ring buffer is also the producer of the monotonic sample counter
 * that gates the Core-0 watchdog kick (see ../safety/watchdog_kick.h):
 * every push() advances the counter, so pushing IS "sampling advanced."
 */
#ifndef NVBAND_SYNC_RING_BUFFER_H
#define NVBAND_SYNC_RING_BUFFER_H

#include <stdbool.h>
#include <stdint.h>
#include "../../shared/nvband_channel_config.h"

typedef struct {
    uint64_t timestamp_us;
    float    eeg_uV[NVBAND_MAX_EEG_CHANNELS];
    uint8_t  eeg_channel_count;
    float    accel_g[3];
    float    gyro_dps[3];
} nvband_sync_sample_t;

#define NVBAND_SYNC_RING_CAPACITY 512u /* power of two */

typedef struct {
    nvband_sync_sample_t samples[NVBAND_SYNC_RING_CAPACITY];
    uint64_t              write_index;   /* monotonic, never wraps logically */
    uint64_t              read_index;
} nvband_sync_ring_buffer_t;

void nvband_sync_ring_init(nvband_sync_ring_buffer_t *rb);

/** Push a synchronous sample. Always succeeds from the writer's
 *  perspective (overwrites the oldest unread sample if the reader has
 *  fallen behind by a full buffer — sampling must never block waiting
 *  for the reader, mirroring the Core0/Core1 IPC design). Returns true
 *  if an unread sample was overwritten (reader fell behind). */
bool nvband_sync_ring_push(nvband_sync_ring_buffer_t *rb,
                            const nvband_sync_sample_t *sample);

/** Pop the oldest unread sample. Returns false if the buffer is empty. */
bool nvband_sync_ring_pop(nvband_sync_ring_buffer_t *rb,
                           nvband_sync_sample_t *out);

/** The monotonic count of samples ever pushed — this is what
 *  watchdog_kick.h's current_sample_count argument should be fed. */
uint64_t nvband_sync_ring_total_pushed(const nvband_sync_ring_buffer_t *rb);

#endif /* NVBAND_SYNC_RING_BUFFER_H */
