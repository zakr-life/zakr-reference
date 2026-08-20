CC ?= gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -g -O1

SRCS_CLAMP = test_stim_command_clamp.c ../inference/stim_command_clamp.c
SRCS_EPOCH = test_epoch_budget.c ../inference/epoch_budget.c
SRCS_ATT   = test_attestation.c ../ble/attestation.c
SRCS_SESS  = test_session_store.c ../session/session_store.c

BINS = bin/test_stim_command_clamp bin/test_epoch_budget bin/test_attestation \
       bin/test_session_store

.PHONY: all test clean
all: test

bin:
	mkdir -p bin

bin/test_stim_command_clamp: $(SRCS_CLAMP) | bin
	$(CC) $(CFLAGS) -o $@ $(SRCS_CLAMP)

bin/test_epoch_budget: $(SRCS_EPOCH) | bin
	$(CC) $(CFLAGS) -o $@ $(SRCS_EPOCH)

bin/test_attestation: $(SRCS_ATT) | bin
	$(CC) $(CFLAGS) -o $@ $(SRCS_ATT)

bin/test_session_store: $(SRCS_SESS) | bin
	$(CC) $(CFLAGS) -o $@ $(SRCS_SESS)

test: $(BINS)
	@fail=0; \
	for b in $(BINS); do echo "=== $$b ==="; ./$$b || fail=1; done; \
	exit $$fail

clean:
	rm -rf bin
