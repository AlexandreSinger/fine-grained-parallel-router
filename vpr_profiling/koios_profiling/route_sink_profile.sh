#!/usr/bin/bash

# Script for profiling the per-connection run-times. This is very similar to
# profile.sh, however this script does not run gprof.
# VTR was modified to collect data on each connection routed.

set -ex

VTR_ROOT=~/vtr-verilog-to-routing
ARCH=$VTR_ROOT/vtr_flow/arch/timing/k6_frac_N10_frac_chain_mem32K_40nm.xml
# ARCH=architectures/k6_frac_N10_frac_chain_mem32K_equivalence_40nm.xml
TEST_FILE_NAME=bwave_like.float.large
BENCHMARK_DIR=../benchmarks/$TEST_FILE_NAME
# BENCHMARK_DIR=../benchmarks/$TEST_FILE_NAME-equiv
CHAN_WIDTH=326

# Create a temporary directory
rm -rf temp_route_sink
mkdir temp_route_sink
cd temp_route_sink

# Pre-generate the RR graph and the router lookahead
$VTR_ROOT/vpr/vpr \
	../$ARCH \
	--net_file $BENCHMARK_DIR/$TEST_FILE_NAME.net \
	--place_file $BENCHMARK_DIR/$TEST_FILE_NAME.place \
	--write_rr_graph $TEST_FILE_NAME.rr_graph.bin \
	--write_router_lookahead $TEST_FILE_NAME.router_lookahead.capnp \
	--route_chan_width $CHAN_WIDTH \
	--route \
	$BENCHMARK_DIR/$TEST_FILE_NAME.pre-vpr.blif

# Helpful delimitor to know where the real profiling begins
echo "HERE!"

# Re-run the VPR with everything pre-calculated (may be required for special profiling).
$VTR_ROOT/vpr/vpr \
	../$ARCH \
	--net_file $BENCHMARK_DIR/$TEST_FILE_NAME.net \
	--place_file $BENCHMARK_DIR/$TEST_FILE_NAME.place \
	--read_rr_graph $TEST_FILE_NAME.rr_graph.bin \
	--read_router_lookahead $TEST_FILE_NAME.router_lookahead.capnp \
	--route \
	--route_chan_width $CHAN_WIDTH \
	$BENCHMARK_DIR/$TEST_FILE_NAME.pre-vpr.blif
