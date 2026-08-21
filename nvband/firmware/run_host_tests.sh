#!/usr/bin/env bash
# Runs every host-testable safety suite in this repository. This is the
# concrete implementation of the CI gate described in CLAUDE.md §9 /
# firmware/docs/ci_gates.md: "no build reaches the fleet-ota-eligible
# artifact stage without passing the full interlock fault-injection
# suite, the charge-balance unit tests, the watchdog-independence stress
# test, and the debug-lockout check." (The debug-lockout check itself is
# a build-config assertion, not a host test — see
# firmware/bootloader/README.md.)
set -euo pipefail
cd "$(dirname "$0")"

echo "== shared module host tests (IPC, etc.) =="
make -C shared/tests clean test

echo
echo "== core0_safety_signal host tests =="
make -C core0_safety_signal/tests clean test

echo
echo "== core1_inference_radio host tests =="
make -C core1_inference_radio/tests clean test

echo
echo "== core1_inference_radio/biometric host tests (Addendum 2 §A) =="
make -C core1_inference_radio/biometric/tests clean test

echo
echo "== core1_inference_radio/audio host tests (Addendum 2 §B) =="
make -C core1_inference_radio/audio/tests clean test

echo
echo "== core1_inference_radio/sleep host tests (Addendum 2 §E) =="
make -C core1_inference_radio/sleep/tests clean test
python3 core1_inference_radio/sleep/tests/check_no_stim_coupling.py

echo
echo "== bootloader host tests =="
make -C bootloader/tests clean test

echo
echo "== firmware/sim software-in-the-loop scenarios =="
make -C sim/tests clean test

echo
echo "ALL FIRMWARE HOST TESTS PASSED"
