/**
 * mic_power_gate.h — Microphone power/clock domain state machine.
 *
 * Addendum 2 §B ("Voiceprint"): "the microphone's power/clock domain is
 * gated by an explicit firmware state machine and can only be enabled
 * during a session-scoped, user-initiated voice-enrollment or
 * voice-verification request. There is no firmware, app, or cloud code
 * path that can power the microphone outside that explicit request
 * window — no 'always listening,' no wake-word, no ambient capture."
 * BOM: U21 — MEMS PDM microphone, `TODO(OI-6)` part/placement pending
 * mechanical BOM reconciliation (see docs/ADDENDUM_2_biometric_federated_sleep.md).
 *
 * This is THE critical safety/privacy module for this subsystem — the
 * hard invariant it exists to guarantee, mirroring CLAUDE.md §0.1's
 * rigor for the interlock chain, is:
 *
 *   1. There is no code path anywhere in this module that can reach a
 *      mic-powered state (CAPTURING) without going through
 *      nvband_mic_gate_request_capture() — the one function that
 *      represents an explicit, session-scoped, user-initiated
 *      enrollment/verification request. No other function in this file
 *      ever calls hal->mic_power_on.
 *   2. Once CAPTURING, the mic is GUARANTEED to be powered off again
 *      after a bounded maximum capture window
 *      (NVBAND_MIC_MAX_CAPTURE_WINDOW_US) even if the caller never calls
 *      nvband_mic_gate_stop_capture() — nvband_mic_gate_tick() is the
 *      watchdog that enforces this, the same "poll on a bounded cadence,
 *      never trust a caller to remember" philosophy as
 *      safety/watchdog_kick.c. In a real build, tick() is driven by a
 *      periodic RTOS timer/task independent of the capture caller — this
 *      module does not itself claim an independent-oscillator hardware
 *      backstop the way U9 backstops the interlock chain (CLAUDE.md
 *      §0.1); the mic rail is not part of the seven-condition
 *      stimulation interlock and never becomes one (see audio/README.md
 *      and Addendum 2 Rule 0).
 *   3. A malformed, repeated, or out-of-order call (a second request
 *      while already capturing, a stop with a stale/wrong session token,
 *      a stop from IDLE, ...) is always rejected without ever calling
 *      hal->mic_power_on a second time or leaving the mic powered with no
 *      route back to IDLE.
 *
 * HAL-injected (function pointers for actually driving the mic power/
 * clock domain) so this is fully host-testable against a fake, exactly
 * like drivers/charger_arbitration.c and safety/watchdog_kick.c.
 */
#ifndef NVBAND_MIC_POWER_GATE_H
#define NVBAND_MIC_POWER_GATE_H

#include <stdbool.h>
#include <stdint.h>

/* Maximum time the mic may remain powered for a single capture, from the
 * moment request_capture() grants it. Chosen with wide margin over a
 * spoken enrollment/verification phrase (Addendum 2 §B: "3 spoken
 * utterances of a fixed enrollment phrase") — a few seconds of speech —
 * while still being a hard, bounded ceiling rather than an estimate.
 * Named here rather than in shared/nvband_constants.h because this
 * subsystem's own timing has no cross-module safety dependency the way
 * e.g. NVBAND_WATCHDOG_MAX_KICK_INTERVAL_MS does. */
#define NVBAND_MIC_MAX_CAPTURE_WINDOW_US (8ull * 1000ull * 1000ull) /* 8 s */

typedef enum {
    NVBAND_MIC_STATE_IDLE = 0,
    NVBAND_MIC_STATE_REQUEST_PENDING,
    NVBAND_MIC_STATE_CAPTURING,
    NVBAND_MIC_STATE_DISCARDING,
} nvband_mic_power_state_t;

