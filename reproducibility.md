# Reproducibility Guide

## Environment

- Ubuntu 22.04
- GCC ARM Embedded 12.2
- TensorFlow 2.15
- CMSIS-DSP 1.15
- TensorFlow Lite Micro

## Build Instructions

```bash

mkdir build
cd build
cmake ..
make

Validation Instructions

python validation/run_validation.py
python validation/check_latency.py
Benchmark Reproduction
1.	Flash firmware to nRF5340
2.	Connect UART serial monitor
3.	Execute benchmark_latency
4.	Capture timing logs
5.	Compare with published timing report

---
