/* Traces: Addendum 2 §B, HAZ-09 ("Unauthorized/ambient audio capture").
 *
 * This suite proves the single most important correctness property in
 * `firmware/core1_inference_radio/audio/`: the mic can never be powered
 * without an explicit, session-scoped request_capture() call, and once
 * powered it is always bounded by the auto-timeout watchdog regardless
 * of whether anything calls to stop it. */
#include "../../../core0_safety_signal/tests/test_framework.h"
#include "../mic_power_gate.h"

typedef struct {
    int power_on_calls;
    int power_off_calls;
    int transition_count;
    nvband_mic_power_state_t last_old_state;
    nvband_mic_power_state_t last_new_state;
    nvband_mic_power_state_t transition_log[16];
    int transition_log_len;
} fake_mic_hal_ctx_t;

static void fake_power_on(void *ctx)
{
    ((fake_mic_hal_ctx_t *)ctx)->power_on_calls++;
}

static void fake_power_off(void *ctx)
{
    ((fake_mic_hal_ctx_t *)ctx)->power_off_calls++;
}

static void fake_on_state_change(void *ctx, nvband_mic_power_state_t old_state,
                                  nvband_mic_power_state_t new_state)
{
    fake_mic_hal_ctx_t *c = (fake_mic_hal_ctx_t *)ctx;
    c->transition_count++;
    c->last_old_state = old_state;
    c->last_new_state = new_state;
    if (c->transition_log_len < 16) {
        c->transition_log[c->transition_log_len++] = new_state;
    }
}

static void reset_ctx(fake_mic_hal_ctx_t *ctx)
{
    ctx->power_on_calls = 0;
    ctx->power_off_calls = 0;
    ctx->transition_count = 0;
    ctx->transition_log_len = 0;
}

static nvband_mic_hal_t make_hal(fake_mic_hal_ctx_t *ctx)
{
    nvband_mic_hal_t hal;
    hal.mic_power_on = fake_power_on;
    hal.mic_power_off = fake_power_off;
    hal.on_state_change = fake_on_state_change;
    hal.ctx = ctx;
    return hal;
}

/* ---- Invariant 1: mic cannot be powered without a request ---- */

static void test_init_state_is_idle_and_mic_never_touched(void)
{
    nvband_mic_power_gate_t gate;
    nvband_mic_gate_init(&gate);
    NVBAND_CHECK(gate.state == NVBAND_MIC_STATE_IDLE);
    NVBAND_CHECK(gate.session_token == 0);
}

static void test_tick_from_idle_never_powers_mic(void)
{
    fake_mic_hal_ctx_t ctx; reset_ctx(&ctx);
    nvband_mic_hal_t hal = make_hal(&ctx);
    nvband_mic_power_gate_t gate;
    nvband_mic_gate_init(&gate);

    for (int i = 0; i < 1000; i++) {
        nvband_mic_gate_tick(&gate, &hal, (uint64_t)i * 1000000ull);
    }

    NVBAND_CHECK(ctx.power_on_calls == 0);
    NVBAND_CHECK(gate.state == NVBAND_MIC_STATE_IDLE);
}

static void test_stop_capture_from_idle_is_rejected_and_never_powers_off(void)
{
    fake_mic_hal_ctx_t ctx; reset_ctx(&ctx);
    nvband_mic_hal_t hal = make_hal(&ctx);
    nvband_mic_power_gate_t gate;
    nvband_mic_gate_init(&gate);

    bool ok = nvband_mic_gate_stop_capture(&gate, &hal, 1, 0);
    NVBAND_CHECK(ok == false);
    NVBAND_CHECK(ctx.power_off_calls == 0);
    NVBAND_CHECK(gate.state == NVBAND_MIC_STATE_IDLE);
}

static void test_request_capture_with_null_hal_makes_no_call_and_fails(void)
{
    nvband_mic_power_gate_t gate;
    nvband_mic_gate_init(&gate);
    uint32_t token = 0;
    bool ok = nvband_mic_gate_request_capture(&gate, NULL, 0, &token);
    NVBAND_CHECK(ok == false);
    NVBAND_CHECK(gate.state == NVBAND_MIC_STATE_IDLE);
}

