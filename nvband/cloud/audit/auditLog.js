/**
 * auditLog.js — Append-only, tamper-evident audit log.
 *
 * CLAUDE.md §6/§7: "a tamper-evident audit log (§ audit/) of every access
 * and export" and "Audit service: append-only, tamper-evident log of
 * data access, consent changes, OTA rollouts, and provisioning events."
 *
 * Implementation: a simple hash chain (each entry's hash includes the
 * previous entry's hash), the same structural idea as a blockchain or a
 * git commit chain, without any of blockchain's distributed-consensus
 * machinery — this is a single append-only log, and the hash chain's
 * only job is to make any retroactive edit or deletion detectable by
 * `verifyChain()`. Not a claim of 21 CFR Part 11 compliance (CLAUDE.md
 * §1: "structured for eventual Part 11-style review, without claiming
 * compliance").
 */
const crypto = require('crypto');

const GENESIS_HASH = '0'.repeat(64);

function hashEntry(prevHash, entry) {
  const h = crypto.createHash('sha256');
  h.update(prevHash);
  h.update(JSON.stringify(entry));
  return h.digest('hex');
}

class AuditLog {
  constructor() {
    /** @type {{entry: object, prevHash: string, hash: string}[]} */
    this.records = [];
  }

  /**
   * Appends an entry. `entry` should include at minimum: type (e.g.
   * 'DATA_ACCESS', 'CONSENT_CHANGED', 'OTA_ROLLOUT', 'PROVISIONING'),
   * actorId, timestampMs, and event-specific fields. Returns the record
   * that was appended (including its hash) so callers can, e.g., store
   * the hash alongside an exported report as a tamper-evidence receipt.
   */
  append(entry) {
    if (!entry || typeof entry !== 'object' || !entry.type) {
      throw new Error('audit entries must be objects with a `type` field');
    }
    const prevHash = this.records.length > 0
      ? this.records[this.records.length - 1].hash
      : GENESIS_HASH;
    const hash = hashEntry(prevHash, entry);
    const record = { entry, prevHash, hash };
    this.records.push(record);
    return record;
  }

  /**
   * Re-derives every entry's hash from scratch and compares to what's
   * stored. Returns { valid: true } or { valid: false, brokenAtIndex }.
   * Detects both a tampered entry (content changed but hash left as-is)
   * and a spliced-out entry (the chain no longer links).
   */
  verifyChain() {
    let prevHash = GENESIS_HASH;
    for (let i = 0; i < this.records.length; i++) {
      const r = this.records[i];
      if (r.prevHash !== prevHash) {
        return { valid: false, brokenAtIndex: i, reason: 'prevHash mismatch (record missing or reordered)' };
      }
      const recomputed = hashEntry(r.prevHash, r.entry);
      if (recomputed !== r.hash) {
        return { valid: false, brokenAtIndex: i, reason: 'entry content does not match its hash (tampered)' };
      }
      prevHash = r.hash;
    }
    return { valid: true };
  }

  /** Read-only export — there is deliberately no `deleteEntry` or
   *  `editEntry` method on this class; append-only is an API-shape
   *  guarantee, not just a convention. */
  toArray() {
    return this.records.map(r => ({ ...r }));
  }
}

module.exports = { AuditLog, hashEntry, GENESIS_HASH };
