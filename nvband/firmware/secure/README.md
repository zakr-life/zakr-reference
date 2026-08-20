# secure/

Implements CLAUDE.md §3.8 / §7: secure element (U14) client, root of
trust, key ceremony support.

- `secure_element_client.{h,c}` — the ONLY firmware surface to U14. By
  construction there is no function here that exports, imports, or reads
  out private key material — see the header comment for why that's a
  hardware-capability fact (ATECC608B/SE05x-class parts), not just a
  convention this code follows.
- `nvband_secure_element_verify_adapter()` wires this module directly to
  `core1_inference_radio/ble/attestation.h`'s verify-function hook, so
  the attestation state machine never sees key material either — it only
  ever gets a boolean back.
- **Not included in this pass:** the physical I2C/SWI transport driver
  for the chosen part (ATECC608B vs SE05x — final selection pending,
  they are not command-compatible) and the manufacturing-time key
  ceremony sequence itself, which lives in `tools/provisioning/` and
  talks to this same HAL surface during manufacture.
- Key ceremony invariant (§8): a unit is not "built" until its identity
  is provisioned here AND its HiPot + swell-gap records are written —
  see `tools/provisioning/README.md` for the combined system test.
