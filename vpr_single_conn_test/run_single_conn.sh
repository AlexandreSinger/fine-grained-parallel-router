#!/usr/bin/sh
# This is a script that will run the VPR router on a given circuit.
# It will pre-compute the benchmark circuit and store its information in a directory,
# to make future runs of the VPR router faster. This will make debugging the router
# easier.

set -ex

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
VTR_ROOT=~/vtr-verilog-to-routing
ARCH=$VTR_ROOT/vtr_flow/arch/timing/k6_frac_N10_frac_chain_mem32K_40nm.xml
CHANNEL_WIDTH=326
CIRCUIT_NAME=bwave_like.float.large
TEST_FILE=$VTR_ROOT/vtr_flow/benchmarks/verilog/koios/$CIRCUIT_NAME.v

TEMP_DIR=$SCRIPT_DIR/temp
rm -rf $TEMP_DIR
mkdir $TEMP_DIR
cd $TEMP_DIR

BENCHMARK_DIR=$SCRIPT_DIR/benchmark_pre_compute_$CIRCUIT_NAME

# Check if anything needs to be pre-calculated
if [ ! -d "$BENCHMARK_DIR" ]; then
	echo "Benchmark directory not found, creating and populating"
	mkdir $BENCHMARK_DIR
	# Run VTR Flow to pre-generate the blif, placement file, netlist file, router lookahead, and rr graph
	$VTR_ROOT/vtr_flow/scripts/run_vtr_flow.py \
		$TEST_FILE \
		$ARCH \
		--write_rr_graph $CIRCUIT_NAME.rr_graph.bin \
		--write_router_lookahead $CIRCUIT_NAME.router_lookahead.capnp \
		--route_chan_width $CHANNEL_WIDTH \

	# Copy the pre-generated files over to the benchmark directory
	cp $TEMP_DIR/temp/$CIRCUIT_NAME.pre-vpr.blif $BENCHMARK_DIR
	cp $TEMP_DIR/temp/$CIRCUIT_NAME.place $BENCHMARK_DIR
	cp $TEMP_DIR/temp/$CIRCUIT_NAME.net $BENCHMARK_DIR
	cp $TEMP_DIR/temp/vpr.out $BENCHMARK_DIR
	cp $TEMP_DIR/temp/$CIRCUIT_NAME.rr_graph.bin $BENCHMARK_DIR
	cp $TEMP_DIR/temp/$CIRCUIT_NAME.router_lookahead.capnp $BENCHMARK_DIR
else
	echo "Benchmark directory already exists. Not overwriting anything."
fi

# Run VPR on the pre-calculated input
$VTR_ROOT/vpr/vpr \
        $ARCH \
        --net_file $BENCHMARK_DIR/$CIRCUIT_NAME.net \
        --place_file $BENCHMARK_DIR/$CIRCUIT_NAME.place \
        --read_rr_graph $BENCHMARK_DIR/$CIRCUIT_NAME.rr_graph.bin \
        --read_router_lookahead $BENCHMARK_DIR/$CIRCUIT_NAME.router_lookahead.capnp \
        --route \
        --route_chan_width $CHANNEL_WIDTH \
        $BENCHMARK_DIR/$CIRCUIT_NAME.pre-vpr.blif


