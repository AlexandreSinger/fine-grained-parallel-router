#ifndef _SSSP_HPP_
#define _SSSP_HPP_

#include <algorithm>
#include <cstddef>
#include <vector>
#include <thread>
#include <mutex>

#include "graph.hpp"

class sssp {
protected:
    graph *g;
    vertex_id src, dst;
    std::vector<std::thread> pool;
    bool *thread_stop;

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

class mq_parallel_sssp : public sssp {
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
    std::mutex *locks; // a lock for each vertex

public:
    mq_parallel_sssp(graph *g_ptr,
                     vertex_id src_id,
                     vertex_id dst_id,
                     size_t num_threads,
                     size_t num_queues)
            : sssp(g_ptr, src_id, dst_id, num_threads) {
        locks = new std::mutex[g->get_num_vertices()];
        mq = new MQ_IO(num_queues, num_threads, 0 /*no need to use batches*/);
        mq->push(std::make_tuple(g->f_distance[src], src)); // emplace src
    }

    ~mq_parallel_sssp() {
        delete mq;
        delete[] locks;
    }

    void thread_func();
};

#include "oneapi/tbb/concurrent_priority_queue.h"

class tbb_parallel_sssp : public sssp {
    struct compare_f {
        bool operator()(const vertex_rec &u, const vertex_rec &v) const {
            return std::get<0>(u) > std::get<0>(v);
        }
    };
    oneapi::tbb::concurrent_priority_queue<vertex_rec, compare_f> open_set; // tentative vertices
    std::mutex *locks; // a lock for each vertex

public:
    tbb_parallel_sssp(graph *g_ptr, vertex_id src_id, vertex_id dst_id, size_t num_threads)
            : sssp(g_ptr, src_id, dst_id, num_threads) {
        locks = new std::mutex[g->get_num_vertices()];
        open_set.emplace(g->f_distance[src], src); // emplace src into open_set
    }

    ~tbb_parallel_sssp() {
        delete[] locks;
    }

    void thread_func();
};

#endif
