/**
 * secure_element_client.h — Client-side interface to the secure element
 * (U14, ATECC608B/SE05x).
 *
 * CLAUDE.md §3.8 / §7: "Firmware requests signatures from the secure
 * element; the private key never transits the SoC, never appears in
 * firmware binaries, never appears in logs." and "Root of trust: secure
 * element (U14) per device, one identity per unit, provisioned at
 * manufacture (§8), never re-derivable off-device."
 *
 * This header defines the ONLY surface firmware has to the secure
 * element: ask it to sign a digest, ask it for the (public) device
 * identity, ask it to verify a signature against a stored public key
 * (used for the app's public key during attestation, and for
 * firmware/model image signature verification during OTA). There is no
 * function here that reads out a private key, imports one, or exports
 * key material — by construction, not by convention, matching the BOM
 * part's actual capability (ATECC608B/SE05x-class parts do not support
 * private key export once generated in-place).
 *
 * The actual I2C/SWI transport driver to the physical part is not
 * included in this pass (needs the part's command-set datasheet,
 * ATECC608B vs SE05x are NOT command-compatible so this choice is also
 * gated on final BOM selection — see firmware/docs/open-items). The
 * interface below is written so that choice is swappable behind it.
 */
#ifndef NVBAND_SECURE_ELEMENT_CLIENT_H
#define NVBAND_SECURE_ELEMENT_CLIENT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    /** Returns the device's public identity (public key + serial), never
     *  the private key. */
    bool (*get_device_identity)(uint8_t *pubkey_out, size_t pubkey_len,
                                 uint8_t *serial_out, size_t serial_len,
                                 void *ctx);
    /** Signs a digest using the device's private key, which never leaves
     *  the part. */
    bool (*sign_digest)(const uint8_t *digest32, uint8_t *signature_out,
                         size_t signature_out_len, void *ctx);
    /** Verifies a signature against a stored public key slot (e.g. the
     *  paired app's key, or ZAKR's OTA signing key). */
    bool (*verify_signature)(uint8_t key_slot,
                              const uint8_t *digest32,
                              const uint8_t *signature, size_t signature_len,
                              void *ctx);
    void *ctx;
} nvband_secure_element_hal_t;

/** Convenience wrapper matching nvband_attest_verify_fn's signature
 *  (see core1_inference_radio/ble/attestation.h) so the attestation state
 *  machine can be wired directly to a secure-element-backed verify call
 *  without any intermediate key handling in the caller. */
bool nvband_secure_element_verify_adapter(const uint8_t *challenge,
                                           size_t challenge_len,
                                           const uint8_t *signature,
                                           size_t signature_len,
                                           void *verify_ctx);

#endif /* NVBAND_SECURE_ELEMENT_CLIENT_H */
