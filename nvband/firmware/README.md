# firmware/

Implements CLAUDE.md §3 (firmware build order) and the hardware half of
§0.1 (firmware may veto stimulation, never solely authorize it).

Run every host-testable safety suite in this tree:

```
./run_host_tests.sh
```

(17 test binaries, ~10,300 assertions as of this pass — all pure C11,
compiled with `gcc -Wall -Wextra -Werror`, no Zephyr SDK required. This
is what CI Gate 1 in `docs/ci_gates.md` runs.)

## Layout

- `core0_safety_signal/` — Core 0 (Cortex-M33): sample clock, safety-chain
  GPIO reads, watchdog kick. Deterministic, real-time, never blocks.
- `core1_inference_radio/` — Core 1: inference, BLE, session store.
  Isolated from Core 0 by the lock-free IPC in `shared/`.
- `secure/` — secure element (U14) client; root of trust.
- `bootloader/` — A/B slot rollback decision logic + MCUboot integration point.
- `shared/` — constants, channel config, Core0<->Core1 IPC — no Zephyr
  dependency, used by both core images.
- `sim/` — bench-test / software-in-the-loop only. Never linked into a
  fleet-ota production image.
- `docs/` — traceability matrix, CI gates, IEC 62304-style artifact stubs.

Every module traces back to a CLAUDE.md section in its own header
comment — start there, not here, for the "why."
