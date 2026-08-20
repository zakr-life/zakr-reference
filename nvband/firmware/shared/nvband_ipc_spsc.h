/**
 * nvband_ipc_spsc.h — Lock-free single-producer/single-consumer queue for
 * Core0<->Core1 IPC over shared memory.
 *
 * CLAUDE.md §3.6: "Core 1 (inference/BLE) must never be able to block,
 * starve, or delay the Core 0 kick. Use IPC design (shared memory +
 * lock-free queues, not blocking mutexes) to guarantee this."
 *
 * This is a classic bounded ring buffer using only atomic loads/stores of
 * head/tail indices — no mutex, no futex, no syscall, so neither core can
 * ever block waiting on the other. A full queue on the producer side
 * simply drops (producer's problem to handle, e.g. Core 1 requesting a
 * stim setpoint that Core 0 hasn't drained yet is not Core 0's problem to
 * solve by waiting). Capacity must be a power of two.
 *
 * On Zephyr/real hardware, backed by a section placed in the shared-RAM
 * region declared in the linker script; the host test build below
 * substitutes a plain array and C11 atomics.
 */
#ifndef NVBAND_IPC_SPSC_H
#define NVBAND_IPC_SPSC_H

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define NVBAND_IPC_MAX_PAYLOAD_BYTES 32u

typedef struct {
    uint8_t  bytes[NVBAND_IPC_MAX_PAYLOAD_BYTES];
    uint16_t len;
    uint16_t msg_type;
} nvband_ipc_msg_t;

typedef struct {
    nvband_ipc_msg_t   *slots;
    size_t               capacity;   /* power of two */
    _Atomic size_t        head;      /* consumer reads here   */
    _Atomic size_t        tail;      /* producer writes here  */
} nvband_ipc_spsc_t;

void nvband_ipc_spsc_init(nvband_ipc_spsc_t *q, nvband_ipc_msg_t *storage,
                           size_t capacity_pow2);

/** Non-blocking. Returns false (message dropped) if the queue is full. */
bool nvband_ipc_spsc_push(nvband_ipc_spsc_t *q, const nvband_ipc_msg_t *msg);

/** Non-blocking. Returns false if the queue is empty. */
bool nvband_ipc_spsc_pop(nvband_ipc_spsc_t *q, nvband_ipc_msg_t *out);

size_t nvband_ipc_spsc_count(const nvband_ipc_spsc_t *q);

#endif /* NVBAND_IPC_SPSC_H */
