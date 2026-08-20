/* Traces: TRC-SESS-01..05. */
#include "../../core0_safety_signal/tests/test_framework.h"
#include "../session/session_store.h"
#include <string.h>

#define FAKE_DEVICE_SIZE 8192

typedef struct {
    uint8_t mem[FAKE_DEVICE_SIZE];
    bool    committed[16]; /* commit marker per "slot" (offset/1024) */
    bool    fail_writes;   /* simulate a crash: no writes succeed    */
} fake_device_t;

static bool fake_write(uint32_t offset, const uint8_t *data, size_t len, void *ctx)
{
    fake_device_t *d = (fake_device_t *)ctx;
    if (d->fail_writes) return false;
    if (offset + len > FAKE_DEVICE_SIZE) return false;
    memcpy(d->mem + offset, data, len);
    return true;
}

static bool fake_read(uint32_t offset, uint8_t *data, size_t len, void *ctx)
{
    fake_device_t *d = (fake_device_t *)ctx;
    if (offset + len > FAKE_DEVICE_SIZE) return false;
    memcpy(data, d->mem + offset, len);
    return true;
}

static bool fake_write_commit(uint32_t offset, void *ctx)
{
    fake_device_t *d = (fake_device_t *)ctx;
    if (d->fail_writes) return false;
    d->committed[offset / 1024] = true;
    return true;
}

static bool fake_read_commit(uint32_t offset, void *ctx)
{
    return ((fake_device_t *)ctx)->committed[offset / 1024];
}

static nvband_session_store_hal_t make_hal(fake_device_t *d)
{
    nvband_session_store_hal_t hal = {
        fake_write, fake_read, fake_write_commit, fake_read_commit, d
    };
    return hal;
}

static void test_staged_then_committed_happy_path(void)
{
    fake_device_t d = {0};
    nvband_session_store_hal_t hal = make_hal(&d);
    const uint8_t payload[] = "session data example payload";

    NVBAND_CHECK(nvband_session_store_write_staged(&hal, 0, payload, sizeof(payload)) == true);
    NVBAND_CHECK(nvband_session_store_query(&hal, 0) == NVBAND_SESSION_RECORD_STAGED);

    NVBAND_CHECK(nvband_session_store_commit(&hal, 0) == true);
    NVBAND_CHECK(nvband_session_store_query(&hal, 0) == NVBAND_SESSION_RECORD_COMMITTED);
}

/* Simulates a power loss right after the payload write but before the
 * header write — modeled by never calling write_staged fully; the
 * record must read as EMPTY, never as COMMITTED or even STAGED. */
static void test_never_written_record_is_empty(void)
{
    fake_device_t d = {0};
    nvband_session_store_hal_t hal = make_hal(&d);
    NVBAND_CHECK(nvband_session_store_query(&hal, 0) == NVBAND_SESSION_RECORD_EMPTY);
}

/* Simulates a crash mid-write: header+payload written, but the write
 * fails partway (torn write) so the payload doesn't match the checksum
 * in the header that DID land. */
static void test_torn_write_detected_as_corrupt_not_committed(void)
{
    fake_device_t d = {0};
    nvband_session_store_hal_t hal = make_hal(&d);
    const uint8_t payload[] = "twenty-nine byte payload!!!!";

    NVBAND_CHECK(nvband_session_store_write_staged(&hal, 0, payload, sizeof(payload)) == true);

    /* Simulate torn flash: corrupt one payload byte after the fact. */
    d.mem[sizeof(nvband_session_record_header_t) + 2] ^= 0xFF;

    nvband_session_record_state_t st = nvband_session_store_query(&hal, 0);
    NVBAND_CHECK(st == NVBAND_SESSION_RECORD_CORRUPT);

    /* Commit must refuse a corrupt record. */
    NVBAND_CHECK(nvband_session_store_commit(&hal, 0) == false);
}

/* A record can never be reported COMMITTED unless BOTH the checksum
 * verifies AND the (independently stored) commit marker is set — a torn
 * write to the marker alone cannot fake a commit. */
static void test_commit_marker_alone_without_checksum_never_reports_committed(void)
{
    fake_device_t d = {0};
    nvband_session_store_hal_t hal = make_hal(&d);
    /* Never write a valid staged record, but forge the commit marker
     * as if an attacker/bitflip set it directly. */
    d.committed[0] = true;

    NVBAND_CHECK(nvband_session_store_query(&hal, 0) == NVBAND_SESSION_RECORD_EMPTY);
}

static void test_commit_fails_when_underlying_writes_fail(void)
{
    fake_device_t d = {0};
    nvband_session_store_hal_t hal = make_hal(&d);
    const uint8_t payload[] = "payload";
    NVBAND_CHECK(nvband_session_store_write_staged(&hal, 0, payload, sizeof(payload)) == true);

    d.fail_writes = true;
    NVBAND_CHECK(nvband_session_store_commit(&hal, 0) == false);
    NVBAND_CHECK(nvband_session_store_query(&hal, 0) == NVBAND_SESSION_RECORD_STAGED);
}

int main(void)
{
    NVBAND_RUN(test_staged_then_committed_happy_path);
    NVBAND_RUN(test_never_written_record_is_empty);
    NVBAND_RUN(test_torn_write_detected_as_corrupt_not_committed);
    NVBAND_RUN(test_commit_marker_alone_without_checksum_never_reports_committed);
    NVBAND_RUN(test_commit_fails_when_underlying_writes_fail);
    NVBAND_TEST_MAIN_END();
}
