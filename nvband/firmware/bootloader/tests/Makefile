CC ?= gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -g -O1
SRCS = test_ab_rollback.c ../ab_rollback.c

.PHONY: all test clean
all: test
bin:
	mkdir -p bin
bin/test_ab_rollback: $(SRCS) | bin
	$(CC) $(CFLAGS) -o $@ $(SRCS)
test: bin/test_ab_rollback
	./bin/test_ab_rollback
clean:
	rm -rf bin
