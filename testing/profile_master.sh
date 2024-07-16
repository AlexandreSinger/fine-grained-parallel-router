
set -x

# Test suite to run
TEST_SUITE=koios_large

# A* offset required to make alg deterministic
OFFSET="7.2e-10"

# Max number of cores on the system. Used to run multiple instances of VTR in parallel.
MAX_NUM_CORES=12
# 12 hour timeout
TIMEOUT=43200
# Location of the reference directory
TEST_REF_DIR="/home/singera8/fine-grained-parallel-router/testing/tests/"

NUM_CORES=$MAX_NUM_CORES

# Directed
echo "=============== RUNNING DIRECTED ==============="
EXTRA_VPR_ARGS=""
./run_test.py $TEST_SUITE -j$NUM_CORES -timeout $TIMEOUT -extra-vpr-args "$EXTRA_VPR_ARGS" -tests-reference-dir-base $TEST_REF_DIR

# A* NO DIRECT DRAINING
echo "=============== RUNNING A* ==============="
EXTRA_VPR_ARGS="--astar_fac 1.0 --astar_offset ${OFFSET}"
./run_test.py $TEST_SUITE -j$NUM_CORES -timeout $TIMEOUT -extra-vpr-args "$EXTRA_VPR_ARGS" -tests-reference-dir-base $TEST_REF_DIR

# Dijkstra's
echo "=============== RUNNING DIJKSTRAS ==============="
EXTRA_VPR_ARGS="--astar_fac 0.0"
./run_test.py $TEST_SUITE -j$NUM_CORES -timeout $TIMEOUT -extra-vpr-args "$EXTRA_VPR_ARGS" -tests-reference-dir-base $TEST_REF_DIR

