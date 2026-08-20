/**
 * interlock_status.h — Firmware-side view of the hardware interlock chain.
 *
 * CLAUDE.md §0.1 / §3.5: U8 (discrete logic, not firmware-loadable) is the
 * sole authority that asserts stimulation ENABLE, as a hardware AND of
 * seven independent conditions. Firmware's role here is strictly:
 *   1. READ the latched fault status (never write it, except its own
 *      NVBAND_INTERLOCK_FIRMWARE_PERMIT bit, which is one INPUT to the
 *      hardware AND, not an override of it).
 *   2. On any fault: stop commanding new stimulation, surface the fault,
 *      log it with which condition(s) tripped, and require an explicit,
 *      deliberate re-arm — NEVER auto-clear a latch.
 *
 * This module has NO function that can assert enable. There is
 * deliberately no `interlock_force_enable()`, no bypass parameter, no
 * "test mode" flag reachable from this header. A bench-test build that
 * needs to exercise the chain without hardware uses the simulator in
 * firmware/sim/, which implements nvband_interlock_hal_t itself and is
 * never linked into a production/fleet-ota image (see CI gate in
 * firmware/docs/ci_gates.md).
 */
#ifndef NVBAND_INTERLOCK_STATUS_H
#define NVBAND_INTERLOCK_STATUS_H

#include <stdbool.h>
#include <stdint.h>
#include "../../shared/nvband_constants.h"

/** Hardware access surface this module needs. A real build supplies this
 *  from the U8 status-register driver; host tests and firmware/sim supply
 *  a fake. Deliberately has no "set_enable" — see file header. */
typedef struct {
    /** Read the live (unlatched) state of one condition. */
    bool (*read_condition)(nvband_interlock_condition_t cond, void *ctx);
    /** Read whether the fault latch is currently asserted. */
    bool (*read_latch_asserted)(void *ctx);
    /** Physically pulse the re-arm line. Only callable after an explicit,
     *  deliberate operator action (see nvband_interlock_request_rearm). */
    void (*pulse_rearm_line)(void *ctx);
    /** The ONE bit firmware legitimately drives: its own permit input to
     *  the hardware AND. This does not enable stimulation by itself. */
    void (*set_firmware_permit)(bool asserted, void *ctx);
    void *ctx;
} nvband_interlock_hal_t;

typedef struct {
    bool     latch_asserted;
    bool     condition_state[NVBAND_INTERLOCK_CONDITION_COUNT];
    uint32_t tripped_mask;      /* bit i set => condition i was NOT satisfied
                                    at last poll */
    uint64_t last_poll_timestamp_us;
} nvband_interlock_snapshot_t;

typedef enum {
    NVBAND_REARM_DENIED_NOT_LATCHED = 0,   /* nothing to re-arm            */
    NVBAND_REARM_DENIED_CONDITIONS_UNMET,  /* a tripped condition persists */
    NVBAND_REARM_OK,
} nvband_rearm_result_t;

/** Poll all seven conditions and the latch. Never blocks; never asserts
 *  firmware_permit as a side effect. */
void nvband_interlock_poll(const nvband_interlock_hal_t *hal,
                            uint64_t now_us,
                            nvband_interlock_snapshot_t *out);

/**
 * Explicit, deliberate re-arm request (called only from a UI/BLE path that
 * required a distinct confirming action — never from fault-detection code
 * itself, and never automatically on a timer). Refuses if the latch isn't
 * asserted, or if any of the seven conditions is still unmet at the time
 * of the request — re-arming is not "try again," it's "confirm cleared."
 */
nvband_rearm_result_t nvband_interlock_request_rearm(
    const nvband_interlock_hal_t *hal,
    const nvband_interlock_snapshot_t *current_snapshot);

/** True if every condition was satisfied at the last poll AND the latch is
 *  not asserted. This does NOT mean stimulation is enabled — only the
 *  hardware AND (U8) knows that; this tells firmware whether it is safe
 *  to *attempt* to assert its own permit bit. */
bool nvband_interlock_all_clear(const nvband_interlock_snapshot_t *snap);

#endif /* NVBAND_INTERLOCK_STATUS_H */
