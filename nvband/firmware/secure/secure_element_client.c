#include "secure_element_client.h"

typedef struct {
    const nvband_secure_element_hal_t *hal;
    uint8_t key_slot;
} nvband_secure_element_verify_ctx_t;

bool nvband_secure_element_verify_adapter(const uint8_t *challenge,
                                           size_t challenge_len,
                                           const uint8_t *signature,
                                           size_t signature_len,
                                           void *verify_ctx)
{
    nvband_secure_element_verify_ctx_t *c =
        (nvband_secure_element_verify_ctx_t *)verify_ctx;
    if (c == NULL || c->hal == NULL || c->hal->verify_signature == NULL) {
        return false;
    }
    if (challenge_len != 32) {
        /* This adapter treats the challenge as an already-formed 32-byte
         * digest (matching nvband_attest_session_t's fixed challenge
         * size); anything else is a caller bug, never a fallback path. */
        return false;
    }
    return c->hal->verify_signature(c->key_slot, challenge, signature,
                                     signature_len, c->hal->ctx);
}
