# Zephyr application CMakeLists for Core 0 (safety/signal image).
#
# This builds against nRF Connect SDK / Zephyr once the SDK is installed
# (`west build -b nrf5340dk_nrf5340_cpuapp`, targeting the network/app
# core split per the nRF5340's dual-core partitioning — Core 0 in this
# repo's naming maps to the deterministic real-time image; confirm against
# the final nRF Connect SDK board overlay before flashing real hardware).
#
# The hardware-independent safety-core sources (safety/, most of
# sampling/) have NO Zephyr dependency and are also built + tested on the
# host via tests/Makefile — that's deliberate, see tests/README.md.
cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nvband_core0_safety_signal)

target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../shared
)

target_sources(app PRIVATE
    safety/charge_balance.c
    safety/interlock_status.c
    safety/ui_state.c
    safety/watchdog_kick.c
    drivers/mux_bbm.c
    drivers/afe_noise_selftest.c
    drivers/charger_arbitration.c
    drivers/pmic_sequencer.c
    sampling/sync_ring_buffer.c
    ../shared/nvband_channel_config.c
    ../shared/nvband_ipc_spsc.c
    # main.c (Zephyr app entry point / RTOS task wiring) — not included in
    # this pass; it is pure glue (thread creation, GPIO/SPI device tree
    # binding, periodic kernel timer driving the sampling loop) over the
    # modules above, gated on the final nRF Connect SDK board overlay.
)
