#include "sssp.hpp"

void stl_sequential_sssp::thread_func() {
    const point _dst = g->vertices[dst];
    while (!pq.empty()) {
        // pop
        const auto cheapest = std::get<1>(pq.top());
        const auto node = cheapest.id;
        const point _node = g->vertices[node];
        pq.pop();

        // kill time (per pop)
        if (dummyCalculation(node) != 1) {
            break;
        }

        // prune
        if (g->best_total_cost[node] <= cheapest.total_cost) {
            continue;
        }

        // update
        g->best_total_cost[node] = cheapest.total_cost;
        g->predecessor[node] = cheapest.pre;

        // exit
        if (node == dst) {
            break;
        }

        // expand
        for (const vertex_id neighbor : g->edges[node]) {
            const point _neighbor = g->vertices[neighbor];
            const path_cost neighbor_total_cost =
                cheapest.total_cost + get_distance(_node, _neighbor);
            const path_cost neighbor_h_cost = heuristic_cost(_neighbor, _dst);
            pq.push(
                { neighbor_total_cost + neighbor_h_cost, { neighbor, node, neighbor_total_cost } });
        }
    }
}

void tbb_parallel_sssp::thread_func() {
    const point _dst = g->vertices[dst];
    pq_node_t pq_top;
    while (pq.try_pop(pq_top)) {
        // pop
        const auto cheapest = std::get<1>(pq_top);
        const auto node = cheapest.id;
        const point _node = g->vertices[node];

        // kill time (per pop)
        if (dummyCalculation(node) != 1) {
            break;
        }

        // prune
        if (g->best_total_cost[node] <= cheapest.total_cost) {
            continue;
        }
        if (g->best_total_cost[dst] <= cheapest.total_cost) {
            continue;
        }

        locks[node].lock();
        // prune
        if (g->best_total_cost[node] <= /*non-deterministic currently*/ cheapest.total_cost) {
            locks[node].unlock();
            continue;
        }
        // update
        g->best_total_cost[node] = cheapest.total_cost;
        g->predecessor[node] = cheapest.pre;
        locks[node].unlock();

        // exit
        if (node == dst) {
            break;
        }

        // expand
        for (const vertex_id neighbor : g->edges[node]) {
            const point _neighbor = g->vertices[neighbor];
            const path_cost neighbor_total_cost =
                cheapest.total_cost + get_distance(_node, _neighbor);
            const path_cost neighbor_h_cost = heuristic_cost(_neighbor, _dst);
            pq.push(
                { neighbor_total_cost + neighbor_h_cost, { neighbor, node, neighbor_total_cost } });
        }
    }
}

void mq_parallel_sssp_mtx::thread_func() {
    const point _dst = g->vertices[dst];
    while (true) {
        // pop
        auto pq_top = pq->tryPop();
        if (!pq_top) {
            break;
        }
        const auto cheapest = std::get<1>(pq_top.get());
        const auto node = cheapest.id;
        const point _node = g->vertices[node];

        // kill time (per pop)
        if (dummyCalculation(node) != 1) {
            break;
        }

        // prune
        if (g->best_total_cost[node] <= cheapest.total_cost) {
            continue;
        }
        if (g->best_total_cost[dst] <= cheapest.total_cost) {
            continue;
        }

        locks[node].lock();
        // prune
        if (g->best_total_cost[node] <= /*non-deterministic currently*/ cheapest.total_cost) {
            locks[node].unlock();
            continue;
        }
        // update
        g->best_total_cost[node] = cheapest.total_cost;
        g->predecessor[node] = cheapest.pre;
        locks[node].unlock();

        // prune, no need to expand the dst node
        if (node == dst) {
            continue;
        }

        // expand
        for (const vertex_id neighbor : g->edges[node]) {
            const point _neighbor = g->vertices[neighbor];
            const path_cost neighbor_total_cost =
                cheapest.total_cost + get_distance(_node, _neighbor);
            const path_cost neighbor_h_cost = heuristic_cost(_neighbor, _dst);
            pq->push(
                { neighbor_total_cost + neighbor_h_cost, { neighbor, node, neighbor_total_cost } });
        }
    }
}

