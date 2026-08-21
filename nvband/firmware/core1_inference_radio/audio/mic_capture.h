/**
 * mic_capture.h — PDM sample buffering during one capture window.
 *
 * Addendum 2 §B: "Raw audio is processed in-memory for feature extraction
 * and is discarded immediately after; it is never written to the session
 * store and never transmitted [...]."
 *
 * This module is a simplified but structurally realistic capture buffer:
 * a fixed-capacity, caller-owned ring of int16 samples (the shape PDM
 * decimation output takes once decimated to PCM-like samples on-device).
 * It deliberately does NOT model real PDM decimation filter math — that
 * is DSP the actual U21 driver would own once written; this module is
 * only concerned with the buffering/lifecycle contract around it.
 *
 * Privacy-by-construction, not just by policy: this header declares no
 * function that writes buffer contents to flash, the session store, BLE,
 * or any radio path. There is no code reachable from this module that
 * could persist or transmit raw audio — the only way raw samples leave
 * this buffer is a caller reading them back via
 * nvband_mic_capture_data()/nvband_mic_capture_count() for immediate,
 * in-memory feature extraction (not part of this pass — see
 * audio/README.md), after which the caller is expected to call
 * nvband_mic_capture_zeroize() before the buffer's storage is reused or
 * released. The buffer's memory itself is always caller-owned (a stack
 * array or a Core-1-local static, never anything DMA'd toward a radio
 * peripheral) — this module never allocates or owns persistent storage.
 */
#ifndef NVBAND_MIC_CAPTURE_H
#define NVBAND_MIC_CAPTURE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    int16_t *samples;    /* caller-owned storage, length == capacity */
    uint32_t capacity;   /* number of int16 slots available */
    uint32_t count;      /* number of valid samples currently held, <= capacity */
    bool     overflowed; /* a push was attempted while full: the sample was
                             DROPPED, never silently overwritten — a full
                             buffer is a signal to stop capture, not a
                             reason to corrupt what's already there */
} nvband_mic_capture_buffer_t;

/**
 * Binds this buffer descriptor to caller-owned storage and clears its
 * bookkeeping (count=0, overflowed=false). Does NOT touch the contents
 * of `storage` — call nvband_mic_capture_zeroize() first if the caller
 * needs the memory itself scrubbed (e.g. reusing a buffer that held a
 * previous, already-processed capture).
 */
void nvband_mic_capture_reset(nvband_mic_capture_buffer_t *buf,
                               int16_t *storage, uint32_t capacity);

/**
 * Appends one sample. Returns false (and sets overflowed=true) if the
 * buffer is already at capacity — the sample is dropped, not wrapped
 * over the oldest data, so a caller can always trust that
 * [0, count) is a contiguous, unmodified prefix of what was captured.
 */
bool nvband_mic_capture_push_sample(nvband_mic_capture_buffer_t *buf, int16_t sample);

bool nvband_mic_capture_is_full(const nvband_mic_capture_buffer_t *buf);

uint32_t nvband_mic_capture_count(const nvband_mic_capture_buffer_t *buf);

/** Read-only view of the samples captured so far ([0, count)). NULL if
 *  buf is NULL or has never been reset() onto storage. */
const int16_t *nvband_mic_capture_data(const nvband_mic_capture_buffer_t *buf);

/**
 * Overwrites every slot in the bound storage with 0 and clears count.
 * Intended to be called by the caller immediately after feature
 * extraction has consumed a capture window's samples, so raw audio does
 * not linger in memory beyond the extraction window (Addendum 2 §B).
 * This module does not call this automatically on any transition of its
 * own — it has no notion of "capture window ended," that lifecycle lives
 * in mic_power_gate.c — so the caller that orchestrates both is
 * responsible for invoking it once mic_power_gate reports DISCARDING.
 */
void nvband_mic_capture_zeroize(nvband_mic_capture_buffer_t *buf);

#endif /* NVBAND_MIC_CAPTURE_H */