/* ---- Normal path: exactly one request powers the mic exactly once ---- */

static void test_request_capture_grants_session_and_powers_mic_once(void)
{
    fake_mic_hal_ctx_t ctx; reset_ctx(&ctx);
    nvband_mic_hal_t hal = make_hal(&ctx);
    nvband_mic_power_gate_t gate;
    nvband_mic_gate_init(&gate);

    uint32_t token = 0;
    bool ok = nvband_mic_gate_request_capture(&gate, &hal, 1000, &token);

    NVBAND_CHECK(ok == true);
    NVBAND_CHECK(ctx.power_on_calls == 1);
    NVBAND_CHECK(ctx.power_off_calls == 0);
    NVBAND_CHECK(gate.state == NVBAND_MIC_STATE_CAPTURING);
    NVBAND_CHECK(token != 0);
    NVBAND_CHECK(gate.session_token == token);
    NVBAND_CHECK(gate.request_count == 1);

    /* The transition log must show the full path through REQUEST_PENDING
     * on the way to CAPTURING, not a direct IDLE->CAPTURING jump. */
    NVBAND_CHECK(ctx.transition_log_len == 2);
    NVBAND_CHECK(ctx.transition_log[0] == NVBAND_MIC_STATE_REQUEST_PENDING);
    NVBAND_CHECK(ctx.transition_log[1] == NVBAND_MIC_STATE_CAPTURING);
}

static void test_explicit_stop_powers_mic_off_and_reaches_idle(void)
{
    fake_mic_hal_ctx_t ctx; reset_ctx(&ctx);
    nvband_mic_hal_t hal = make_hal(&ctx);
    nvband_mic_power_gate_t gate;
    nvband_mic_gate_init(&gate);

    uint32_t token = 0;
    nvband_mic_gate_request_capture(&gate, &hal, 0, &token);
    bool stopped = nvband_mic_gate_stop_capture(&gate, &hal, token, 500000);

    NVBAND_CHECK(stopped == true);
    NVBAND_CHECK(ctx.power_off_calls == 1);
    NVBAND_CHECK(gate.state == NVBAND_MIC_STATE_DISCARDING);
    NVBAND_CHECK(gate.explicit_stop_count == 1);
    NVBAND_CHECK(gate.auto_timeout_count == 0);

    nvband_mic_gate_tick(&gate, &hal, 500001);
    NVBAND_CHECK(gate.state == NVBAND_MIC_STATE_IDLE);
    /* Reaching IDLE via the explicit-stop path must not have powered the
     * mic off a second time. */
    NVBAND_CHECK(ctx.power_off_calls == 1);
}

/* ---- Invariant 2: bounded auto-timeout even with no stop call ---- */

static void test_mic_auto_powers_off_after_max_window_with_no_stop_call(void)
{
    fake_mic_hal_ctx_t ctx; reset_ctx(&ctx);
    nvband_mic_hal_t hal = make_hal(&ctx);
    nvband_mic_power_gate_t gate;
    nvband_mic_gate_init(&gate);

    uint64_t start_us = 42;
    uint32_t token = 0;
    nvband_mic_gate_request_capture(&gate, &hal, start_us, &token);
    NVBAND_CHECK(ctx.power_on_calls == 1);

    /* Poll well within the window: mic must stay on, no auto power-off. */
    nvband_mic_gate_tick(&gate, &hal, start_us + 1000000);      /* +1s */
    nvband_mic_gate_tick(&gate, &hal, start_us + 4000000);      /* +4s */
    NVBAND_CHECK(gate.state == NVBAND_MIC_STATE_CAPTURING);
    NVBAND_CHECK(ctx.power_off_calls == 0);

    /* Poll past the max window with NO stop_capture() call at all: the
     * watchdog must power the mic off on its own. */
    uint64_t past_deadline_us = start_us + NVBAND_MIC_MAX_CAPTURE_WINDOW_US + 1;
    nvband_mic_gate_tick(&gate, &hal, past_deadline_us);

    NVBAND_CHECK(ctx.power_off_calls == 1);
    NVBAND_CHECK(gate.state == NVBAND_MIC_STATE_DISCARDING);
    NVBAND_CHECK(gate.auto_timeout_count == 1);
    NVBAND_CHECK(gate.explicit_stop_count == 0);

    /* A further tick resolves DISCARDING -> IDLE without another
     * power-off call. */
    nvband_mic_gate_tick(&gate, &hal, past_deadline_us + 1);
    NVBAND_CHECK(gate.state == NVBAND_MIC_STATE_IDLE);
    NVBAND_CHECK(ctx.power_off_calls == 1);
}

