/**
 * watchdog_kick.h — Core 0 watchdog (U9) kick, gated on real sampling.
 *
 * CLAUDE.md §3.6: "Core 0 kicks the external watchdog on a fixed cadence
 * tied to real sample acquisition ... the kick must be provably contingent
 * on the sampling loop actually running, not just on the core being
 * alive." and "Core 1 must never be able to block, starve, or delay the
 * Core 0 kick."
 *
 * Design: the sampling ISR/loop increments a monotonic sample counter on
 * every completed EEG+IMU sample (see sampling/). This module's kick
 * function refuses to kick if the sample counter has not advanced since
 * the previous kick attempt — i.e. a core that is "alive" but whose
 * sampling loop has wedged will starve the watchdog exactly as if the
 * core had crashed, and U9 (independent oscillator, external reset) will
 * reset the board. This is the intended fail-safe: a stalled sampler is
 * indistinguishable, from U9's perspective, from a dead core.
 *
 * This module never touches Core 1 / IPC / BLE. It has exactly one
 * hardware dependency (the GPIO/pin driving U9's kick input) and one
 * software dependency (the sample counter), both injected via HAL so it
 * is host-testable.
 */
#ifndef NVBAND_WATCHDOG_KICK_H
#define NVBAND_WATCHDOG_KICK_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    void (*pulse_kick_line)(void *ctx);
    void *ctx;
} nvband_watchdog_hal_t;

typedef struct {
    uint64_t last_kicked_sample_count;
    uint64_t last_kick_time_us;
    uint32_t missed_kick_count;   /* diagnostic only; U9 doesn't care why */
} nvband_watchdog_state_t;

void nvband_watchdog_state_init(nvband_watchdog_state_t *state);

/**
 * Called on the fixed Core 0 tick. current_sample_count is the sampling
 * loop's monotonic counter; now_us is the current time. Kicks U9 only if
 * the sample counter has advanced since the last successful kick AND the
 * elapsed time is within NVBAND_WATCHDOG_MAX_KICK_INTERVAL_MS of the last
 * kick. Returns true if it kicked.
 */
bool nvband_watchdog_tick(const nvband_watchdog_hal_t *hal,
                           nvband_watchdog_state_t *state,
                           uint64_t current_sample_count,
                           uint64_t now_us);

#endif /* NVBAND_WATCHDOG_KICK_H */
