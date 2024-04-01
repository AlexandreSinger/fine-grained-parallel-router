# Testing

This directory contains a script used for testing the fine-grained parallel router.

Instead of running packing and place every time, this script uses pre-calculated .net and .place files for
different test suites to save time.

## Test Suites

The following test suites are supported:
| Suite | Run time | Description | Source |
| ----- | -------- | ----------- | ------ |
| `titan` | ~2 hours on 10 cores | Large Titan circuits | `regression_tests/vtr_reg_nightly_test2/titan_quick_qor` |
| `bwave_like` | ~20 mins on 1 core | Large bwave-like circuit for profiling | Koios Benchmark Suite |
| `koios_medium` | ~10 mins on 10 cores | Medium Koios circuits | `regression_tests/vtr_reg_nightly_test4/koios_medium` |
| `mcnc_min_search` | ~2 mins on 10 cores | MCNC circuits with minimum channel width search | `regression_tests/vtr_reg_nightly_test1/vpr_reg_mcnc` |
| `mcnc` | ~10 seconds on 10 cores | MCNC circuits with minimum channel width set to 100 | `regression_tests/vtr_reg_nightly_test1/vpr_reg_mcnc` |

## Run Instructions

Minimum command:
```
./run_test.py mcnc
```
This will run the mcnc test suite on one core with the vtr directory assumed to be at `~/vtr-verilog-to-routing`

You can specify more cores with `-jN` where N is the number of cores.

You can specify a different directory for VTR using `-vtr_dir`.

For example:
```
./run_test.py mcnc -j10 -vtr_dir ~/vtr-verilog-to-routing/
```

The script will produce a summary of the QoR and runtime for the router.

