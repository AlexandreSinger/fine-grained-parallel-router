#ifndef _SSSP_HPP_
#define _SSSP_HPP_

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <vector>
#include <thread>
#include <mutex>
#include <cstdint>

#include "graph.hpp"

class sssp {
protected:
    struct pq_node_tag_t {
        vertex_id id;
        vertex_id pre;
        path_cost total_cost;
        pq_node_tag_t() {}
        pq_node_tag_t(int dont_care) {}
        pq_node_tag_t(vertex_id id, vertex_id pre, path_cost total_cost)
                : id(id),
                  pre(pre),
                  total_cost(total_cost) {}
    };
    using pq_node_t = std::tuple<path_cost /*priority*/, pq_node_tag_t>;
    struct pq_compare {
        bool operator()(const pq_node_t &u, const pq_node_t &v) {
            return std::get<0>(u) > std::get<0>(v);
        }
    };

    graph *g;
    vertex_id src, dst;
    path_cost src_to_dist_heuristic_cost;

    std::vector<std::thread> pool;
    bool *thread_stop;

    int dummyCalculation(int num) {
#ifndef DISABLE_DUMMY_CALC
        if (num > 0) {
            float x = 1.0;
            for (int i = -250; i < 250; ++i) {
                x *= (float)(std::abs(x) + x);
            }
            return x != 1.0;
        } else {
            return 1;
        }
#else
        return 1;
#endif
    }

    path_cost heuristic_cost(const point &p1, const point &p2) {
        return get_distance(p1, p2);
    }

public:
    sssp(graph *graph_ptr, vertex_id src_id, vertex_id dst_id, size_t num_threads) {
        pool.resize(num_threads);
        thread_stop = new bool[num_threads];
        src = src_id;
        dst = dst_id;
        g = graph_ptr;
        g->reset();
        src_to_dist_heuristic_cost = heuristic_cost(g->vertices[src], g->vertices[dst]);
        g->use_packed_predecessor_and_best_total_cost = false;
    }

    void fork() {
        const size_t num_threads = pool.size();
        std::fill(thread_stop, thread_stop + num_threads, false);
        for (int i = 0; i < num_threads; ++i) {
            pool[i] = std::thread([this, i, num_threads] {
                bool pool_stop;
                do {
                    this->thread_func();
                    thread_stop[i] = true;
                    pool_stop = true;
                    for (int j = 0; j < num_threads; ++j) {
                        if (thread_stop[j] == false) { // no need to use lock
                            pool_stop = false;
                            break;
                        }
                    }
                } while (!pool_stop);
            });
        }
    }

    void join() {
        for (auto &thr : pool) {
            thr.join();
        }
    }

    virtual void thread_func() = 0;

    virtual ~sssp() {
        delete[] thread_stop;
    }
};

//
// C++ STL
//

#include <queue>
class stl_sequential_sssp : public sssp {
    std::priority_queue<pq_node_t, std::vector<pq_node_t>, pq_compare> pq; // tentative vertices

public:
    stl_sequential_sssp(graph *g_ptr, vertex_id src_id, vertex_id dst_id)
            : sssp(g_ptr, src_id, dst_id, 1) {
        pq.push({ src_to_dist_heuristic_cost, { src, g->get_num_vertices() /*pre*/, 0 } });
    }

    void thread_func();
};

//
// OneTBB
//

#include "oneapi/tbb/concurrent_priority_queue.h"
#include "oneapi/tbb/spin_mutex.h"

class tbb_parallel_sssp : public sssp {
    oneapi::tbb::concurrent_priority_queue<pq_node_t, pq_compare> pq; // tentative vertices
    oneapi::tbb::spin_mutex *locks; // a lock for each vertex

public:
    tbb_parallel_sssp(graph *g_ptr, vertex_id src_id, vertex_id dst_id, size_t num_threads)
            : sssp(g_ptr, src_id, dst_id, num_threads) {
        locks = new oneapi::tbb::spin_mutex[g->get_num_vertices()];
        pq.push({ src_to_dist_heuristic_cost, { src, g->get_num_vertices() /*pre*/, 0 } });
    }

    ~tbb_parallel_sssp() {
        delete[] locks;
    }

    void thread_func();
};

//
// Multi-Queue (MQ)
//

#include "MultiQueueIO.h"

class mq_parallel_sssp_base : public sssp {
protected:
    using MQ_IO = MultiQueueIO<pq_compare, path_cost, pq_node_tag_t>;
    MQ_IO *pq;

public:
    mq_parallel_sssp_base(graph *g_ptr,
                          vertex_id src_id,
                          vertex_id dst_id,
                          size_t num_threads,
                          size_t num_queues)
            : sssp(g_ptr, src_id, dst_id, num_threads) {
        pq = new MQ_IO(num_queues, num_threads, 0 /*no need to use batches*/);
        pq->push({ src_to_dist_heuristic_cost, { src, g->get_num_vertices() /*pre*/, 0 } });
    }

    virtual ~mq_parallel_sssp_base() {
        delete pq;
    }

    virtual void thread_func() = 0;
};

//
// MQ with STL Mutex
//

class mq_parallel_sssp_mtx : public mq_parallel_sssp_base {
    std::vector<std::mutex> locks;

public:
    mq_parallel_sssp_mtx(graph *g_ptr,
                         vertex_id src_id,
                         vertex_id dst_id,
                         size_t num_threads,
                         size_t num_queues)
            : mq_parallel_sssp_base(g_ptr, src_id, dst_id, num_threads, num_queues) {
        locks = std::vector<std::mutex>(g->get_num_vertices());
    }

    ~mq_parallel_sssp_mtx() {}

    void thread_func();
};

//
// MQ with Test-and-Set Spin Lock
//

class mq_parallel_sssp_spin : public mq_parallel_sssp_base {
    std::vector<std::atomic_flag> lock_flags;

public:
    mq_parallel_sssp_spin(graph *g_ptr,
                          vertex_id src_id,
                          vertex_id dst_id,
                          size_t num_threads,
                          size_t num_queues)
            : mq_parallel_sssp_base(g_ptr, src_id, dst_id, num_threads, num_queues) {
        lock_flags = std::vector<std::atomic_flag>(g->get_num_vertices());
    }

    ~mq_parallel_sssp_spin() {}

    void thread_func();
};

//
// MQ with update-with-min lock-free technique
//

class mq_parallel_sssp_update_with_min : public mq_parallel_sssp_base {
public:
    mq_parallel_sssp_update_with_min(graph *g_ptr,
                                     vertex_id src_id,
                                     vertex_id dst_id,
                                     size_t num_threads,
                                     size_t num_queues)
            : mq_parallel_sssp_base(g_ptr, src_id, dst_id, num_threads, num_queues) {
        g->use_packed_predecessor_and_best_total_cost = true;
    }

    ~mq_parallel_sssp_update_with_min() {}

    void thread_func();
};

#endif
