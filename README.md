# fine-grained-parallel-router
Scripts and experiments used to help implement a fine-grained parallel router in VTR.

## VPR Profiling

The `vpr_profiling` directory contains scripts used to collect and analyze data on the AIR router found in VPR.

## Priority Queue Profiling

The `pq_profiling` directory contains the priority queue (STL, oneTBB, and Multi-Queue) benchmark based on the A* single-source shortest path algorithm. The STL priority queue represents the sequential baseline, the oneTBB stands for the conventional parallel priority queue, and the Multi-Queue-based priority queue is the magic we need. To build and run the benchmark,

```bash
cd /path/to/pq_profiling/astar_sssp
mkdir build && cd build
cmake .. && make && ./benchmark
```

## Testing

The `testing` directory contains a script used to test the parallel router on real circuits of varying sizes. Used for debugging the parallel router and profiling.

