# tools/provisioning/

Implements CLAUDE.md §8. Run the tests:

```
node --test tests/*.js
```

10 tests, including the system-level invariant from §8: **"a unit
missing any of the three [now four, including key ceremony] required
records is not a built unit"** — `test_end_to_end` builds that up from
zero records to fully built and checks every intermediate state along
the way.

- `keyCeremony.js` — talks to U14 during manufacture (via a bench-fixture
  callback this repo can't provide hardware access for — see the
  module's header comment), enforces one identity per unit.
- `manufacturingRecords.js` — the combined record store for all four
  ship-blocking gates: shield-fence electrical test, HiPot, swell-gap
  check, and key ceremony identity. `isUnitBuilt()` is the single source
  of truth firmware's §3.2 provisioning-record gate and this tooling
  both key off of.

**Not included:** the physical bench-fixture I/O (talking to a real U14
part, a real HiPot tester, a real swell-gap fixture) — those are
hardware integrations this repo cannot provide without the actual bench
rig; the interfaces above (`secureElementGenerateIdentity` callback,
`writeRecord()`) are the seam a real bench-fixture driver plugs into.