static void test_auto_timeout_fires_exactly_at_deadline_not_before(void)
{
    fake_mic_hal_ctx_t ctx; reset_ctx(&ctx);
    nvband_mic_hal_t hal = make_hal(&ctx);
    nvband_mic_power_gate_t gate;
    nvband_mic_gate_init(&gate);

    uint64_t start_us = 0;
    uint32_t token = 0;
    nvband_mic_gate_request_capture(&gate, &hal, start_us, &token);

    uint64_t one_us_before = NVBAND_MIC_MAX_CAPTURE_WINDOW_US - 1;
    nvband_mic_gate_tick(&gate, &hal, one_us_before);
    NVBAND_CHECK(gate.state == NVBAND_MIC_STATE_CAPTURING);
    NVBAND_CHECK(ctx.power_off_calls == 0);

    nvband_mic_gate_tick(&gate, &hal, NVBAND_MIC_MAX_CAPTURE_WINDOW_US);
    NVBAND_CHECK(gate.state == NVBAND_MIC_STATE_DISCARDING);
    NVBAND_CHECK(ctx.power_off_calls == 1);
}

/* ---- Invariant 3: repeated/malformed calls cannot leave it stuck on ---- */

static void test_second_request_while_capturing_is_rejected_no_double_power_on(void)
{
    fake_mic_hal_ctx_t ctx; reset_ctx(&ctx);
    nvband_mic_hal_t hal = make_hal(&ctx);
    nvband_mic_power_gate_t gate;
    nvband_mic_gate_init(&gate);

    uint32_t token1 = 0, token2 = 0;
    bool ok1 = nvband_mic_gate_request_capture(&gate, &hal, 0, &token1);
    bool ok2 = nvband_mic_gate_request_capture(&gate, &hal, 100, &token2);

    NVBAND_CHECK(ok1 == true);
    NVBAND_CHECK(ok2 == false);
    NVBAND_CHECK(ctx.power_on_calls == 1);
    NVBAND_CHECK(gate.rejected_request_count == 1);
    NVBAND_CHECK(gate.state == NVBAND_MIC_STATE_CAPTURING);
    /* Original session's token is untouched by the rejected request. */
    NVBAND_CHECK(gate.session_token == token1);
}

static void test_stop_with_stale_token_after_auto_timeout_is_rejected(void)
{
    fake_mic_hal_ctx_t ctx; reset_ctx(&ctx);
    nvband_mic_hal_t hal = make_hal(&ctx);
    nvband_mic_power_gate_t gate;
    nvband_mic_gate_init(&gate);

    uint32_t stale_token = 0;
    nvband_mic_gate_request_capture(&gate, &hal, 0, &stale_token);
    nvband_mic_gate_tick(&gate, &hal, NVBAND_MIC_MAX_CAPTURE_WINDOW_US); /* auto timeout */
    NVBAND_CHECK(gate.state == NVBAND_MIC_STATE_DISCARDING);
    NVBAND_CHECK(ctx.power_off_calls == 1);

    /* Someone (a slow app callback, e.g.) finally calls stop_capture()
     * with the now-stale token. Must be rejected, must not call
     * power_off a second time, must not change state. */
    bool ok = nvband_mic_gate_stop_capture(&gate, &hal, stale_token,
                                            NVBAND_MIC_MAX_CAPTURE_WINDOW_US + 5);
    NVBAND_CHECK(ok == false);
    NVBAND_CHECK(ctx.power_off_calls == 1);
    NVBAND_CHECK(gate.state == NVBAND_MIC_STATE_DISCARDING);
}

