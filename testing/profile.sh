
set -x

export MQ_NUM_THREADS=1
export MQ_NUM_QUEUES_PER_THREAD=4
./run_test.py bwave_like -j1

export MQ_NUM_THREADS=2
export MQ_NUM_QUEUES_PER_THREAD=4
./run_test.py bwave_like -j1

export MQ_NUM_THREADS=4
export MQ_NUM_QUEUES_PER_THREAD=4
./run_test.py bwave_like -j1

export MQ_NUM_THREADS=8
export MQ_NUM_QUEUES_PER_THREAD=4
./run_test.py bwave_like -j1

export MQ_NUM_THREADS=16
export MQ_NUM_QUEUES_PER_THREAD=4
./run_test.py bwave_like -j1

export MQ_NUM_THREADS=32
export MQ_NUM_QUEUES_PER_THREAD=4
./run_test.py bwave_like -j1

