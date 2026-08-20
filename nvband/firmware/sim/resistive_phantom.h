/**
 * resistive_phantom.h — Resistive head-phantom model for closed-loop
 * testing without a person (CLAUDE.md §9).
 *
 * A minimal linear model: commanded current -> measured voltage across a
 * configurable electrode-skin impedance, plus optional injected series
 * resistance faults (lifted electrode => very high impedance) and
 * temperature. This is a BENCH-TEST / SIMULATION-ONLY component —
 * nowhere in this file or its callers does it assert stimulation enable;
 * it only produces synthetic sensor readings for the interlock/charge
 * modules to react to. Never linked into a fleet-ota production image
 * (see firmware/docs/ci_gates.md).
 */
#ifndef NVBAND_RESISTIVE_PHANTOM_H
#define NVBAND_RESISTIVE_PHANTOM_H

#include <stdint.h>

typedef struct {
    float impedance_ohm;      /* nominal electrode-skin impedance        */
    float electrode_temp_c;   /* skin-contact sensor reading, simulated  */
    int   lifted;             /* 1 => electrode lifted (impedance -> huge) */
} nvband_phantom_config_t;

typedef struct {
    float measured_voltage_mV;
    float measured_impedance_ohm;
    float measured_temp_c;
} nvband_phantom_reading_t;

void nvband_phantom_config_default(nvband_phantom_config_t *cfg);

/** Given a commanded current (mA) and the phantom config, compute the
 *  synthetic sensor reading (Ohm's law + a lifted-electrode fault
 *  model). Pure function. */
void nvband_phantom_measure(const nvband_phantom_config_t *cfg,
                             float commanded_current_mA,
                             nvband_phantom_reading_t *out);

#endif /* NVBAND_RESISTIVE_PHANTOM_H */
