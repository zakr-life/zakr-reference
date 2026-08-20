/**
 * deviceAuth.js — Attestation-gated device authentication for session
 * ingestion.
 *
 * CLAUDE.md §6: "device-authenticated (attestation-based, not
 * password-based) session upload API; reject any payload without a
 * valid device identity signature."
 *
 * The actual signature scheme mirrors firmware/secure/: a per-device
 * public key established at provisioning time (tools/provisioning) is
 * the trust anchor — this module verifies a signature against that
 * known public key, it never accepts a bare device ID as proof of
 * identity (that would be exactly the password-like pattern CLAUDE.md
 * §6 rules out).
 */
const crypto = require('crypto');

/**
 * @param {Map<string, string>} deviceIdToPublicKeyPem registered device
 *   identities (populated by tools/provisioning at manufacture time)
 */
class DeviceAuthenticator {
  constructor(deviceIdToPublicKeyPem) {
    this.registry = deviceIdToPublicKeyPem;
  }

  /**
   * @param {{deviceId: string, payload: Buffer, signature: Buffer}} upload
   * @returns {{accepted: boolean, reason: string}}
   */
  verifyUpload(upload) {
    if (!upload || !upload.deviceId || !upload.payload || !upload.signature) {
      return { accepted: false, reason: 'malformed upload: missing device id, payload, or signature' };
    }

    const publicKeyPem = this.registry.get(upload.deviceId);
    if (!publicKeyPem) {
      return { accepted: false, reason: 'unknown device id — not provisioned' };
    }

    let verified = false;
    try {
      verified = crypto.verify(
        null, // Ed25519 doesn't take a separate hash algorithm
        upload.payload,
        publicKeyPem,
        upload.signature
      );
    } catch (e) {
      return { accepted: false, reason: `signature verification error: ${e.message}` };
    }

    if (!verified) {
      return { accepted: false, reason: 'signature does not verify against registered device key' };
    }

    return { accepted: true, reason: 'ok' };
  }
}

module.exports = { DeviceAuthenticator };
