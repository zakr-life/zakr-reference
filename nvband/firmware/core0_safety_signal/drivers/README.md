# drivers/

Implements CLAUDE.md §3.1–§3.3, §3.6.

## What's implemented at full logic depth (host-testable, see `../tests/`)

- `afe_noise_selftest.{h,c}` — startup noise-floor self-test pass/fail logic (§3.3).
- `charger_arbitration.{h,c}` — USB/Qi dual-input arbitration policy (§3.1).
- `pmic_sequencer.{h,c}` — rail bring-up sequencing + AFE-reset-release gate (§3.1).
- `mux_bbm.{h,c}` — break-before-make mux sequencing + anomaly detection (§3.3).

These are hardware-independent: each takes a small HAL struct of function
pointers and is unit-tested against a fake. That's deliberate — it's what
makes them testable without real silicon, and it's the seam a real Zephyr
sensor/regulator driver plugs into.

## What's a structural placeholder, not a working implementation

Register-level SPI/I2C drivers for the actual BOM parts:

- `afe_ads1299_zephyr.c` (U1, TI ADS1299/ADS1299-4) — **not included**.
  Needs the datasheet-verified register map (config1-4, CH1SET-CH8SET,
  bias drive, lead-off detect current source) before it can be written
  honestly. Wire it to call `nvband_afe_noise_selftest_evaluate()` with
  the shorted-input capture once available.
- `dac_ad5687r_zephyr.c` (U5) — **not included**. Only ever *requests* a
  setpoint over the isolated SPI path (CLAUDE.md §3.4); never writes to
  any interlock GPIO. Every waveform it's asked to send must have already
  passed `charge_balance.h`'s validator.
- `imu_bmi270_zephyr.c` (U17) — **not included**. Must be triggered from
  the same clock/trigger as the AFE sample (see `../sampling/`), not
  polled independently — synchronicity is the requirement, not just
  "close enough" timing.
- `rtc_pcf85063_zephyr.c` (U20) — **not included**. Session timestamps
  are clinical evidence (§1); must survive a flat main battery via the
  backup cell.

Each of these is a small, mechanical Zephyr sensor/regulator driver once
a qualified reviewer signs off the register map against the actual part
datasheet — deliberately not fabricated here, per CLAUDE.md's rule
against unearned claims (§10, §12.5: "if implementing a requirement would
require weakening §0.1 [or inventing facts], stop... surface the
conflict"). Guessing register addresses for a device that injects current
into a person is exactly the kind of guess this repo does not make.
