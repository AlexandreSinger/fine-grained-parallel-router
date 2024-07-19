
set -x

# Test suite to run
TEST_SUITE=koios_large

# A* offset required to make alg deterministic
OFFSET="7.2e-10"
# OFFSET="3.05e-10" # vtr_chain_largest

# Number of queues per thread
NUM_QUEUES_PER_THREAD=4
# Max number of cores on the system. Used to run multiple instances of VTR in parallel.
MAX_NUM_CORES=12
# 12 hour timeout
TIMEOUT=43200
# Location of the reference directory
TEST_REF_DIR="/home/singera8/fine-grained-parallel-router/testing/tests/"

# Run 12T and 1T
for NUM_THREADS in 1 2 3 6 12
do

NUM_QUEUES=$(expr $NUM_THREADS \* $NUM_QUEUES_PER_THREAD)
NUM_CORES=$(expr $MAX_NUM_CORES / $NUM_THREADS)

# Directed
echo "=============== RUNNING DIRECTED ==============="
EXTRA_VPR_ARGS="--astar_fac 1.2 --post_target_prune_fac 1.2 --post_target_prune_offset 0.0"
./run_test.py $TEST_SUITE -T$NUM_THREADS -Q$NUM_QUEUES -j$NUM_CORES -direct-draining -timeout $TIMEOUT -extra-vpr-args "$EXTRA_VPR_ARGS" -tests-reference-dir-base $TEST_REF_DIR

# A* NO DIRECT DRAINING
echo "=============== RUNNING A* ==============="
EXTRA_VPR_ARGS="--astar_fac 0.9 --post_target_prune_fac 1.0 --post_target_prune_offset ${OFFSET}"
./run_test.py $TEST_SUITE -T$NUM_THREADS -Q$NUM_QUEUES -j$NUM_CORES -timeout $TIMEOUT -extra-vpr-args "$EXTRA_VPR_ARGS" -tests-reference-dir-base $TEST_REF_DIR

# Dijkstra's
echo "=============== RUNNING DIJKSTRAS ==============="
EXTRA_VPR_ARGS="--astar_fac 0.0 --post_target_prune_fac 0.0 --post_target_prune_offset 0.0"
./run_test.py $TEST_SUITE -T$NUM_THREADS -Q$NUM_QUEUES -j$NUM_CORES -direct-draining -timeout $TIMEOUT -extra-vpr-args "$EXTRA_VPR_ARGS" -tests-reference-dir-base $TEST_REF_DIR

done


