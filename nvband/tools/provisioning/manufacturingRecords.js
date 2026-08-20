/**
 * manufacturingRecords.js — The three ship-blocking manufacturing gates,
 * combined into one system-level invariant.
 *
 * CLAUDE.md §8: "records the resulting record to the same system that
 * will later store each unit's HiPot pass/fail (§1) and swell-gap check
 * record (mirrors the dossier's three ship-blocking manufacturing gates:
 * shield-fence electrical test, HiPot, swell-gap check)." and "treat 'a
 * unit missing any of the three records is not a built unit' as a
 * system-level invariant with an automated test, not just a process
 * note."
 *
 * This store is the same one firmware's provisioning-record gate
 * (CLAUDE.md §3.2) checks at runtime before arming a session — a real
 * deployment would sync this to on-device secure storage at
 * provisioning time; this module is the manufacturing-line-side source
 * of truth those records come from.
 */

const REQUIRED_RECORD_TYPES = Object.freeze([
  'shield_fence_electrical_test',
  'hipot',
  'swell_gap_check',
  'key_ceremony_identity', // device identity from secure_element_client.h's ceremony
]);

class ManufacturingRecordStore {
  constructor() {
    /** @type {Map<string, Map<string, object>>} unitId -> recordType -> record */
    this.records = new Map();
  }

  writeRecord(unitId, recordType, record) {
    if (!REQUIRED_RECORD_TYPES.includes(recordType)) {
      throw new Error(`unknown record type: ${recordType}`);
    }
    if (!record || typeof record !== 'object' || !('pass' in record)) {
      throw new Error(`record for ${recordType} must include a boolean 'pass' field`);
    }
    if (!this.records.has(unitId)) {
      this.records.set(unitId, new Map());
    }
    this.records.get(unitId).set(recordType, { ...record, writtenAtMs: Date.now() });
  }

  /**
   * The system-level invariant: a unit is "built" only if EVERY required
   * record type is present AND every one of them passed. Missing OR
   * failed records both count as "not built" — there is no partial-credit
   * state.
   */
  isUnitBuilt(unitId) {
    const unitRecords = this.records.get(unitId);
    if (!unitRecords) {
      return { built: false, reason: 'no records at all for this unit' };
    }
    for (const requiredType of REQUIRED_RECORD_TYPES) {
      const rec = unitRecords.get(requiredType);
      if (!rec) {
        return { built: false, reason: `missing record: ${requiredType}` };
      }
      if (rec.pass !== true) {
        return { built: false, reason: `record ${requiredType} did not pass` };
      }
    }
    return { built: true, reason: 'all required records present and passing' };
  }

  /** What firmware's §3.2 gate would see: a single boolean + the HiPot
   *  reference it specifically requires (CLAUDE.md §3.2: "refuse to arm
   *  any stimulation session on a unit whose provisioning record does
   *  not carry a valid stored HiPot pass reference"). */
  hasValidHipotReference(unitId) {
    const unitRecords = this.records.get(unitId);
    const hipot = unitRecords && unitRecords.get('hipot');
    return !!(hipot && hipot.pass === true);
  }
}

module.exports = { ManufacturingRecordStore, REQUIRED_RECORD_TYPES };
