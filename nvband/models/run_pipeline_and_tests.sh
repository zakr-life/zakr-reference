#!/usr/bin/env bash
# Runs the full model pipeline end to end (generate -> train -> evaluate
# -> export -> register) AND the unit test suites, mirroring
# firmware/run_host_tests.sh. Pure Python 3 standard library — no numpy/
# tensorflow/sklearn install required, so this runs anywhere.
set -euo pipefail
cd "$(dirname "$0")"

echo "== unit tests =="
python3 -m unittest discover -s training/tests -p 'test_*.py' -v
python3 -m unittest discover -s export/tests -p 'test_*.py' -v
python3 -m unittest discover -s versioning/tests -p 'test_*.py' -v

echo
echo "== full pipeline: train =="
(cd training && python3 train_state_classifier.py)

echo
echo "== full pipeline: evaluate =="
(cd evaluation && python3 evaluate.py)

echo
echo "== full pipeline: export (int8 quantize + latency/size gate + model card) =="
(cd export && python3 export_model.py)

echo
echo "== full pipeline: register + fleet-ota compatibility decision =="
(cd versioning && python3 registry.py)

echo
echo "ALL MODEL PIPELINE STAGES + TESTS PASSED"
