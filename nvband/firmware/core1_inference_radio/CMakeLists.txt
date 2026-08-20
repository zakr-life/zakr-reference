# Zephyr application CMakeLists for Core 1 (inference/radio image).
# See ../core0_safety_signal/CMakeLists.txt for the sibling image and the
# note on nRF Connect SDK board/core targeting.
cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nvband_core1_inference_radio)

target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../shared
)

target_sources(app PRIVATE
    inference/stim_command_clamp.c
    inference/epoch_budget.c
    ble/attestation.c
    session/session_store.c
    ../shared/nvband_ipc_spsc.c
    ../secure/secure_element_client.c
    # ble/gatt_services.c (GATT service/characteristic table), the TFLite
    # Micro runtime integration under inference/, and main.c are not
    # included in this pass — gated on final nRF Connect SDK BLE stack
    # config and the exported model format from models/export/.
)
