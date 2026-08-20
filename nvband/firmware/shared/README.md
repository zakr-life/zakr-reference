# shared/

HAL-agnostic code used by both core images — no Zephyr dependency, so
everything here is host-testable.

- `nvband_constants.h` — every named physical/timing constant from
  CLAUDE.md §1, including derived relationships (e.g. current ceiling is
  computed from pad area * max density, not hardcoded), so a future
  pad-diameter or battery-capacity change propagates instead of drifting.
- `nvband_channel_config.{h,c}` — data-driven stim/EEG channel
  configuration (TODO(OI-1): electrode/channel count unresolved — see
  CLAUDE.md §11).
- `nvband_ipc_spsc.{h,c}` — the lock-free single-producer/single-consumer
  queue that is the ENTIRE Core0<->Core1 communication surface. No
  mutex, no blocking primitive anywhere in it — see
  `tests/test_ipc_spsc.c` for the property this guarantees.
