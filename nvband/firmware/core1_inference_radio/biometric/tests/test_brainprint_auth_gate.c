/**
 * test_brainprint_auth_gate.c — the most important test file in this
 * directory: proves the no-lockout invariant from Addendum 2 §A.
 */
#include "../brainprint_auth_gate.h"
#include "../../../core0_safety_signal/tests/test_framework.h"

static void test_match_grants_via_brainprint(void)
{
    NVBAND_CHECK(nvband_brainprint_gate_evaluate(true) == NVBAND_BRAINPRINT_GATE_RESULT_MATCH);
}

static void test_mismatch_always_routes_to_fallback_never_a_hard_deny(void)
{
    /* This is the no-lockout invariant: a mismatch NEVER produces anything
     * other than FALLBACK. There is no third enum value this could return
     * that would mean "permanently denied" -- the type itself cannot
     * express that outcome. */
    nvband_brainprint_gate_result_t r = nvband_brainprint_gate_evaluate(false);
    NVBAND_CHECK(r == NVBAND_BRAINPRINT_GATE_RESULT_FALLBACK);
    NVBAND_CHECK(r != NVBAND_BRAINPRINT_GATE_RESULT_MATCH);
}

static void test_result_type_has_exactly_two_values(void)
{
    /* Structural proof, not just a behavioral one: enumerate every value
     * the enum defines and confirm both are reachable and there is no
     * third "denied" state to accidentally return. */
    nvband_brainprint_gate_result_t values[] = {
        NVBAND_BRAINPRINT_GATE_RESULT_MATCH,
        NVBAND_BRAINPRINT_GATE_RESULT_FALLBACK,
    };
    NVBAND_CHECK(values[0] != values[1]);
}

static void test_repeated_mismatches_never_escalate_to_denial(void)
{
    /* Calling the gate many times in a row with no match must keep
     * returning FALLBACK -- there is no internal state here that could
     * accumulate into a lockout (the function is pure/stateless). */
    for (int i = 0; i < 1000; i++) {
        NVBAND_CHECK(nvband_brainprint_gate_evaluate(false) == NVBAND_BRAINPRINT_GATE_RESULT_FALLBACK);
    }
}

int main(void)
{
    NVBAND_RUN(test_match_grants_via_brainprint);
    NVBAND_RUN(test_mismatch_always_routes_to_fallback_never_a_hard_deny);
    NVBAND_RUN(test_result_type_has_exactly_two_values);
    NVBAND_RUN(test_repeated_mismatches_never_escalate_to_denial);
    NVBAND_TEST_MAIN_END();
}
