#!/usr/bin/bash

# Script to use a pre-generated Titan benchmark to profile the router.

set -ex

VTR_ROOT=~/vtr-verilog-to-routing
ARCH=$VTR_ROOT/vtr_flow/arch/titan/stratixiv_arch.timing.xml
CIRCUIT_NAME=bitcoin_miner
CHANNEL_WIDTH=300
FULL_CIRCUIT_NAME=$CIRCUIT_NAME\_stratixiv_arch_timing
BENCHMARK=$VTR_ROOT/vtr_flow/benchmarks/titan_blif/titan23/stratixiv/$FULL_CIRCUIT_NAME.blif

# Move into a temporary directory
rm -rf temp_route_sink
mkdir temp_route_sink
cd temp_route_sink

# Use the pre-computed benchmark to just run the router.
BENCHMARK_DIR=../benchmarks/$CIRCUIT_NAME

$VTR_ROOT/vpr/vpr \
	$ARCH \
	--net_file $BENCHMARK_DIR/$FULL_CIRCUIT_NAME.net \
	--place_file $BENCHMARK_DIR/$FULL_CIRCUIT_NAME.place \
	--read_rr_graph $BENCHMARK_DIR/$CIRCUIT_NAME.rr_graph.xml \
	--read_router_lookahead $BENCHMARK_DIR/$CIRCUIT_NAME.router_lookahead.capnp \
	--route \
	--route_chan_width $CHANNEL_WIDTH \
	--max_router_iterations 400 \
	$BENCHMARK

