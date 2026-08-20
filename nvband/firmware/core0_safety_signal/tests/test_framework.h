/**
 * test_framework.h — minimal, dependency-free host test harness.
 *
 * No third-party framework required (no Unity/CMock submodule to vendor),
 * so `make -C firmware/core0_safety_signal/tests` runs anywhere gcc is
 * available. Deliberately tiny: this exists to make CLAUDE.md §9's CI
 * gates ("no build reaches fleet-ota-eligible without passing the
 * interlock fault-injection suite, charge-balance unit tests, watchdog-
 * independence stress test...") actually runnable, not to be a general
 * test framework.
 */
#ifndef NVBAND_TEST_FRAMEWORK_H
#define NVBAND_TEST_FRAMEWORK_H

#include <stdio.h>

static int g_nvband_test_failures = 0;
static int g_nvband_test_count = 0;

#define NVBAND_CHECK(cond) \
    do { \
        g_nvband_test_count++; \
        if (!(cond)) { \
            g_nvband_test_failures++; \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        } \
    } while (0)

#define NVBAND_RUN(fn) \
    do { \
        fprintf(stderr, "-- running %s\n", #fn); \
        fn(); \
    } while (0)

#define NVBAND_TEST_MAIN_END() \
    do { \
        fprintf(stderr, "%d/%d assertions passed\n", \
                g_nvband_test_count - g_nvband_test_failures, \
                g_nvband_test_count); \
        return g_nvband_test_failures == 0 ? 0 : 1; \
    } while (0)

#endif /* NVBAND_TEST_FRAMEWORK_H */
