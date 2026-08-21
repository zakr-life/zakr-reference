#include "mic_capture.h"
#include <string.h>

void nvband_mic_capture_reset(nvband_mic_capture_buffer_t *buf,
                               int16_t *storage, uint32_t capacity)
{
    if (buf == NULL) {
        return;
    }
    buf->samples = storage;
    buf->capacity = (storage != NULL) ? capacity : 0u;
    buf->count = 0u;
    buf->overflowed = false;
}

bool nvband_mic_capture_push_sample(nvband_mic_capture_buffer_t *buf, int16_t sample)
{
    if (buf == NULL || buf->samples == NULL) {
        return false;
    }
    if (buf->count >= buf->capacity) {
        buf->overflowed = true;
        return false;
    }
    buf->samples[buf->count] = sample;
    buf->count++;
    return true;
}

bool nvband_mic_capture_is_full(const nvband_mic_capture_buffer_t *buf)
{
    if (buf == NULL) {
        return false;
    }
    return buf->count >= buf->capacity;
}

uint32_t nvband_mic_capture_count(const nvband_mic_capture_buffer_t *buf)
{
    return (buf != NULL) ? buf->count : 0u;
}

const int16_t *nvband_mic_capture_data(const nvband_mic_capture_buffer_t *buf)
{
    if (buf == NULL) {
        return NULL;
    }
    return buf->samples;
}

void nvband_mic_capture_zeroize(nvband_mic_capture_buffer_t *buf)
{
    if (buf == NULL || buf->samples == NULL) {
        return;
    }
    memset(buf->samples, 0, (size_t)buf->capacity * sizeof(buf->samples[0]));
    buf->count = 0u;
    buf->overflowed = false;
}
