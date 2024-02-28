#!/usr/bin/bash

# Script that runs VTR flow on a circuit to pre-generate data once and save
# time on subsequent profiles (since running gprof can slow things down).
# This script will find the minimum channel width.

set -ex

VTR_ROOT=~/vtr-verilog-to-routing
ARCH=$VTR_ROOT/vtr_flow/arch/timing/k6_frac_N10_frac_chain_mem32K_40nm.xml
TEST_FILE_NAME=bwave_like.float.large
TEST_FILE=$VTR_ROOT/vtr_flow/benchmarks/verilog/koios/$TEST_FILE_NAME.v

# Clean up the data from previous runs
rm -rf temp/

# Run the VTR flow on the circuit
$VTR_ROOT/vtr_flow/scripts/run_vtr_flow.py \
	$TEST_FILE \
	$ARCH \

# Save the circuit's blif file, packing, and placement.
BENCHMARK_DIR=benchmarks/$TEST_FILE_NAME

if [ ! -d "$BENCHMARK_DIR" ]; then
	echo "Benchmark directory not found, creating and populating"
	mkdir $BENCHMARK_DIR
	cp temp/$TEST_FILE_NAME.pre-vpr.blif $BENCHMARK_DIR
	cp temp/$TEST_FILE_NAME.place $BENCHMARK_DIR
	cp temp/$TEST_FILE_NAME.net $BENCHMARK_DIR
	grep -nr "Best routing used a channel width factor of" temp/vpr.out > $BENCHMARK_DIR/min_channel_w.txt
else
	echo "Benchmark directory already exists. Not overwriting anything; update by hand."
	grep -nr "Best routing used a channel width factor of" temp/vpr.out
fi