typedef struct {
    /* Actually powers the mic's clock/power domain (U21 PDM interface
     * enable). Called from exactly one place in this file:
     * nvband_mic_gate_request_capture(). */
    void (*mic_power_on)(void *ctx);
    /* Actually powers the mic's clock/power domain off. Called from
     * nvband_mic_gate_stop_capture() (explicit stop) and
     * nvband_mic_gate_tick() (auto-timeout watchdog) — the only two
     * places in this file that end a capture. */
    void (*mic_power_off)(void *ctx);
    /* Optional. Invoked on every state transition this module makes, in
     * order, so a caller (or a test) can observe the FULL sequence of
     * states — including the transient REQUEST_PENDING step inside a
     * single request_capture() call — not just a before/after snapshot.
     * May be NULL. */
    void (*on_state_change)(void *ctx, nvband_mic_power_state_t old_state,
                             nvband_mic_power_state_t new_state);
    void *ctx;
} nvband_mic_hal_t;

typedef struct {
    nvband_mic_power_state_t state;
    uint32_t session_token;         /* increments on every GRANTED request;
                                        stop_capture() must present the
                                        exact token it was granted, so a
                                        stale call from an old/expired
                                        session can never touch a newer
                                        one's mic-power state. 0 means "no
                                        session granted yet." */
    uint64_t capture_deadline_us;   /* meaningful only while CAPTURING */
    uint32_t request_count;         /* accepted requests (mic was powered) */
    uint32_t rejected_request_count;/* requests refused (mic never touched) */
    uint32_t auto_timeout_count;    /* watchdog, not caller, ended capture */
    uint32_t explicit_stop_count;   /* caller ended capture in time */
} nvband_mic_power_gate_t;

void nvband_mic_gate_init(nvband_mic_power_gate_t *gate);

/**
 * THE only function in this module that can power the microphone.
 * Valid only when the gate is IDLE. On success: grants a new session
 * token (written to *session_token_out if non-NULL), arms the auto-
 * timeout deadline at now_us + NVBAND_MIC_MAX_CAPTURE_WINDOW_US, calls
 * hal->mic_power_on exactly once, and transitions IDLE ->
 * REQUEST_PENDING -> CAPTURING (both transitions reported via
 * hal->on_state_change if set). On failure (gate/hal NULL, or state is
 * not IDLE): returns false, makes NO hal call, and leaves state
 * unchanged — there is no partial-success path.
 */
bool nvband_mic_gate_request_capture(nvband_mic_power_gate_t *gate,
                                      const nvband_mic_hal_t *hal,
                                      uint64_t now_us,
                                      uint32_t *session_token_out);

/**
 * Explicit, caller-initiated end of a capture in progress. Valid only
 * when state is CAPTURING AND session_token matches the token that was
 * granted by request_capture() — a stale token (e.g. from a session that
 * has already auto-timed-out and been superseded) is rejected, never
 * treated as a no-op power-off of whatever session happens to be active
 * now. On success: calls hal->mic_power_off exactly once and transitions
 * CAPTURING -> DISCARDING.
 */
bool nvband_mic_gate_stop_capture(nvband_mic_power_gate_t *gate,
                                   const nvband_mic_hal_t *hal,
                                   uint32_t session_token,
                                   uint64_t now_us);

/**
 * Must be polled on a bounded cadence (e.g. from the same periodic task
 * cadence as safety/watchdog_kick.c on Core 0 — this module lives on
 * Core 1, so in practice a Core-1 periodic timer/task). This is the
 * watchdog:
 *   - If CAPTURING and now_us has reached/passed capture_deadline_us:
 *     powers the mic off (hal->mic_power_off), transitions to
 *     DISCARDING, and counts it in auto_timeout_count — regardless of
 *     whether anything ever calls stop_capture().
 *   - If DISCARDING: resolves to IDLE (the mic's power state, the
 *     safety-relevant fact this module tracks, already reached OFF
 *     before this call — buffer-level cleanup is mic_capture.c's
 *     concern, not this module's).
 *   - If IDLE or REQUEST_PENDING: no-op.
 * Safe to call at any cadence, including much faster than needed —
 * idempotent when there is nothing to do.
 */
void nvband_mic_gate_tick(nvband_mic_power_gate_t *gate,
                           const nvband_mic_hal_t *hal,
                           uint64_t now_us);

#endif /* NVBAND_MIC_POWER_GATE_H */
