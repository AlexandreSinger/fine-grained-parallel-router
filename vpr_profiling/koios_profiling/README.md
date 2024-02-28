# Profiling the Serial Router in VTR
Professor Mark Jeffrey recommended that we profile the AIR router and see how much time is spent routing the nets (basically how much time is the A-star taking).

We are interested in knowing if this is a long function called a few times, or a short function called many times.

We want a more tangible number than the one provided by Gort and Anderson.

## Helpful Advice from Amin
Got some helpful info from Amin:
- SerialNetlistRouter.tpp:route_netlist calls route_net
- route_net.tpp:route_net calls route_sink
- route_net.tpp:route_sink calls timing_driven_route_connection_from_route_tree
	- note: there is a version for high fanout and low fanout
- connection_router.cpp:timing_driven_route_connection_from_route_tree actually performs the expansion and search

Asked Amin about a good architecture that represents FPGAs well. He recommended the VTR flagship architecture
- vtr-verilog-to-routing/vtr_flow/arch/timing/k6_frac_N10_frac_chain_mem32K_40nm.xml
- "It has all of the common features (hard blocks, chain, etc), and it is not as complicated at SIV"

To profile VTR, Amin recommended the following documentation:
- https://docs.verilogtorouting.org/en/latest/README.developers/#profiling-vtr

To get the number of times the router is called, GCOV may be better.

Amin also sent a list of testcases and their sizes
- largest by far was mcml

## Instructions

### 1. Generate the Benchmark

For this step, I recommend building VPR using the regular make command. Building VPR in profiling mode can make this step extremely slow.

The benchmark files should be pre-generated to save time since we may want to profile the same circuit many times. Update the `pack_and_place.sh` script's `TEST_FILE` variable to point to a verilog file that you wish to generate the benchmark for.

To run the script, just run:
```sh
bash pack_and_place.sh
```

This will generate a `temp` folder that contains all of the files that represent the benchmark. The script also prints out the minimum channel width. If you did not see the minimum channel width, it can be found by searching for the line "Best routing used a channel width factor of" in the `vpr.out` file in the `temp` folder.

Copy over the pre-vpr blif, the .net, and .place files from the `temp` folder into the benchmarks folder.
```sh
mkdir benchmarks/<name_of_testcase>
cp temp/<name_of_testcase>.pre-vpr.blif benchmarks/<name_of_testcase>
cp temp/<name_of_testcase>.net benchmarks/<name_of_testcase>
cp temp/<name_of_testcase>.place benchmarks/<name_of_testcase>
```

### 2. Profiling a Benchmark

Once the benchmark has been generated we can profile it. Set the channel width and the benchmark name in the `profile.sh` script. I recommend setting the channel width to be equal to 1.3x the minimum channel width found in the previous step.

You need to build VPR for profiling. Follow the steps outlined in the documentation:
https://docs.verilogtorouting.org/en/latest/README.developers/#profiling-vtr

In short, in the vpr directory:
```sh
make CMAKE_PARAMS="-DVTR_ENABLE_PROFILING=ON" vpr
```

Then run the script:
```sh
bash profile.sh
```

This will generate all of the vpr temp files in the `temp_profile` directory and generate a gprof file called `gprof.txt`.

I recommend copying the vpr output and the gprof file into the results folder for safe keeping.
```sh
mkdir results/<name_of_testcase>
cp temp_profile/vpr_stdout.log results/<name_of_testcase>
cp gprof.txt results/<name_of_testcase>
```

## Profiling Some VTR Benchmarks

Chose 4 test circuits from the VTR benchmark. Each circuit is around 10x larger than the previous. Boundtop is the smallest and mcml is the largest.

| circuit | min_chan_width | chan_width |
| ------- | -------------- | ---------- |
| mcml | 144 | 188 |
| LU32PEEng | 132 | 172 |
| blob_merge | 70 | 92 |
| boundtop | 38 | 50 |

### Results Including Router Lookahead and RR Graph

I accidently included the time it takes to create the Router Lookahead and the RR graph in these runs. It masks how long it actually takes to run the router.

results from mcml:
- Overall run: 126.47 seconds
- VPR flow: 100.96 seconds
- VPR Route flow: 80.60 seconds
- route: 61.31 seconds
- Route netlist: 29.15 (called 23 times)
- Route sink: 27.60 (called 487733 times)
	- 45% of route
	- 0.05659 ms per call
	- HOWEVER: The speed of this may actually get faster as time progresses. Need to talk to Vaughn about the outputs of VPRs

