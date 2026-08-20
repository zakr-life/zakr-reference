/**
 * session_store.h — Write-then-commit session record pattern (U16).
 *
 * CLAUDE.md §3.9: "Never allow a partially-written session record to be
 * uploaded or marked complete; use a write-then-commit pattern with a
 * checksum."
 *
 * A session record is written to a staging area, checksummed, and only
 * then does a separate, small, atomic commit operation mark it complete
 * (analogous to a journaled filesystem's commit record). If power is
 * lost mid-write, the record is left in STAGED (not COMMITTED) state and
 * is never surfaced to the app/cloud as a real session — see
 * nvband_session_store_scan_recoverable().
 *
 * The actual NAND/NOR (U16) block I/O is a Zephyr flash-map driver (not
 * included in this pass — depends on the wear-levelling FS chosen, e.g.
 * LittleFS over the NAND controller); this module is the record-format
 * and commit-protocol logic, independent of the underlying block device,
 * so it's host-testable against an in-memory fake.
 */
#ifndef NVBAND_SESSION_STORE_H
#define NVBAND_SESSION_STORE_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef enum {
    NVBAND_SESSION_RECORD_EMPTY = 0,
    NVBAND_SESSION_RECORD_STAGED,     /* written, not yet committed       */
    NVBAND_SESSION_RECORD_COMMITTED,  /* checksum verified, safe to sync  */
    NVBAND_SESSION_RECORD_CORRUPT,    /* checksum mismatch on verify      */
} nvband_session_record_state_t;

typedef struct {
    /** Read/write a fixed-size block device abstraction. Real
     *  implementation backs onto the wear-levelled FS over U16. */
    bool (*write_block)(uint32_t offset, const uint8_t *data, size_t len, void *ctx);
    bool (*read_block)(uint32_t offset, uint8_t *data, size_t len, void *ctx);
    /** Small, separately-flushed commit marker write — must itself be a
     *  single atomic block write on the real device. */
    bool (*write_commit_marker)(uint32_t record_offset, void *ctx);
    bool (*read_commit_marker)(uint32_t record_offset, void *ctx);
    void *ctx;
} nvband_session_store_hal_t;

typedef struct {
    uint32_t offset;
    uint32_t payload_len;
    uint32_t crc32;
} nvband_session_record_header_t;

/**
 * Stage 1: write payload + header (with CRC32 over the payload) to
 * `offset`. Does NOT mark the record committed — a crash here leaves the
 * record recoverable-but-discardable.
 */
bool nvband_session_store_write_staged(const nvband_session_store_hal_t *hal,
                                        uint32_t offset,
                                        const uint8_t *payload,
                                        uint32_t payload_len);

/**
 * Stage 2: re-read the staged payload, verify CRC32, and ONLY if it
 * verifies, write the commit marker. A record is never committed on
 * unverified data.
 */
bool nvband_session_store_commit(const nvband_session_store_hal_t *hal,
                                  uint32_t offset);

/** Returns the record's current state by reading header + commit marker
 *  and re-verifying the checksum (never trusts the commit marker alone
 *  without a matching checksum — a torn write to the marker itself must
 *  not be able to fake a commit). */
nvband_session_record_state_t nvband_session_store_query(
    const nvband_session_store_hal_t *hal, uint32_t offset);

uint32_t nvband_crc32(const uint8_t *data, size_t len);

#endif /* NVBAND_SESSION_STORE_H */
