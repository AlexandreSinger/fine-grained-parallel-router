#!/usr/bin/bash

# Script to pre-generate a circuit with a special architecture where the input
# pins of hard blocks (DSPs and BRAMs) are marked as equivalent.
# This script will not find the minimum channel width.

set -ex

VTR_ROOT=~/vtr-verilog-to-routing
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
ARCH=$SCRIPT_DIR/architectures/k6_frac_N10_frac_chain_mem32K_equivalence_40nm.xml
TEST_FILE_NAME=bwave_like.float.large
TEST_FILE=$VTR_ROOT/vtr_flow/benchmarks/verilog/koios/$TEST_FILE_NAME.v
CHANNEL_WIDTH=326

# Create a temp directory
TEMP_DIR=$SCRIPT_DIR/temp_equivalence_place_and_route
rm -rf $TEMP_DIR
mkdir $TEMP_DIR
cd $TEMP_DIR

# Run VTR flow.
$VTR_ROOT/vtr_flow/scripts/run_vtr_flow.py \
	$TEST_FILE \
	$ARCH \
	--route_chan_width $CHANNEL_WIDTH

cd ..

# Save the pre-computed circuit data
BENCHMARK_DIR=$SCRIPT_DIR/benchmarks/$TEST_FILE_NAME-equiv

if [ ! -d "$BENCHMARK_DIR" ]; then
	echo "Benchmark directory not found, creating and populating"
	mkdir $BENCHMARK_DIR
	cp $TEMP_DIR/temp/$TEST_FILE_NAME.pre-vpr.blif $BENCHMARK_DIR
	cp $TEMP_DIR/temp/$TEST_FILE_NAME.place $BENCHMARK_DIR
	cp $TEMP_DIR/temp/$TEST_FILE_NAME.net $BENCHMARK_DIR
	cp $TEMP_DIR/temp/vpr.out $BENCHMARK_DIR
else
	echo "Benchmark directory already exists. Not overwriting anything; update by hand."
fi

