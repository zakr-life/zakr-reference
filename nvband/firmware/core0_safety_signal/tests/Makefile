# Host test build for the Core-0 safety core. No Zephyr SDK required —
# this exercises the hardware-independent safety logic (charge balance,
# interlock status, watchdog kick contingency, UI state exhaustiveness,
# channel config) with a fake HAL. This is the suite CLAUDE.md §9 requires
# to gate any build reaching the fleet-ota-eligible stage.
#
# Usage: make -C firmware/core0_safety_signal/tests
CC ?= gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -g -O1
SAFETY_DIR = ../safety
SHARED_DIR = ../../shared

SRCS_CHARGE = test_charge_balance.c $(SAFETY_DIR)/charge_balance.c
SRCS_ILK    = test_interlock_status.c $(SAFETY_DIR)/interlock_status.c
SRCS_WDG    = test_watchdog_kick.c $(SAFETY_DIR)/watchdog_kick.c
SRCS_UI     = test_ui_state.c $(SAFETY_DIR)/ui_state.c
SRCS_CFG    = test_channel_config.c $(SHARED_DIR)/nvband_channel_config.c
SRCS_MUX    = test_mux_bbm.c ../drivers/mux_bbm.c
SRCS_AFE    = test_afe_noise_selftest.c ../drivers/afe_noise_selftest.c
SRCS_CHGARB = test_charger_arbitration.c ../drivers/charger_arbitration.c
SRCS_PMIC   = test_pmic_sequencer.c ../drivers/pmic_sequencer.c
SRCS_RING   = test_sync_ring_buffer.c ../sampling/sync_ring_buffer.c

BINS = bin/test_charge_balance bin/test_interlock_status \
       bin/test_watchdog_kick bin/test_ui_state bin/test_channel_config \
       bin/test_mux_bbm bin/test_afe_noise_selftest bin/test_charger_arbitration \
       bin/test_pmic_sequencer bin/test_sync_ring_buffer

.PHONY: all test clean
all: test

bin:
	mkdir -p bin

bin/test_charge_balance: $(SRCS_CHARGE) | bin
	$(CC) $(CFLAGS) -o $@ $(SRCS_CHARGE) -lm

bin/test_interlock_status: $(SRCS_ILK) | bin
	$(CC) $(CFLAGS) -o $@ $(SRCS_ILK)

bin/test_watchdog_kick: $(SRCS_WDG) | bin
	$(CC) $(CFLAGS) -o $@ $(SRCS_WDG)

bin/test_ui_state: $(SRCS_UI) | bin
	$(CC) $(CFLAGS) -o $@ $(SRCS_UI)

bin/test_channel_config: $(SRCS_CFG) | bin
	$(CC) $(CFLAGS) -o $@ $(SRCS_CFG)

bin/test_mux_bbm: $(SRCS_MUX) | bin
	$(CC) $(CFLAGS) -o $@ $(SRCS_MUX)

bin/test_afe_noise_selftest: $(SRCS_AFE) | bin
	$(CC) $(CFLAGS) -o $@ $(SRCS_AFE) -lm

bin/test_charger_arbitration: $(SRCS_CHGARB) | bin
	$(CC) $(CFLAGS) -o $@ $(SRCS_CHGARB)

bin/test_pmic_sequencer: $(SRCS_PMIC) | bin
	$(CC) $(CFLAGS) -o $@ $(SRCS_PMIC)

bin/test_sync_ring_buffer: $(SRCS_RING) | bin
	$(CC) $(CFLAGS) -o $@ $(SRCS_RING)

test: $(BINS)
	@fail=0; \
	for b in $(BINS); do \
		echo "=== $$b ==="; \
		./$$b || fail=1; \
	done; \
	exit $$fail

clean:
	rm -rf bin
