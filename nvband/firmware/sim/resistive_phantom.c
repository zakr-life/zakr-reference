#include "resistive_phantom.h"
#include <string.h>

void nvband_phantom_config_default(nvband_phantom_config_t *cfg)
{
    if (cfg == NULL) {
        return;
    }
    memset(cfg, 0, sizeof(*cfg));
    cfg->impedance_ohm = 5000.0f;   /* typical dry-electrode skin impedance */
    cfg->electrode_temp_c = 32.0f;  /* within NVBAND_ELECTRODE_TEMP window */
    cfg->lifted = 0;
}

void nvband_phantom_measure(const nvband_phantom_config_t *cfg,
                             float commanded_current_mA,
                             nvband_phantom_reading_t *out)
{
    if (cfg == NULL || out == NULL) {
        return;
    }
    memset(out, 0, sizeof(*out));

    float effective_ohm = cfg->lifted ? 5.0e6f : cfg->impedance_ohm;

    /* V = I * R; mA * Ohm = mV (1e-3 A * Ohm = 1e-3 V). */
    out->measured_voltage_mV = commanded_current_mA * effective_ohm;
    out->measured_impedance_ohm = effective_ohm;
    out->measured_temp_c = cfg->electrode_temp_c;
}
