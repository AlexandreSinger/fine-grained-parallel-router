#!/usr/bin/bash

# Script to take a pre-generated circuit from pack_and_place.sh and profile it.
# This script expects VTR to be compiled in profile mode.

set -ex

VTR_ROOT=~/vtr-verilog-to-routing
ARCH=$VTR_ROOT/vtr_flow/arch/timing/k6_frac_N10_frac_chain_mem32K_40nm.xml
TEST_FILE_NAME=blob_merge
CHAN_WIDTH=92

# Clean up the results from previous runs
rm -f gprof.txt

# Create a temporary directory
rm -rf temp_profile
mkdir temp_profile
cd temp_profile

# Pre-generate the RR graph and the router lookahead
$VTR_ROOT/vpr/vpr \
	$ARCH \
	--net_file ../benchmarks/$TEST_FILE_NAME/$TEST_FILE_NAME.net \
	--place_file ../benchmarks/$TEST_FILE_NAME/$TEST_FILE_NAME.place \
	--write_rr_graph $TEST_FILE_NAME.rr_graph.bin \
	--write_router_lookahead $TEST_FILE_NAME.router_lookahead.bin \
	--route_chan_width $CHAN_WIDTH \
	--route \
	../benchmarks/$TEST_FILE_NAME/$TEST_FILE_NAME.pre-vpr.blif

# Run VPR with all of the circuit information pre-computed so only the router is
# running.
$VTR_ROOT/vpr/vpr \
	$ARCH \
	--net_file ../benchmarks/$TEST_FILE_NAME/$TEST_FILE_NAME.net \
	--place_file ../benchmarks/$TEST_FILE_NAME/$TEST_FILE_NAME.place \
	--read_rr_graph $TEST_FILE_NAME.rr_graph.bin \
	--read_router_lookahead $TEST_FILE_NAME.router_lookahead.bin \
	--route \
	--route_chan_width $CHAN_WIDTH \
	../benchmarks/$TEST_FILE_NAME/$TEST_FILE_NAME.pre-vpr.blif

# Save the results of the gprof profile
RESULT_DIR=../results/$TEST_FILE_NAME

if [ ! -d "$RESULT_DIR" ]; then
	echo "Result directory not found, creating and populating"
	mkdir $RESULT_DIR
	gprof $VTR_ROOT/vpr/vpr gmon.out > $RESULT_DIR/gprof.txt
	cp vpr_stdout.log $RESULT_DIR
else
	echo "Result directory already exists. Not overwriting anything; update by hand."
	gprof $VTR_ROOT/vpr/vpr gmon.out > ../gprof.txt
fi

