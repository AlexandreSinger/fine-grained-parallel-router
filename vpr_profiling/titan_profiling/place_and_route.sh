#!/usr/bin/bash

# Script to run place and route on a Titan benchmark to precompute aspects of the
# circuit to make profiling faster and more accurate.

set -ex

VTR_ROOT=~/vtr-verilog-to-routing
ARCH=$VTR_ROOT/vtr_flow/arch/titan/stratixiv_arch.timing.xml
CIRCUIT_NAME=bitcoin_miner
CHANNEL_WIDTH=300
BENCHMARK=$VTR_ROOT/vtr_flow/benchmarks/titan_blif/titan23/stratixiv/$CIRCUIT_NAME\_stratixiv_arch_timing.blif

# Move into a temporary folder to contain the generated files.
TEMP_FOLDER=temp_place_and_route
rm -rf $TEMP_FOLDER
mkdir $TEMP_FOLDER
cd $TEMP_FOLDER

# Run the benchmark and write out the rr graph and the route lookahead.
# The .net and .place files should be generated as well.
$VTR_ROOT/vpr/vpr \
    $ARCH \
    $BENCHMARK \
    --write_rr_graph $CIRCUIT_NAME.rr_graph.xml \
    --write_router_lookahead $CIRCUIT_NAME.router_lookahead.capnp \
    --route_chan_width $CHANNEL_WIDTH \
    --max_router_iterations 400

cd ..

# Store the packing, placement, rr_graph, and router lookahead for the circuit
# into the benchmark directory.
BENCHMARK_DIR=benchmarks/$CIRCUIT_NAME

if [ ! -d "$BENCHMARK_DIR" ]; then
	echo "Benchmark directory not found, making new one"
	mkdir $BENCHMARK_DIR
	cp $TEMP_FOLDER/$CIRCUIT_NAME\_stratixiv_arch_timing.net $BENCHMARK_DIR
	cp $TEMP_FOLDER/$CIRCUIT_NAME\_stratixiv_arch_timing.place $BENCHMARK_DIR
	cp $TEMP_FOLDER/$CIRCUIT_NAME.rr_graph.xml $BENCHMARK_DIR
	cp $TEMP_FOLDER/$CIRCUIT_NAME.router_lookahead.capnp $BENCHMARK_DIR
	cp $TEMP_FOLDER/vpr_stdout.log $BENCHMARK_DIR
else
	echo "Benchmark directory already exists. Not doing anything. Copy anything yourself."
fi
