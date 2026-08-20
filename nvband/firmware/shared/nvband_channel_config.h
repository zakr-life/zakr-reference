/**
 * nvband_channel_config.h — Data-driven stim/EEG channel configuration.
 *
 * TODO(OI-1): Electrode/channel count is unresolved (CLAUDE.md §11,
 * OI-1). The mechanical carrier drawing does not yet reconcile with the
 * assumed 2 stim + 4 EEG electrode figure. That figure directly changes
 * the total delivered charge per session (more stim channels => more
 * charge-budget accounting), so it CANNOT be a compile-time constant in
 * a shipping build.
 *
 * Design: channel count and electrode-to-mux-input mapping are read from
 * the per-unit provisioning record (see tools/provisioning, §8) at boot,
 * validated, and only then used to configure the mux driver and the
 * charge-budget accountant. NVBAND_PLACEHOLDER_*_CHANNEL_COUNT in
 * nvband_constants.h is the CURRENT DEFAULT used only when no
 * provisioning record is present (bench-test / simulation builds) — a
 * production unit with no provisioning record fails closed per §3.2, it
 * does not fall back to this default.
 */
#ifndef NVBAND_CHANNEL_CONFIG_H
#define NVBAND_CHANNEL_CONFIG_H

#include <stdbool.h>
#include <stdint.h>
#include "nvband_constants.h"

#define NVBAND_MAX_STIM_CHANNELS 4u   /* upper bound for static buffers only;
                                          NOT the operative channel count */
#define NVBAND_MAX_EEG_CHANNELS  8u   /* matches ADS1299 8-ch part option */

typedef struct {
    uint8_t stim_channel_count;                       /* from provisioning */
    uint8_t eeg_channel_count;                         /* from provisioning */
    uint8_t stim_mux_input[NVBAND_MAX_STIM_CHANNELS];  /* mux input index per
                                                            logical stim channel */
    uint8_t eeg_mux_input[NVBAND_MAX_EEG_CHANNELS];
    bool    loaded_from_provisioning;                  /* false => bench
                                                            default in use;
                                                            never true in a
                                                            production boot
                                                            path without a
                                                            valid record */
} nvband_channel_config_t;

/** Populate a bench/simulation default config using the OI-1 placeholder
 *  figures. Callers on a production boot path must NOT use this in place
 *  of the real provisioning loader — see channel_config.c. */
void nvband_channel_config_bench_default(nvband_channel_config_t *out);

/** True iff the config is internally consistent: channel counts fit
 *  within the static bounds and are nonzero. Does not validate against
 *  the mechanical/electrical reality of a specific unit — that is the
 *  provisioning tool's job (tools/provisioning). */
bool nvband_channel_config_is_valid(const nvband_channel_config_t *cfg);

#endif /* NVBAND_CHANNEL_CONFIG_H */
