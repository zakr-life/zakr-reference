/* Traces: TRC-ATT-01..06. Threat-model test cases from CLAUDE.md §7:
 * a malicious BLE peer attempting to command stimulation without
 * authentication must be refused for START but never for STOP. */
#include "../../core0_safety_signal/tests/test_framework.h"
#include "../ble/attestation.h"
#include <string.h>

static bool verify_always_true(const uint8_t *c, size_t cl,
                                const uint8_t *sig, size_t sl, void *ctx)
{
    (void)c; (void)cl; (void)sig; (void)sl; (void)ctx;
    return true;
}
static bool verify_always_false(const uint8_t *c, size_t cl,
                                 const uint8_t *sig, size_t sl, void *ctx)
{
    (void)c; (void)cl; (void)sig; (void)sl; (void)ctx;
    return false;
}

static void test_unauthenticated_peer_cannot_start_session(void)
{
    nvband_attest_session_t s;
    nvband_attest_session_init(&s);

    NVBAND_CHECK(nvband_attest_command_allowed(&s, NVBAND_BLE_CMD_SESSION_START) == false);
    NVBAND_CHECK(nvband_attest_command_allowed(&s, NVBAND_BLE_CMD_OTA_BEGIN) == false);
    NVBAND_CHECK(nvband_attest_command_allowed(&s, NVBAND_BLE_CMD_REARM_REQUEST) == false);
}

static void test_unauthenticated_peer_can_always_stop_or_pause(void)
{
    nvband_attest_session_t s;
    nvband_attest_session_init(&s);
    NVBAND_CHECK(nvband_attest_command_allowed(&s, NVBAND_BLE_CMD_SESSION_STOP) == true);
    NVBAND_CHECK(nvband_attest_command_allowed(&s, NVBAND_BLE_CMD_SESSION_PAUSE) == true);
    NVBAND_CHECK(nvband_attest_command_allowed(&s, NVBAND_BLE_CMD_READ_STATUS) == true);

    nvband_attest_revoke(&s); /* even in the worst state */
    NVBAND_CHECK(nvband_attest_command_allowed(&s, NVBAND_BLE_CMD_SESSION_STOP) == true);
    NVBAND_CHECK(nvband_attest_command_allowed(&s, NVBAND_BLE_CMD_SESSION_PAUSE) == true);
}

static void test_successful_challenge_response_authenticates(void)
{
    nvband_attest_session_t s;
    nvband_attest_session_init(&s);
    uint8_t rnd[32]; memset(rnd, 0xAB, sizeof(rnd));
    uint8_t challenge[32];
    nvband_attest_begin_challenge(&s, rnd, challenge);
    NVBAND_CHECK(memcmp(challenge, rnd, 32) == 0);
    NVBAND_CHECK(s.state == NVBAND_ATTEST_STATE_CHALLENGE_SENT);

    uint8_t fake_sig[64] = {0};
    nvband_attest_complete(&s, fake_sig, sizeof(fake_sig),
                            verify_always_true, NULL);

    NVBAND_CHECK(s.state == NVBAND_ATTEST_STATE_AUTHENTICATED);
    NVBAND_CHECK(nvband_attest_command_allowed(&s, NVBAND_BLE_CMD_SESSION_START) == true);
}

static void test_failed_signature_verification_stays_unauthenticated(void)
{
    nvband_attest_session_t s;
    nvband_attest_session_init(&s);
    uint8_t rnd[32] = {0};
    uint8_t challenge[32];
    nvband_attest_begin_challenge(&s, rnd, challenge);

    uint8_t fake_sig[64] = {0};
    nvband_attest_complete(&s, fake_sig, sizeof(fake_sig),
                            verify_always_false, NULL);

    NVBAND_CHECK(s.state == NVBAND_ATTEST_STATE_UNAUTHENTICATED);
    NVBAND_CHECK(nvband_attest_command_allowed(&s, NVBAND_BLE_CMD_SESSION_START) == false);
}

/* Malicious peer replaying a stale/unsolicited "complete" without a live
 * challenge must not authenticate. */
static void test_complete_without_outstanding_challenge_ignored(void)
{
    nvband_attest_session_t s;
    nvband_attest_session_init(&s);
    uint8_t fake_sig[64] = {0};
    nvband_attest_complete(&s, fake_sig, sizeof(fake_sig),
                            verify_always_true, NULL);
    NVBAND_CHECK(s.state == NVBAND_ATTEST_STATE_UNAUTHENTICATED);
}

static void test_revoked_session_cannot_start(void)
{
    nvband_attest_session_t s;
    nvband_attest_session_init(&s);
    uint8_t rnd[32] = {0};
    uint8_t challenge[32];
    nvband_attest_begin_challenge(&s, rnd, challenge);
    uint8_t fake_sig[64] = {0};
    nvband_attest_complete(&s, fake_sig, sizeof(fake_sig),
                            verify_always_true, NULL);
    NVBAND_CHECK(s.state == NVBAND_ATTEST_STATE_AUTHENTICATED);

    nvband_attest_revoke(&s);
    NVBAND_CHECK(nvband_attest_command_allowed(&s, NVBAND_BLE_CMD_SESSION_START) == false);
}

int main(void)
{
    NVBAND_RUN(test_unauthenticated_peer_cannot_start_session);
    NVBAND_RUN(test_unauthenticated_peer_can_always_stop_or_pause);
    NVBAND_RUN(test_successful_challenge_response_authenticates);
    NVBAND_RUN(test_failed_signature_verification_stays_unauthenticated);
    NVBAND_RUN(test_complete_without_outstanding_challenge_ignored);
    NVBAND_RUN(test_revoked_session_cannot_start);
    NVBAND_TEST_MAIN_END();
}
