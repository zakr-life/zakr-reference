# sim/

Hardware-in-the-loop / software-in-the-loop simulation only (CLAUDE.md
§9). **Never linked into a fleet-ota production image** — see
`../docs/ci_gates.md` Gate 2.

- `resistive_phantom.{h,c}` — a minimal resistive head-phantom model
  (commanded current -> synthetic measured voltage/impedance), including
  a lifted-electrode fault injection knob.
- `tests/test_end_to_end_scenarios.c` — ties the phantom to the real
  interlock/charge-balance/mux modules from `../core0_safety_signal/`,
  including the "make-before-break mux" fault injector required by
  CLAUDE.md §9 even though the real U19 part is break-before-make by
  datasheet — proving firmware would detect that anomaly if it ever
  occurred.

Nothing in this directory has, or may ever gain, a code path that
asserts stimulation enable (CLAUDE.md §0.1) — it only produces synthetic
sensor readings and observes how the real safety modules react.
