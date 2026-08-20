/**
 * retentionPolicy.js — Explicit, configurable data retention (never
 * indefinite by default) and store-separation scoping.
 *
 * CLAUDE.md §6: "encrypted at rest, explicit data retention policy
 * configuration (not indefinite by default), separation between raw
 * session data, derived analytics, and identity/PII stores so access can
 * be scoped independently."
 */

/** No default here is "forever" — a caller MUST supply a finite
 *  retentionDays, or construction throws. This is deliberate: "not
 *  indefinite by default" means there is no silent infinite default to
 *  fall back to. */
class RetentionPolicy {
  constructor({ rawSessionDataDays, derivedAnalyticsDays, identityStoreDays }) {
    for (const [name, v] of Object.entries({ rawSessionDataDays, derivedAnalyticsDays, identityStoreDays })) {
      if (!(Number.isFinite(v) && v > 0)) {
        throw new Error(`${name} must be a finite positive number of days (no indefinite default)`);
      }
    }
    this.rawSessionDataDays = rawSessionDataDays;
    this.derivedAnalyticsDays = derivedAnalyticsDays;
    this.identityStoreDays = identityStoreDays;
  }

  shouldPurge(store, recordAgeDays) {
    const limit = this._limitFor(store);
    return recordAgeDays > limit;
  }

  _limitFor(store) {
    switch (store) {
      case 'raw_session_data': return this.rawSessionDataDays;
      case 'derived_analytics': return this.derivedAnalyticsDays;
      case 'identity_store': return this.identityStoreDays;
      default: throw new Error(`unknown store: ${store}`);
    }
  }
}

/**
 * Access scoping: which stores a given role may read. Enforced here as
 * pure policy so it's independently testable from any HTTP framework
 * choice — the clinician-portal RBAC layer (rbac.js) calls this for the
 * data-access dimension of its decision, on top of its own role checks.
 */
const STORE_ACCESS_BY_ROLE = Object.freeze({
  clinician: ['raw_session_data', 'derived_analytics'],       // never identity_store directly
  caregiver: ['derived_analytics'],                            // adherence view only
  researcher: ['derived_analytics'],                           // de-identified only, never raw
  admin: ['raw_session_data', 'derived_analytics', 'identity_store'],
});

function canAccessStore(role, store) {
  const allowed = STORE_ACCESS_BY_ROLE[role];
  return Array.isArray(allowed) && allowed.includes(store);
}

module.exports = { RetentionPolicy, STORE_ACCESS_BY_ROLE, canAccessStore };
