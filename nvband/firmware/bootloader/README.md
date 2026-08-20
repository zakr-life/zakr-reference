# bootloader/

Implements CLAUDE.md §3.8 (A/B slots, rollback) and the debug-lockout
half of §7/§9.

- `ab_rollback.{h,c}` — the confirm/rollback DECISION logic (real,
  host-tested — see `tests/`). MCUboot itself (image swap, Ed25519/ECDSA
  signature verification, secure-boot chain-of-trust to U14's root key)
  is a west-managed module, not vendored source in this repo; this
  module is the application-level "did the new image pass self-test"
  hook MCUboot calls into.
- **Debug-lockout CI gate** (CLAUDE.md §7: "production build pipeline
  must fail if debug-enabled artifacts are tagged for the fleet-ota
  release channel"): implemented as a build-config check, not a host
  test — see `../docs/ci_gates.md`. The engineering/debug build type
  (`prj.conf` with `CONFIG_DEBUG=y`, SWD open) is produced by both core
  images by default in this repo; a distinct `*_release.conf` overlay
  (locks SWD, disables `CONFIG_DEBUG`, enables secure boot enforcement)
  is required before any image may be tagged for `cloud/fleet-ota/` —
  not yet generated, gated on final nRF Connect SDK secure-boot Kconfig
  surface.
- Bring-up bypass reminder (CLAUDE.md §0.1): any hardware bypass used
  during bring-up is a soldered link manufacturing physically removes,
  never a firmware/bootloader flag. No Kconfig option in this directory
  may gate stimulation interlock behavior — only debug/logging verbosity
  and OTA channel selection.
