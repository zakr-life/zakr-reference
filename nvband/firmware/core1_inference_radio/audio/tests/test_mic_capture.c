#include "../../../core0_safety_signal/tests/test_framework.h"
#include "../mic_capture.h"

static void test_reset_binds_storage_and_starts_empty(void)
{
    int16_t storage[8];
    nvband_mic_capture_buffer_t buf;
    nvband_mic_capture_reset(&buf, storage, 8);

    NVBAND_CHECK(nvband_mic_capture_count(&buf) == 0);
    NVBAND_CHECK(nvband_mic_capture_is_full(&buf) == false);
    NVBAND_CHECK(nvband_mic_capture_data(&buf) == storage);
}

static void test_push_appends_in_order(void)
{
    int16_t storage[4];
    nvband_mic_capture_buffer_t buf;
    nvband_mic_capture_reset(&buf, storage, 4);

    NVBAND_CHECK(nvband_mic_capture_push_sample(&buf, 10) == true);
    NVBAND_CHECK(nvband_mic_capture_push_sample(&buf, -20) == true);
    NVBAND_CHECK(nvband_mic_capture_push_sample(&buf, 30) == true);

    NVBAND_CHECK(nvband_mic_capture_count(&buf) == 3);
    const int16_t *data = nvband_mic_capture_data(&buf);
    NVBAND_CHECK(data[0] == 10);
    NVBAND_CHECK(data[1] == -20);
    NVBAND_CHECK(data[2] == 30);
}

static void test_push_beyond_capacity_is_dropped_not_wrapped(void)
{
    int16_t storage[2];
    nvband_mic_capture_buffer_t buf;
    nvband_mic_capture_reset(&buf, storage, 2);

    NVBAND_CHECK(nvband_mic_capture_push_sample(&buf, 1) == true);
    NVBAND_CHECK(nvband_mic_capture_push_sample(&buf, 2) == true);
    NVBAND_CHECK(nvband_mic_capture_is_full(&buf) == true);

    bool third = nvband_mic_capture_push_sample(&buf, 999);
    NVBAND_CHECK(third == false);
    NVBAND_CHECK(buf.overflowed == true);
    NVBAND_CHECK(nvband_mic_capture_count(&buf) == 2);
    /* The existing samples must be untouched, not overwritten. */
    const int16_t *data = nvband_mic_capture_data(&buf);
    NVBAND_CHECK(data[0] == 1);
    NVBAND_CHECK(data[1] == 2);
}

static void test_push_with_no_bound_storage_fails_safely(void)
{
    nvband_mic_capture_buffer_t buf;
    nvband_mic_capture_reset(&buf, NULL, 0);
    NVBAND_CHECK(nvband_mic_capture_push_sample(&buf, 5) == false);
    NVBAND_CHECK(nvband_mic_capture_data(&buf) == NULL);
}

static void test_zeroize_clears_storage_and_count(void)
{
    int16_t storage[4] = {0};
    nvband_mic_capture_buffer_t buf;
    nvband_mic_capture_reset(&buf, storage, 4);
    nvband_mic_capture_push_sample(&buf, 111);
    nvband_mic_capture_push_sample(&buf, 222);

    nvband_mic_capture_zeroize(&buf);

    NVBAND_CHECK(nvband_mic_capture_count(&buf) == 0);
    NVBAND_CHECK(buf.overflowed == false);
    for (int i = 0; i < 4; i++) {
        NVBAND_CHECK(storage[i] == 0);
    }
}

static void test_reset_after_zeroize_allows_a_fresh_capture(void)
{
    int16_t storage[2];
    nvband_mic_capture_buffer_t buf;
    nvband_mic_capture_reset(&buf, storage, 2);
    nvband_mic_capture_push_sample(&buf, 7);
    nvband_mic_capture_zeroize(&buf);

    nvband_mic_capture_reset(&buf, storage, 2);
    NVBAND_CHECK(nvband_mic_capture_count(&buf) == 0);
    NVBAND_CHECK(nvband_mic_capture_push_sample(&buf, 42) == true);
    NVBAND_CHECK(nvband_mic_capture_data(&buf)[0] == 42);
}

int main(void)
{
    NVBAND_RUN(test_reset_binds_storage_and_starts_empty);
    NVBAND_RUN(test_push_appends_in_order);
    NVBAND_RUN(test_push_beyond_capacity_is_dropped_not_wrapped);
    NVBAND_RUN(test_push_with_no_bound_storage_fails_safely);
    NVBAND_RUN(test_zeroize_clears_storage_and_count);
    NVBAND_RUN(test_reset_after_zeroize_allows_a_fresh_capture);
    NVBAND_TEST_MAIN_END();
}
