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
    graph *g;
    vertex_id src, dst;
    std::vector<std::thread> pool;
    bool *thread_stop;
    bool early_exit;

    int dummyCalculation(int num) {
#ifndef DISABLE_DUMMY_CALC
        if (num > 0) {
            float x = 1.0;
            for (int i = -250; i < 250; ++i) {
                x *= (float)(std::abs(x) + x);
            }
            return x != 1.0;
        }
        else {
            return 1;
        }
#else
        return 1;
#endif
    }

public:
    sssp(graph *graph_ptr, vertex_id src_id, vertex_id dst_id, size_t num_threads) {
        pool.resize(num_threads);
        thread_stop = new bool[num_threads];
        src = src_id;
        dst = dst_id;
        g = graph_ptr;
        g->reset();
        g->g_distance[src] = 0.0; // src's distance from src is zero
        g->f_distance[src] =
            get_distance(g->vertices[src], g->vertices[dst]); // estimate distance from src to dst
        g->pre_g_dist[src] = 0.0; // src's distance from src is zero
        g->use_packed_predecessor_and_g_dist = false;
    }

    void fork() {
        const size_t num_threads = pool.size();
        std::fill(thread_stop, thread_stop + num_threads, false);
        early_exit = false;
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

    virtual ~sssp() {}
};

#include <queue>

class stl_sequential_sssp : public sssp {
    std::priority_queue<vertex_rec, std::vector<vertex_rec>, std::greater<vertex_rec>>
        pq; // tentative vertices

public:
    stl_sequential_sssp(graph *g_ptr, vertex_id src_id, vertex_id dst_id)
            : sssp(g_ptr, src_id, dst_id, 1) {
        pq.emplace(g->f_distance[src], src); // emplace src
    }

    void thread_func();
};

#include "MultiQueueIO.h"

class mq_parallel_sssp_base : public sssp {
protected:
    template <typename T = vertex_rec, class func = std::greater<T>>
    class Comparator {
        func f;

    public:
        bool operator()(T a, T b) {
            return func()(a, b);
        }
    };

    using MQ_IO =
        MultiQueueIO<Comparator<vertex_rec, std::greater<vertex_rec>>, path_cost, vertex_id>;

    MQ_IO *mq;

public:
    mq_parallel_sssp_base(graph *g_ptr,
                          vertex_id src_id,
                          vertex_id dst_id,
                          size_t num_threads,
                          size_t num_queues)
            : sssp(g_ptr, src_id, dst_id, num_threads) {
        mq = new MQ_IO(num_queues, num_threads, 0 /*no need to use batches*/);
        const path_cost f_dist_src = g->f_distance[src];
        mq->push(std::make_tuple(f_dist_src, src)); // emplace src
    }

    virtual ~mq_parallel_sssp_base() {
        delete mq;
    }

    virtual void thread_func() = 0;
};

class mq_parallel_sssp_lock : public mq_parallel_sssp_base {
    std::vector<std::mutex> locks; // a lock for each vertex

public:
    mq_parallel_sssp_lock(graph *g_ptr,
                          vertex_id src_id,
                          vertex_id dst_id,
                          size_t num_threads,
                          size_t num_queues)
            : mq_parallel_sssp_base(g_ptr, src_id, dst_id, num_threads, num_queues) {
        locks = std::vector<std::mutex>(g->get_num_vertices());
    }

    ~mq_parallel_sssp_lock() {}

    void thread_func();
};

class mq_parallel_sssp_ttas : public mq_parallel_sssp_base {
    std::vector<std::atomic_flag> lock_flags;

public:
    mq_parallel_sssp_ttas(graph *g_ptr,
                          vertex_id src_id,
                          vertex_id dst_id,
                          size_t num_threads,
                          size_t num_queues)
            : mq_parallel_sssp_base(g_ptr, src_id, dst_id, num_threads, num_queues) {
        lock_flags = std::vector<std::atomic_flag>(g->get_num_vertices());
    }

    ~mq_parallel_sssp_ttas() {}

    void thread_func();
};

class mq_parallel_sssp_update_with_min : public mq_parallel_sssp_base {
public:
    mq_parallel_sssp_update_with_min(graph *g_ptr,
                                     vertex_id src_id,
                                     vertex_id dst_id,
                                     size_t num_threads,
                                     size_t num_queues)
            : mq_parallel_sssp_base(g_ptr, src_id, dst_id, num_threads, num_queues) {
        g->use_packed_predecessor_and_g_dist = true;
    }

    ~mq_parallel_sssp_update_with_min() {}

    void thread_func();
};

#include "oneapi/tbb/concurrent_priority_queue.h"
#include "oneapi/tbb/spin_mutex.h"

class tbb_parallel_sssp : public sssp {
    struct compare_f {
        bool operator()(const vertex_rec &u, const vertex_rec &v) const {
            return std::get<0>(u) > std::get<0>(v);
        }
    };
    oneapi::tbb::concurrent_priority_queue<vertex_rec, compare_f> open_set; // tentative vertices
    oneapi::tbb::spin_mutex *locks; // a lock for each vertex

public:
    tbb_parallel_sssp(graph *g_ptr, vertex_id src_id, vertex_id dst_id, size_t num_threads)
            : sssp(g_ptr, src_id, dst_id, num_threads) {
        locks = new oneapi::tbb::spin_mutex[g->get_num_vertices()];
        open_set.emplace(g->f_distance[src], src); // emplace src into open_set
    }

    ~tbb_parallel_sssp() {
        delete[] locks;
    }

    void thread_func();
};

#endif
