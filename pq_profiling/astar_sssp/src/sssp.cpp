#include "sssp.hpp"

void stl_sequential_sssp::thread_func() {
    while (!pq.empty()) {
        vertex_id u;
        double f;
        std::tie(f, u) = pq.top();
        pq.pop();
        if (u == dst)
            continue;
        double old_g_u = 0.0;

        if (f > g->f_distance[u])
            continue; // prune search space
        old_g_u = g->g_distance[u];

        for (std::size_t i = 0; i < g->edges[u].size(); ++i) {
            vertex_id v = g->edges[u][i];
            double new_g_v = old_g_u + get_distance(g->vertices[u], g->vertices[v]);
            double new_f_v = 0.0;

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

void mq_parallel_sssp::thread_func() {
    while (true) {
        vertex_id u;
        double f;

        auto u_rec = mq->tryPop();
        if (u_rec) {
            std::tie(f, u) = u_rec.get(); // try pop succeed
        }
        else
            break;

        if (u == dst)
            continue;
        double old_g_u = 0.0;
        {
            oneapi::tbb::spin_mutex::scoped_lock l(locks[u]);
            if (f > g->f_distance[u])
                continue; // prune search space
            old_g_u = g->g_distance[u];
        }
        for (std::size_t i = 0; i < g->edges[u].size(); ++i) {
            vertex_id v = g->edges[u][i];
            double new_g_v = old_g_u + get_distance(g->vertices[u], g->vertices[v]);
            double new_f_v = 0.0;
            // the push flag lets us move some work out of the critical section below
            bool push = false;
            {
                oneapi::tbb::spin_mutex::scoped_lock l(locks[v]);
                if (new_g_v < g->g_distance[v]) {
                    g->predecessor[v] = u;
                    g->g_distance[v] = new_g_v;
                    new_f_v = g->f_distance[v] =
                        g->g_distance[v] + get_distance(g->vertices[v], g->vertices[dst]);
                    push = true;
                }
            }
            if (push) {
                mq->push(std::make_tuple(new_f_v, v));
            }
        }
    }
}

void tbb_parallel_sssp::thread_func() {
    vertex_rec u_rec;
    while (open_set.try_pop(u_rec)) {
        vertex_id u;
        double f;
        std::tie(f, u) = (u_rec);

        if (u == dst)
            continue;
        double old_g_u = 0.0;
        {
            oneapi::tbb::spin_mutex::scoped_lock l(locks[u]);
            if (f > g->f_distance[u])
                continue; // prune search space
            old_g_u = g->g_distance[u];
        }
        for (std::size_t i = 0; i < g->edges[u].size(); ++i) {
            vertex_id v = g->edges[u][i];
            double new_g_v = old_g_u + get_distance(g->vertices[u], g->vertices[v]);
            double new_f_v = 0.0;
            // the push flag lets us move some work out of the critical section below
            bool push = false;
            {
                oneapi::tbb::spin_mutex::scoped_lock l(locks[v]);
                if (new_g_v < g->g_distance[v]) {
                    g->predecessor[v] = u;
                    g->g_distance[v] = new_g_v;
                    new_f_v = g->f_distance[v] =
                        g->g_distance[v] + get_distance(g->vertices[v], g->vertices[dst]);
                    push = true;
                }
            }
            if (push) {
                open_set.push(std::make_pair(new_f_v, v));
            }
        }
    }
}
