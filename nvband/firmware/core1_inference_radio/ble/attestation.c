#include "attestation.h"
#include <string.h>

void nvband_attest_session_init(nvband_attest_session_t *s)
{
    if (s == NULL) {
        return;
    }
    memset(s, 0, sizeof(*s));
    s->state = NVBAND_ATTEST_STATE_UNAUTHENTICATED;
}

void nvband_attest_begin_challenge(nvband_attest_session_t *s,
                                    const uint8_t *random32,
                                    uint8_t *challenge_out)
{
    if (s == NULL || random32 == NULL || challenge_out == NULL) {
        return;
    }
    memcpy(s->pending_challenge, random32, sizeof(s->pending_challenge));
    memcpy(challenge_out, random32, sizeof(s->pending_challenge));
    s->challenge_id++;
    s->state = NVBAND_ATTEST_STATE_CHALLENGE_SENT;
}

void nvband_attest_complete(nvband_attest_session_t *s,
                             const uint8_t *signature, size_t signature_len,
                             nvband_attest_verify_fn verify_fn,
                             void *verify_ctx)
{
    if (s == NULL || verify_fn == NULL) {
        return;
    }
    if (s->state != NVBAND_ATTEST_STATE_CHALLENGE_SENT) {
        /* Completing without an outstanding challenge is never valid,
         * regardless of what the signature claims. */
        return;
    }

    bool ok = verify_fn(s->pending_challenge, sizeof(s->pending_challenge),
                         signature, signature_len, verify_ctx);

    s->state = ok ? NVBAND_ATTEST_STATE_AUTHENTICATED
                  : NVBAND_ATTEST_STATE_UNAUTHENTICATED;
}

void nvband_attest_revoke(nvband_attest_session_t *s)
{
    if (s == NULL) {
        return;
    }
    s->state = NVBAND_ATTEST_STATE_REVOKED;
}

bool nvband_attest_command_allowed(const nvband_attest_session_t *s,
                                    nvband_ble_command_t cmd)
{
    if (s == NULL) {
        return false;
    }

    /* STOP/PAUSE are never gated by attestation state — an unauthenticated
     * or even actively malicious BLE peer must still be able to stop a
     * session. See file header / CLAUDE.md §5, §7. */
    if (cmd == NVBAND_BLE_CMD_SESSION_STOP ||
        cmd == NVBAND_BLE_CMD_SESSION_PAUSE) {
        return true;
    }

    if (cmd == NVBAND_BLE_CMD_READ_STATUS) {
        return true;
    }

    return s->state == NVBAND_ATTEST_STATE_AUTHENTICATED;
}
