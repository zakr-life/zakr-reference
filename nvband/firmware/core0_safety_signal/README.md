# core0_safety_signal/

Core 0 (Cortex-M33, real-time/deterministic) image. Owns the sample
clock, safety-chain GPIO reads, and the watchdog kick. Nothing here may
block on inference or radio (those live entirely on Core 1).

- `safety/` — interlock status reader (read-only + firmware-permit bit
  only), charge-balance waveform validator, watchdog kick contingent on
  real sampling, three-LED+buzzer UI state model. **Read `safety/interlock_status.h`
  first** — it's the concrete implementation of CLAUDE.md §0.1.
- `drivers/` — mux break-before-make sequencing, AFE noise-floor
  self-test, dual-input charger arbitration, PMIC rail sequencing. See
  `drivers/README.md` for what's real logic vs. a structural placeholder
  awaiting a datasheet-verified register map.
- `sampling/` — synchronous EEG+IMU ring buffer; its push counter is what
  gates the Core 0 watchdog kick.
- `tests/` — host-testable (plain `gcc`, no Zephyr SDK) unit and
  fault-injection tests for everything above. `make test` here is part of
  `../run_host_tests.sh`.
- `CMakeLists.txt` / `prj.conf` — Zephyr/nRF Connect SDK project files,
  ready for `west build` once the SDK is installed; `main.c` (RTOS task
  wiring, device-tree bindings) is not included in this pass — it's glue
  over the modules above, gated on the final nRF Connect SDK board
  overlay for the nRF5340 core split.
