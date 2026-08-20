CC ?= gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -g -O1
LDLIBS = -lm

SRCS_TEMPLATE  = test_brainprint_template.c ../brainprint_template.c
SRCS_MATCHER   = test_brainprint_matcher.c ../brainprint_template.c ../brainprint_matcher.c
SRCS_AUTH_GATE = test_brainprint_auth_gate.c ../brainprint_auth_gate.c

BINS = bin/test_brainprint_template bin/test_brainprint_matcher bin/test_brainprint_auth_gate

.PHONY: all test clean
all: test

bin:
	mkdir -p bin

bin/test_brainprint_template: $(SRCS_TEMPLATE) | bin
	$(CC) $(CFLAGS) -o $@ $(SRCS_TEMPLATE) $(LDLIBS)

bin/test_brainprint_matcher: $(SRCS_MATCHER) | bin
	$(CC) $(CFLAGS) -o $@ $(SRCS_MATCHER) $(LDLIBS)

bin/test_brainprint_auth_gate: $(SRCS_AUTH_GATE) | bin
	$(CC) $(CFLAGS) -o $@ $(SRCS_AUTH_GATE) $(LDLIBS)

test: $(BINS)
	@fail=0; \
	for b in $(BINS); do echo "=== $$b ==="; ./$$b || fail=1; done; \
	exit $$fail

clean:
	rm -rf bin
