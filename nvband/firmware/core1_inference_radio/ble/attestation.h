/**
 * attestation.h — Mutual attestation handshake state machine.
 *
 * CLAUDE.md §3.10 / §7: "Pairing requires mutual attestation: phone
 * confirms device identity via secure-element-signed challenge before
 * any session control commands are accepted; device requires an
 * authenticated app before accepting session-start commands (never
 * before accepting session-stop or STOP-adjacent commands)."
 *
 * This module is the protocol state machine ONLY — it decides which BLE
 * commands are acceptable given the current attestation state. Actual
 * signing happens in firmware/secure/ (the private key never leaves
 * U14); this module calls out to a verify function, it never sees or
 * holds key material itself.
 *
 * Critical invariant, directly tested: STOP and STOP-adjacent commands
 * (emergency stop, pause) are accepted in EVERY attestation state,
 * including UNAUTHENTICATED. A malicious/unauthenticated BLE peer can
 * never be blocked from stopping a session, only from starting one — see
 * CLAUDE.md §5 ("stop must always be immediately available") and the
 * threat model in §7.
 */
#ifndef NVBAND_ATTESTATION_H
#define NVBAND_ATTESTATION_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    NVBAND_ATTEST_STATE_UNAUTHENTICATED = 0,
    NVBAND_ATTEST_STATE_CHALLENGE_SENT,
    NVBAND_ATTEST_STATE_AUTHENTICATED,
    NVBAND_ATTEST_STATE_REVOKED,
} nvband_attest_state_t;

typedef enum {
    NVBAND_BLE_CMD_SESSION_START = 0,
    NVBAND_BLE_CMD_SESSION_STOP,
    NVBAND_BLE_CMD_SESSION_PAUSE,
    NVBAND_BLE_CMD_OTA_BEGIN,
    NVBAND_BLE_CMD_READ_STATUS,
    NVBAND_BLE_CMD_REARM_REQUEST,
} nvband_ble_command_t;

typedef struct {
    nvband_attest_state_t state;
    uint8_t                pending_challenge[32];
    uint32_t                challenge_id;
} nvband_attest_session_t;

void nvband_attest_session_init(nvband_attest_session_t *s);

/** Device generates and records a challenge, moves to CHALLENGE_SENT.
 *  challenge_out must be >= 32 bytes. */
void nvband_attest_begin_challenge(nvband_attest_session_t *s,
                                    const uint8_t *random32,
                                    uint8_t *challenge_out);

/**
 * verify_fn is supplied by the caller and delegates to firmware/secure/
 * (ATECC608B/SE05x signature verification against the app's known public
 * key). This function itself holds no cryptographic material — it only
 * sequences the state machine based on verify_fn's boolean answer.
 */
typedef bool (*nvband_attest_verify_fn)(const uint8_t *challenge,
                                         size_t challenge_len,
                                         const uint8_t *signature,
                                         size_t signature_len,
                                         void *verify_ctx);

void nvband_attest_complete(nvband_attest_session_t *s,
                             const uint8_t *signature, size_t signature_len,
                             nvband_attest_verify_fn verify_fn,
                             void *verify_ctx);

void nvband_attest_revoke(nvband_attest_session_t *s);

/**
 * The core policy decision: is this command acceptable given the current
 * attestation state? STOP/PAUSE always return true (see file header).
 * SESSION_START, OTA_BEGIN, and REARM_REQUEST require AUTHENTICATED.
 * READ_STATUS is always allowed (device state, not control).
 */
bool nvband_attest_command_allowed(const nvband_attest_session_t *s,
                                    nvband_ble_command_t cmd);

#endif /* NVBAND_ATTESTATION_H */