static void test_stop_with_wrong_token_while_capturing_is_rejected(void)
{
    fake_mic_hal_ctx_t ctx; reset_ctx(&ctx);
    nvband_mic_hal_t hal = make_hal(&ctx);
    nvband_mic_power_gate_t gate;
    nvband_mic_gate_init(&gate);

    uint32_t token = 0;
    nvband_mic_gate_request_capture(&gate, &hal, 0, &token);
    bool ok = nvband_mic_gate_stop_capture(&gate, &hal, token + 999, 10);

    NVBAND_CHECK(ok == false);
    NVBAND_CHECK(ctx.power_off_calls == 0);
    NVBAND_CHECK(gate.state == NVBAND_MIC_STATE_CAPTURING);
}

static void test_repeated_ticks_after_discarding_settle_at_idle_no_extra_power_calls(void)
{
    fake_mic_hal_ctx_t ctx; reset_ctx(&ctx);
    nvband_mic_hal_t hal = make_hal(&ctx);
    nvband_mic_power_gate_t gate;
    nvband_mic_gate_init(&gate);

    uint32_t token = 0;
    nvband_mic_gate_request_capture(&gate, &hal, 0, &token);
    nvband_mic_gate_stop_capture(&gate, &hal, token, 10);

    for (int i = 0; i < 50; i++) {
        nvband_mic_gate_tick(&gate, &hal, (uint64_t)(20 + i));
    }

    NVBAND_CHECK(gate.state == NVBAND_MIC_STATE_IDLE);
    NVBAND_CHECK(ctx.power_on_calls == 1);
    NVBAND_CHECK(ctx.power_off_calls == 1);
}

static void test_new_request_after_full_cycle_gets_a_fresh_distinct_token(void)
{
    fake_mic_hal_ctx_t ctx; reset_ctx(&ctx);
    nvband_mic_hal_t hal = make_hal(&ctx);
    nvband_mic_power_gate_t gate;
    nvband_mic_gate_init(&gate);

    uint32_t token1 = 0, token2 = 0;
    nvband_mic_gate_request_capture(&gate, &hal, 0, &token1);
    nvband_mic_gate_stop_capture(&gate, &hal, token1, 10);
    nvband_mic_gate_tick(&gate, &hal, 11); /* DISCARDING -> IDLE */
    NVBAND_CHECK(gate.state == NVBAND_MIC_STATE_IDLE);

    bool ok = nvband_mic_gate_request_capture(&gate, &hal, 12, &token2);
    NVBAND_CHECK(ok == true);
    NVBAND_CHECK(token2 != token1);
    NVBAND_CHECK(ctx.power_on_calls == 2);
    NVBAND_CHECK(gate.state == NVBAND_MIC_STATE_CAPTURING);

    /* The OLD token must still be rejected even though a new session is
     * now active — this is the exact scenario that would let a stale
     * caller stop (or worse, be believed to have stopped) someone else's
     * capture. */
    bool stale_ok = nvband_mic_gate_stop_capture(&gate, &hal, token1, 13);
    NVBAND_CHECK(stale_ok == false);
    NVBAND_CHECK(ctx.power_off_calls == 1); /* still just the first session's stop */
}

int main(void)
{
    NVBAND_RUN(test_init_state_is_idle_and_mic_never_touched);
    NVBAND_RUN(test_tick_from_idle_never_powers_mic);
    NVBAND_RUN(test_stop_capture_from_idle_is_rejected_and_never_powers_off);
    NVBAND_RUN(test_request_capture_with_null_hal_makes_no_call_and_fails);
    NVBAND_RUN(test_request_capture_grants_session_and_powers_mic_once);
    NVBAND_RUN(test_explicit_stop_powers_mic_off_and_reaches_idle);
    NVBAND_RUN(test_mic_auto_powers_off_after_max_window_with_no_stop_call);
    NVBAND_RUN(test_auto_timeout_fires_exactly_at_deadline_not_before);
    NVBAND_RUN(test_second_request_while_capturing_is_rejected_no_double_power_on);
    NVBAND_RUN(test_stop_with_stale_token_after_auto_timeout_is_rejected);
    NVBAND_RUN(test_stop_with_wrong_token_while_capturing_is_rejected);
    NVBAND_RUN(test_repeated_ticks_after_discarding_settle_at_idle_no_extra_power_calls);
    NVBAND_RUN(test_new_request_after_full_cycle_gets_a_fresh_distinct_token);
    NVBAND_TEST_MAIN_END();
}
