/**
 * keyCeremony.js — Manufacturing-line key ceremony client.
 *
 * CLAUDE.md §8: "Key ceremony client that talks to U14 during
 * manufacture, records one identity per unit."
 *
 * The actual I2C/SWI transport to the secure element (U14) at
 * manufacture time is a hardware-in-the-loop concern (a bench fixture
 * talking to firmware/secure/secure_element_client.h's HAL over a
 * physical connection to the unit under manufacture) — not reproducible
 * in this repo. This module is the ceremony PROTOCOL/POLICY: generate
 * the key ON the part (never off-device — see secure_element_client.h's
 * header comment on why export never exists), read back the resulting
 * public identity, and write exactly one manufacturingRecords entry per
 * unit. It refuses to run the ceremony twice for the same unit (one
 * identity per unit, per CLAUDE.md §7 — "never re-derivable off-device"
 * implies never re-generated either, without an explicit, audited
 * exception process this module does not implement).
 */
const { ManufacturingRecordStore } = require('./manufacturingRecords');

class KeyCeremonyClient {
  /**
   * @param {ManufacturingRecordStore} recordStore
   * @param {(unitId: string) => {publicKeyPem: string, serial: string}} secureElementGenerateIdentity
   *   Bench-fixture callback that actually talks to U14 over its
   *   physical transport and returns the newly-generated public identity
   *   (never a private key — see file header).
   */
  constructor(recordStore, secureElementGenerateIdentity) {
    this.recordStore = recordStore;
    this.generateIdentity = secureElementGenerateIdentity;
  }

  runCeremony(unitId) {
    const existing = this.recordStore.records.get(unitId)?.get('key_ceremony_identity');
    if (existing) {
      throw new Error(
        `unit ${unitId} already has an identity on record — refusing to ` +
        `re-run the ceremony (one identity per unit, CLAUDE.md §7)`
      );
    }

    const identity = this.generateIdentity(unitId);
    if (!identity || !identity.publicKeyPem || !identity.serial) {
      this.recordStore.writeRecord(unitId, 'key_ceremony_identity', {
        pass: false, error: 'secure element did not return a valid identity',
      });
      return { success: false };
    }

    this.recordStore.writeRecord(unitId, 'key_ceremony_identity', {
      pass: true, publicKeyPem: identity.publicKeyPem, serial: identity.serial,
    });
    return { success: true, identity };
  }
}

module.exports = { KeyCeremonyClient };