void mq_parallel_sssp_spin::thread_func() {
    const point _dst = g->vertices[dst];
    while (true) {
        // pop
        auto pq_top = pq->tryPop();
        if (!pq_top) {
            break;
        }
        const auto cheapest = std::get<1>(pq_top.get());
        const auto node = cheapest.id;
        const point _node = g->vertices[node];

        // kill time (per pop)
        if (dummyCalculation(node) != 1) {
            break;
        }

        // prune
        if (g->best_total_cost[node] <= cheapest.total_cost) {
            continue;
        }
        if (g->best_total_cost[dst] <= cheapest.total_cost) {
            continue;
        }

        while (std::atomic_flag_test_and_set_explicit(&lock_flags[node], std::memory_order_acquire))
            _mm_pause();
        // prune
        if (g->best_total_cost[node] <= /*non-deterministic currently*/ cheapest.total_cost) {
            std::atomic_flag_clear_explicit(&lock_flags[node], std::memory_order_release);
            continue;
        }
        // update
        g->best_total_cost[node] = cheapest.total_cost;
        g->predecessor[node] = cheapest.pre;
        std::atomic_flag_clear_explicit(&lock_flags[node], std::memory_order_release);

        // prune, no need to expand the dst node
        if (node == dst) {
            continue;
        }

        // expand
        for (const vertex_id neighbor : g->edges[node]) {
            const point _neighbor = g->vertices[neighbor];
            const path_cost neighbor_total_cost =
                cheapest.total_cost + get_distance(_node, _neighbor);
            const path_cost neighbor_h_cost = heuristic_cost(_neighbor, _dst);
            pq->push(
                { neighbor_total_cost + neighbor_h_cost, { neighbor, node, neighbor_total_cost } });
        }
    }
}

void mq_parallel_sssp_update_with_min::thread_func() {
    const point _dst = g->vertices[dst];
    while (true) {
        // pop
        auto pq_top = pq->tryPop();
        if (!pq_top) {
            break;
        }
        const auto cheapest = std::get<1>(pq_top.get());
        const auto node = cheapest.id;
        const point _node = g->vertices[node];

        // kill time (per pop)
        if (dummyCalculation(node) != 1) {
            break;
        }

        uint64_t node_pack =
            g->packed_predecessor_and_best_total_cost[node].load(std::memory_order_relaxed);
        uint64_t dst_pack =
            g->packed_predecessor_and_best_total_cost[dst].load(std::memory_order_relaxed);
        path_cost node_best_total_cost = get_g_dist_from_pack(node_pack);
        path_cost dst_best_total_cost = get_g_dist_from_pack(dst_pack);

        // prune
        if (node_best_total_cost <= cheapest.total_cost) {
            continue;
        }
        if (dst_best_total_cost <= cheapest.total_cost) {
            continue;
        }

        // update
        uint64_t updated_pack = pack_predecessor_g_dist(cheapest.pre, cheapest.total_cost);
        bool updated = false;
        while (cheapest.total_cost < get_g_dist_from_pack(node_pack)) {
            if (g->packed_predecessor_and_best_total_cost[node].compare_exchange_weak(
                    node_pack, updated_pack)) {
                updated = true;
                break;
            }
        }
        if (!updated) {
            continue;
        }

        // prune, no need to expand the dst node
        if (node == dst) {
            continue;
        }

        // expand
        for (const vertex_id neighbor : g->edges[node]) {
            const point _neighbor = g->vertices[neighbor];
            const path_cost neighbor_total_cost =
                cheapest.total_cost + get_distance(_node, _neighbor);
            const path_cost neighbor_h_cost = heuristic_cost(_neighbor, _dst);
            pq->push(
                { neighbor_total_cost + neighbor_h_cost, { neighbor, node, neighbor_total_cost } });
        }
    }
}
