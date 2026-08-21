# models/brainprint/

EEG-based local-authentication pipeline, per
`../../docs/ADDENDUM_2_biometric_federated_sleep.md` §A. Python reference
implementation and evaluation harness for the same algorithm as
`firmware/core1_inference_radio/biometric/`.

## Pipeline

```
synthetic_eeg_biometric.py  ->  template.py  ->  evaluate_verification.py
(per-subject signature)        (feature/template   (1:1 FRR/FAR on held-out
                                 extraction)          verification sessions)
```

Run from the `nvband/` directory:

```bash
python3 models/brainprint/synthetic_eeg_biometric.py
python3 models/brainprint/evaluate_verification.py
python3 -m unittest discover -s models/brainprint/tests -v
```

## Data provenance

**Synthetic only.** Each synthetic subject is given a persistent,
subject-specific band-power signature plus per-session drift and
per-epoch noise, specifically so genuine-vs-impostor verification trials
are meaningful to measure on this data. This is not a claim about real
EEG-biometric separability — no real human EEG data exists for this
device (it has not been prototyped).

## Measured result (this pipeline, this synthetic dataset, threshold 0.90)

```json
{
  "num_subjects": 20,
  "genuine_trials": 320,
  "false_reject_rate": 0.0,
  "impostor_trials": 320,
  "false_accept_rate": 0.0469
}
```

Reported as measured, not hand-tuned to a target. A ~4.7% false-accept
rate on synthetic data says nothing about real-world performance — see
`TODO(OI-7)` (no anti-spoofing/liveness claim) in
`firmware/core1_inference_radio/biometric/README.md`.

## Relationship to the firmware module

`template.py` implements the same per-channel band-power-ratio extraction
and epoch-averaging algorithm as
`firmware/core1_inference_radio/biometric/brainprint_template.c` — kept
algorithmically consistent deliberately. The firmware C is what would run
on-device; this Python is the pipeline-evaluation reference, mirroring how
the rest of this repository keeps both.

## What this is not

Not a source of cryptographic key material (see the top-level README in
`firmware/core1_inference_radio/biometric/` for why), not a 1:N
identification/search system (verification here is always 1:1, one claimed
identity against its own enrolled template), and not evidence of real-world
biometric accuracy.
