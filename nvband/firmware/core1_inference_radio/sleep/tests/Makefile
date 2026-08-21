CC ?= gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -g -O1

SRCS_CFG = test_overnight_session_config.c ../overnight_session_config.c

BINS = bin/test_overnight_session_config

.PHONY: all test clean check-no-stim-coupling
all: test

bin:
	mkdir -p bin

bin/test_overnight_session_config: $(SRCS_CFG) | bin
	$(CC) $(CFLAGS) -o $@ $(SRCS_CFG)

# Structural safety check first: this must pass before we even care
# whether the binaries run correctly (Addendum 2 §E's "one rule" is more
# important than any functional test in this directory).
check-no-stim-coupling:
	@echo "=== structural check: no stimulation-coupling in sleep/*.c,*.h ==="
	python3 check_no_stim_coupling.py

test: check-no-stim-coupling $(BINS)
	@fail=0; \
	for b in $(BINS); do echo "=== $$b ==="; ./$$b || fail=1; done; \
	exit $$fail

clean:
	rm -rf bin
