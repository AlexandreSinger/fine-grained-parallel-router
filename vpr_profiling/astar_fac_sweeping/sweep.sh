#!/usr/bin/bash

set -x

# ./run_sweep.py mcnc seq.pre-vpr.blif -j13

./run_sweep.py titan LU230_stratixiv_arch_timing.blif -j13
