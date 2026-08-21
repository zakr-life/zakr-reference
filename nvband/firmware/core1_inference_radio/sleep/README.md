# core1_inference_radio/sleep/

Implements Addendum 2 §E ("Sleep-state monitoring and physician
informing"). Reuses the existing EEG (U1) + IMU (U17) synchronous
acquisition path already used for waking sessions — no new hardware, no
new driver, no register-level code in this directory.

- `overnight_session_config.h` / `.c` — HAL-independent (no register
  access, no RTOS calls) configuration type
  (`nvband_overnight_session_config_t`: lower EEG sample rate, 30s epoch
  length, extended session duration cap, IMU channel state) and output
  type (`nvband_sleep_epoch_result_t`: a stage enum + a confidence value,
  and nothing else). Host-testable the same way
  `../inference/epoch_budget.h` is.
- `tests/` — `Makefile` (`make test`), a host-test binary exercising both
  structs, and `check_no_stim_coupling.py`, a mechanical static-analysis
  gate (see below). `make test` runs the structural check *before* the
  functional tests — the safety property matters more than the
  functionality.

## The one rule for this subsystem (Addendum 2 §E)

> Sleep monitoring is sensing, scoring, and reporting only. It adds zero
> new stimulation authority. The sleep-stage classifier's output type has
> no field that can reach the stim-command hard clamp or the interlock
> chain — this is a structural fact about the data type, not a policy
> that could be bypassed by a future change. CLAUDE.md §0.1 is completely
> unaffected.

This directory has **zero** dependency on, `#include` of, or function
call into:

- `firmware/core1_inference_radio/inference/stim_command_clamp.h`
- `firmware/core0_safety_signal/safety/interlock_status.h`
- `firmware/core0_safety_signal/safety/charge_balance.h`

`nvband_sleep_epoch_result_t` carries a stage and a confidence value and
**nothing else** — no current, no burst duration, no charge, no
enable/permit flag of any kind. That is a structural promise, not a
description of the current state of the code, and it is checked
mechanically, not just asserted here:

```
cd tests && make test
```

`check_no_stim_coupling.py` statically scans every `.c`/`.h` file in this
directory (comments and string/char literals stripped first, so the
header's own explanatory comments about what this module must never
become don't trip the scan on themselves) for identifiers suggesting
stimulation coupling (`stim`, `clamp`, `interlock`, `dac`, `current_mA`,
`charge`, `enable`, `permit`) and for any `#include` of the three headers
listed above. `test_overnight_session_config.c` additionally asserts,
via `sizeof()`, that `nvband_sleep_epoch_result_t` contains exactly a
stage field and a confidence field — that assertion starts failing the
moment a field is added or removed, forcing a deliberate, reviewed change
to the expected size right here rather than a silent drift.

This is the firmware-layer sibling of
`models/sleep_staging/tests/test_no_stimulation_coupling.py` (same rule,
model-pipeline layer) and `app/tests/sleepReport.test.js`'s
no-diagnostic-language check (same rule, app-copy layer) — three
independent, mechanical proofs of Addendum 2 §E's one rule at three
different layers of the stack.

## Not included in this pass

The actual sleep-stage classifier runtime (executing
`models/sleep_staging/`'s exported int8 artifact on Cortex-M33) is not
wired into Core 1 in this pass, in the same honest category as
`STATUS.md`'s existing "TinyML runtime integration is not done" item for
the state classifier. `overnight_session_config.{h,c}` is the config/
output-type contract that runtime will produce and consume once it
exists; it does not itself run any model.
