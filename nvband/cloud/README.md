# cloud/

Implements CLAUDE.md §6. Node.js, standard library `node:crypto` +
`node:test` only — no framework dependency installed in this pass (see
"Not included" below), so the decision logic that actually matters is
runnable and tested today:

```
node --test tests/*.test.js
```

34 tests across device authentication, audit-log tamper-evidence,
retention/access scoping, clinician RBAC, staged-rollout halt logic, and
analytics de-identification.

## Layout

- `ingestion/deviceAuth.js` — Ed25519 signature verification against a
  per-device public key established at provisioning (never a bare device
  ID or password as proof of identity).
- `storage/retentionPolicy.js` — retention has no indefinite default
  (construction throws without an explicit finite value per store); read
  access is scoped per store per role.
- `clinician-portal/rbac.js` — `authorizeAndAudit()` makes the access
  decision and writes the audit entry in the same call, so the two can't
  drift apart.
- `fleet-ota/rollout.js` — staged percentage rollout; halts (never
  auto-reverts) on a statistically meaningful fault-rate spike. See the
  module docstring for why "surface it for human decision" is enforced
  at the API-shape level (no revert/rollback method exists).
- `analytics/deidentify.js` — opt-in gated; strips direct identifiers;
  k-anonymity gate before any aggregate publishes.
- `audit/auditLog.js` — hash-chained append-only log; `verifyChain()`
  detects both tampering and deletion. Backs every module above.

## Not included in this pass

HTTP framework wiring (Express/Fastify routes, request validation
middleware, database persistence) — the modules above are the actual
policy decisions CLAUDE.md §6 requires, framework-agnostic on purpose so
they don't need to change if the HTTP layer choice changes. Wiring them
behind real endpoints is mechanical once ZAKR picks a framework/hosting
target; guessing one here would be an unreviewed infrastructure decision,
not a design decision this repo should make silently.
