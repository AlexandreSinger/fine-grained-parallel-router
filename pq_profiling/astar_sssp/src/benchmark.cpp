#include "graph.hpp"
#include "sssp.hpp"

#include <functional>

const size_t num_test_runs = 8;

inline double test_harness(std::function<sssp*()> factory, size_t num_runs) {
    long long t_accumulated_ms = 0;
    for (int i = 0; i < num_runs; ++i) {
        sssp* impl = factory();
        auto t_start = std::chrono::system_clock::now();
        impl->fork();
        impl->join();
        t_accumulated_ms += std::chrono::duration_cast<std::chrono::milliseconds>(
                                std::chrono::system_clock::now() - t_start)
                                .count();
        delete impl;
    }
    return (double)t_accumulated_ms / num_runs;
}

int main(int argc, char** argv) {
    const size_t N = (argc == 1) ? 200000 : std::stoi(argv[1]);
    const unsigned src = 0;
    const unsigned dst = N - 1;

    setbuf(stdout, NULL);

    auto t_start = std::chrono::system_clock::now();
    graph g(N);
    auto t_graph_init_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::system_clock::now() - t_start)
                               .count();
    printf("Graph initialization time: [%6.3f ms]\n", (double)t_graph_init_ms);

    auto stl = [&] {
        return new stl_sequential_sssp(&g, src, dst);
    };
    printf("[%6.3f ms] STL (sequential) shortest path from %d to %d is: ",
           test_harness(stl, num_test_runs),
           src,
           dst);
    g.print_path(src, dst);

    auto tbb_test_harness = [&](size_t num_threads) {
        auto tbb = [&] {
            return new tbb_parallel_sssp(&g, src, dst, num_threads);
        };
        printf("[%6.3f ms] TBB (%zu threads): ", test_harness(tbb, num_test_runs), num_threads);
        g.print_path(src, dst);
    };

    for (size_t num_threads : { 1, 2, 4, 8, 16 }) {
        tbb_test_harness(num_threads);
    }

    auto mq_mtx_test_harness = [&](size_t num_threads, size_t num_queues) {
        auto mq = [&] {
            return new mq_parallel_sssp_mtx(&g, src, dst, num_threads, num_queues);
        };
        printf("[%6.3f ms] MQ (mutex) (#T=%zu, #Q=%zu): ",
               test_harness(mq, num_test_runs),
               num_threads,
               num_queues);
        g.print_path(src, dst);
    };

    auto mq_spin_test_harness = [&](size_t num_threads, size_t num_queues) {
        auto mq = [&] {
            return new mq_parallel_sssp_spin(&g, src, dst, num_threads, num_queues);
        };
        printf("[%6.3f ms] MQ (TAS) (#T=%zu, #Q=%zu): ",
               test_harness(mq, num_test_runs),
               num_threads,
               num_queues);
        g.print_path(src, dst);
    };

    auto mq_um_test_harness = [&](size_t num_threads, size_t num_queues) {
        auto mq = [&] {
            return new mq_parallel_sssp_update_with_min(&g, src, dst, num_threads, num_queues);
        };
        printf("[%6.3f ms] MQ (update-min) (#T=%zu, #Q=%zu): ",
               test_harness(mq, num_test_runs),
               num_threads,
               num_queues);
        g.print_path(src, dst);
    };

    std::vector<std::pair<size_t, size_t>> test_vec;

    for (size_t num_threads : { 1, 2, 4, 8, 16 }) {
        for (size_t num_queues_per_thread : { 2, 4, 8, 16, 32 }) {
            test_vec.push_back({ num_threads, num_queues_per_thread * num_threads });
        }
    }

    for (auto x : test_vec) {
        mq_mtx_test_harness(x.first, x.second);
        mq_spin_test_harness(x.first, x.second);
        mq_um_test_harness(x.first, x.second);
    }

    return 0;
}
