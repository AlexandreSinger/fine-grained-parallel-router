#include "sssp.hpp"

void stl_sequential_sssp::thread_func() {
    while (!pq.empty()) {
        vertex_id u;
        path_cost f;
        std::tie(f, u) = pq.top();
        pq.pop();
        if (u == dst)
            continue;
        path_cost old_g_u = 0.0;

        if (f > g->f_distance[u])
            continue; // prune search space
        old_g_u = g->g_distance[u];

        for (std::size_t i = 0; i < g->edges[u].size(); ++i) {
            vertex_id v = g->edges[u][i];
            path_cost new_g_v = old_g_u + get_distance(g->vertices[u], g->vertices[v]);
            path_cost new_f_v = 0.0;

            if (new_g_v < g->g_distance[v]) {
                g->predecessor[v] = u;
                g->g_distance[v] = new_g_v;
                new_f_v = g->f_distance[v] =
                    g->g_distance[v] + get_distance(g->vertices[v], g->vertices[dst]);
                pq.push(std::make_pair(new_f_v, v));
            }
        }
    }
}

void mq_parallel_sssp_lock::thread_func() {
    while (true) {
        vertex_id u;
        path_cost f;

        auto u_rec = mq->tryPop();
        if (u_rec) {
            std::tie(f, u) = u_rec.get(); // try pop succeed
        }
        else
            break;

        if (u == dst)
            continue;
        if (f > g->f_distance[u])
            continue; // prune search space

        path_cost old_g_u = g->g_distance[u];
        for (std::size_t i = 0; i < g->edges[u].size(); ++i) {
            vertex_id v = g->edges[u][i];
            path_cost new_g_v = old_g_u + get_distance(g->vertices[u], g->vertices[v]);
            path_cost new_f_v = 0.0;
            locks[v].lock();
            if (new_g_v < g->g_distance[v]) {
                g->predecessor[v] = u;
                g->g_distance[v] = new_g_v;
                new_f_v = g->f_distance[v] =
                    g->g_distance[v] + get_distance(g->vertices[v], g->vertices[dst]);
                locks[v].unlock();
                mq->push(std::make_tuple(new_f_v, v));
            }
            else {
                locks[v].unlock();
            }
        }
    }
}

void mq_parallel_sssp_ttas::thread_func() {
    while (true) {
        vertex_id u;
        path_cost f;

        auto u_rec = mq->tryPop();
        if (u_rec) {
            std::tie(f, u) = u_rec.get(); // try pop succeed
        }
        else
            break;

        if (u == dst)
            continue;
        if (f > g->f_distance[u])
            continue; // prune search space

        path_cost old_g_u = g->g_distance[u];
        for (std::size_t i = 0; i < g->edges[u].size(); ++i) {
            vertex_id v = g->edges[u][i];
            path_cost new_g_v = old_g_u + get_distance(g->vertices[u], g->vertices[v]);
            if (new_g_v < g->g_distance[v]) {
                path_cost new_f_v = new_g_v + get_distance(g->vertices[v], g->vertices[dst]);
                bool need_push = false;
                while (std::atomic_flag_test_and_set_explicit(&lock_flags[v],
                                                              std::memory_order_acquire))
                    ;
                if (new_g_v < g->g_distance[v]) {
                    g->predecessor[v] = u;
                    g->g_distance[v] = new_g_v;
                    g->f_distance[v] = new_f_v;
                    need_push = true;
                }
                std::atomic_flag_clear_explicit(&lock_flags[v], std::memory_order_release);
                if (need_push)
                    mq->push(std::make_tuple(new_f_v, v));
            }
        }
    }
}

void mq_parallel_sssp_update_with_min::thread_func() {
    while (true) {
        vertex_id u;
        path_cost f;

        auto u_rec = mq->tryPop();
        if (u_rec) {
            std::tie(f, u) = u_rec.get(); // try pop succeed
        }
        else
            break;

        if (u == dst)
            continue;

        uint64_t old_pack_u = g->pre_g_dist[u].load(std::memory_order_relaxed);
        path_cost old_g_u = get_g_dist_from_pack(old_pack_u);
        if (f > old_g_u + get_distance(g->vertices[u], g->vertices[dst]))
            continue; // prune search space

        for (std::size_t i = 0; i < g->edges[u].size(); ++i) {
            vertex_id v = g->edges[u][i];
            path_cost new_g_v = old_g_u + get_distance(g->vertices[u], g->vertices[v]);
            path_cost new_f_v = new_g_v + get_distance(g->vertices[v], g->vertices[dst]);

            uint64_t old_pack_v = g->pre_g_dist[v].load(std::memory_order_relaxed);
            uint64_t new_pack_v = pack_predecessor_g_dist(u, new_g_v);

            while (new_g_v < get_g_dist_from_pack(old_pack_v)) {
                if (g->pre_g_dist[v].compare_exchange_weak(old_pack_v, new_pack_v)) {
                    mq->push(std::make_tuple(new_f_v, v));
                    break;
                }
            }
        }
    }
}

void tbb_parallel_sssp::thread_func() {
    vertex_rec u_rec;
    while (open_set.try_pop(u_rec)) {
        vertex_id u;
        path_cost f;
        std::tie(f, u) = (u_rec);

        if (u == dst)
            continue;
        if (f > g->f_distance[u])
            continue; // prune search space

        path_cost old_g_u = g->g_distance[u];
        for (std::size_t i = 0; i < g->edges[u].size(); ++i) {
            vertex_id v = g->edges[u][i];
            path_cost new_g_v = old_g_u + get_distance(g->vertices[u], g->vertices[v]);
            path_cost new_f_v = 0.0;
            locks[v].lock();
            if (new_g_v < g->g_distance[v]) {
                g->predecessor[v] = u;
                g->g_distance[v] = new_g_v;
                new_f_v = g->f_distance[v] =
                    g->g_distance[v] + get_distance(g->vertices[v], g->vertices[dst]);
                locks[v].unlock();
                open_set.push(std::make_pair(new_f_v, v));
            }
            else {
                locks[v].unlock();
            }
        }
    }
}
