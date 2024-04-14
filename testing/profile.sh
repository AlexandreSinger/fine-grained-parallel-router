
set -x

TEST_SUITE=bwave_like

export MQ_NUM_THREADS=1
export MQ_NUM_QUEUES_PER_THREAD=4
./run_test.py $TEST_SUITE -j1

export MQ_NUM_THREADS=2
export MQ_NUM_QUEUES_PER_THREAD=4
./run_test.py $TEST_SUITE -j1

export MQ_NUM_THREADS=3
export MQ_NUM_QUEUES_PER_THREAD=4
./run_test.py $TEST_SUITE -j1

export MQ_NUM_THREADS=6
export MQ_NUM_QUEUES_PER_THREAD=4
./run_test.py $TEST_SUITE -j1

export MQ_NUM_THREADS=12
export MQ_NUM_QUEUES_PER_THREAD=4
./run_test.py $TEST_SUITE -j1

export MQ_NUM_THREADS=24
export MQ_NUM_QUEUES_PER_THREAD=4
./run_test.py $TEST_SUITE -j1