results from LU32PEEng:
- Overall run: 204.61 seconds
- VPR flow: 177.71 seconds
- VPR Route flow: 157.58 seconds
- route: 128.29 seconds
- Route netlist: 86.10 (called 20 times)
- Route sink: 84.13 (called 897577 times)
	- 65.6% of route
	- 0.0937 ms per call

results from blob_merge:
- Overall run: 4.35 seconds
- VPR flow: 3.08 seconds
- VPR Route flow: 2.11 seconds
- route: 1.49 seconds
- Route netlist: 0.56 (called 14 times)
- Route sink: 0.49 (called 64374 times)
	- 32.89% of route
	- 0.00761 ms per call

results from boundtop:
- Overall run: 0.51 seconds
- VPR flow: 0.33 seconds
- VPR Route flow: 0.24 seconds
- route: 0.16 seconds
- Route netlist: 0.04 (called 11 times)
- Route sink: 0.04 (called 2762 times)
	- 25% of route
	- 0.0145 ms per call

### Results Not Including Router Lookahead and RR Graph (Real Results)

Another piece of advice from Amin:
- The time it takes to build the RR graph and the router lookahead should not be considered.
- His reasoning for this is that in a real FPGA CAD flow, these would be pre-calculated for a given FPGA architecture.
- I should pre-compute these and pass them into the profiler.

Updated results:

results from mcml:
- Overall run: 77.47 seconds
- VPR flow: 50.87 seconds
- VPR Route flow: 38.02 seconds
- route: 36.65 seconds
- Route netlist: 30.73 (called 23 times)
- Route sink: 29.40 (called 487733 times)
	- 80.22% of route
	- 38% of VPR call (only running the router)
	- 0.0603 ms per call

results from LU32PEEng:
- Overall run: 139.37 seconds
- VPR flow: 111.64 seconds
- VPR Route flow: 98.12 seconds
- route: 96.61 seconds
- Route netlist: 64.0 (called 20 times)
- Route sink: 87.01 (called 897577 times)
	- 90.06% of route
	- 62.5% of VPR call (only running the router)
	- 0.0969 ms per call

results from blob_merge:
- Overall run: 2.62 seconds
- VPR flow: 1.52 seconds
- VPR Route flow: 0.77 seconds
- route: 0.71 seconds
- Route netlist: 0.49 (called 14 times)
- Route sink: 0.39 (called 64336 times)
	- 54.93% of route
	- 15.4% of VPR call (only running the router)
	- 0.006062 ms per call

results from boundtop:
- Overall run: 0.25 seconds
- VPR flow: 0.14 seconds
- VPR Route flow: 0.06 seconds
- route: 0.06 seconds
- Route netlist: 0.04 (called 11 times)
- Route sink: 0.04 (called 2762 times)
    - 66.67% of route (note: there is a lot of rounding here...)
	- 16% of VPR call (only running the router)
	- 0.01448 ms per call


geomean % of route: 71.72%
- Honestly, its really close to the Gort and Anderson result.

## Koios Benchmarks

List of benchmarks for use in VPR:
https://docs.verilogtorouting.org/en/latest/vtr/benchmarks/

Doing some of the Koios benchmarks

| circuit | min_chan_width | chan_width |
| ------- | -------------- | ---------- |
| tpu_like.large.os | 78 | 102 |
| spmv | 50 | 66 |

results from tpu_like.large.os:
- Overall run: 94.80 seconds
- VPR flow: 74.70 seconds
- VPR Route flow: 58.73 seconds
- route: 57.07 seconds
- Route netlist: 46.94 (called 14 times)
- Route sink: 45.77 (called 344089 times)
	- 80.20% of route
	- 48.3% of VPR call (only running the router)
	- 0.133 ms per call

results from spmv:
- Overall run: 37.90 seconds
- VPR flow: 34.83 seconds
- VPR Route flow: 32.04 seconds
- route: 31.75 seconds
- Route netlist: 29.67 (called 13 times)
- Route sink: 29.51 (called 46769 times)
	- 92.95% of route
	- 77.9% of VPR call (only running the router)
	- 0.631 ms per call


It looks like spmv is VERY ammeable to parallelism!

## Profiling Connections Instructions

After running pack_and_place as described previously, one can run `route_sink_profile.sh` to profile specifically the connections.

This requires modifying VPR to output the connection data during the routing process.