# Software-in-the-loop scenario suite. Ties resistive_phantom.c to the
# safety-core modules. Bench-test / simulation only — never linked into a
# fleet-ota production image.
CC ?= gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -g -O1
CORE0 = ../../core0_safety_signal

SRCS = test_end_to_end_scenarios.c \
       ../resistive_phantom.c \
       $(CORE0)/safety/interlock_status.c \
       $(CORE0)/safety/charge_balance.c \
       $(CORE0)/drivers/mux_bbm.c

.PHONY: all test clean
all: test

bin:
	mkdir -p bin

bin/test_end_to_end_scenarios: $(SRCS) | bin
	$(CC) $(CFLAGS) -o $@ $(SRCS) -lm

test: bin/test_end_to_end_scenarios
	./bin/test_end_to_end_scenarios

clean:
	rm -rf bin
