# models/

Implements CLAUDE.md §4 (on-device AI / signal processing). Pure Python
3 standard library — no numpy/TensorFlow/sklearn install required, so
the whole pipeline runs anywhere:

```
./run_pipeline_and_tests.sh
```

Runs 23 unit tests, then the full pipeline: generate synthetic data ->
subject-level split -> train (Task 1: state classification) -> evaluate
(held-out accuracy + artifact-robustness slice) -> int8 quantize/export
with a latency+size budget gate -> sign and register with a
firmware-compatibility range.

**All data is synthetic** (`data_provenance: "synthetic"` on every
record) — CLAUDE.md §12.3: no real session data exists yet for a device
that hasn't been prototyped. See `models/export/model_card.md` (generated
by the pipeline) for the honest accounting of what this buys and doesn't.

## Layout

- `training/` — `synthetic_eeg.py` (data generator), `split.py`
  (subject-level train/val/test), `features.py` (motion-contaminated
  epoch rejection, Task 1), `softmax_classifier.py` (the actual
  classifier), `train_state_classifier.py` (entry point),
  `phase_locked_loop.py` (Task 2), `adaptation.py` (Task 3, with
  explainability logging).
- `evaluation/` — held-out scoring, subject-leakage re-verification,
  artifact-robustness slice reporting.
- `export/` — int8 symmetric quantization, latency/size budget gate
  (fails the build if exceeded), model card generator.
- `versioning/` — signed model registry with firmware-compatibility
  ranges; the `fleet_ota_would_distribute()` decision point that
  `cloud/fleet-ota/` calls before distributing anything.

## No model output is ever the sole gate for stimulation

CLAUDE.md §4: enforced at the model->firmware boundary by
`firmware/core1_inference_radio/inference/stim_command_clamp.c`, unit-
tested independently of any model (`stim_command_clamp` tests never
import anything from this directory). Nothing in `models/` has, or
should ever gain, authority to command hardware.
