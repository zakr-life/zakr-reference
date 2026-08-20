# core1_inference_radio/

Core 1 image: inference, BLE, session storage. Isolated from Core 0 via
the lock-free SPSC queue in `../shared/nvband_ipc_spsc.h` — nothing in
this directory can block, starve, or delay the Core 0 watchdog kick.

- `inference/` — `stim_command_clamp.{h,c}` is the hard boundary between
  whatever a model outputs and what firmware will ever consider sending
  toward the DAC (CLAUDE.md §4: "no model output may ever be the sole
  gate for delivering current"). `epoch_budget.{h,c}` drops (never
  blocks on) an inference epoch that overruns its time budget. The
  TinyML runtime that actually executes an exported model
  (`../../models/export/`) is not included in this pass — it's a
  straightforward TFLite-Micro (or comparable) integration once a real
  exported model exists to benchmark against.
- `ble/` — `attestation.{h,c}` is the mutual-attestation state machine
  (CLAUDE.md §3.10, §7): governs which BLE commands are acceptable given
  authentication state. STOP/PAUSE are allowed in every state, by
  construction. GATT service/characteristic table definitions
  (`gatt_services.h`) are not included in this pass — straightforward
  once the exact characteristic UUIDs are assigned.
- `session/` — `session_store.{h,c}`: write-then-commit + checksum
  session record protocol (CLAUDE.md §3.9), independent of the
  underlying NAND (U16) block device so it's host-testable against a
  fake.
- `tests/` — host-testable suites for all of the above.
