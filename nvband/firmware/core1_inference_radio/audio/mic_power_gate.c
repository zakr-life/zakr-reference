#include "mic_power_gate.h"
#include <string.h>

static void transition(nvband_mic_power_gate_t *gate, const nvband_mic_hal_t *hal,
                        nvband_mic_power_state_t new_state)
{
    nvband_mic_power_state_t old_state = gate->state;
    gate->state = new_state;
    if (hal != NULL && hal->on_state_change != NULL) {
        hal->on_state_change(hal->ctx, old_state, new_state);
    }
}

void nvband_mic_gate_init(nvband_mic_power_gate_t *gate)
{
    if (gate == NULL) {
        return;
    }
    memset(gate, 0, sizeof(*gate));
    gate->state = NVBAND_MIC_STATE_IDLE;
}

bool nvband_mic_gate_request_capture(nvband_mic_power_gate_t *gate,
                                      const nvband_mic_hal_t *hal,
                                      uint64_t now_us,
                                      uint32_t *session_token_out)
{
    if (gate == NULL || hal == NULL) {
        return false;
    }

    if (gate->state != NVBAND_MIC_STATE_IDLE) {
        /* Already pending/capturing/discarding: refuse rather than stack
         * a second request or restart the timeout window early. This is
         * the guard that makes a repeated/malformed request_capture()
         * call harmless — it can never call mic_power_on a second time
         * for an already-active session. */
        gate->rejected_request_count++;
        return false;
    }

    /* Step 1: register the request. Mic is still off here — no HAL call
     * has been made yet at this point in the function. */
    transition(gate, hal, NVBAND_MIC_STATE_REQUEST_PENDING);

    /* Step 2: grant a new session and actually power the mic. This is
     * the ONLY call to hal->mic_power_on anywhere in this file. */
    gate->session_token++;
    uint32_t granted_token = gate->session_token;
    gate->capture_deadline_us = now_us + NVBAND_MIC_MAX_CAPTURE_WINDOW_US;

    if (hal->mic_power_on != NULL) {
        hal->mic_power_on(hal->ctx);
    }
    transition(gate, hal, NVBAND_MIC_STATE_CAPTURING);

    gate->request_count++;
    if (session_token_out != NULL) {
        *session_token_out = granted_token;
    }
    return true;
}

bool nvband_mic_gate_stop_capture(nvband_mic_power_gate_t *gate,
                                   const nvband_mic_hal_t *hal,
                                   uint32_t session_token,
                                   uint64_t now_us)
{
    (void)now_us;
    if (gate == NULL || hal == NULL) {
        return false;
    }

    if (gate->state != NVBAND_MIC_STATE_CAPTURING) {
        return false;
    }
    if (session_token != gate->session_token) {
        /* Stale/wrong token: never touch a mic-power state that belongs
         * to a different (possibly already-ended) session. */
        return false;
    }

    if (hal->mic_power_off != NULL) {
        hal->mic_power_off(hal->ctx);
    }
    transition(gate, hal, NVBAND_MIC_STATE_DISCARDING);
    gate->explicit_stop_count++;
    return true;
}

void nvband_mic_gate_tick(nvband_mic_power_gate_t *gate,
                           const nvband_mic_hal_t *hal,
                           uint64_t now_us)
{
    if (gate == NULL) {
        return;
    }

    if (gate->state == NVBAND_MIC_STATE_CAPTURING) {
        if (now_us >= gate->capture_deadline_us) {
            /* The watchdog: nothing else ever had to call stop_capture()
             * for this to happen. This is the bounded ceiling on how
             * long the mic can ever be powered from one request. */
            if (hal != NULL && hal->mic_power_off != NULL) {
                hal->mic_power_off(hal->ctx);
            }
            transition(gate, hal, NVBAND_MIC_STATE_DISCARDING);
            gate->auto_timeout_count++;
        }
        return;
    }

    if (gate->state == NVBAND_MIC_STATE_DISCARDING) {
        /* Mic power is already guaranteed off by the time we ever enter
         * DISCARDING (see stop_capture() and the timeout branch above) —
         * this transition is bookkeeping only, never a HAL call. */
        transition(gate, hal, NVBAND_MIC_STATE_IDLE);
        return;
    }

    /* IDLE / REQUEST_PENDING: nothing for the watchdog to do. Note
     * REQUEST_PENDING is never observed here in practice because
     * request_capture() resolves it synchronously within one call; the
     * state still exists so on_state_change can observe it and so a
     * future asynchronous precondition check has a well-defined seam to
     * extend into without changing this function's contract. */
}
