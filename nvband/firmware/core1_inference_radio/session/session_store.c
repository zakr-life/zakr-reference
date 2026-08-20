#include "session_store.h"
#include <stdlib.h>
#include <string.h>

uint32_t nvband_crc32(const uint8_t *data, size_t len)
{
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++) {
            uint32_t mask = -(crc & 1u);
            crc = (crc >> 1) ^ (0xEDB88320u & mask);
        }
    }
    return ~crc;
}

#define HEADER_SIZE ((uint32_t)sizeof(nvband_session_record_header_t))

bool nvband_session_store_write_staged(const nvband_session_store_hal_t *hal,
                                        uint32_t offset,
                                        const uint8_t *payload,
                                        uint32_t payload_len)
{
    if (hal == NULL || payload == NULL || payload_len == 0) {
        return false;
    }
    if (hal->write_block == NULL) {
        return false;
    }

    nvband_session_record_header_t hdr = {
        .offset = offset,
        .payload_len = payload_len,
        .crc32 = nvband_crc32(payload, payload_len),
    };

    /* Payload written first, header (which references its CRC) written
     * second — if power is lost between the two, the header either
     * doesn't exist yet (record reads as EMPTY) or references a fully-
     * written payload (record is verifiable). Either way, never a state
     * that looks committed without a checksummed payload behind it. */
    if (!hal->write_block(offset + HEADER_SIZE, payload, payload_len, hal->ctx)) {
        return false;
    }
    if (!hal->write_block(offset, (const uint8_t *)&hdr, HEADER_SIZE, hal->ctx)) {
        return false;
    }
    return true;
}

nvband_session_record_state_t nvband_session_store_query(
    const nvband_session_store_hal_t *hal, uint32_t offset)
{
    if (hal == NULL || hal->read_block == NULL) {
        return NVBAND_SESSION_RECORD_EMPTY;
    }

    nvband_session_record_header_t hdr;
    if (!hal->read_block(offset, (uint8_t *)&hdr, HEADER_SIZE, hal->ctx)) {
        return NVBAND_SESSION_RECORD_EMPTY;
    }
    if (hdr.payload_len == 0 || hdr.payload_len > (16u * 1024u * 1024u)) {
        return NVBAND_SESSION_RECORD_EMPTY;
    }

    uint8_t stack_buf[4096];
    uint8_t *payload_buf = stack_buf;
    bool heap_used = false;
    uint8_t *heap_buf = NULL;
    if (hdr.payload_len > sizeof(stack_buf)) {
        heap_buf = (uint8_t *)malloc(hdr.payload_len);
        if (heap_buf == NULL) {
            return NVBAND_SESSION_RECORD_CORRUPT;
        }
        payload_buf = heap_buf;
        heap_used = true;
    }

    bool read_ok = hal->read_block(offset + HEADER_SIZE, payload_buf,
                                    hdr.payload_len, hal->ctx);
    nvband_session_record_state_t result;
    if (!read_ok) {
        result = NVBAND_SESSION_RECORD_EMPTY;
    } else {
        uint32_t actual_crc = nvband_crc32(payload_buf, hdr.payload_len);
        if (actual_crc != hdr.crc32) {
            result = NVBAND_SESSION_RECORD_CORRUPT;
        } else {
            bool committed = hal->read_commit_marker != NULL &&
                              hal->read_commit_marker(offset, hal->ctx);
            result = committed ? NVBAND_SESSION_RECORD_COMMITTED
                                : NVBAND_SESSION_RECORD_STAGED;
        }
    }

    if (heap_used) {
        free(heap_buf);
    }
    return result;
}

bool nvband_session_store_commit(const nvband_session_store_hal_t *hal,
                                  uint32_t offset)
{
    if (hal == NULL || hal->write_commit_marker == NULL) {
        return false;
    }

    /* Only commit if an independent re-verification of the checksum
     * passes right now — never trust that "we just wrote it" implies
     * it's still good. */
    nvband_session_record_state_t state = nvband_session_store_query(hal, offset);
    if (state != NVBAND_SESSION_RECORD_STAGED) {
        return false;
    }

    return hal->write_commit_marker(offset, hal->ctx);
}
