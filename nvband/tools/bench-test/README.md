# tools/bench-test/

**Explicitly non-production** (CLAUDE.md §2, §0.1, §9).

```
python3 run_bench_validation.py --unit-id <id>
```

Orchestrates the firmware host test suite (`firmware/run_host_tests.sh`,
which includes `firmware/sim/`'s phantom-driven scenarios) and writes a
timestamped JSON report to `reports/` (gitignored — these are per-run
artifacts, not source). Every report carries an explicit disclaimer that
it is not IEC 60601-1 electrical safety testing, biocompatibility
testing, or clinical validation (CLAUDE.md §9's stated non-goal, §10, and
OI-5), and a `non_production: true` flag.

This tool has no code path to real stimulation hardware. It validates
firmware software behavior against the simulated resistive phantom in
`firmware/sim/` — nothing here can, or is intended to, replace a
manufacturing-line electrical safety gate (see `../provisioning/`).
