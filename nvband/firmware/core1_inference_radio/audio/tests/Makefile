# Host test build for the Core-1 voiceprint audio subsystem
# (Addendum 2 §B). No Zephyr SDK required -- exercises the
# hardware-independent mic power-gate state machine, capture buffer, and
# voice-activity gate with a fake HAL, same pattern as
# firmware/core0_safety_signal/tests/ and
# firmware/core1_inference_radio/tests/.
#
# Usage: make -C firmware/core1_inference_radio/audio/tests
CC ?= gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -g -O1

SRCS_GATE = test_mic_power_gate.c ../mic_power_gate.c
SRCS_CAP  = test_mic_capture.c ../mic_capture.c
SRCS_VAD  = test_voice_activity_gate.c ../voice_activity_gate.c

BINS = bin/test_mic_power_gate bin/test_mic_capture bin/test_voice_activity_gate

.PHONY: all test clean
all: test

bin:
	mkdir -p bin

bin/test_mic_power_gate: $(SRCS_GATE) | bin
	$(CC) $(CFLAGS) -o $@ $(SRCS_GATE)

bin/test_mic_capture: $(SRCS_CAP) | bin
	$(CC) $(CFLAGS) -o $@ $(SRCS_CAP)

bin/test_voice_activity_gate: $(SRCS_VAD) | bin
	$(CC) $(CFLAGS) -o $@ $(SRCS_VAD) -lm

test: $(BINS)
	@fail=0; \
	for b in $(BINS); do \
		echo "=== $$b ==="; \
		./$$b || fail=1; \
	done; \
	exit $$fail

clean:
	rm -rf bin
