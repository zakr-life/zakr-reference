CC ?= gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -g -O1

SRCS = test_ipc_spsc.c ../nvband_ipc_spsc.c

.PHONY: all test clean
all: test

bin:
	mkdir -p bin

bin/test_ipc_spsc: $(SRCS) | bin
	$(CC) $(CFLAGS) -o $@ $(SRCS)

test: bin/test_ipc_spsc
	./bin/test_ipc_spsc

clean:
	rm -rf bin
