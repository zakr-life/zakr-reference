/**
 * mux_bbm.h — Break-before-make sequencing for the stim/sense mux (U19).
 *
 * CLAUDE.md §3.3: "Drive the mux (U19) with explicit break-before-make
 * timing margins in the driver; add a unit test that asserts the break
 * interval is never zero under any code path, including error recovery."
 * The real part (ADG1608/ADG5208-class) is break-before-make in silicon;
 * this module adds a firmware-enforced minimum break interval on top, so
 * a part substitution with weaker break timing cannot silently erode
 * safety margin, and so the SAME channel-switch call is used on the
 * error-recovery path as on the normal path (no separate "fast path"
 * that skips the break).
 */
#ifndef NVBAND_MUX_BBM_H
#define NVBAND_MUX_BBM_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    void (*set_all_inputs_off)(void *ctx);
    void (*select_input)(uint8_t input, void *ctx);
    /** Busy-wait or hw-timer wait for at least min_ns nanoseconds. */
    void (*wait_ns)(uint32_t min_ns, void *ctx);
    /** OPTIONAL: read back whether any mux input is currently electrically
     *  connected, sampled during the break window. A correct
     *  break-before-make part always reads "none connected" here. This
     *  hook exists so firmware/sim can inject an anomalous
     *  make-before-break readback (a part defect or the wrong part
     *  populated) and prove firmware detects it, even though the real
     *  U19 part is break-before-make by datasheet. May be NULL on
     *  hardware without a readback path; then no anomaly check runs. */
    bool (*any_input_connected)(void *ctx);
    void *ctx;
} nvband_mux_hal_t;

typedef struct {
    uint8_t  current_input;
    bool     input_selected;
    uint32_t switch_count;
    bool     break_anomaly_detected; /* true => any_input_connected() read
                                         "connected" during a break window;
                                         latches until explicitly cleared */
    uint32_t break_anomaly_count;
} nvband_mux_state_t;

void nvband_mux_state_init(nvband_mux_state_t *state);

/**
 * Switch the mux to `new_input`, always going through an explicit
 * all-off state held for at least NVBAND_MUX_BREAK_BEFORE_MAKE_MIN_NS
 * before the new input is selected. Used on both the normal path and
 * error-recovery path — there is deliberately only one function that can
 * change the mux selection.
 */
void nvband_mux_switch_input(const nvband_mux_hal_t *hal,
                              nvband_mux_state_t *state,
                              uint8_t new_input);

/**
 * Returns true if the last switch's break window verified clean (or no
 * readback hook exists to check). Returns false, and leaves
 * break_anomaly_detected latched, if an anomalous "still connected"
 * readback was ever observed during a break window. This is diagnostic
 * only — it does not and cannot gate stimulation (see interlock_status.h
 * / CLAUDE.md §0.1); it exists so a detected anomaly can be surfaced as a
 * fault and logged for field diagnosis.
 */
bool nvband_mux_break_ok(const nvband_mux_state_t *state);

#endif /* NVBAND_MUX_BBM_H */
